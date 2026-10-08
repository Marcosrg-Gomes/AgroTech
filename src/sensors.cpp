#include "sensors.h"
#include "config.h"
#include <DHT.h>

namespace {

DHT dht(cfg::PIN_DHT, DHT22);
uint32_t lastSample = 0;
uint8_t dhtFailures = 0;

// Lê o LDR várias vezes e devolve a média em % (100 = muita luz).
int readLightPct() {
  uint32_t acc = 0;
  for (uint8_t i = 0; i < cfg::LDR_OVERSAMPLE; i++) {
    acc += analogRead(cfg::PIN_LDR);
    delayMicroseconds(200);
  }
  const int raw = acc / cfg::LDR_OVERSAMPLE;          // 0..4095
  const int pct = constrain(map(raw, 0, 4095, 0, 100), 0, 100);
  return cfg::LDR_DARK_IS_HIGH ? 100 - pct : pct;
}

// Valores fora da faixa física do DHT22 são tratados como erro de leitura.
bool plausible(float t, float h) {
  return !isnan(t) && !isnan(h) && t >= -40.0f && t <= 80.0f && h >= 0.0f && h <= 100.0f;
}

}  // namespace

namespace sensors {

void begin() {
  dht.begin();
  analogReadResolution(12);
  pinMode(cfg::PIN_LDR, INPUT);
  lastSample = millis();   // o DHT22 precisa de ~2 s após ligar
}

bool update(Reading& r) {
  const uint32_t now = millis();
  if (now - lastSample < cfg::SAMPLE_INTERVAL_MS) return false;
  lastSample = now;

  const float t = dht.readTemperature();
  const float h = dht.readHumidity();

  if (plausible(t, h)) {
    r.temp = t;
    r.hum = h;
    r.dhtOk = true;
    r.sensorFault = false;
    dhtFailures = 0;
  } else {
    r.dhtOk = false;
    if (dhtFailures < 255) dhtFailures++;
    Serial.printf("[DHT22] Leitura invalida (%u/%u)\n", dhtFailures, cfg::DHT_FAILS_TO_FAULT);
    if (dhtFailures >= cfg::DHT_FAILS_TO_FAULT) {   // não reaproveita dado velho
      r.sensorFault = true;
      r.temp = NAN;
      r.hum = NAN;
    }
  }

  r.lightPct = readLightPct();
  return true;
}

}  // namespace sensors
