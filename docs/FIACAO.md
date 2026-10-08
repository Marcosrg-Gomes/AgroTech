# Fiação

Organização: **sensores no lado esquerdo** do ESP32 e **atuadores no lado direito**. Cada função tem uma cor de fio.

## Tabela de ligações

| Origem (ESP32) | Destino | Cor do fio | Função |
|---|---|---|---|
| 3V3 | DHT22 `VCC` | vermelho | Alimentação |
| GND.1 | DHT22 `GND` | preto | Terra |
| GPIO 32 | DHT22 `SDA` | azul | Dados |
| 3V3 | LDR `VCC` | vermelho | Alimentação |
| GND.1 | LDR `GND` | preto | Terra |
| GPIO 34 | LDR `AO` | verde | Saída analógica |
| GPIO 18 | Buzzer `+` (pino 2) | roxo | Sinal |
| Buzzer `−` (pino 1) | GND.3 | preto | Terra |
| GPIO 17 | R1 (220 Ω) → LED vermelho (A) | vermelho | Alarme crítico |
| GPIO 16 | R2 (220 Ω) → LED amarelo (A) | amarelo | Atenção |
| GPIO 4 | R3 (220 Ω) → LED verde (A) | verde | OK |
| LEDs (C) | GND.3 | preto | Terra |

## Por que esses pinos?

- **GPIO 34**: entrada exclusiva do ADC1 (funciona com Wi-Fi ligado).
- **GPIO 32**: pino de uso geral no mesmo lado dos sensores.
- **GPIO 4, 16, 17, 18**: ficam na coluna direita, em ordem vertical igual à dos LEDs no diagrama, para os fios não se cruzarem.
- Evitados: GPIO 0, 2, 12, 15 (pinos de *boot*) e 6–11 (flash).

## Resistores

220 Ω em série com cada LED (corrente ≈ (3,3 V − 2 V) / 220 Ω ≈ 6 mA, dentro do limite do GPIO).

## Ajustes no Wokwi

As coordenadas das peças no `diagram.json` são uma sugestão de layout. Se algum fio ficar sobre outro, arraste a peça no editor; as conexões não mudam.
