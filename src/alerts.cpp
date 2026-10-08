#include "alerts.h"
#include "config.h"

namespace {

struct Raw {
  AlertLevel level;
  Reason reason;
};

AlertLevel curLevel = AlertLevel::Ok;
Reason curReason = Reason::Ideal;
AlertLevel pendLevel = AlertLevel::Ok;
Reason pendReason = Reason::Ideal;
uint8_t pendCount = 0;
bool firstEvaluation = true;

// Classifica a amostra sem memória de confirmação.
// Histerese: se já estamos em alarme, os limites "encolhem" — o valor precisa
// voltar com folga para a faixa ideal antes de o alarme sair (evita pisca-pisca).
Raw evaluate(const Reading& r, AlertLevel cur) {
  if (r.sensorFault) return {AlertLevel::Attention, Reason::SensorFault};

  const bool inCrit  = (cur == AlertLevel::Critical);
  const bool inAlarm = (cur != AlertLevel::Ok);

  const float tc = inCrit  ? cfg::HYST_TEMP : 0.0f;
  const float hc = inCrit  ? cfg::HYST_HUM  : 0.0f;
  const float ta = inAlarm ? cfg::HYST_TEMP : 0.0f;
  const float ha = inAlarm ? cfg::HYST_HUM  : 0.0f;
  const int   la = inAlarm ? cfg::HYST_LIGHT : 0;

  // 1) Críticos primeiro
  if (r.temp > cfg::TEMP_CRIT_MAX - tc) return {AlertLevel::Critical, Reason::TempCritHigh};
  if (r.temp < cfg::TEMP_CRIT_MIN + tc) return {AlertLevel::Critical, Reason::TempCritLow};
  if (r.hum  > cfg::HUM_CRIT_MAX  - hc) return {AlertLevel::Critical, Reason::HumCritHigh};
  if (r.hum  < cfg::HUM_CRIT_MIN  + hc) return {AlertLevel::Critical, Reason::HumCritLow};

  // 2) Atenção
  if (r.temp > cfg::TEMP_MAX - ta) return {AlertLevel::Attention, Reason::TempHigh};
  if (r.temp < cfg::TEMP_MIN + ta) return {AlertLevel::Attention, Reason::TempLow};
  if (r.hum  > cfg::HUM_MAX  - ha) return {AlertLevel::Attention, Reason::HumHigh};
  if (r.hum  < cfg::HUM_MIN  + ha) return {AlertLevel::Attention, Reason::HumLow};
  if (r.lightPct < cfg::LIGHT_MIN_PCT + la) return {AlertLevel::Attention, Reason::LowLight};

  return {AlertLevel::Ok, Reason::Ideal};
}

}  // namespace

namespace alerts {

AlertResult update(const Reading& r) {
  AlertResult res{curLevel, curReason, false};

  // Ainda sem nenhum dado de temperatura/umidade: não decide nada.
  if (!r.sensorFault && (isnan(r.temp) || isnan(r.hum))) return res;

  const Raw raw = evaluate(r, curLevel);

  // Primeira avaliação: aplica direto (não faz sentido "confirmar" o estado inicial).
  if (firstEvaluation) {
    firstEvaluation = false;
    curLevel = raw.level;
    curReason = raw.reason;
    pendCount = 0;
    return {curLevel, curReason, true};
  }

  if (raw.level == curLevel && raw.reason == curReason) {
    pendCount = 0;                                   // estável: descarta candidato
    return res;
  }

  // Confirmação: o mesmo novo estado precisa aparecer N vezes seguidas.
  if (pendCount > 0 && raw.level == pendLevel && raw.reason == pendReason) {
    pendCount++;
  } else {
    pendLevel = raw.level;
    pendReason = raw.reason;
    pendCount = 1;
  }

  if (pendCount >= cfg::CONFIRM_READINGS) {
    curLevel = pendLevel;
    curReason = pendReason;
    pendCount = 0;
    return {curLevel, curReason, true};
  }
  return res;
}

const char* reasonText(Reason reason) {
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

const char* levelText(AlertLevel level) {
  switch (level) {
    case AlertLevel::Ok:        return "OK";
    case AlertLevel::Attention: return "ATENCAO";
    case AlertLevel::Critical:  return "CRITICO";
  }
  return "";
}

}  // namespace alerts
