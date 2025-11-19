#include <stdio.h>
#include "pico/stdlib.h"
#include "sensor.h"
#include "display.h"

int main() {
    stdio_init_all();
    sleep_ms(1500);
    printf("=== Sistema VL53L0X + SSD1306 Modular - Estacionamento V8 (Funcionalidade Completa, Display Simples) ===\n");

    // Inicializa o sensor e o display
    vl53l0x_dev sensor_dev;
    sensor_init(&sensor_dev);

    ssd1306_t oled;
    display_init(&oled);

    // --- VARIÁVEIS DE CONTROLE DE MOVIMENTO E DISTÂNCIA ---
    uint16_t distance_anterior = 0; 
    const uint16_t TOLERANCIA_MOVIMENTO_MM = 5; // Tolerância para considerar 'parado'
    
    // --- PARÂMETROS DE ALARME PARA ESPAÇO PEQUENO ---
    // Distâncias em milímetros (mm)
    const uint16_t LIMITE_PARADO_MM = 110;       // Buzzer PÁRA aqui (Estacionado).
    const uint16_t LIMITE_ALERTA_4_MM = 140;     // Bipe MUITO RÁPIDO (Zona Crítica)
    const uint16_t LIMITE_ALERTA_3_MM = 190;     // Bipe RÁPIDO
    const uint16_t LIMITE_ALERTA_2_MM = 240;     // Bipe MÉDIO
    const uint16_t LIMITE_ALERTA_1_MM = 300;     // Inicia o buzzer (Alerta Suave).
    const uint16_t LIMITE_VAGA_LIVRE_MM = 400;   // Acima disso, Vaga Livre.
    
    // Parâmetros do Buzzer
    const uint16_t BUZZER_DURACAO_MS = 50; 
    const uint16_t FREQ_BUZZER = 1000;
    const float DUTY_CYCLE = 0.5f;

    // Loop principal
    while (true) {
        uint16_t distance = sensor_read_distance(&sensor_dev);
        printf("Distancia: %d mm\n", distance);
        display_show_distance(&oled, distance); // Exibe distância e barra

        uint16_t tempo_silencio_ms = 0;
        // O status é "VAGA LIVRE" por padrão e só muda para "OCUPADA" se estiver parado.
        const char *display_status = "VAGA LIVRE"; 
        
        // --- 1. DETECÇÃO DE MOVIMENTO/PARADA (Prioridade) ---
        if (distance <= LIMITE_PARADO_MM) {
            
            // Verifica se o carro está efetivamente parado
            uint16_t diff = (distance > distance_anterior) ? (distance - distance_anterior) : (distance_anterior - distance);

            if (diff < TOLERANCIA_MOVIMENTO_MM) {
                // CARRO PARADO (Vaga Ocupada Estática)
                buzzer_off();
                gpio_put(LED_VERMELHO, 1); // Vermelho ON Sólido
                gpio_put(LED_VERDE, 0);
                distance_anterior = distance;
                display_status = "VAGA OCUPADA"; // <<< ÚNICO STATUS DE OCUPADA
                
                display_show_status(&oled, display_status);
                sleep_ms(150);
                continue; 
            }
        }
        
        // --- 2. LÓGICA DO ALARME EM MOVIMENTO (Todas as zonas de alerta mantêm status "VAGA LIVRE") ---
        
        // 2.1. VAGA LIVRE (Acima de 400mm)
        if (distance > LIMITE_VAGA_LIVRE_MM) {
            buzzer_off();
            gpio_put(LED_VERMELHO, 0); 
            gpio_put(LED_VERDE, 1);    // Verde ON
            sleep_ms(100); 

        // 2.2. PRÉ-ALERTA (400mm a 301mm) - Sem som
        } else if (distance > LIMITE_ALERTA_1_MM) {
            buzzer_off();
            gpio_put(LED_VERMELHO, 0); 
            gpio_put(LED_VERDE, 0); 
            sleep_ms(100); 

        // 2.3. ALERTA SUAVE (300mm a 241mm) - Início do Som
        } else if (distance > LIMITE_ALERTA_2_MM) {
            tempo_silencio_ms = 600; // Cadência MUITO LENTA
            gpio_put(LED_VERMELHO, 0); 
            gpio_put(LED_VERDE, 0); 

        // 2.4. ALERTA MÉDIO (240mm a 191mm)
        } else if (distance > LIMITE_ALERTA_3_MM) {
            tempo_silencio_ms = 300; // Cadência MÉDIA
            gpio_put(LED_VERMELHO, 1); 
            gpio_put(LED_VERDE, 0);

        // 2.5. ALERTA RÁPIDO (190mm a 141mm)
        } else if (distance > LIMITE_ALERTA_4_MM) {
            tempo_silencio_ms = 150; // Cadência RÁPIDA
            gpio_put(LED_VERMELHO, 1); 
            gpio_put(LED_VERDE, 0);

        // 2.6. ALERTA CRÍTICO (140mm até 111mm)
        } else if (distance > LIMITE_PARADO_MM) {
            tempo_silencio_ms = 50; // Cadência MUITO RÁPIDA
            gpio_put(LED_VERMELHO, 1); 
            gpio_put(LED_VERDE, 0);
        
        // 2.7. EMERGÊNCIA (Abaixo de 110mm, mas ainda em movimento)
        } else {
            tempo_silencio_ms = 10; // Som contínuo
            gpio_put(LED_VERMELHO, 1); 
            gpio_put(LED_VERDE, 0);
        }
        
        // --- 3. EXECUÇÃO DO BIPE E ATUALIZAÇÃO DO STATUS NO DISPLAY ---
        if (tempo_silencio_ms > 0) {
            buzzer_pwm(FREQ_BUZZER, DUTY_CYCLE);
            sleep_ms(BUZZER_DURACAO_MS);
            
            buzzer_off();
            sleep_ms(tempo_silencio_ms); 
        }

        // Atualiza o texto de status no display (VAGA LIVRE ou VAGA OCUPADA)
        display_show_status(&oled, display_status);
        
        // --- 4. ATUALIZAÇÃO DA DISTÂNCIA ANTERIOR ---
        distance_anterior = distance;
    }
    return 0;
}