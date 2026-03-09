#include "ds18b20.h"

#include <string.h>

// =========================
// Comandos 1-Wire / DS18B20
// =========================
#define ONEWIRE_CMD_SKIP_ROM        0xCC
#define DS18B20_CMD_CONVERT_T       0x44
#define DS18B20_CMD_READ_SCRATCHPAD 0xBE

// =========================
// Helpers de baixo nível 1-Wire
// =========================

// Simula open-drain: puxa para LOW colocando como saída em 0
static inline void ow_drive_low(uint pin) {
    gpio_put(pin, 0);
    gpio_set_dir(pin, GPIO_OUT);
}

// Libera a linha: coloca como entrada (pull-up externo mantém em HIGH)
static inline void ow_release(uint pin) {
    gpio_set_dir(pin, GPIO_IN);
}

// Lê estado da linha
static inline bool ow_read_line(uint pin) {
    return gpio_get(pin) ? true : false;
}

// Reset + presença
// Retorna true se detectou presença de dispositivo
static bool ow_reset_pulse(uint pin) {
    // Mestre segura LOW por ~480us
    ow_drive_low(pin);
    sleep_us(480);

    // Libera e espera janela de presença
    ow_release(pin);
    sleep_us(70);

    // Dispositivo responde puxando LOW (presence pulse)
    bool presence = !ow_read_line(pin);

    // Completa a janela de reset
    sleep_us(410);

    return presence;
}

static void ow_write_bit(uint pin, bool bit) {
    if (bit) {
        // Escreve '1': LOW curto e libera
        ow_drive_low(pin);
        sleep_us(6);
        ow_release(pin);
        sleep_us(64);
    } else {
        // Escreve '0': LOW por mais tempo
        ow_drive_low(pin);
        sleep_us(60);
        ow_release(pin);
        sleep_us(10);
    }
}

static bool ow_read_bit(uint pin) {
    bool bit;

    // Mestre inicia slot de leitura
    ow_drive_low(pin);
    sleep_us(6);

    // Libera e aguarda um pouco para amostrar
    ow_release(pin);
    sleep_us(9);

    bit = ow_read_line(pin);

    // Completa o slot (~60us total)
    sleep_us(55);

    return bit;
}

static void ow_write_byte(uint pin, uint8_t value) {
    for (int i = 0; i < 8; i++) {
        ow_write_bit(pin, (value >> i) & 0x01u); // LSB first
    }
}

static uint8_t ow_read_byte(uint pin) {
    uint8_t value = 0;
    for (int i = 0; i < 8; i++) {
        if (ow_read_bit(pin)) {
            value |= (1u << i); // LSB first
        }
    }
    return value;
}

// CRC-8 Dallas/Maxim (polinômio 0x31, refletido 0x8C)
static uint8_t ds18b20_crc8(const uint8_t *data, size_t len) {
    uint8_t crc = 0;
    for (size_t i = 0; i < len; i++) {
        uint8_t inbyte = data[i];
        for (int j = 0; j < 8; j++) {
            uint8_t mix = (crc ^ inbyte) & 0x01u;
            crc >>= 1;
            if (mix) crc ^= 0x8Cu;
            inbyte >>= 1;
        }
    }
    return crc;
}

// =========================
// API pública
// =========================

void ds18b20_init(uint pin) {
    gpio_init(pin);

    // Mantém linha liberada em HIGH (ideal: resistor externo 4.7k)
    gpio_pull_up(pin);

    // Começa em modo entrada (linha solta)
    ow_release(pin);
}

bool ds18b20_start_conversion(uint pin) {
    if (!ow_reset_pulse(pin)) {
        return false;
    }

    ow_write_byte(pin, ONEWIRE_CMD_SKIP_ROM);   // único sensor no barramento
    ow_write_byte(pin, DS18B20_CMD_CONVERT_T);  // inicia conversão

    return true;
}

bool ds18b20_is_conversion_done(uint pin) {
    // Durante conversão, o DS18B20 retorna 0 no read slot; ao terminar, retorna 1
    return ow_read_bit(pin);
}

bool ds18b20_read_temp_c_after_conversion(uint pin, float *temp_c) {
    if (temp_c == NULL) return false;

    uint8_t scratch[9];

    if (!ow_reset_pulse(pin)) {
        return false;
    }

    ow_write_byte(pin, ONEWIRE_CMD_SKIP_ROM);
    ow_write_byte(pin, DS18B20_CMD_READ_SCRATCHPAD);

    for (int i = 0; i < 9; i++) {
        scratch[i] = ow_read_byte(pin);
    }

    // Valida CRC (bytes 0..7 -> byte 8)
    uint8_t crc_calc = ds18b20_crc8(scratch, 8);
    if (crc_calc != scratch[8]) {
        return false;
    }

    // Temperatura bruta (16 bits, signed, LSB first)
    int16_t raw = (int16_t)((scratch[1] << 8) | scratch[0]);

    // Ajusta bits inválidos de acordo com a resolução configurada
    // Config register = scratch[4], bits R1:R0 = [6:5]
    uint8_t resolution_cfg = scratch[4] & 0x60;
    switch (resolution_cfg) {
        case 0x00: // 9-bit
            raw &= ~0x0007;
            break;
        case 0x20: // 10-bit
            raw &= ~0x0003;
            break;
        case 0x40: // 11-bit
            raw &= ~0x0001;
            break;
        case 0x60: // 12-bit
        default:
            // sem ajuste
            break;
    }

    // DS18B20: 1 LSB = 0.0625°C em 12 bits
    *temp_c = (float)raw / 16.0f;

    return true;
}

bool ds18b20_read_temp_c(uint pin, float *temp_c) {
    if (temp_c == NULL) return false;

    // 1) Inicia conversão
    if (!ds18b20_start_conversion(pin)) {
        return false;
    }

    // 2) Espera terminar (timeout ~1000ms)
    //    Conversão em 12 bits leva até ~750ms
    absolute_time_t deadline = make_timeout_time_ms(1000);

    while (!ds18b20_is_conversion_done(pin)) {
        if (absolute_time_diff_us(get_absolute_time(), deadline) <= 0) {
            // timeout
            return false;
        }
        sleep_ms(5);
    }

    // 3) Lê scratchpad e converte
    return ds18b20_read_temp_c_after_conversion(pin, temp_c);
}