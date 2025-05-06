<div align="center">
    <img src="https://moodle.embarcatech.cepedi.org.br/pluginfile.php/1/theme_moove/logo/1733422525/Group%20658.png" alt="Logo Embarcatech" height="100">
</div>

<br>

# Semáforo por multitarefas

## Sumário

- [Descrição](#descrição)
- [Funcionalidades Implementadas](#funcionalidades-implementadas)
- [Ferramentas utilizadas](#ferramentas-utilizadas)
- [Objetivos](#objetivos)
- [Instruções de uso](#instruções-de-uso)
- [Vídeo de apresentação](#vídeo-de-apresentação)
- [Aluno e desenvolvedor do projeto](#aluno-e-desenvolvedor-do-projeto)
- [Licensa](#licença)

## Descrição

Este projeto implementa um sistema de semáforo inteligente com dois modos de operação distintos, utilizando o microcontrolador RP2040 e o sistema operacional em tempo real FreeRTOS. O sistema é composto por matriz de LEDs, LED RGB, display SSD1306, buzzer e botão, e foi desenvolvido exclusivamente com tarefas do FreeRTOS.
No Modo Normal, o semáforo realiza o ciclo tradicional (verde → amarelo → vermelho), com sinalização visual e sonora para pessoas cegas. No Modo Noturno, apenas a luz amarela pisca lentamente, com beep sonoro a cada 2 segundos. A troca entre os modos ocorre por meio do botão.

## Funcionalidades Implementadas

1. Lógica do semáforo:

   - A lógica de temporização do semáforo foi desenvolvida em uma tarefa separada, onde foram utilizadas duas variáveis auxiliares, uma de tempo e uma para definir a cor do semáforo. Dentro do loop principal da tarefa é verificado pela função “if - else” se o semáforo está no modo noturno, caso não, a variável auxiliar de tempo é verificada, também por “if - else”, para decidir qual a cor do semáforo deve ser acesa.
   - A variável tempo foi utilizada para que a verificação do modo noturno aconteça a cada 100ms, pois com a utilização de delays maiores o modo noturno era alterado e a sinalização só alterava ao final do ciclo de sinalização.
   - Foi utilizado variáveis globais auxiliares, pois as tarefas que vão implementar as outras funcionalidades precisarão ler qual sinalização está acesa naquele momento.
   - O tempo dos sinais do semáforo está exposto na Tabela.
  
<div align="center">
    <img width = "60%" src = "https://github.com/user-attachments/assets/60a8abde-56d4-40dc-9d10-4cf148d29896">
</div>

2. LED RGB:

   - Na tarefa do LED RGB foi feito inicialmente a inicialização dos LEDs verde e vermelho (GPIO 11 e 13 respectivamente) e dentro do loop principal é verificado por uma função “While” o valor da variável “cor_semaforo”, em cada caso o conjunto de LEDs verde e vermelho é aceso ou apagado para formar as cores do semáforo, esse loop de leitura acontece a cada 100ms.

3. Matriz de LEDs:

   - O funcionamento da tarefa de matriz de LEDs é similar ao do LED RGB, inicialmente é feita a inicialização e configuração do PIO e dentro do loop principal é verificado pela função “While” o  valor da variável “cor_semaforo”, em cada caso é aceso o LED correspondente com a cor correspondente, esse loop de leitura acontece a cada 100ms.
    
4. Display OLED:

   - Para o Display OLED, foi feita a inicialização e configuração do I2C e dentro do loop principal foi feito todo o desenho do layout apresentado, com bordas, cabeçalho com o nome da tarefa, uma área para exibir qual o modo do semáforo e por fim uma representação visual do semáforo com a indicação escrita de cada cor.
  
5. Buzzer:

   - A tarefa do buzzer também foi similar às anteriores, feita a inicialização e configuração dos GPIOs dos buzzers e do PWM e dentro do loop principal é verificado pela função “Switch” o valor da variável “cor_semaforo”, em cada caso os buzzers emitem um sinal sonoro com diferentes tempos de ligado e desligado, formando beeps diferentes, o tempo referente a cada cor do semáforo está exposto na Tabela.

<div align="center">
    <img width = "65%" src = "https://github.com/user-attachments/assets/65af02f5-cee5-424a-8116-82251911d557">
</div>
  
6. Modo noturno:

   - No modo noturno apenas a cor amarela do semáforo é acesa, alternando entre acesa e apagada para indicar que o semáforo está desligado e deve se ter atenção ao cruzá-lo.
   - A alteração dos modos entre normal e noturno foi feita através do botão A, foi utilizado interrupção e debouncing para a implementação deste botão.
  
## Ferramentas utilizadas

- **Ferramenta educacional BitDogLab (versão 6.3)**: Placa de desenvolvimento utilizada para programar o microcontrolador.
- **Microcontrolador Raspberry Pi Pico W**: Responsável pela execução das tarefas do FreeRTOS, controle dos LEDs, do Display OLED e dos Buzzers.
- **LED RGB**: Ascende a cor atual de acordo com o sinal do semáforo.
- **Matriz de LEDs RGB**: Exibi a cor atual de acordo com o sinal do semáforo.
- **Display OLED SSD1306**: Apresenta os dados em tempo real (modo e sinal atual do semáforo).
- **Buzzers**: Toca os sinais sonoros de acordo com o sinal do semáforo. 
- **Visual Studio Code (VS Code)**: IDE utilizada para o desenvolvimento do código com integração ao Pico SDK.
- **Pico SDK**: Kit de desenvolvimento de software utilizado para programar o Raspberry Pi Pico W em linguagem C.
- **FreeRTOS**: Sistema operacional em tempo real, utilizado para gerenciar as multitarefas desenvolvidas.

## Objetivos

1. Consolidar os conceitos estudados sobre tarefas no FreeRTOS.
2. Implementar um sistema de sinal semafórico com dois modos: normal e noturno.
3. Fornecer retorno visual, através de cores, do sinal do semáforo na Matriz de LEDs RGB e no LED RGB.
4. Exibir informações de modo e sinal atual do semáforo no Display OLED.
5. Fornecer retorno sonoro do sinal do semáforo nos Buzzers (acessibilidade para pessoas cegas).
6. Implementar todas as funcionalidades em tarefas distintas.

## Instruções de uso

1. **Clonar o Repositório**:

```bash
git clone https://github.com/bigodinhojf/Embarcatech_F2T2_semaforo.git
```

2. **Compilar e Carregar o Código**:
   No VS Code, configure o ambiente e compile o projeto com os comandos:

```bash	
cmake -G Ninja ..
ninja
```

3. **Interação com o Sistema**:
   - Conecte a placa ao computador.
   - Clique em run usando a extensão do raspberry pi pico.
   - Observe o funcionamento do sinal semafórico.
   - Altere entre os modos pressionando o botão A.

## Vídeo de apresentação

O vídeo apresentando o projeto pode ser assistido [clicando aqui](https://youtu.be/5bqItWLqsVY).

## Aluno e desenvolvedor do projeto

<a href="https://github.com/bigodinhojf">
        <img src="https://github.com/bigodinhojf.png" width="150px;" alt="João Felipe"/><br>
        <sub>
          <b>João Felipe</b>
        </sub>
</a>

## Licença

Este projeto está licenciado sob a licença MIT.
