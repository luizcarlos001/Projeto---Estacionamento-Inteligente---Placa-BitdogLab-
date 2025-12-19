#include "display.h" 
#include "ssd1306_i2c.h"
#include <stdio.h>
#include <string.h>

// Definição dos limites de área da tela
#define STATUS_LINE_HEIGHT 1 // Número de linhas (páginas) do OLED usadas para o status (8 pixels)
#define STATUS_AREA_SIZE (OLED_WIDTH * STATUS_LINE_HEIGHT) // 128 bytes para limpar a primeira linha

void display_init(ssd1306_t *oled) {
    i2c_init(I2C_DISPLAY, 400 * 1000);
    gpio_set_function(SDA_DISPLAY, GPIO_FUNC_I2C);
    gpio_set_function(SCL_DISPLAY, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_DISPLAY);
    gpio_pull_up(SCL_DISPLAY);

    ssd1306_init_bm(oled, OLED_WIDTH, OLED_HEIGHT, false, ssd1306_i2c_address, I2C_DISPLAY);
    ssd1306_config(oled);
    ssd1306_init();

    ssd1306_draw_string(oled->ram_buffer + 1, 10, 25, "Display OK!");
    ssd1306_send_data(oled);
    printf("[DISPLAY] SSD1306 inicializado.\n");
}

void display_show_distance(ssd1306_t *oled, uint16_t distance) {
    // Limpa apenas a área da tela APÓS a linha de status
    // Para que o status (VAGA LIVRE/OCUPADA) não seja apagado por esta função.
    memset(oled->ram_buffer + 1 + STATUS_AREA_SIZE, 0, oled->bufsize - 1 - STATUS_AREA_SIZE);

    char texto[32]; 

    if (distance == 65535) {
        // Texto neutro para erro de leitura
        sprintf(texto, "Distancia: ---- mm"); 
    } else if (distance > 8000) {
        // Texto neutro para fora de alcance
        sprintf(texto, "Distancia: +8000 mm"); 
    } else {
        // Exibe a distância normalmente
        sprintf(texto, "Distancia: %4d mm", distance);
    }
    
    ssd1306_draw_string(oled->ram_buffer + 1, 10, 20, texto);

    // Lógica da barra de progresso
    int barra = (distance < 400) ? (400 - distance) / 3 : 0; // Inverte o valor para encher a barra ao se aproximar
    barra = (barra > 120) ? 120 : barra; // Limita o tamanho máximo da barra

    for (int x = 5; x < 5 + barra; x++)
        for (int y = 45; y < 52; y++)
            ssd1306_set_pixel(oled->ram_buffer + 1, x, y, 1);
}

// Função para exibir o status (Vaga Livre/Ocupada)
void display_show_status(ssd1306_t *oled, const char *status_text) {
    // Limpa apenas a primeira linha (Page 0)
    memset(oled->ram_buffer + 1, 0, STATUS_AREA_SIZE);
    
    // Desenha o status na linha superior (posição y=0)
    ssd1306_draw_string(oled->ram_buffer + 1, 5, 0, status_text); 

    // Envia os dados para o display
    ssd1306_send_data(oled);
}