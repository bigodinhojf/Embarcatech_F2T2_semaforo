// -- Inclusão de bibliotecas
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "hardware/pwm.h"
#include "hardware/pio.h"
#include "ws2812.pio.h"
#include "lib/ssd1306.h"
#include "lib/font.h"
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"
#include <stdio.h>
#include "pico/bootrom.h"

// -- Definição de constantes
// GPIO
#define button_A 5 // Botão A GPIO 5
#define button_B 6 // Botão B GPIO 6
#define matriz_leds 7 // Matriz de LEDs GPIO 7
#define NUM_LEDS 25 // Número de LEDs na matriz
#define buzzer_A 21 // Buzzer A GPIO 21
#define buzzer_B 10 // Buzzer B GPIO 10
#define LED_Green 11 // LED Verde GPIO 11
#define LED_Red 13 // LED Vermelho GPIO 13

// Display I2C
#define display_i2c_port i2c1 // Define a porta I2C
#define display_i2c_sda 14 // Define o pino SDA na GPIO 14
#define display_i2c_scl 15 // Define o pino SCL na GPIO 15
#define display_i2c_endereco 0x3C // Define o endereço do I2C
ssd1306_t ssd; // Inicializa a estrutura do display

// -- Definição de variáveis globais
static volatile uint32_t last_time = 0; // Armazena o tempo do último clique dos botões (debounce)
volatile bool modo_noturno = false; // Modo noturno ligado ou desligado
volatile int tempo = 100; // Variável auxiliar para definir a cor do semáforo sem utilizar delay na task semáforo
volatile int cor_semaforo = 0; // Cor do semáforo atual (0 = verde, 1 = amarelo, 2 = vermelho, 3 = amarelo do modo noturno, 4 = apagado)


// --- Funções necessária para a manipulação da matriz de LEDs

// Estrutura do pixel GRB (Padrão do WS2812)
struct pixel_t {
    uint8_t G, R, B; // Define variáveis de 8-bits (0 a 255) para armazenar a cor
};
typedef struct pixel_t pixel_t;
typedef pixel_t npLED_t; // Permite declarar variáveis utilizando apenas "npLED_t"

// Declaração da Array que representa a matriz de LEDs
npLED_t leds[NUM_LEDS];

// Variáveis para máquina PIO
PIO np_pio;
uint sm;

// Função para definir a cor de um LED específico
void cor(const uint index, const uint8_t r, const uint8_t g, const uint8_t b) {
    leds[index].R = r;
    leds[index].G = g;
    leds[index].B = b;
}

// Função para desligar todos os LEDs
void desliga() {
    for (uint i = 0; i < NUM_LEDS; ++i) {
        cor(i, 0, 0, 0);
    }
}

// Função para enviar o estado atual dos LEDs ao hardware  - buffer de saída
void buffer() {
    for (uint i = 0; i < NUM_LEDS; ++i) {
        pio_sm_put_blocking(np_pio, sm, leds[i].G);
        pio_sm_put_blocking(np_pio, sm, leds[i].R);
        pio_sm_put_blocking(np_pio, sm, leds[i].B);
    }
}

// Função para converter a posição da matriz para uma posição do vetor.
int getIndex(int x, int y) {
    // Se a linha for par (0, 2, 4), percorremos da esquerda para a direita.
    // Se a linha for ímpar (1, 3), percorremos da direita para a esquerda.
    if (y % 2 == 0) {
        return 24-(y * 5 + x); // Linha par (esquerda para direita).
    } else {
        return 24-(y * 5 + (4 - x)); // Linha ímpar (direita para esquerda).
    }
}

// --- Final das funções necessária para a manipulação da matriz de LEDs


// Tarefa que guarda a lógica de tempo do semáforo
void vSemaforoTask(){
    while (true){
        if(modo_noturno == false){
            if(tempo == 100){
                cor_semaforo = 0;
            }else if(tempo == 10000){ // Tempo de 10s para o verde
                cor_semaforo = 1;
            }else if(tempo == 13000){ // Tempo de 3s para o amarelo (10 + 3 = 13)
                cor_semaforo = 2;
            }else if(tempo == 20000){ // tempo de 7s para o vermelho (13 + 7 = 20)
                tempo = 0;
            }
        }else if(modo_noturno == true){
            if(tempo == 100){
                cor_semaforo = 3;
            }else if(tempo == 1000){ // Tempo de 1s de amarelo aceso
                cor_semaforo = 4;
            }else if(tempo == 3000){ // Tempo de 2s de amarelo apagado (1 + 2 = 3)
                tempo = 0;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(100));
        tempo += 100;
    }
}

// Tarefa que processa as cores do semáforo no LED RGB
void vLEDTask(){
    gpio_init(LED_Green); // Inicia a GPIO 11 do LED Verde
    gpio_set_dir(LED_Green, GPIO_OUT); // Define a direção da GPIO 11 do LED Verde como saída
    gpio_put(LED_Green, false); // Estado inicial do LED apagado
    gpio_init(LED_Red); // Inicia a GPIO 13 do LED Vermelho
    gpio_set_dir(LED_Red, GPIO_OUT); // Define a direção da GPIO 13 do LED Vermelho como saída
    gpio_put(LED_Red, false); // Estado inicial do LED apagado
    
    while (true){
        switch (cor_semaforo){
            case 0:
                gpio_put(LED_Green, true);
                gpio_put(LED_Red, false);
                break;
            case 1:
                gpio_put(LED_Green, true);
                gpio_put(LED_Red, true);
                break;
            case 2:
                gpio_put(LED_Green, false);
                gpio_put(LED_Red, true);
                break;
            case 3:
                gpio_put(LED_Green, true);
                gpio_put(LED_Red, true);
                break;
            default:
                gpio_put(LED_Green, false);
                gpio_put(LED_Red, false);
                break;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// Tarefa que processa as cores do semáforo na Matriz de LEDs
void vMatrizTask(){
    // Inicialização do PIO
    np_pio = pio0;
    sm = pio_claim_unused_sm(np_pio, true);
    uint offset = pio_add_program(pio0, &ws2818b_program);
    ws2818b_program_init(np_pio, sm, offset, matriz_leds, 800000);
    desliga(); // Para limpar o buffer dos LEDs
    buffer(); // Atualiza a matriz de LEDs

    while (true){
        desliga();
        switch (cor_semaforo){
            case 0:
                cor(7, 0, 100, 0); // Posição e cor do sinal verde
                break;
            case 1:
                cor(12, 100, 100, 0); // Posição e cor do sinal amarelo
                break;
            case 2:
                cor(17, 100, 0, 0); // Posição e cor do sinal vermelho
                break;
            case 3:
                cor(12, 100, 100, 0); // Posição e cor do sinal amarelo
                break;
            default:
                desliga();
                break;
        }
        buffer();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// Tarefa que atualiza as informaçõs do display
void vDisplayTask(){
    // Inicialização dp Display I2C
    i2c_init(display_i2c_port, 400 * 1000); // Inicializa o I2C usando 400kHz
    gpio_set_function(display_i2c_sda, GPIO_FUNC_I2C); // Define o pino SDA (GPIO 14) na função I2C
    gpio_set_function(display_i2c_scl, GPIO_FUNC_I2C); // Define o pino SCL (GPIO 15) na função I2C
    gpio_pull_up(display_i2c_sda); // Ativa o resistor de pull up para o pino SDA (GPIO 14)
    gpio_pull_up(display_i2c_scl); // Ativa o resistor de pull up para o pino SCL (GPIO 15)
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, display_i2c_endereco, display_i2c_port); // Inicializa o display
    ssd1306_config(&ssd); // Configura o display
    ssd1306_send_data(&ssd); // Envia os dados para o display
    ssd1306_fill(&ssd, false); // Limpa o display
    ssd1306_send_data(&ssd); // Atualiza o display

    while (true){
        // Cria as bordas do display
        ssd1306_rect(&ssd, 0, 0, 127, 63, true, false); // Borda principal
        ssd1306_line(&ssd, 1, 12, 126, 12, true); // Desenha uma linha horizontal
        ssd1306_line(&ssd, 1, 24, 126, 24, true); // Desenha uma linha horizontal

        // Limpa a parte variável do display
        ssd1306_draw_string(&ssd, "        ", 52, 15); // Desenha uma string
        ssd1306_draw_string(&ssd, " ", 24, 28); // Desenha uma string
        ssd1306_draw_string(&ssd, " ", 24, 39); // Desenha uma string
        ssd1306_draw_string(&ssd, " ", 24, 50); // Desenha uma string

        // Cabeçalho
        ssd1306_draw_string(&ssd, "EMB Semaforo", 16, 3); // Desenha uma string
        ssd1306_draw_string(&ssd, "Modo:", 12, 15); // Desenha uma string
        if(modo_noturno){
            ssd1306_draw_string(&ssd, "Noturno", 54, 15); // Desenha uma string
        }else{
            ssd1306_draw_string(&ssd, "Normal", 60, 15); // Desenha uma string
        }

        // Cores do semáforo
        ssd1306_draw_string(&ssd, "Vermelho", 48, 28); // Desenha uma string
        ssd1306_draw_string(&ssd, "Amarelo", 48, 39); // Desenha uma string
        ssd1306_draw_string(&ssd, "Verde", 48, 50); // Desenha uma string

        // Semáforo
        ssd1306_rect(&ssd, 28, 24, 8, 8, true, false); // Desenha um retângulo
        ssd1306_rect(&ssd, 39, 24, 8, 8, true, false); // Desenha um retângulo
        ssd1306_rect(&ssd, 50, 24, 8, 8, true, false); // Desenha um retângulo

        // Preenche o quadrado referente a cor do semáforo
        switch (cor_semaforo){
            case 0:
                ssd1306_rect(&ssd, 50, 24, 8, 8, true, true); // Desenha um retângulo
                break;
            case 1:
                ssd1306_rect(&ssd, 39, 24, 8, 8, true, true); // Desenha um retângulo
                break;
            case 2:
                ssd1306_rect(&ssd, 28, 24, 8, 8, true, true); // Desenha um retângulo
                break;
            case 3:
                ssd1306_rect(&ssd, 39, 24, 8, 8, true, true); // Desenha um retângulo
                break;
            default:
                break;
        }
        ssd1306_send_data(&ssd); // Atualiza o display
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// Tarefa que processa o sinal sonoro do semáforo nos Buzzers
void vBuzzerTask(){
    gpio_set_function(buzzer_A, GPIO_FUNC_PWM); // Define a função da porta GPIO como PWM
    gpio_set_function(buzzer_B, GPIO_FUNC_PWM); // Define a função da porta GPIO como PWM

    uint freq = 1000; // Frequência do buzzer
    uint clock_div = 4; // Divisor do clock
    uint wrap = (125000000 / (clock_div * freq)) - 1; // Define o valor do wrap para frequência escolhida

    uint slice_A = pwm_gpio_to_slice_num(buzzer_A); // Define o slice do buzzer A
    uint slice_B = pwm_gpio_to_slice_num(buzzer_B); // Define o slice do buzzer B

    pwm_set_clkdiv(slice_A, clock_div); // Define o divisor do clock para o buzzer A
    pwm_set_clkdiv(slice_B, clock_div); // Define o divisor do clock para o buzzer B
    pwm_set_wrap(slice_A, wrap); // Define o valor do wrap para o buzzer A
    pwm_set_wrap(slice_B, wrap); // Define o valor do wrap para o buzzer B
    pwm_set_chan_level(slice_A, pwm_gpio_to_channel(buzzer_A), wrap / 40); // Duty cycle para definir o Volume do buzzer A
    pwm_set_chan_level(slice_B, pwm_gpio_to_channel(buzzer_B), wrap / 40); // Duty cycle para definir o volume do buzzer B

    while(true){
        switch (cor_semaforo){
        case 0:
            // Para a cor verde os buzzers ficam 200ms ligado e 800ms desligado
            pwm_set_enabled(slice_A, true);
            pwm_set_enabled(slice_B, true);
            vTaskDelay(pdMS_TO_TICKS(200));
            pwm_set_enabled(slice_A, false);
            pwm_set_enabled(slice_B, false);
            vTaskDelay(pdMS_TO_TICKS(800));
            break;
        case 1:
            // Para a cor amarela os buzzers ficam 100ms ligado e 100ms desligado
            pwm_set_enabled(slice_A, true);
            pwm_set_enabled(slice_B, true);
            vTaskDelay(pdMS_TO_TICKS(100));
            pwm_set_enabled(slice_A, false);
            pwm_set_enabled(slice_B, false);
            vTaskDelay(pdMS_TO_TICKS(100));
            break;
        case 2:
            // Para a cor vermelho os buzzers ficam 500ms ligado e 1500ms desligado
            pwm_set_enabled(slice_A, true);
            pwm_set_enabled(slice_B, true);
            vTaskDelay(pdMS_TO_TICKS(500));
            pwm_set_enabled(slice_A, false);
            pwm_set_enabled(slice_B, false);
            vTaskDelay(pdMS_TO_TICKS(1500));
            break;
        case 3:
            // Para a cor amarela do modo noturno os buzzers ficam ligados enquanto o LED está aceso...
            pwm_set_enabled(slice_A, true);
            pwm_set_enabled(slice_B, true);
            break;
        case 4:
            // ...E desligados enquanto o LED está apagado
            pwm_set_enabled(slice_A, false);
            pwm_set_enabled(slice_B, false);
        default:
            break;
        }
    }

}

// Função de interrupção dos botões
void gpio_irq_handler(uint gpio, uint32_t events){
    //Debouncing
    uint32_t current_time = to_us_since_boot(get_absolute_time()); // Pega o tempo atual e transforma em us
    if(current_time - last_time > 1000000){
        if(gpio == button_A){
            // Botão A inverte o valor do modo noturno (alternando entre normal e noturno)
            modo_noturno = !modo_noturno;
            tempo = 0;
        }else if(gpio == button_B){
            reset_usb_boot(0, 0);
        }
    }
}

// Função principal
int main(){
    stdio_init_all(); // Inicialização do monitor serial

    gpio_init(button_A); // Inicia a GPIO 5 do botão A
    gpio_set_dir(button_A, GPIO_IN); // Define a direção da GPIO 5 do botão A como entrada
    gpio_pull_up(button_A); // Habilita o resistor de pull up da GPIO 5 do botão A
    gpio_set_irq_enabled_with_callback(button_A, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler); // Interrupção do botão A

    gpio_init(button_B); // Inicia a GPIO 6 do botão B
    gpio_set_dir(button_B, GPIO_IN); // Define a direção da GPIO 6 do botão B como entrada
    gpio_pull_up(button_B); // Habilita o resistor de pull up da GPIO 6 do botão B
    gpio_set_irq_enabled_with_callback(button_B, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler); // Interrupção do botão B

    xTaskCreate(vSemaforoTask, "Semaforo", configMINIMAL_STACK_SIZE,
         NULL, tskIDLE_PRIORITY, NULL);
    xTaskCreate(vLEDTask, "LED RGB", configMINIMAL_STACK_SIZE,
         NULL, tskIDLE_PRIORITY, NULL);
     xTaskCreate(vMatrizTask, "Matriz de LEDs", configMINIMAL_STACK_SIZE,
        NULL, tskIDLE_PRIORITY, NULL);
    xTaskCreate(vDisplayTask, "Display", configMINIMAL_STACK_SIZE, 
        NULL, tskIDLE_PRIORITY, NULL);
    xTaskCreate(vBuzzerTask, "Buzzer", configMINIMAL_STACK_SIZE, 
        NULL, tskIDLE_PRIORITY, NULL);
    vTaskStartScheduler();
    panic_unsupported();
}