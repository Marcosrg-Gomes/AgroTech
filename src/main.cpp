/*
 * AgroTech — Estufa Inteligente
 * ESP32 + DHT22 + LDR + LEDs + buzzer -> ThingSpeak (HTTP)
 *
 * Fluxo do loop():
 *   sensors  -> lê DHT22/LDR (a cada 2 s)
 *   alerts   -> classifica (OK / ATENCAO / CRITICO) com histerese e confirmação
 *   actuators-> LEDs e buzzer (não bloqueante)
 *   cloud    -> Wi-Fi e envio ao ThingSpeak
 *
 * Ajustes: src/config.h   |   Credenciais: src/secrets.h
 */
#include <Arduino.h>
#include "actuators.h"
#include "alerts.h"
#include "cloud.h"
#include "config.h"
#include "sensors.h"
#include "types.h"

namespace {

Reading reading;
AlertResult alert{AlertLevel::Ok, Reason::Ideal, false};
bool hasAlert = false;      // só mostra LEDs depois da 1ª avaliação

void logReading() {
  Serial.printf("[SENSORES] T=%.1fC  U=%.1f%%  Luz=%d%%  | %s - %s\n",
                reading.temp, reading.hum, reading.lightPct,
                alerts::levelText(alert.level), alerts::reasonText(alert.reason));
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println(F("\n=== AgroTech Estufa Inteligente ==="));

  actuators::begin();
  sensors::begin();
  actuators::selfTest();
  cloud::begin();
}

void loop() {
  cloud::loop();

  if (sensors::update(reading)) {
    const AlertResult res = alerts::update(reading);
    alert = res;
    hasAlert = true;
    logReading();
    if (res.changed) {
      Serial.printf("[ALERTA] Novo estado: %s (%s)\n",
                    alerts::levelText(res.level), alerts::reasonText(res.reason));
    }
  }

  if (hasAlert) actuators::apply(alert.level);
  cloud::publish(reading, alert);
}
