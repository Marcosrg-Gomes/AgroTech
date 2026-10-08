#pragma once
#include "types.h"

namespace cloud {
void begin();                                             // inicia o Wi-Fi
void loop();                                              // mantém/reconecta o Wi-Fi (não bloqueia)
void publish(const Reading& r, const AlertResult& a);     // decide quando enviar ao ThingSpeak
}  // namespace cloud
