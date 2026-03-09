#include <stdio.h>
#include <stdbool.h>
#include <math.h>

#include "pico/stdlib.h"
#include "pico/bootrom.h"    // reset_usb_boot()
#include "pico/cyw43_arch.h" // Wi-Fi Pico W
#include "hardware/i2c.h"

#include "mpu6050.h" // <- header do MPU (lib/headers/mpu6050.h)
#include "ds18b20.h"

#define DS18B20_PIN 4

// =========================
// Configuração Wi-Fi
// =========================
#define WIFI_SSID "Bruna"
#define WIFI_PASSWORD "12345678"

// =========================
// I2C / MPU6050
// =========================
#define I2C_PORT i2c0
#define I2C_SDA 0
#define I2C_SCL 1

#define ACCEL_LSB_PER_G 16384.0f // ±2g
#define GYRO_LSB_PER_DPS 131.0f  // ±250°/s

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// =========================
// Botões
// =========================
#define BUTTON_BOOTSEL 6 // Botão B
#define BUTTON_USER 5    // Botão A (opcional)

// Estado de botão (opcional)
volatile bool button_user_pressed = false;

// =========================
// Protótipos
// =========================
void button_init(uint pin);
void gpio_irq_callback(uint gpio, uint32_t events);
bool wifi_connect_blocking(void);
void sensor_init_after_wifi(void);
float mpu_temp_c(int16_t raw_temp);

int main(void)
{
    stdio_init_all();
    sleep_ms(1500); // tempo para serial USB subir

    // Inicializa botões
    button_init(BUTTON_BOOTSEL);
    button_init(BUTTON_USER);

    // Callback de interrupção para os botões
    gpio_set_irq_enabled_with_callback(BUTTON_BOOTSEL, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_callback);
    gpio_set_irq_enabled(BUTTON_USER, GPIO_IRQ_EDGE_FALL, true);

    printf("Inicializando projeto de rede + sensor (Pico W)...\n");

    // =========================
    // 1) Inicializa Wi-Fi
    // =========================
    if (cyw43_arch_init())
    {
        printf("ERRO: falha ao inicializar cyw43_arch.\n");
        while (true)
        {
            sleep_ms(1000);
        }
    }

    cyw43_arch_enable_sta_mode();

    if (!wifi_connect_blocking())
    {
        printf("ERRO: nao foi possivel conectar ao Wi-Fi.\n");
        while (true)
        {
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
            sleep_ms(200);
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
            sleep_ms(800);
        }
    }

    printf("Wi-Fi conectado. Agora inicializando MPU6050...\n");

    // =========================
    // 2) Inicializa sensor (depois da rede)
    // =========================
    sensor_init_after_wifi();
    mpu6050_reset();

    printf("MPU6050 pronto. Iniciando monitoramento...\n\n");

    int16_t accel_raw[3], gyro_raw[3], temp_raw;

    uint32_t last_print_ms = 0;
    uint32_t last_led_ms = 0;
    bool led_state = false;

    // Variáveis para detecção simples (simulação)
    float prev_roll = 0.0f, prev_pitch = 0.0f;
    bool first_read = true;

    float temp_corpo_c = -999.0f; // temperatura do DS18B20 (estimativa corporal)
    bool ds_ok = false;

    uint32_t last_ds_read_ms = 0;
    const uint32_t ds_interval_ms = 1000; // lê a cada 1s (bom pra serial)

    ds18b20_init(DS18B20_PIN);
    printf("DS18B20 inicializado no GPIO %d.\n", DS18B20_PIN);

    while (true)
    {
        // Se seu projeto usa pico_cyw43_arch_poll em vez de threadsafe_background,
        // descomente a linha abaixo:
        // cyw43_arch_poll();

        // Lê sensor
        mpu6050_read_raw(accel_raw, gyro_raw, &temp_raw);

        // Converte para valores entendíveis
        float ax = accel_raw[0] / ACCEL_LSB_PER_G;
        float ay = accel_raw[1] / ACCEL_LSB_PER_G;
        float az = accel_raw[2] / ACCEL_LSB_PER_G;

        float gx = gyro_raw[0] / GYRO_LSB_PER_DPS; // °/s
        float gy = gyro_raw[1] / GYRO_LSB_PER_DPS;
        float gz = gyro_raw[2] / GYRO_LSB_PER_DPS;

        float temp_c = mpu_temp_c(temp_raw);

        float accel_mag = sqrtf(ax * ax + ay * ay + az * az);
        float gyro_mag = sqrtf(gx * gx + gy * gy + gz * gz);

        float roll = atan2f(ay, az) * 180.0f / (float)M_PI;
        float pitch = atan2f(-ax, sqrtf(ay * ay + az * az)) * 180.0f / (float)M_PI;

        float droll = 0.0f, dpitch = 0.0f;
        if (!first_read)
        {
            droll = fabsf(roll - prev_roll);
            dpitch = fabsf(pitch - prev_pitch);
        }
        prev_roll = roll;
        prev_pitch = pitch;
        first_read = false;

        // Alertas heurísticos (simulação de monitoramento)
        bool alerta_movimento_brusco = (gyro_mag > 180.0f) || (fabsf(accel_mag - 1.0f) > 0.6f);
        bool alerta_possivel_queda = (accel_mag > 2.2f && (droll > 35.0f || dpitch > 35.0f));

        // Impressão serial a cada 250 ms (pra ficar legível)
        uint32_t now = to_ms_since_boot(get_absolute_time());

        // Leitura do DS18B20 a cada 1 segundo
        if ((now - last_ds_read_ms) >= ds_interval_ms)
        {
            last_ds_read_ms = now;

            ds_ok = ds18b20_read_temp_c(DS18B20_PIN, &temp_corpo_c);

            if (!ds_ok)
            {
                printf("[DS18B20] Falha na leitura (GPIO %d)\n", DS18B20_PIN);
            }
        }
        if ((now - last_print_ms) >= 250)
        {
            last_print_ms = now;

            const char *status = "NORMAL";
            if (alerta_possivel_queda)
            {
                status = "ALERTA: POSSIVEL QUEDA";
            }
            else if (alerta_movimento_brusco)
            {
                status = "ALERTA: MOVIMENTO BRUSCO";
            }

            printf("\n========================================\n");
            printf("MONITORAMENTO DO PACIENTE (IoT Saude)\n");
            printf("Tempo: %lu ms\n", (unsigned long)now);
            printf("----------------------------------------\n");

            // Temperaturas
            printf("Temp MPU6050 (placa/sensor): %.2f C\n", temp_c);

            if (ds_ok)
            {
                printf("Temp DS18B20 (contato/estimativa corporal): %.2f C\n", temp_corpo_c);
            }
            else
            {
                printf("Temp DS18B20 (contato/estimativa corporal): ---\n");
            }

            printf("----------------------------------------\n");

            // Acelerometro
            printf("ACC [g]\n");
            printf("  X: % .2f | Y: % .2f | Z: % .2f | |A|: %.2f g\n", ax, ay, az, accel_mag);

            // Giroscopio
            printf("GYRO [deg/s]\n");
            printf("  X: % .1f | Y: % .1f | Z: % .1f | |G|: %.1f\n", gx, gy, gz, gyro_mag);

            // Orientacao
            printf("ORIENTACAO\n");
            printf("  Roll: % .1f deg | Pitch: % .1f deg\n", roll, pitch);

            printf("----------------------------------------\n");
            printf("STATUS: %s\n", status);
            printf("========================================\n\n");
        }

        // LED onboard piscando (conectado)
        if ((now - last_led_ms) >= 1000)
        {
            last_led_ms = now;
            led_state = !led_state;
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, led_state);
        }

        // Botão A (debug)
        if (button_user_pressed)
        {
            button_user_pressed = false;
            printf("Botao A pressionado.\n");
        }

        sleep_ms(50);
    }

    // (normalmente nunca chega aqui)
    cyw43_arch_deinit();
    return 0;
}

// =========================
// Wi-Fi
// =========================
bool wifi_connect_blocking(void)
{
    printf("Conectando ao Wi-Fi SSID: %s\n", WIFI_SSID);

    int err = cyw43_arch_wifi_connect_timeout_ms(
        WIFI_SSID,
        WIFI_PASSWORD,
        CYW43_AUTH_WPA2_AES_PSK,
        30000);

    if (err)
    {
        printf("Falha na conexao Wi-Fi. Codigo de erro: %d\n", err);
        return false;
    }

    printf("Wi-Fi conectado com sucesso!\n");
    return true;
}

// =========================
// Sensor (inicializa I2C do MPU)
// =========================
void sensor_init_after_wifi(void)
{
    i2c_init(I2C_PORT, 400 * 1000);

    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);

    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    printf("I2C0 inicializado para MPU6050 (GPIO %d/%d).\n", I2C_SDA, I2C_SCL);
}

// Fórmula correta do MPU6050
float mpu_temp_c(int16_t raw_temp)
{
    return (raw_temp / 340.0f) + 36.53f;
}

// =========================
// Funções auxiliares
// =========================
void button_init(uint pin)
{
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_IN);
    gpio_pull_up(pin);
}

void gpio_irq_callback(uint gpio, uint32_t events)
{
    (void)events;

    static uint32_t last_time_us = 0;
    uint32_t now_us = to_us_since_boot(get_absolute_time());

    // Debounce simples (200 ms)
    if ((now_us - last_time_us) < 200000)
    {
        return;
    }
    last_time_us = now_us;

    if (gpio == BUTTON_USER)
    {
        button_user_pressed = true;
    }

    if (gpio == BUTTON_BOOTSEL)
    {
        printf("Entrando em modo BOOTSEL...\n");
        reset_usb_boot(0, 0);
    }
}