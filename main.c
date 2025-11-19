#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "sensor.h"
#include "display.h"

// === CONFIGURAÇÃO DO SERVO NO GP16 ===
#define SERVO_PIN 16

uint slice_servo;

// Converte ângulo (0–180°) para largura de pulso PWM
void servo_set_angle(float angle) {
    if (angle < 0) angle = 0;
    if (angle > 180) angle = 180;

    float duty_us = 500 + (angle / 180.0f) * 2000.0f; // 500–2500 µs
    float duty = duty_us / 20000.0f; // período de 20ms (50Hz)

    pwm_set_gpio_level(SERVO_PIN, duty * 65535);
}

void servo_init() {
    gpio_set_function(SERVO_PIN, GPIO_FUNC_PWM);
    slice_servo = pwm_gpio_to_slice_num(SERVO_PIN);

    pwm_set_wrap(slice_servo, 65535);
    pwm_set_clkdiv(slice_servo, 76.0f);  // Aproxima 50Hz
    pwm_set_enabled(slice_servo, true);

    servo_set_angle(0); // Cancela aberta inicialmente
}


int main() {
    stdio_init_all();
    sleep_ms(1500);
    printf("=== Sistema VL53L0X + SSD1306 + SERVO (Cancela Automática) ===\n");

    // Inicializa sensor, display e SERVO
    vl53l0x_dev sensor_dev;
    sensor_init(&sensor_dev);

    ssd1306_t oled;
    display_init(&oled);

    servo_init();

    // --- VARIÁVEIS ---
    uint16_t distance_anterior = 0;
    const uint16_t TOLERANCIA_MOVIMENTO_MM = 5;

    // --- PARÂMETROS DE DISTÂNCIA ---
    const uint16_t LIMITE_PARADO_MM   = 110;
    const uint16_t LIMITE_ALERTA_4_MM = 140;
    const uint16_t LIMITE_ALERTA_3_MM = 190;
    const uint16_t LIMITE_ALERTA_2_MM = 240;
    const uint16_t LIMITE_ALERTA_1_MM = 300;
    const uint16_t LIMITE_VAGA_LIVRE_MM = 400;

    const uint16_t BUZZER_DURACAO_MS = 50; 
    const uint16_t FREQ_BUZZER = 1000;
    const float DUTY_CYCLE = 0.5f;

    while (true) {

        uint16_t distance = sensor_read_distance(&sensor_dev);
        printf("Distancia: %d mm\n", distance);

        display_show_distance(&oled, distance);
        uint16_t tempo_silencio_ms = 0;
        const char *display_status = "VAGA LIVRE";

        // ============================================================
        //                CONTROLE DA CANCELA AUTOMÁTICA
        // ============================================================

        // 1. Carro longe → cancela aberta
        if (distance > LIMITE_ALERTA_1_MM) {
            servo_set_angle(0);   // Abre totalmente
        }
        // 2. Aproximação → cancela meio termo (atenção)
        else if (distance > LIMITE_PARADO_MM) {
            servo_set_angle(45);  // Meio termo
        }
        // 3. Carro MUITO PERTO (<=110mm)
        else {
            uint16_t diff = (distance > distance_anterior)
                            ? (distance - distance_anterior)
                            : (distance_anterior - distance);

            // EMERGÊNCIA → FECHA A CANCELA IMEDIATAMENTE
            if (distance <= LIMITE_PARADO_MM) {
                servo_set_angle(90);   // FECHA TOTAL
                display_status = "VAGA OCUPADA";
            }
        }


        // ============================================================
        //      DETECÇÃO DE PARADA (vaga realmente ocupada)
        // ============================================================
        if (distance <= LIMITE_PARADO_MM) {

            uint16_t diff = (distance > distance_anterior)
                            ? (distance - distance_anterior)
                            : (distance_anterior - distance);

            // Parado de verdade → buzzer off + LED vermelho + cancela fechada
            if (diff < TOLERANCIA_MOVIMENTO_MM) {
                buzzer_off();
                gpio_put(LED_VERMELHO, 1);
                gpio_put(LED_VERDE, 0);

                servo_set_angle(90); // cancela fechada fixa

                display_status = "VAGA OCUPADA";
                display_show_status(&oled, display_status);

                distance_anterior = distance;
                sleep_ms(150);
                continue;
            }
        }


        // ============================================================
        //                LÓGICA DO BUZZER / ALERTAS
        // ============================================================
        if (distance > LIMITE_VAGA_LIVRE_MM) {
            buzzer_off();
            gpio_put(LED_VERDE, 1);
            gpio_put(LED_VERMELHO, 0);
        }
        else if (distance > LIMITE_ALERTA_1_MM) {
            buzzer_off();
        }
        else if (distance > LIMITE_ALERTA_2_MM) tempo_silencio_ms = 600;
        else if (distance > LIMITE_ALERTA_3_MM) tempo_silencio_ms = 300;
        else if (distance > LIMITE_ALERTA_4_MM) tempo_silencio_ms = 150;
        else if (distance > LIMITE_PARADO_MM)   tempo_silencio_ms = 50;

        // ============================================================
        //            ZONA DE EMERGÊNCIA (<=110mm)
        // ============================================================
        else {
            tempo_silencio_ms = 10;  // Som quase contínuo
            servo_set_angle(90);      // FECHA A CANCELA AQUI TAMBÉM
            display_status = "EMERGÊNCIA / MUITO PERTO";
        }


        // ============================================================
        //                  EXECUÇÃO DO BUZZER
        // ============================================================
        if (tempo_silencio_ms > 0) {
            buzzer_pwm(FREQ_BUZZER, DUTY_CYCLE);
            sleep_ms(BUZZER_DURACAO_MS);
            buzzer_off();
            sleep_ms(tempo_silencio_ms);
        }

        display_show_status(&oled, display_status);
        distance_anterior = distance;
    }
}
