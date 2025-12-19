#ifndef PARKING_STATE_H
#define PARKING_STATE_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    bool ocupada;
    uint32_t tempo_ocupada_ms;
} vaga_status_t;

extern vaga_status_t vaga1_status;
extern vaga_status_t vaga2_status;

// parking_state.h - Adicione esta linha no final da struct ou como extern
extern bool localizar_vaga1;
extern bool localizar_vaga2;

#endif
