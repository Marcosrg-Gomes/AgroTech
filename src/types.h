#pragma once
/* types.h — tipos compartilhados entre os módulos. */
#include <Arduino.h>

// Nível de alerta. Valor numérico = valor enviado ao ThingSpeak (field4).
enum class AlertLevel : uint8_t { Ok = 0, Attention = 1, Critical = 2 };

// Motivo do estado atual (usado no log e no campo "status" do ThingSpeak).
enum class Reason : uint8_t {
  Ideal,
  TempHigh, TempLow, HumHigh, HumLow, LowLight,
  TempCritHigh, TempCritLow, HumCritHigh, HumCritLow,
  SensorFault
};

// Uma amostra dos sensores.
struct Reading {
  float temp = NAN;          // °C (NAN = sem dado válido)
  float hum = NAN;           // % UR
  int   lightPct = 0;        // 0..100 (100 = muita luz)
  bool  dhtOk = false;       // a última leitura do DHT22 foi válida?
  bool  sensorFault = false; // falhas consecutivas do DHT22
};

// Resultado da avaliação de alertas.
struct AlertResult {
  AlertLevel level;
  Reason reason;
  bool changed;              // true quando o estado mudou nesta avaliação
};
