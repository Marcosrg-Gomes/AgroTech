#pragma once
/*
 * config.h — todos os parâmetros ajustáveis do projeto em um só lugar.
 * Para mudar limites, pinos ou intervalos, edite apenas este arquivo.
 */
#include <Arduino.h>

namespace cfg {

// ----------------------------- Pinos -----------------------------
// Lado esquerdo do ESP32 = sensores | lado direito = atuadores
constexpr uint8_t PIN_DHT          = 32;   // DHT22 (dados)
constexpr uint8_t PIN_LDR          = 34;   // LDR (analógico, ADC1 — ADC2 não funciona com Wi-Fi)
constexpr uint8_t PIN_BUZZER       = 18;
constexpr uint8_t PIN_LED_RED      = 17;
constexpr uint8_t PIN_LED_YELLOW   = 16;
constexpr uint8_t PIN_LED_GREEN    = 4;

constexpr uint8_t  BUZZER_CHANNEL  = 0;    // canal LEDC (core 2.x)
constexpr uint16_t BUZZER_FREQ_HZ  = 2000;

// ------------------- Limites agronômicos (ajuste à cultura) -------------------
constexpr float TEMP_MIN = 18.0f,  TEMP_MAX = 30.0f;    // faixa ideal (°C)
constexpr float TEMP_CRIT_MIN = 10.0f, TEMP_CRIT_MAX = 35.0f;
constexpr float HUM_MIN  = 50.0f,  HUM_MAX  = 80.0f;    // faixa ideal (%)
constexpr float HUM_CRIT_MIN = 30.0f,  HUM_CRIT_MAX = 90.0f;
constexpr int   LIGHT_MIN_PCT = 20;                     // abaixo = pouca luz (%)

// Histerese: para SAIR de um alarme o valor precisa recuar esta margem
constexpr float HYST_TEMP  = 0.5f;   // °C
constexpr float HYST_HUM   = 2.0f;   // % UR
constexpr int   HYST_LIGHT = 3;      // %

// Confirmação: nº de leituras seguidas necessárias para mudar de estado
constexpr uint8_t CONFIRM_READINGS = 2;

// ----------------------------- Sensores -----------------------------
// No Wokwi o módulo LDR retorna valor MAIOR no escuro. Se o seu ficar invertido, use false.
constexpr bool    LDR_DARK_IS_HIGH   = true;
constexpr uint8_t LDR_OVERSAMPLE     = 16;     // média de N leituras (reduz ruído)
constexpr uint8_t DHT_FAILS_TO_FAULT = 3;      // falhas seguidas até declarar "falha de sensor"

// ----------------------------- Tempos (ms) -----------------------------
constexpr uint32_t SAMPLE_INTERVAL_MS = 2000;   // DHT22 aceita no mínimo 2 s
constexpr uint32_t CLOUD_INTERVAL_MS  = 20000;  // envio periódico (ThingSpeak free: > 15 s)
constexpr uint32_t CLOUD_MIN_GAP_MS   = 16000;  // intervalo mínimo entre envios (inclui envio por mudança de estado)
constexpr uint32_t CLOUD_RETRY_BASE_MS = 5000;  // backoff: 5 s, 10 s, 20 s... até CLOUD_RETRY_MAX_MS
constexpr uint32_t CLOUD_RETRY_MAX_MS  = 60000;
constexpr uint32_t WIFI_RETRY_MS      = 10000;
constexpr uint32_t HTTP_TIMEOUT_MS    = 5000;

// ----------------------------- ThingSpeak -----------------------------
constexpr const char* TS_URL = "http://api.thingspeak.com/update";

}  // namespace cfg
