#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "sensor_ultrasonico.h"

// Velocidade do som: 340 m/s = 0.034 cm/us
#define SOUND_SPEED_CM_US 0.034f

// Timeout para evitar travamento (em microssegundos)
#define ECHO_TIMEOUT_US 30000  // ~5 metros

// ================= INIT =================
void sensor_ultrasonico_init(void) {
    gpio_init(TRIG_PIN);
    gpio_set_dir(TRIG_PIN, GPIO_OUT);
    gpio_put(TRIG_PIN, 0);

    gpio_init(ECHO_PIN);
    gpio_set_dir(ECHO_PIN, GPIO_IN);

    sleep_ms(50);
}

// ================= LEITURA =================
float sensor_ultrasonico_ler_distancia_cm(void) {
    absolute_time_t inicio, fim, timeout;

    // Pulso de trigger
    gpio_put(TRIG_PIN, 0);
    sleep_us(2);
    gpio_put(TRIG_PIN, 1);
    sleep_us(10);
    gpio_put(TRIG_PIN, 0);

    // Aguarda ECHO subir (com timeout)
    timeout = make_timeout_time_us(ECHO_TIMEOUT_US);
    while (gpio_get(ECHO_PIN) == 0) {
        if (time_reached(timeout)) {
            return -1.0f; // erro / fora de alcance
        }
    }
    inicio = get_absolute_time();

    // Aguarda ECHO descer (com timeout)
    timeout = make_timeout_time_us(ECHO_TIMEOUT_US);
    while (gpio_get(ECHO_PIN) == 1) {
        if (time_reached(timeout)) {
            return -1.0f;
        }
    }
    fim = get_absolute_time();

    int64_t tempo_us = absolute_time_diff_us(inicio, fim);

    return (tempo_us * SOUND_SPEED_CM_US) / 2.0f;
}
