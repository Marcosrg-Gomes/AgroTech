#include "cloud.h"
#include "alerts.h"
#include "config.h"
#include "secrets.h"
#include <HTTPClient.h>
#include <WiFi.h>

namespace {

uint32_t lastWifiAttempt = 0;
bool wasConnected = false;

uint32_t lastSend = 0;       // quando enviamos (ou tentamos) pela última vez
uint32_t nextSendAt = 0;     // próximo envio periódico/retentativa
uint8_t  failures = 0;
bool     everSent = false;
AlertLevel lastSentLevel = AlertLevel::Ok;
uint32_t sentOk = 0, sentFail = 0;

String urlEncode(const char* s) {
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

// Campos do canal: 1=Temperatura 2=Umidade 3=Luminosidade 4=Nivel (0/1/2)
bool sendToThingSpeak(const Reading& r, const AlertResult& a) {
  String url = String(cfg::TS_URL) + "?api_key=" + secrets::TS_API_KEY;
  if (!isnan(r.temp)) url += "&field1=" + String(r.temp, 1);   // em falha de sensor, omite
  if (!isnan(r.hum))  url += "&field2=" + String(r.hum, 1);
  url += "&field3=" + String(r.lightPct);
  url += "&field4=" + String(static_cast<int>(a.level));
  url += "&status=" + urlEncode(alerts::reasonText(a.reason));

  WiFiClient client;
  HTTPClient http;
  http.setConnectTimeout(cfg::HTTP_TIMEOUT_MS);
  http.setTimeout(cfg::HTTP_TIMEOUT_MS);
  if (!http.begin(client, url)) return false;

  const int code = http.GET();
  const String body = (code > 0) ? http.getString() : String();
  http.end();

  // ThingSpeak responde o ID da entrada gravada; "0" significa que rejeitou.
  const bool ok = (code == 200 && body.toInt() > 0);
  Serial.printf("[NUVEM] HTTP %d | entrada #%s | %s (ok=%lu falhas=%lu)\n",
                code, body.c_str(), ok ? "ENVIADO" : "FALHA",
                static_cast<unsigned long>(sentOk + (ok ? 1 : 0)),
                static_cast<unsigned long>(sentFail + (ok ? 0 : 1)));
  return ok;
}

}  // namespace

namespace cloud {

void begin() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(secrets::WIFI_SSID, secrets::WIFI_PASS);
  lastWifiAttempt = millis();
  Serial.printf("[WIFI] Conectando a %s...\n", secrets::WIFI_SSID);
}

void loop() {
  const bool connected = (WiFi.status() == WL_CONNECTED);

  if (connected && !wasConnected) {
    Serial.printf("[WIFI] Conectado. IP=%s RSSI=%d dBm\n",
                  WiFi.localIP().toString().c_str(), WiFi.RSSI());
  } else if (!connected && wasConnected) {
    Serial.println("[WIFI] Conexao perdida.");
  }
  wasConnected = connected;

  if (!connected && millis() - lastWifiAttempt >= cfg::WIFI_RETRY_MS) {
    lastWifiAttempt = millis();
    WiFi.disconnect();
    WiFi.begin(secrets::WIFI_SSID, secrets::WIFI_PASS);
    Serial.println("[WIFI] Tentando reconectar...");
  }
}

void publish(const Reading& r, const AlertResult& a) {
  if (WiFi.status() != WL_CONNECTED) return;
  if (isnan(r.temp) && !r.sensorFault) return;          // ainda sem dados

  const uint32_t now = millis();
  const bool periodicDue = static_cast<int32_t>(now - nextSendAt) >= 0;
  const bool levelChanged = everSent && a.level != lastSentLevel &&
                            (now - lastSend) >= cfg::CLOUD_MIN_GAP_MS;
  if (!periodicDue && !levelChanged) return;

  lastSend = now;
  if (sendToThingSpeak(r, a)) {
    sentOk++;
    failures = 0;
    everSent = true;
    lastSentLevel = a.level;
    nextSendAt = now + cfg::CLOUD_INTERVAL_MS;
  } else {
    sentFail++;
    if (failures < 8) failures++;
    uint32_t wait = cfg::CLOUD_RETRY_BASE_MS << (failures - 1);   // backoff exponencial
    if (wait > cfg::CLOUD_RETRY_MAX_MS) wait = cfg::CLOUD_RETRY_MAX_MS;
    nextSendAt = now + wait;
  }
}

}  // namespace cloud
