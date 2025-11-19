#ifndef SENSOR_H
#define SENSOR_H

#include "hardware/i2c.h"
#include "vl53l0x.h"

// Sensor VL53L0X -> I2C0 (GPIO0 SDA, GPIO1 SCL)
#define I2C_SENSOR i2c0
#define SDA_SENSOR 0
#define SCL_SENSOR 1

// Buzzer PWM correto (BitDogLab -> BUZZER B = GPIO10)
#define BUZZER_PWM 10

// LEDs da BitDogLab
#define LED_VERDE 11
#define LED_VERMELHO 13

void sensor_init(vl53l0x_dev *sensor_dev);
uint16_t sensor_read_distance(vl53l0x_dev *sensor_dev);

// Controle do buzzer
void buzzer_pwm(uint16_t freq, float duty);
void buzzer_off(void);

#endif
