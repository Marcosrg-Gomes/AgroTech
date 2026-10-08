#pragma once
#include "types.h"

namespace sensors {
void begin();
// Faz uma nova amostragem a cada cfg::SAMPLE_INTERVAL_MS.
// Retorna true quando 'r' foi atualizado.
bool update(Reading& r);
}  // namespace sensors
