#pragma once
#include "types.h"

namespace alerts {
// Avalia a amostra e devolve o estado (com histerese e confirmação).
AlertResult update(const Reading& r);

const char* reasonText(Reason reason);
const char* levelText(AlertLevel level);
}  // namespace alerts
