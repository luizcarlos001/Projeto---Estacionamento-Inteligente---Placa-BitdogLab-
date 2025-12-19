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
// Reduzi o limite para fechar para ser mais reativo à passagem
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

    // Clock do Pico é 125MHz. 
    // Divisor 125 -> 1MHz (1us por tick)
    // Wrap 20000 -> 20.000us = 20ms (50Hz)
    pwm_set_clkdiv(slice_servo, 125.0f); 
    pwm_set_wrap(slice_servo, 20000);    
    pwm_set_enabled(slice_servo, true);
}

void servo_set_angle(float angle) {
    if (angle < 0) angle = 0;
    if (angle > 180) angle = 180;

    // SG90 normalmente usa: 500us (0°) a 2400us (180°)
    // Com wrap de 20000, o nível é o próprio valor em microssegundos
    uint16_t duty_us = (uint16_t)(500 + (angle / 180.0f) * 1900.0f);
    pwm_set_gpio_level(SERVO_PIN, duty_us);
}

// ============================================================
// BUZZER (PWM)
// ============================================================
void buzzer_init_pwm() {
    // Buzzer 21 (Grave/Padrão)
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);
    pwm_set_clkdiv(pwm_gpio_to_slice_num(BUZZER_PIN), 4.0f);
    pwm_set_wrap(pwm_gpio_to_slice_num(BUZZER_PIN), 10000);
    pwm_set_gpio_level(BUZZER_PIN, 0);
    pwm_set_enabled(pwm_gpio_to_slice_num(BUZZER_PIN), true);

    // Buzzer 10 (Agudo/Divertido)
    gpio_set_function(BUZZER_LOC, GPIO_FUNC_PWM);
    uint slice_loc = pwm_gpio_to_slice_num(BUZZER_LOC);
    // Som grave tipo buzina eletrônica (~1 kHz)
    pwm_set_clkdiv(slice_loc, 64.0f);
    pwm_set_wrap(slice_loc, 2000);

    pwm_set_gpio_level(BUZZER_LOC, 0);
    pwm_set_enabled(slice_loc, true);
}

void buzzer_som(bool ligado) {
    if(ligado) {
        // Usando 5000 para um som nítido (50% de 10000)
        pwm_set_gpio_level(BUZZER_PIN, 5000); 
    } else {
        pwm_set_gpio_level(BUZZER_PIN, 0);
    }
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

    // --- TESTE DE VARREDURA DO SERVO (Para verificar se está vivo) ---
    servo_set_angle(0);   // Abre
    sleep_ms(1000);
    servo_set_angle(90);  // Fecha
    sleep_ms(1000);
    servo_set_angle(0);   // Abre final
    // -----------------------------------------------------------------

    bool mostrar_vaga1 = true;
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

// --- LÓGICA DE LOCALIZAÇÃO (BLINDADA - 3 BIPES) ---
        static absolute_time_t localizar_timeout = 0;
        static int localizar_beeps = 0;

        if (localizar_vaga1 || localizar_vaga2) {
            // Verifica se a vaga está ocupada antes de iniciar
            if ((localizar_vaga1 && vaga1_status.ocupada) || (localizar_vaga2 && vaga2_status.ocupada)) {
                localizar_beeps = 6; // 6 estados = 3 bipes
            }
            localizar_vaga1 = false; 
            localizar_vaga2 = false;
        }

        if (localizar_beeps > 0) {
            if (absolute_time_diff_us(localizar_timeout, get_absolute_time()) >= 200 * 1000) {
                localizar_timeout = get_absolute_time();
                
                if (localizar_beeps % 2 != 0) {
                    // LIGA: Som agudo no Pino 10
                    pwm_set_gpio_level(BUZZER_LOC, 1000); // 50% de 2000

                } else {
                    // DESLIGA: Silêncio total em ambos os pinos para garantir
                    pwm_set_gpio_level(BUZZER_LOC, 0);
                    pwm_set_gpio_level(BUZZER_PIN, 0); 
                }
                
                localizar_beeps--;

                // RESGATE DE SEGURANÇA: Se é o último bipe, mata o PWM de vez
                if (localizar_beeps == 0) {
                    pwm_set_gpio_level(BUZZER_LOC, 0);
                    pwm_set_gpio_level(BUZZER_PIN, 0);
                    beep_on = false; // Reseta o estado da manobra também
                }
            }
        }

            // ======================================================
            // NOVA LÓGICA DA CANCELA (ABRE -> FECHA -> ABRE)
            // ======================================================
            bool carro_passando = (d1 > LIMITE_FECHAR_MM && d1 < LIMITE_ABRIR_MM) || 
                                 (d2 > LIMITE_FECHAR_MM && d2 < LIMITE_ABRIR_MM);

            if (carro_passando) {
                if (!servo_fechado) {
                    servo_set_angle(90); // Fecha a cancela enquanto o carro passa
                    servo_fechado = true;
                    gpio_put(LED_VERMELHO, 1);
                    gpio_put(LED_VERDE, 0);
                }
            } else {
                if (servo_fechado) {
                    servo_set_angle(0); // Abre a cancela
                    servo_fechado = false;
                    gpio_put(LED_VERDE, 1);
                    gpio_put(LED_VERMELHO, 0);
                }
            }

            // ================= BUZZER DE MANOBRA (CORRIGIDO SEM QUEBRAR NADA) =================

            // 1. PARADO → SILÊNCIO TOTAL
            if (d1 <= ZONA_PARADO_MM || d2 <= ZONA_PARADO_MM) {
                buzzer_som(false);
                beep_on = false;
            }

            // 2. MANOBRA → BEEP PROGRESSIVO
            else if (
                (d1 > ZONA_PARADO_MM && d1 < ZONA_LIVRE_MM) ||
                (d2 > ZONA_PARADO_MM && d2 < ZONA_LIVRE_MM)
            ) {
                uint16_t distancia_ativa = 9999;

                if (d1 > ZONA_PARADO_MM && d1 < ZONA_LIVRE_MM)
                    distancia_ativa = d1;

                if (d2 > ZONA_PARADO_MM && d2 < ZONA_LIVRE_MM && d2 < distancia_ativa)
                    distancia_ativa = d2;

                int intervalo = distancia_ativa / 1.5f;
                if (intervalo < 40) intervalo = 40;

                if (absolute_time_diff_us(last_beep_time, get_absolute_time()) >= intervalo * 1000) {
                    beep_on = !beep_on;
                    buzzer_som(beep_on);
                    last_beep_time = get_absolute_time();
                }
            }

            // 3. FORA DA ZONA → SILÊNCIO
            else {
                buzzer_som(false);
                beep_on = false;
            }
        }

        if (absolute_time_diff_us(last_display_time, get_absolute_time()) >= DISPLAY_INTERVAL_MS * 1000) {
            last_display_time = get_absolute_time();
            if (mostrar_vaga1) {
                display_show_distance(&oled, d1);
                const char* st = (d1 > ZONA_LIVRE_MM) ? "LIVRE" : (d1 < ZONA_PARADO_MM) ? "OCUPADA" : "ATENCAO";
                display_show_status(&oled, st);
            } else {
                display_show_distance(&oled, d2);
                const char* st = (d2 > ZONA_LIVRE_MM) ? "LIVRE" : (d2 < ZONA_PARADO_MM) ? "OCUPADA" : "ATENCAO";
                display_show_status(&oled, st);
            }
            mostrar_vaga1 = !mostrar_vaga1;
        }

        sleep_ms(WIFI_LOOP_DELAY_MS);
    }
}