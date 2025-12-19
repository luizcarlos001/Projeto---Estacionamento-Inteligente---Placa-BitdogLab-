#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "pico/cyw43_arch.h"

// === WIFI / HTTP ===
#include "wifi_ap.h"
#include "http_server.h"

// === PROJETO ===
#include "sensor.h"
#include "sensor_ultrasonico.h"
#include "display.h"
#include "parking_state.h"

// === PINOS ===
#define SERVO_PIN 16
#define BUZZER_PIN 21        // Manobra
#define BUZZER_LOC 10        // Localização (Pino divertido) 


// === LIMITES SERVO (Cancela) ===
#define LIMITE_FECHAR_MM 100 
#define LIMITE_ABRIR_MM  250

// === LIMITES BUZZER ===
#define ZONA_LIVRE_MM    800  
#define ZONA_PARADO_MM   150  

// === INTERVALOS (ms) ===
#define SENSOR_INTERVAL_MS   100 
#define DISPLAY_INTERVAL_MS  500
#define WIFI_LOOP_DELAY_MS   5

uint slice_servo;
uint slice_buzzer;

// ============================================================
// SERVO - AJUSTADO PARA SG90 (50Hz)
// ============================================================
void servo_init(void) {
    gpio_set_function(SERVO_PIN, GPIO_FUNC_PWM);
    slice_servo = pwm_gpio_to_slice_num(SERVO_PIN);
    pwm_set_clkdiv(slice_servo, 125.0f); 
    pwm_set_wrap(slice_servo, 20000);    
    pwm_set_enabled(slice_servo, true);
}

void servo_set_angle(float angle) {
    if (angle < 0) angle = 0;
    if (angle > 180) angle = 180;
    uint16_t duty_us = (uint16_t)(500 + (angle / 180.0f) * 1900.0f);
    pwm_set_gpio_level(SERVO_PIN, duty_us);
}

// ============================================================
// BUZZER (PWM)
// ============================================================
void buzzer_init_pwm() {
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);
    pwm_set_clkdiv(pwm_gpio_to_slice_num(BUZZER_PIN), 4.0f);
    pwm_set_wrap(pwm_gpio_to_slice_num(BUZZER_PIN), 10000);
    pwm_set_gpio_level(BUZZER_PIN, 0);
    pwm_set_enabled(pwm_gpio_to_slice_num(BUZZER_PIN), true);

    gpio_set_function(BUZZER_LOC, GPIO_FUNC_PWM);
    uint slice_loc = pwm_gpio_to_slice_num(BUZZER_LOC);
    pwm_set_clkdiv(slice_loc, 64.0f);
    pwm_set_wrap(slice_loc, 2000);
    pwm_set_gpio_level(BUZZER_LOC, 0);
    pwm_set_enabled(slice_loc, true);
}

void buzzer_som(bool ligado) {
    if(ligado) pwm_set_gpio_level(BUZZER_PIN, 5000); 
    else pwm_set_gpio_level(BUZZER_PIN, 0);
}

// ============================================================
// MAIN
// ============================================================
int main() {
    stdio_init_all();
    sleep_ms(2000);

    printf("=== Sistema de Cancela Ativa ===\n");

    // GPIOs
    gpio_init(LED_VERDE);
    gpio_set_dir(LED_VERDE, GPIO_OUT);
    gpio_init(LED_VERMELHO);
    gpio_set_dir(LED_VERMELHO, GPIO_OUT);

    // WiFi
    if (cyw43_arch_init()) {
        printf("Erro CYW43\n");
        return -1;
    }
    wifi_ap_init();
    http_server_init();

    // Sensores
    vl53l0x_dev sensor_vlx;
    sensor_init(&sensor_vlx);
    sensor_ultrasonico_init();

    // Display
    ssd1306_t oled;
    display_init(&oled);

    // Atuadores
    servo_init();
    buzzer_init_pwm();

    // TESTE SERVO
    servo_set_angle(0);   
    sleep_ms(1000);
    servo_set_angle(90);  
    sleep_ms(1000);
    servo_set_angle(0);   

    bool servo_fechado = false;
    absolute_time_t last_beep_time = 0;
    bool beep_on = false;
    absolute_time_t last_sensor_time = 0;
    absolute_time_t last_display_time = 0;

    uint16_t d1 = 9999; 
    uint16_t d2 = 9999; 

    while (true) {
        cyw43_arch_poll();

        if (absolute_time_diff_us(last_sensor_time, get_absolute_time()) >= SENSOR_INTERVAL_MS * 1000) {
            last_sensor_time = get_absolute_time();

            d1 = sensor_read_distance(&sensor_vlx);
            float ultra_cm = sensor_ultrasonico_ler_distancia_cm();
            d2 = (ultra_cm < 0) ? 9999 : (uint16_t)(ultra_cm * 10.0f);

            vaga1_status.ocupada = (d1 < ZONA_PARADO_MM);
            if (vaga1_status.ocupada) vaga1_status.tempo_ocupada_ms += SENSOR_INTERVAL_MS;
            else vaga1_status.tempo_ocupada_ms = 0;

            vaga2_status.ocupada = (d2 < ZONA_PARADO_MM);
            if (vaga2_status.ocupada) vaga2_status.tempo_ocupada_ms += SENSOR_INTERVAL_MS;
            else vaga2_status.tempo_ocupada_ms = 0;

            // --- LÓGICA DE LOCALIZAÇÃO ---
            static absolute_time_t localizar_timeout = 0;
            static int localizar_beeps = 0;

            if (localizar_vaga1 || localizar_vaga2) {
                if ((localizar_vaga1 && vaga1_status.ocupada) || (localizar_vaga2 && vaga2_status.ocupada)) {
                    localizar_beeps = 6; 
                }
                localizar_vaga1 = false; 
                localizar_vaga2 = false;
            }

            if (localizar_beeps > 0) {
                if (absolute_time_diff_us(localizar_timeout, get_absolute_time()) >= 200 * 1000) {
                    localizar_timeout = get_absolute_time();
                    if (localizar_beeps % 2 != 0) pwm_set_gpio_level(BUZZER_LOC, 1000); 
                    else { pwm_set_gpio_level(BUZZER_LOC, 0); pwm_set_gpio_level(BUZZER_PIN, 0); }
                    localizar_beeps--;
                    if (localizar_beeps == 0) {
                        pwm_set_gpio_level(BUZZER_LOC, 0);
                        pwm_set_gpio_level(BUZZER_PIN, 0);
                        beep_on = false;
                    }
                }
            }

            // --- CANCELA ---
            bool carro_passando = (d1 > LIMITE_FECHAR_MM && d1 < LIMITE_ABRIR_MM) || 
                                 (d2 > LIMITE_FECHAR_MM && d2 < LIMITE_ABRIR_MM);

            if (carro_passando) {
                if (!servo_fechado) {
                    servo_set_angle(90); servo_fechado = true;
                    gpio_put(LED_VERMELHO, 1); gpio_put(LED_VERDE, 0);
                }
            } else {
                if (servo_fechado) {
                    servo_set_angle(0); servo_fechado = false;
                    gpio_put(LED_VERDE, 1); gpio_put(LED_VERMELHO, 0);
                }
            }

            // --- BUZZER MANOBRA ---
            if (d1 <= ZONA_PARADO_MM || d2 <= ZONA_PARADO_MM) {
                buzzer_som(false); beep_on = false;
            } else if ((d1 > ZONA_PARADO_MM && d1 < ZONA_LIVRE_MM) || (d2 > ZONA_PARADO_MM && d2 < ZONA_LIVRE_MM)) {
                uint16_t dist = (d1 < d2) ? d1 : d2;
                int intervalo = dist / 1.5f;
                if (intervalo < 40) intervalo = 40;
                if (absolute_time_diff_us(last_beep_time, get_absolute_time()) >= intervalo * 1000) {
                    beep_on = !beep_on; buzzer_som(beep_on); last_beep_time = get_absolute_time();
                }
            } else {
                buzzer_som(false); beep_on = false;
            }
        }

        // ================= BLOCO DO DISPLAY CORRIGIDO =================
        if (absolute_time_diff_us(last_display_time, get_absolute_time()) >= DISPLAY_INTERVAL_MS * 1000) {
            last_display_time = get_absolute_time();

            // Limpa o display
            memset(oled.ram_buffer + 1, 0, oled.bufsize - 1);

            char txt1[32], txt2[32];
            const char* st1 = (d1 > ZONA_LIVRE_MM) ? "LIVRE" : (d1 < ZONA_PARADO_MM) ? "OCUPADA" : "ATENCAO";
            const char* st2 = (d2 > ZONA_LIVRE_MM) ? "LIVRE" : (d2 < ZONA_PARADO_MM) ? "OCUPADA" : "ATENCAO";

            sprintf(txt1, "Vaga 1: %s", st1);
            sprintf(txt2, "Vaga 2: %s", st2);

            // Escreve os dois na tela simultaneamente
            ssd1306_draw_string(oled.ram_buffer + 1, 5, 10, txt1);
            ssd1306_draw_string(oled.ram_buffer + 1, 5, 40, txt2);

            ssd1306_send_data(&oled); 
        }

        sleep_ms(WIFI_LOOP_DELAY_MS);
    }
}