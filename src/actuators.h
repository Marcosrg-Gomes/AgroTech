#pragma once
#include "types.h"

namespace actuators {
void begin();
void selfTest();                 // acende os LEDs em sequência e apita (≈1 s)
void apply(AlertLevel level);    // chamar a cada loop(); não bloqueia
}  // namespace actuators
