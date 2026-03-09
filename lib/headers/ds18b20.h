#ifndef DS18B20_H
#define DS18B20_H

#include <stdbool.h>
#include <stdint.h>
#include "pico/stdlib.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Inicializa o pino do DS18B20.
 * Recomendado usar resistor pull-up externo de 4.7k no pino de dados.
 */
void ds18b20_init(uint pin);

/**
 * Leitura completa (bloqueante): inicia conversão, espera terminar e lê a temperatura.
 * Pode levar até ~750 ms (resolução 12 bits).
 *
 * Retorna true se a leitura foi bem-sucedida.
 * Retorna false em caso de falha (sem presença, timeout, CRC inválido, etc.).
 */
bool ds18b20_read_temp_c(uint pin, float *temp_c);

/**
 * Inicia a conversão de temperatura (não bloqueante).
 * Depois use ds18b20_is_conversion_done() e ds18b20_read_temp_c_after_conversion().
 */
bool ds18b20_start_conversion(uint pin);

/**
 * Verifica se a conversão terminou (não bloqueante).
 * true = terminou, false = ainda convertendo
 */
bool ds18b20_is_conversion_done(uint pin);

/**
 * Lê a temperatura após a conversão já ter sido concluída.
 * Faz validação de CRC do scratchpad.
 */
bool ds18b20_read_temp_c_after_conversion(uint pin, float *temp_c);

#ifdef __cplusplus
}
#endif

#endif // DS18B20_H