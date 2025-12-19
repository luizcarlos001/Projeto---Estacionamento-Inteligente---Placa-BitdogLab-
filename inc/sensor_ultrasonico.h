#ifndef SENSOR_ULTRASONICO_H
#define SENSOR_ULTRASONICO_H

// ================= CONFIGURAÇÃO =================
#define TRIG_PIN 18
#define ECHO_PIN 19

// ================= API =================
void sensor_ultrasonico_init(void);
float sensor_ultrasonico_ler_distancia_cm(void);

#endif
    