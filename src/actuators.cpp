#include "actuators.h"
#include "config.h"

// Compatibilidade: a API LEDC mudou no Arduino-ESP32 3.x (usa o pino em vez do canal).
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
  #define BUZZER_ATTACH()   ledcAttach(cfg::PIN_BUZZER, cfg::BUZZER_FREQ_HZ, 8)
  #define BUZZER_TONE(f)    ledcWriteTone(cfg::PIN_BUZZER, (f))
#else
  #define BUZZER_ATTACH()   do { ledcSetup(cfg::BUZZER_CHANNEL, cfg::BUZZER_FREQ_HZ, 8); \
                                 ledcAttachPin(cfg::PIN_BUZZER, cfg::BUZZER_CHANNEL); } while (0)
  #define BUZZER_TONE(f)    ledcWriteTone(cfg::BUZZER_CHANNEL, (f))
#endif

namespace {

bool buzzerOn = false;

void buzzer(bool on) {
  if (on == buzzerOn) return;          // só mexe no hardware quando muda
  buzzerOn = on;
  BUZZER_TONE(on ? cfg::BUZZER_FREQ_HZ : 0);
}

void leds(bool green, bool yellow, bool red) {
  digitalWrite(cfg::PIN_LED_GREEN, green);
  digitalWrite(cfg::PIN_LED_YELLOW, yellow);
  digitalWrite(cfg::PIN_LED_RED, red);
}

}  // namespace

namespace actuators {

void begin() {
  pinMode(cfg::PIN_LED_GREEN, OUTPUT);
  pinMode(cfg::PIN_LED_YELLOW, OUTPUT);
  pinMode(cfg::PIN_LED_RED, OUTPUT);
  BUZZER_ATTACH();
  buzzerOn = true;                     // força o primeiro desligamento
  buzzer(false);
  leds(false, false, false);
}

void selfTest() {
  leds(true, false, false);  delay(250);
  leds(false, true, false);  delay(250);
  leds(false, false, true);  delay(250);
  buzzer(true);              delay(150);
  buzzer(false);
  leds(false, false, false);
}

void apply(AlertLevel level) {
  leds(level == AlertLevel::Ok,
       level == AlertLevel::Attention,
       level == AlertLevel::Critical);

  // Crítico: bipe intermitente (200 ms ligado / 800 ms desligado), sem delay().
  buzzer(level == AlertLevel::Critical && (millis() % 1000UL) < 200UL);
}

}  // namespace actuators
