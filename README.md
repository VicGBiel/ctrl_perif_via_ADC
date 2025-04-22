# Controle de Periféricos no RP2040

Este projeto integra múltiplos periféricos em uma placa baseada no **RP2040** (como o Raspberry Pi Pico W), utilizando linguagem **C**. O sistema alterna entre dois modos de exibição usando botões físicos e reage à movimentação de um joystick analógico, controlando um display OLED, uma matriz de LEDs WS2812 e um buzzer.

## Componentes Utilizados

- **RP2040 (Raspberry Pi Pico)**
- **Display OLED SSD1306 (I2C)**
- **Joystick analógico (dois eixos + botão)**
- **Matriz de LEDs WS2812 (25 LEDs)**
- **Buzzer (PWM)**
- **3 botões físicos (incluindo o do joystick)**

## Modos de Operação

- **Modo 1 - Display OLED ativo (inicial):**
  - O joystick move um quadrado 8x8 na tela.
  - A matriz de LEDs permanece apagada.

- **Modo 2 - Matriz WS2812 ativa:**
  - O eixo X do joystick define quantos LEDs são acesos (de 0 a 25).
  - O eixo Y controla a intensidade do som emitido pelo buzzer (se estiver ativado pelo botão B).
  - O display OLED permanece limpo.

## Mapeamento de Pinos

| Função               | Pino GPIO |
|----------------------|-----------|
| Joystick Eixo X      | 26 (ADC0) |
| Joystick Eixo Y      | 27 (ADC1) |
| Botão do Joystick    | 22        |
| Botão A              | 5         |
| Botão B              | 6         |
| Matriz WS2812        | 7         |
| Buzzer (PWM)         | 10        |
| Display SDA (I2C)    | 14        |
| Display SCL (I2C)    | 15        |


## Link para video demonstrativo

*https://youtu.be/ezUWzF8zxKk*

---

## Autor

Desenvolvido por Victor Gabriel Guimarães Lopes

---
