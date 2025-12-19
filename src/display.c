#include "display.h" 
#include "ssd1306_i2c.h"
#include <stdio.h>
#include <string.h>

void display_init(ssd1306_t *oled) {
    i2c_init(I2C_DISPLAY, 400 * 1000);
    gpio_set_function(SDA_DISPLAY, GPIO_FUNC_I2C);
    gpio_set_function(SCL_DISPLAY, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_DISPLAY);
    gpio_pull_up(SCL_DISPLAY);

    ssd1306_init_bm(oled, OLED_WIDTH, OLED_HEIGHT, false, ssd1306_i2c_address, I2C_DISPLAY);
    ssd1306_config(oled);
    ssd1306_init();

    // Limpa e avisa que iniciou
    memset(oled->ram_buffer + 1, 0, oled->bufsize - 1);
    ssd1306_draw_string(oled->ram_buffer + 1, 15, 25, "SISTEMA OK");
    ssd1306_send_data(oled);
}

void display_update_vagas(ssd1306_t *oled, uint16_t d1, uint16_t d2) {
    // 1. Limpa o buffer completamente (Essencial para não travar "SISTEMA INICI")
    memset(oled->ram_buffer + 1, 0, oled->bufsize - 1);

    char txt1[32], txt2[32];
    const char *st1, *st2;

    // Lógica de Status Vaga 1 (VL53L0X)
    if (d1 > 800 || d1 == 65535) st1 = "LIVRE";
    else if (d1 < 150) st1 = "OCUPADA";
    else st1 = "ALERTA";

    // Lógica de Status Vaga 2 (Ultrassônico)
    if (d2 > 800 || d2 == 0) st2 = "LIVRE";
    else if (d2 < 150) st2 = "OCUPADA";
    else st2 = "ALERTA";

    // Formatação das strings
    sprintf(txt1, "V1:%4dmm %s", (d1 == 65535) ? 0 : d1, st1);
    sprintf(txt2, "V2:%4dmm %s", d2, st2);

    // Desenha na tela
    ssd1306_draw_string(oled->ram_buffer + 1, 2, 5, "ESTACIONAMENTO:");
    ssd1306_draw_string(oled->ram_buffer + 1, 2, 20, txt1);
    
    // Linha divisória horizontal
    ssd1306_draw_line(oled->ram_buffer + 1, 0, 35, 127, 35, true);

    ssd1306_draw_string(oled->ram_buffer + 1, 2, 45, txt2);

    // 2. Envia os dados para o display físico
    ssd1306_send_data(oled);
}

// Mantidas apenas para evitar erros de compilação caso chamadas em outro lugar
void display_show_distance(ssd1306_t *oled, uint16_t d) {}
void display_show_status(ssd1306_t *oled, const char *s) {}