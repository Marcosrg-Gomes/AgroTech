/*
 * AgroTech — Estufa Inteligente (ESP32 + Wokwi + ThingSpeak)
 * Versão em ARQUIVO ÚNICO para colar no wokwi.com (sketch.ino).
 *
 * Sensores:  DHT22 (GPIO32) · LDR (GPIO34)
 * Atuadores: LED verde (4) · amarelo (16) · vermelho (17) · buzzer (18)
 * Nuvem:     ThingSpeak via HTTP (field1=temp, field2=umid, field3=luz, field4=nivel)
 */
#include <Arduino.h>
#include <DHT.h>
#include <HTTPClient.h>
#include <WiFi.h>

// ======================================================================
// CONFIGURAÇÃO (limites, pinos e tempos)
// ======================================================================
namespace cfg {
constexpr uint8_t PIN_DHT        = 32;
constexpr uint8_t PIN_LDR        = 34;   // ADC1 (ADC2 não funciona com Wi-Fi)
constexpr uint8_t PIN_BUZZER     = 18;
constexpr uint8_t PIN_LED_RED    = 17;
constexpr uint8_t PIN_LED_YELLOW = 16;
constexpr uint8_t PIN_LED_GREEN  = 4;

constexpr uint8_t  BUZZER_CHANNEL = 0;   // canal LEDC (core 2.x)
constexpr uint16_t BUZZER_FREQ_HZ = 2000;

// Limites (ajuste à cultura)
constexpr float TEMP_MIN = 18.0f, TEMP_MAX = 30.0f;
constexpr float TEMP_CRIT_MIN = 10.0f, TEMP_CRIT_MAX = 35.0f;
constexpr float HUM_MIN = 50.0f, HUM_MAX = 80.0f;
constexpr float HUM_CRIT_MIN = 30.0f, HUM_CRIT_MAX = 90.0f;
constexpr int   LIGHT_MIN_PCT = 20;

// Histerese e confirmação
constexpr float HYST_TEMP = 0.5f;
constexpr float HYST_HUM  = 2.0f;
constexpr int   HYST_LIGHT = 3;
constexpr uint8_t CONFIRM_READINGS = 2;

// Sensores (no Wokwi o LDR dá valor MAIOR no escuro; se ficar invertido, use false)
constexpr bool    LDR_DARK_IS_HIGH   = true;
constexpr uint8_t LDR_OVERSAMPLE     = 16;
constexpr uint8_t DHT_FAILS_TO_FAULT = 3;

// Tempos (ms)
constexpr uint32_t SAMPLE_INTERVAL_MS  = 2000;
constexpr uint32_t CLOUD_INTERVAL_MS   = 20000;
constexpr uint32_t CLOUD_MIN_GAP_MS    = 16000;
constexpr uint32_t CLOUD_RETRY_BASE_MS = 5000;
constexpr uint32_t CLOUD_RETRY_MAX_MS  = 60000;
constexpr uint32_t WIFI_RETRY_MS       = 10000;
constexpr uint32_t HTTP_TIMEOUT_MS     = 5000;

constexpr const char* TS_URL = "http://api.thingspeak.com/update";
}  // namespace cfg

// ======================================================================
// CREDENCIAIS
// ======================================================================
namespace secrets {
constexpr const char* WIFI_SSID  = "Wokwi-GUEST";
constexpr const char* WIFI_PASS  = "";
constexpr const char* TS_API_KEY = "I3BUKQ4VJV5R3CBG";
}  // namespace secrets

// ======================================================================
// TIPOS
// ======================================================================
enum class AlertLevel : uint8_t { Ok = 0, Attention = 1, Critical = 2 };

enum class Reason : uint8_t {
  Ideal,
  TempHigh, TempLow, HumHigh, HumLow, LowLight,
  TempCritHigh, TempCritLow, HumCritHigh, HumCritLow,
  SensorFault
};

struct Reading {
  float temp = NAN;
  float hum = NAN;
  int   lightPct = 0;
  bool  dhtOk = false;
  bool  sensorFault = false;
};

struct AlertResult {
  AlertLevel level;
  Reason reason;
  bool changed;
};

struct Raw {
  AlertLevel level;
  Reason reason;
};

// ======================================================================
// TEXTOS
// ======================================================================
static const char* reasonText(Reason reason) {
  switch (reason) {
    case Reason::Ideal:        return "Condicoes ideais";
    case Reason::TempHigh:     return "Atencao: temperatura alta";
    case Reason::TempLow:      return "Atencao: temperatura baixa";
    case Reason::HumHigh:      return "Atencao: umidade alta";
    case Reason::HumLow:       return "Atencao: umidade baixa";
    case Reason::LowLight:     return "Atencao: pouca luz";
    case Reason::TempCritHigh: return "CRITICO: temperatura muito alta";
    case Reason::TempCritLow:  return "CRITICO: temperatura muito baixa";
    case Reason::HumCritHigh:  return "CRITICO: umidade muito alta";
    case Reason::HumCritLow:   return "CRITICO: umidade muito baixa";
    case Reason::SensorFault:  return "Atencao: falha no sensor DHT22";
  }
  return "";
}

static const char* levelText(AlertLevel level) {
  switch (level) {
    case AlertLevel::Ok:        return "OK";
    case AlertLevel::Attention: return "ATENCAO";
    case AlertLevel::Critical:  return "CRITICO";
  }
  return "";
}

// ======================================================================
// SENSORES
// ======================================================================
static DHT dht(cfg::PIN_DHT, DHT22);
static uint32_t sensLastSample = 0;
static uint8_t  sensDhtFailures = 0;

static int readLightPct() {
  uint32_t acc = 0;
  for (uint8_t i = 0; i < cfg::LDR_OVERSAMPLE; i++) {
    acc += analogRead(cfg::PIN_LDR);
    delayMicroseconds(200);
  }
  const int raw = acc / cfg::LDR_OVERSAMPLE;
  const int pct = constrain(map(raw, 0, 4095, 0, 100), 0, 100);
  return cfg::LDR_DARK_IS_HIGH ? 100 - pct : pct;
}

static bool plausible(float t, float h) {
  return !isnan(t) && !isnan(h) && t >= -40.0f && t <= 80.0f && h >= 0.0f && h <= 100.0f;
}

static void sensorsBegin() {
  dht.begin();
  analogReadResolution(12);
  pinMode(cfg::PIN_LDR, INPUT);
  sensLastSample = millis();
}

static bool sensorsUpdate(Reading& r) {
  const uint32_t now = millis();
  if (now - sensLastSample < cfg::SAMPLE_INTERVAL_MS) return false;
  sensLastSample = now;

  const float t = dht.readTemperature();
  const float h = dht.readHumidity();

  if (plausible(t, h)) {
    r.temp = t;
    r.hum = h;
    r.dhtOk = true;
    r.sensorFault = false;
    sensDhtFailures = 0;
  } else {
    r.dhtOk = false;
    if (sensDhtFailures < 255) sensDhtFailures++;
    Serial.printf("[DHT22] Leitura invalida (%u/%u)\n", sensDhtFailures, cfg::DHT_FAILS_TO_FAULT);
    if (sensDhtFailures >= cfg::DHT_FAILS_TO_FAULT) {
      r.sensorFault = true;
      r.temp = NAN;
      r.hum = NAN;
    }
  }

  r.lightPct = readLightPct();
  return true;
}

// ======================================================================
// ALERTAS (regras + histerese + confirmação)
// ======================================================================
static AlertLevel alCurLevel = AlertLevel::Ok;
static Reason     alCurReason = Reason::Ideal;
static AlertLevel alPendLevel = AlertLevel::Ok;
static Reason     alPendReason = Reason::Ideal;
static uint8_t    alPendCount = 0;
static bool       alFirst = true;

static Raw evaluate(const Reading& r, AlertLevel cur) {
  if (r.sensorFault) return {AlertLevel::Attention, Reason::SensorFault};

  const bool inCrit  = (cur == AlertLevel::Critical);
  const bool inAlarm = (cur != AlertLevel::Ok);

  const float tc = inCrit  ? cfg::HYST_TEMP : 0.0f;
  const float hc = inCrit  ? cfg::HYST_HUM  : 0.0f;
  const float ta = inAlarm ? cfg::HYST_TEMP : 0.0f;
  const float ha = inAlarm ? cfg::HYST_HUM  : 0.0f;
  const int   la = inAlarm ? cfg::HYST_LIGHT : 0;

  if (r.temp > cfg::TEMP_CRIT_MAX - tc) return {AlertLevel::Critical, Reason::TempCritHigh};
  if (r.temp < cfg::TEMP_CRIT_MIN + tc) return {AlertLevel::Critical, Reason::TempCritLow};
  if (r.hum  > cfg::HUM_CRIT_MAX  - hc) return {AlertLevel::Critical, Reason::HumCritHigh};
  if (r.hum  < cfg::HUM_CRIT_MIN  + hc) return {AlertLevel::Critical, Reason::HumCritLow};

  if (r.temp > cfg::TEMP_MAX - ta) return {AlertLevel::Attention, Reason::TempHigh};
  if (r.temp < cfg::TEMP_MIN + ta) return {AlertLevel::Attention, Reason::TempLow};
  if (r.hum  > cfg::HUM_MAX  - ha) return {AlertLevel::Attention, Reason::HumHigh};
  if (r.hum  < cfg::HUM_MIN  + ha) return {AlertLevel::Attention, Reason::HumLow};
  if (r.lightPct < cfg::LIGHT_MIN_PCT + la) return {AlertLevel::Attention, Reason::LowLight};

  return {AlertLevel::Ok, Reason::Ideal};
}

static AlertResult alertsUpdate(const Reading& r) {
  AlertResult res{alCurLevel, alCurReason, false};

  if (!r.sensorFault && (isnan(r.temp) || isnan(r.hum))) return res;

  const Raw raw = evaluate(r, alCurLevel);

  if (alFirst) {
    alFirst = false;
    alCurLevel = raw.level;
    alCurReason = raw.reason;
    alPendCount = 0;
    return {alCurLevel, alCurReason, true};
  }

  if (raw.level == alCurLevel && raw.reason == alCurReason) {
    alPendCount = 0;
    return res;
  }

  if (alPendCount > 0 && raw.level == alPendLevel && raw.reason == alPendReason) {
    alPendCount++;
  } else {
    alPendLevel = raw.level;
    alPendReason = raw.reason;
    alPendCount = 1;
  }

  if (alPendCount >= cfg::CONFIRM_READINGS) {
    alCurLevel = alPendLevel;
    alCurReason = alPendReason;
    alPendCount = 0;
    return {alCurLevel, alCurReason, true};
  }
  return res;
}

// ======================================================================
// ATUADORES (LEDs + buzzer, sem delay no loop)
// ======================================================================
// A API LEDC mudou no Arduino-ESP32 3.x (usa o pino em vez do canal).
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
  #define BUZZER_ATTACH()   ledcAttach(cfg::PIN_BUZZER, cfg::BUZZER_FREQ_HZ, 8)
  #define BUZZER_TONE(f)    ledcWriteTone(cfg::PIN_BUZZER, (f))
#else
  #define BUZZER_ATTACH()   do { ledcSetup(cfg::BUZZER_CHANNEL, cfg::BUZZER_FREQ_HZ, 8); \
                                 ledcAttachPin(cfg::PIN_BUZZER, cfg::BUZZER_CHANNEL); } while (0)
  #define BUZZER_TONE(f)    ledcWriteTone(cfg::BUZZER_CHANNEL, (f))
#endif

static bool buzzerOn = false;

static void buzzerSet(bool on) {
  if (on == buzzerOn) return;
  buzzerOn = on;
  BUZZER_TONE(on ? cfg::BUZZER_FREQ_HZ : 0);
}

static void ledsSet(bool green, bool yellow, bool red) {
  digitalWrite(cfg::PIN_LED_GREEN, green);
  digitalWrite(cfg::PIN_LED_YELLOW, yellow);
  digitalWrite(cfg::PIN_LED_RED, red);
}

static void actuatorsBegin() {
  pinMode(cfg::PIN_LED_GREEN, OUTPUT);
  pinMode(cfg::PIN_LED_YELLOW, OUTPUT);
  pinMode(cfg::PIN_LED_RED, OUTPUT);
  BUZZER_ATTACH();
  buzzerOn = true;
  buzzerSet(false);
  ledsSet(false, false, false);
}

static void actuatorsSelfTest() {
  ledsSet(true, false, false);  delay(250);
  ledsSet(false, true, false);  delay(250);
  ledsSet(false, false, true);  delay(250);
  buzzerSet(true);              delay(150);
  buzzerSet(false);
  ledsSet(false, false, false);
}

static void actuatorsApply(AlertLevel level) {
  ledsSet(level == AlertLevel::Ok,
          level == AlertLevel::Attention,
          level == AlertLevel::Critical);
  // Crítico: bipe 200 ms ligado / 800 ms desligado
  buzzerSet(level == AlertLevel::Critical && (millis() % 1000UL) < 200UL);
}

// ======================================================================
// NUVEM (Wi-Fi + ThingSpeak)
// ======================================================================
static uint32_t clLastWifiAttempt = 0;
static bool     clWasConnected = false;
static uint32_t clLastSend = 0;
static uint32_t clNextSendAt = 0;
static uint8_t  clFailures = 0;
static bool     clEverSent = false;
static AlertLevel clLastSentLevel = AlertLevel::Ok;
static uint32_t clSentOk = 0, clSentFail = 0;

static String urlEncode(const char* s) {
  String out;
  char buf[4];
  for (; *s; ++s) {
    const char c = *s;
    if (isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_' || c == '.' || c == '~') {
      out += c;
    } else {
      snprintf(buf, sizeof(buf), "%%%02X", static_cast<unsigned char>(c));
      out += buf;
    }
  }
  return out;
}

static bool sendToThingSpeak(const Reading& r, const AlertResult& a) {
  String url = String(cfg::TS_URL) + "?api_key=" + secrets::TS_API_KEY;
  if (!isnan(r.temp)) url += "&field1=" + String(r.temp, 1);
  if (!isnan(r.hum))  url += "&field2=" + String(r.hum, 1);
  url += "&field3=" + String(r.lightPct);
  url += "&field4=" + String(static_cast<int>(a.level));
  url += "&status=" + urlEncode(reasonText(a.reason));

  WiFiClient client;
  HTTPClient http;
  http.setConnectTimeout(cfg::HTTP_TIMEOUT_MS);
  http.setTimeout(cfg::HTTP_TIMEOUT_MS);
  if (!http.begin(client, url)) return false;

  const int code = http.GET();
  const String body = (code > 0) ? http.getString() : String();
  http.end();

  const bool ok = (code == 200 && body.toInt() > 0);
  Serial.printf("[NUVEM] HTTP %d | entrada #%s | %s (ok=%lu falhas=%lu)\n",
                code, body.c_str(), ok ? "ENVIADO" : "FALHA",
                static_cast<unsigned long>(clSentOk + (ok ? 1 : 0)),
                static_cast<unsigned long>(clSentFail + (ok ? 0 : 1)));
  return ok;
}

static void cloudBegin() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(secrets::WIFI_SSID, secrets::WIFI_PASS);
  clLastWifiAttempt = millis();
  Serial.printf("[WIFI] Conectando a %s...\n", secrets::WIFI_SSID);
}

static void cloudLoop() {
  const bool connected = (WiFi.status() == WL_CONNECTED);

  if (connected && !clWasConnected) {
    Serial.printf("[WIFI] Conectado. IP=%s RSSI=%d dBm\n",
                  WiFi.localIP().toString().c_str(), WiFi.RSSI());
  } else if (!connected && clWasConnected) {
    Serial.println("[WIFI] Conexao perdida.");
  }
  clWasConnected = connected;

  if (!connected && millis() - clLastWifiAttempt >= cfg::WIFI_RETRY_MS) {
    clLastWifiAttempt = millis();
    WiFi.disconnect();
    WiFi.begin(secrets::WIFI_SSID, secrets::WIFI_PASS);
    Serial.println("[WIFI] Tentando reconectar...");
  }
}

static void cloudPublish(const Reading& r, const AlertResult& a) {
  if (WiFi.status() != WL_CONNECTED) return;
  if (isnan(r.temp) && !r.sensorFault) return;

  const uint32_t now = millis();
  const bool periodicDue = static_cast<int32_t>(now - clNextSendAt) >= 0;
  const bool levelChanged = clEverSent && a.level != clLastSentLevel &&
                            (now - clLastSend) >= cfg::CLOUD_MIN_GAP_MS;
  if (!periodicDue && !levelChanged) return;

  clLastSend = now;
  if (sendToThingSpeak(r, a)) {
    clSentOk++;
    clFailures = 0;
    clEverSent = true;
    clLastSentLevel = a.level;
    clNextSendAt = now + cfg::CLOUD_INTERVAL_MS;
  } else {
    clSentFail++;
    if (clFailures < 8) clFailures++;
    uint32_t wait = cfg::CLOUD_RETRY_BASE_MS << (clFailures - 1);
    if (wait > cfg::CLOUD_RETRY_MAX_MS) wait = cfg::CLOUD_RETRY_MAX_MS;
    clNextSendAt = now + wait;
  }
}

// ======================================================================
// PROGRAMA PRINCIPAL
// ======================================================================
static Reading reading;
static AlertResult alertState{AlertLevel::Ok, Reason::Ideal, false};
static bool hasAlert = false;

static void logReading() {
  Serial.printf("[SENSORES] T=%.1fC  U=%.1f%%  Luz=%d%%  | %s - %s\n",
                reading.temp, reading.hum, reading.lightPct,
                levelText(alertState.level), reasonText(alertState.reason));
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\n=== AgroTech Estufa Inteligente ===");

  actuatorsBegin();
  sensorsBegin();
  actuatorsSelfTest();
  cloudBegin();
}

void loop() {
  cloudLoop();

  if (sensorsUpdate(reading)) {
    const AlertResult res = alertsUpdate(reading);
    alertState = res;
    hasAlert = true;
    logReading();
    if (res.changed) {
      Serial.printf("[ALERTA] Novo estado: %s (%s)\n", levelText(res.level), reasonText(res.reason));
    }
  }

  if (hasAlert) actuatorsApply(alertState.level);
  cloudPublish(reading, alertState);
}
