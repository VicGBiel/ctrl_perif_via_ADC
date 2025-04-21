#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "lib/ssd1306.h"
#include "lib/font.h"
#include "hardware/pio.h" 
#include "lib/ws2812.pio.h" 
#include "pico/bootrom.h" 


#define BTN_A 5 // Pino do botão A
#define BTN_B 6 // Pino do botão B
#define LED_R 13 // Pino do LED vermelho
#define LED_B 12 // Pino do LED azul
#define BTN_JOY 22 // Pino do botão do joystick
#define BUZZER_PIN 10 // Pino do Buzzer
#define IS_RGBW false // Define se os LEDs são RGBW ou apenas RGB
#define NUM_LEDS 25 // Número de LEDs na matriz
#define WS2812_PIN 7 // Pino onde os LEDs WS2812 estão conectados
#define JOY_X 26 // Pino do eixo X do joystick
#define JOY_Y 27 // Pino do eixo y do joystick
#define I2C_SDA 14 // SDA
#define I2C_SCL 15 // SCL
#define I2C_PORT i2c1 
#define SCREEN_WIDTH 128 // Comprimento do display
#define SCREEN_HEIGHT 64 // Largura do display
#define SQUARE_SIZE 8 // Tamanho do quadrado (8x8)
#define JOY_MIN 22 // Minimo medido do joystick
#define JOY_MAX 4085 // Máximo medido do joystick

// Variáveis globais
bool estado = 0; // Armazena o estado (display ativo vs matriz ativa)
bool cor = true; // Cor do display
bool buzzer_on = 0; // Armazena estado do buzzer
int square_x = SCREEN_WIDTH / 2 - SQUARE_SIZE / 2; // Ajuste horizontal do display
int square_y = SCREEN_HEIGHT / 2 - SQUARE_SIZE / 2; // Ajuste vertical do display
uint joy_x, joy_y; // Armazena os estados do ADC
ssd1306_t ssd; // Inicializa a estrutura do display
uint slice_buz; // Slice do buzzer
static volatile uint32_t last_time = 0; // Armazena o tempo do último evento (em microssegundos)

//protótipos das funções
void initGpio(); // Inicializa os pinos GPIO
void setupDisplay(); // Configura o display
void WS2812_setup(); // Configura a matriz de LEDS
static void gpio_irq_handler(uint gpio, uint32_t event); // Lida com as interrupções
int map(int val, int in_min, int in_max, int out_min, int out_max); // Mapeia o joystick (limita para o quadrado não sair do display)
void joystick_read(); // Leitura e tratamento do ADC
static inline void put_pixel(uint32_t pixel_grb); //Acende o pixel na matriz
void set_led_color(uint32_t color, int count); // Controla a cor do LED
void buzzer_init(uint slice_num, uint freq, uint duty_cycle); // Ativa ou desativa o buzzer

int main()
{
    initGpio();
    setupDisplay();
    WS2812_setup();

    adc_init();
    adc_gpio_init(JOY_X);
    adc_gpio_init(JOY_Y);  

    gpio_set_irq_enabled_with_callback(BTN_A, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    gpio_set_irq_enabled_with_callback(BTN_B, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    gpio_set_irq_enabled_with_callback(BTN_JOY, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    while (true) {
        joystick_read();
        printf("eixo x: %d\neixo y: %d\n", joy_x, joy_y);
        sleep_ms(100);
    }
}

void initGpio() {
    // Inicialização do LED vermelho
    gpio_init(LED_R);
    gpio_set_dir(LED_R, GPIO_OUT);
    // Inicialização do LED azul
    gpio_init(LED_B);
    gpio_set_dir(LED_B, GPIO_OUT);
    //inicialização do botão A
    gpio_init(BTN_A);
    gpio_set_dir(BTN_A, false);
    gpio_pull_up(BTN_A);
    //inicialização do botão B
    gpio_init(BTN_B);
    gpio_set_dir(BTN_B, false);
    gpio_pull_up(BTN_B);
    //inicialização do botão do joystick
    gpio_init(BTN_JOY);
    gpio_set_dir(BTN_JOY, false);
    gpio_pull_up(BTN_JOY);
    //Inicialização do buzzer pin
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);
    slice_buz = pwm_gpio_to_slice_num(BUZZER_PIN);
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 4.0);
    pwm_init(slice_buz, &config, true);
}

void setupDisplay(){
    i2c_init(I2C_PORT, 400 * 1000); //Inicializa o i2c em 400kHz
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C); // Set the GPIO pin function to I2C
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C); // Set the GPIO pin function to I2C
    gpio_pull_up(I2C_SDA); // Pull up the data line
    gpio_pull_up(I2C_SCL); // Pull up the clock line
    ssd1306_init(&ssd, SCREEN_WIDTH, SCREEN_HEIGHT, false, 0x3C, I2C_PORT);
    ssd1306_config(&ssd); // Configura o display
    ssd1306_send_data(&ssd); // Envia os dados para o display
    
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);
}

void WS2812_setup(){
    PIO pio = pio0;
    int sm = 0;
    uint offset = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, sm, offset, WS2812_PIN, 800000, IS_RGBW);
}

int map(int val, int in_min, int in_max, int out_min, int out_max) {
    if (val < in_min) val = in_min;
    if (val > in_max) val = in_max;
    return out_min + (val - in_min) * (out_max - out_min) / (in_max - in_min);
}

void joystick_read() {
    adc_select_input(0);
    joy_y = adc_read();
    adc_select_input(1);
    joy_x = adc_read();

    square_x = map(joy_x, JOY_MIN, JOY_MAX, 0, SCREEN_WIDTH - SQUARE_SIZE);
    square_y = map(joy_y, JOY_MIN, JOY_MAX, SCREEN_HEIGHT - SQUARE_SIZE, 0);  

    if (!estado){
        set_led_color(0x000000, 25); // Limpa a matriz
        gpio_put(LED_R, GPIO_IN);
        
        ssd1306_fill(&ssd, !cor);
        ssd1306_rect(&ssd, square_y, square_x, SQUARE_SIZE, SQUARE_SIZE, cor, !cor);
        ssd1306_send_data(&ssd);

    } else {
        ssd1306_fill(&ssd, !cor);
        ssd1306_send_data(&ssd); // Limpa o display
        gpio_put(LED_R, GPIO_OUT);
        // Mapeia valor para número de LEDs a acender
        int leds_to_light = (joy_x * NUM_LEDS) / JOY_MAX;
        // Define cor 
        set_led_color(0x010101, leds_to_light);

        if(buzzer_on){
            gpio_put(LED_B, GPIO_OUT);
            buzzer_init(slice_buz, 300, abs(joy_y-1900)/500);
        } else {
            gpio_put(LED_B, GPIO_IN);
            buzzer_init(slice_buz, 300, 0);
        }
    }

}

void gpio_irq_handler(uint gpio, uint32_t events) {
    uint32_t current_time = to_us_since_boot(get_absolute_time());

    if (current_time - last_time > 200000) { // 200ms de debounce
        last_time = current_time;
        if (gpio == BTN_JOY) {

            reset_usb_boot(0, 0);

        } else if (gpio == BTN_A) {
            
            estado = !estado;

        } else if (gpio == BTN_B) {

            buzzer_on = !buzzer_on;

        }
    }
}

static inline void put_pixel(uint32_t pixel_grb) {
    pio_sm_put_blocking(pio0, 0, pixel_grb << 8u);
}

void set_led_color(uint32_t color, int count) {
    for (int i = 0; i < NUM_LEDS; i++) {
        if (i < count) {
            put_pixel(color);
        } else {
            put_pixel(0); // LEDs restantes apagados
        }
    }
}

void buzzer_init(uint slice_num, uint freq, uint duty_cycle) {
    uint32_t clock_divider = 4;
    uint32_t wrap = 125000000 / (clock_divider * freq);
    pwm_set_wrap(slice_num, wrap);
    pwm_set_chan_level(slice_num, PWM_CHAN_A, wrap * duty_cycle / 100);
}

