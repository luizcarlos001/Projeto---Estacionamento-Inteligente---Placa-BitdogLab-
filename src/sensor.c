#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/pwm.h"
#include "vl53l0x.h"

#include "sensor.h"

// =======================================================
// === CONTROLE DE PWM PARA BUZZER =======================
// =======================================================

void buzzer_pwm(uint16_t freq_hz, float duty) {

    gpio_set_function(BUZZER_PWM, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(BUZZER_PWM);

    uint32_t clock = 125000000;
    float divider = 100.0f;
    uint32_t top = (clock / divider) / freq_hz;

    pwm_set_clkdiv(slice, divider);
    pwm_set_wrap(slice, top);

    pwm_set_gpio_level(BUZZER_PWM, (uint16_t)(top * duty));
    pwm_set_enabled(slice, true);
}

void buzzer_off(void) {
    uint slice = pwm_gpio_to_slice_num(BUZZER_PWM);
    pwm_set_enabled(slice, false);
    gpio_put(BUZZER_PWM, 0);
    gpio_set_function(BUZZER_PWM, GPIO_FUNC_SIO);
    gpio_set_dir(BUZZER_PWM, GPIO_OUT);
}


// =======================================================
// === INICIALIZAÇÃO DO VL53L0X + LEDS E BUZZER ===========
// =======================================================

void sensor_init(vl53l0x_dev *sensor_dev) {

    // ---- Inicializa I2C0 (pinos 0 e 1) ----
    i2c_init(I2C_SENSOR, 50 * 1000);
    gpio_set_function(SDA_SENSOR, GPIO_FUNC_I2C);
    gpio_set_function(SCL_SENSOR, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_SENSOR);
    gpio_pull_up(SCL_SENSOR);

    // ---- Inicializa LEDs ----
    gpio_init(LED_VERDE);
    gpio_init(LED_VERMELHO);
    gpio_set_dir(LED_VERDE, GPIO_OUT);
    gpio_set_dir(LED_VERMELHO, GPIO_OUT);

    gpio_put(LED_VERDE, 1);
    gpio_put(LED_VERMELHO, 0);

    // ---- Inicializa Buzzer ----
    gpio_init(BUZZER_PWM);
    gpio_set_dir(BUZZER_PWM, GPIO_OUT);

    sleep_ms(500);

    // ---- Inicializa sensor ----
    if (!vl53l0x_init(sensor_dev, I2C_SENSOR)) {
        printf(" Erro ao inicializar VL53L0X!\n");
        while (1);
    }

    vl53l0x_start_continuous(sensor_dev, 0);

    printf(" Sensor VL53L0X inicializado (I2C0)!\n");
}


// =======================================================
// === LEITURA DE DISTÂNCIA ===============================
// =======================================================

uint16_t sensor_read_distance(vl53l0x_dev *sensor_dev) {
    return vl53l0x_read_range_continuous_millimeters(sensor_dev);
}
