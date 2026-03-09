#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>

#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/i2c.h"
#include "pico/bootrom.h"


#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// =========================
// I2C MPU6050
// =========================
#define I2C_PORT i2c0
#define I2C_SDA 0
#define I2C_SCL 1

// =========================
// I2C Display SSD1306
// =========================
#define I2C_PORT_DISP i2c1
#define I2C_SDA_DISP 14
#define I2C_SCL_DISP 15
#define ENDERECO_DISP 0x3C
#define DISP_W 128
#define DISP_H 64

// =========================
// MPU6050
// =========================
static int addr = 0x68;

// Escalas padrão (MPU6050 após reset, normalmente ±2g e ±250 °/s)
#define ACCEL_LSB_PER_G   16384.0f
#define GYRO_LSB_PER_DPS  131.0f

// =========================
// Botão BOOTSEL (BitDogLab / Pico)
// =========================
#define botaoB 6

void gpio_irq_handler(uint gpio, uint32_t events)
{
    (void)gpio;
    (void)events;
    reset_usb_boot(0, 0);
}

void mpu6050_reset(void)
{
    uint8_t buf[] = {0x6B, 0x80}; // reset
    i2c_write_blocking(I2C_PORT, addr, buf, 2, false);
    sleep_ms(100);

    buf[1] = 0x00; // acorda sensor
    i2c_write_blocking(I2C_PORT, addr, buf, 2, false);
    sleep_ms(10);
}

void mpu6050_read_raw(int16_t accel[3], int16_t gyro[3], int16_t *temp)
{
    uint8_t buffer[6];
    uint8_t val;

    // Acelerômetro: 0x3B a 0x40
    val = 0x3B;
    i2c_write_blocking(I2C_PORT, addr, &val, 1, true);
    i2c_read_blocking(I2C_PORT, addr, buffer, 6, false);
    for (int i = 0; i < 3; i++) {
        accel[i] = (int16_t)((buffer[i * 2] << 8) | buffer[(i * 2) + 1]);
    }

    // Giroscópio: 0x43 a 0x48
    val = 0x43;
    i2c_write_blocking(I2C_PORT, addr, &val, 1, true);
    i2c_read_blocking(I2C_PORT, addr, buffer, 6, false);
    for (int i = 0; i < 3; i++) {
        gyro[i] = (int16_t)((buffer[i * 2] << 8) | buffer[(i * 2) + 1]);
    }

    // Temperatura: 0x41 / 0x42
    val = 0x41;
    i2c_write_blocking(I2C_PORT, addr, &val, 1, true);
    i2c_read_blocking(I2C_PORT, addr, buffer, 2, false);
    *temp = (int16_t)((buffer[0] << 8) | buffer[1]);
}

static float mpu_temp_c(int16_t raw_temp)
{
    // Fórmula do datasheet do MPU6050
    return (raw_temp / 340.0f) + 36.53f;
}

// int main()
// {
//     // -------- Botão BOOTSEL --------
//     gpio_init(botaoB);
//     gpio_set_dir(botaoB, GPIO_IN);
//     gpio_pull_up(botaoB);
//     gpio_set_irq_enabled_with_callback(botaoB, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

//     // -------- Serial USB --------
//     stdio_init_all();
//     sleep_ms(5000); // tempo para abrir monitor serial

//     printf("\n=============================================\n");
//     printf("Simulacao IoT Saude - Monitoramento com MPU6050\n");
//     printf("Leitura: acelerometro (g), giroscopio (deg/s), roll/pitch\n");
//     printf("Alertas: movimento brusco / possivel queda (heuristico)\n");
//     printf("=============================================\n\n");

//     // -------- Display SSD1306 --------
//     i2c_init(I2C_PORT_DISP, 400 * 1000);
//     gpio_set_function(I2C_SDA_DISP, GPIO_FUNC_I2C);
//     gpio_set_function(I2C_SCL_DISP, GPIO_FUNC_I2C);
//     gpio_pull_up(I2C_SDA_DISP);
//     gpio_pull_up(I2C_SCL_DISP);

//     ssd1306_t ssd;
//     ssd1306_init(&ssd, DISP_W, DISP_H, false, ENDERECO_DISP, I2C_PORT_DISP);
//     ssd1306_config(&ssd);
//     ssd1306_send_data(&ssd);

//     ssd1306_fill(&ssd, false);
//     ssd1306_send_data(&ssd);

//     // -------- MPU6050 --------
//     i2c_init(I2C_PORT, 400 * 1000);
//     gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
//     gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
//     gpio_pull_up(I2C_SDA);
//     gpio_pull_up(I2C_SCL);

//     bi_decl(bi_2pins_with_func(I2C_SDA, I2C_SCL, GPIO_FUNC_I2C));
//     mpu6050_reset();

//     int16_t aceleracao[3], gyro[3], temp;

//     // ------------------------------
//     // Variáveis do display (seu código)
//     // ------------------------------
//     int center_x_gyro_bar = DISP_W / 2;
//     int bar_width_max = (DISP_W / 2) - 10;

//     int y_pos_gx = 12;
//     int y_pos_gy = 23;
//     int y_pos_gz = 34;

//     float gyro_scale_factor = (float)bar_width_max / 32767.0f;

//     float last_gx = 0.0f, last_gy = 0.0f, last_gz = 0.0f;
//     const float gyro_limiar = 100.0f;

//     int center_x_accel_line = DISP_W / 2;
//     int y_pos_accel_ref = 55;

//     float pixels_per_degree_roll = 0.8f;
//     float pixels_per_degree_pitch = 0.8f;
//     int accel_line_length = 25;

//     float last_roll = 0.0f, last_pitch = 0.0f;
//     const float accel_limiar = 1.0f;

//     // ------------------------------
//     // Variáveis para serial e alertas
//     // ------------------------------
//     uint32_t last_serial_print_ms = 0;
//     const uint32_t serial_period_ms = 250; // imprime a cada 250 ms

//     // Heurísticas simples (ajustáveis para demonstração)
//     const float TH_FREE_FALL_G = 0.55f;       // quase "sem peso" -> possível queda iniciando
//     const float TH_IMPACT_G = 2.20f;          // impacto
//     const float TH_GYRO_BRUSCO_DPS = 180.0f;  // rotação brusca
//     const float TH_TILT_CHANGE_DEG = 35.0f;   // mudança brusca de orientação
//     const uint32_t FALL_WINDOW_MS = 1200;     // janela para associar queda + impacto

//     bool free_fall_armed = false;
//     uint32_t free_fall_start_ms = 0;

//     float prev_roll_alert = 0.0f;
//     float prev_pitch_alert = 0.0f;

//     while (1)
//     {
//         mpu6050_read_raw(aceleracao, gyro, &temp);

//         // ---------- Conversões para valores entendíveis ----------
//         // Acelerômetro em g
//         float ax = aceleracao[0] / ACCEL_LSB_PER_G;
//         float ay = aceleracao[1] / ACCEL_LSB_PER_G;
//         float az = aceleracao[2] / ACCEL_LSB_PER_G;

//         // Giroscópio em graus por segundo (°/s)
//         float gx_dps = gyro[0] / GYRO_LSB_PER_DPS;
//         float gy_dps = gyro[1] / GYRO_LSB_PER_DPS;
//         float gz_dps = gyro[2] / GYRO_LSB_PER_DPS;

//         // Magnitudes úteis para monitoramento
//         float accel_mag_g = sqrtf(ax * ax + ay * ay + az * az);
//         float gyro_mag_dps = sqrtf(gx_dps * gx_dps + gy_dps * gy_dps + gz_dps * gz_dps);

//         // Ângulos (úteis para postura / inclinação)
//         float roll  = atan2f(ay, az) * 180.0f / (float)M_PI;
//         float pitch = atan2f(-ax, sqrtf(ay * ay + az * az)) * 180.0f / (float)M_PI;

//         float temp_c = mpu_temp_c(temp);

//         // ---------- Lógica simples de alerta (demonstração) ----------
//         uint32_t now_ms = to_ms_since_boot(get_absolute_time());

//         float d_roll  = fabsf(roll  - prev_roll_alert);
//         float d_pitch = fabsf(pitch - prev_pitch_alert);

//         bool alerta_movimento_brusco = false;
//         bool alerta_possivel_queda = false;

//         // "Arma" possível queda se entrar em condição parecida com queda livre
//         if (accel_mag_g < TH_FREE_FALL_G) {
//             free_fall_armed = true;
//             free_fall_start_ms = now_ms;
//         }

//         // Se está armado, procura impacto + mudança de orientação dentro da janela
//         if (free_fall_armed) {
//             uint32_t dt = now_ms - free_fall_start_ms;

//             if (dt <= FALL_WINDOW_MS) {
//                 bool impacto = (accel_mag_g > TH_IMPACT_G);
//                 bool giro_brusco = (gyro_mag_dps > TH_GYRO_BRUSCO_DPS);
//                 bool mudou_orientacao = (d_roll > TH_TILT_CHANGE_DEG) || (d_pitch > TH_TILT_CHANGE_DEG);

//                 if (impacto && (giro_brusco || mudou_orientacao)) {
//                     alerta_possivel_queda = true;
//                     free_fall_armed = false; // reinicia após detectar
//                 }
//             } else {
//                 free_fall_armed = false; // expirou a janela
//             }
//         }

//         // Movimento brusco (sem queda)
//         if (!alerta_possivel_queda) {
//             if (gyro_mag_dps > TH_GYRO_BRUSCO_DPS || fabsf(accel_mag_g - 1.0f) > 0.60f) {
//                 alerta_movimento_brusco = true;
//             }
//         }

//         prev_roll_alert = roll;
//         prev_pitch_alert = pitch;

//         // ---------- Atualização do display (mantida) ----------
//         // Aqui mantive sua lógica de atualização por limiar, usando giro em LSB como antes
//         float gx_lsb = (float)gyro[0];
//         float gy_lsb = (float)gyro[1];
//         float gz_lsb = (float)gyro[2];

//         if (fabsf(gx_lsb - last_gx) > gyro_limiar ||
//             fabsf(gy_lsb - last_gy) > gyro_limiar ||
//             fabsf(gz_lsb - last_gz) > gyro_limiar ||
//             fabsf(roll - last_roll) > accel_limiar ||
//             fabsf(pitch - last_pitch) > accel_limiar)
//         {
//             last_gx = gx_lsb;
//             last_gy = gy_lsb;
//             last_gz = gz_lsb;
//             last_roll = roll;
//             last_pitch = pitch;

//             ssd1306_fill(&ssd, false);

//             // --- Seção do Giroscópio ---
//             ssd1306_draw_string(&ssd, "GIRO:", 2, 0);

//             int bar_end_x_gx = center_x_gyro_bar + (int)(gx_lsb * gyro_scale_factor);
//             if (bar_end_x_gx < 0) bar_end_x_gx = 0;
//             if (bar_end_x_gx > DISP_W - 1) bar_end_x_gx = DISP_W - 1;
//             ssd1306_line(&ssd, center_x_gyro_bar, y_pos_gx, bar_end_x_gx, y_pos_gx, true);

//             int bar_end_x_gy = center_x_gyro_bar + (int)(gy_lsb * gyro_scale_factor);
//             if (bar_end_x_gy < 0) bar_end_x_gy = 0;
//             if (bar_end_x_gy > DISP_W - 1) bar_end_x_gy = DISP_W - 1;
//             ssd1306_line(&ssd, center_x_gyro_bar, y_pos_gy, bar_end_x_gy, y_pos_gy, true);

//             int bar_end_x_gz = center_x_gyro_bar + (int)(gz_lsb * gyro_scale_factor);
//             if (bar_end_x_gz < 0) bar_end_x_gz = 0;
//             if (bar_end_x_gz > DISP_W - 1) bar_end_x_gz = DISP_W - 1;
//             ssd1306_line(&ssd, center_x_gyro_bar, y_pos_gz, bar_end_x_gz, y_pos_gz, true);

//             ssd1306_line(&ssd, 0, 42, DISP_W - 1, 42, true);

//             // --- Seção do Acelerômetro ---
//             ssd1306_draw_string(&ssd, "ACEL:", 2, 45);

//             int offset_x_roll = (int)(roll * pixels_per_degree_roll);
//             int line_x_roll = center_x_accel_line + offset_x_roll;
//             if (line_x_roll < 0) line_x_roll = 0;
//             if (line_x_roll > DISP_W - 1) line_x_roll = DISP_W - 1;

//             int offset_y_pitch = (int)(pitch * pixels_per_degree_pitch);
//             int line_y_pitch = y_pos_accel_ref + offset_y_pitch;
//             if (line_y_pitch < 43) line_y_pitch = 43;
//             if (line_y_pitch > DISP_H - 1) line_y_pitch = DISP_H - 1;

//             ssd1306_line(&ssd,
//                          center_x_accel_line - (accel_line_length / 8), y_pos_accel_ref,
//                          center_x_accel_line + (accel_line_length / 8), y_pos_accel_ref, true);
//             ssd1306_line(&ssd,
//                          center_x_accel_line, y_pos_accel_ref - 2,
//                          center_x_accel_line, y_pos_accel_ref + 2, true);

//             ssd1306_line(&ssd,
//                          line_x_roll, y_pos_accel_ref - (accel_line_length / 2) + 5,
//                          line_x_roll, y_pos_accel_ref + (accel_line_length / 2) - 5, true);

//             ssd1306_line(&ssd,
//                          center_x_accel_line - (accel_line_length / 2) + 5, line_y_pitch,
//                          center_x_accel_line + (accel_line_length / 2) - 5, line_y_pitch, true);

//             // Temperatura no display
//             char str_tmp[10];
//             snprintf(str_tmp, sizeof(str_tmp), "%.1fC", temp_c);
//             ssd1306_draw_string(&ssd, str_tmp, 80, 0);

//             ssd1306_send_data(&ssd);
//         }

//         // ---------- Impressão serial (principal para o seminário) ----------
//         if ((now_ms - last_serial_print_ms) >= serial_period_ms) {
//             last_serial_print_ms = now_ms;

//             const char *status = "NORMAL";
//             if (alerta_possivel_queda) {
//                 status = "ALERTA: POSSIVEL QUEDA";
//             } else if (alerta_movimento_brusco) {
//                 status = "ALERTA: MOVIMENTO BRUSCO";
//             }

//             printf(
//                 "[%8lu ms] "
//                 "TEMP=%.2f C | "
//                 "ACC[g] X=% .2f Y=% .2f Z=% .2f | |A|=% .2f g | "
//                 "GYRO[dps] X=% .1f Y=% .1f Z=% .1f | |G|=% .1f | "
//                 "ROLL=% .1f deg PITCH=% .1f deg | %s\n",
//                 (unsigned long)now_ms,
//                 temp_c,
//                 ax, ay, az, accel_mag_g,
//                 gx_dps, gy_dps, gz_dps, gyro_mag_dps,
//                 roll, pitch,
//                 status
//             );

//             // (Opcional) bruto para calibração:
//             // printf("RAW ACC: %d %d %d | RAW GYRO: %d %d %d | RAW TEMP: %d\n",
//             //        aceleracao[0], aceleracao[1], aceleracao[2],
//             //        gyro[0], gyro[1], gyro[2], temp);
//         }

//         sleep_ms(50);
//     }
// }