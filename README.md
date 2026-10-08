# 🌱 AgroTech — Estufa Inteligente (ESP32 + Wokwi + ThingSpeak)

Estação de monitoramento para estufa que mede **temperatura, umidade e luminosidade**, sinaliza o estado do ambiente com **LEDs e buzzer** e envia os dados em tempo real para um **dashboard na nuvem (ThingSpeak)**.

Projeto do **Desafio Prático de IoT** — disciplina *Fundamentos de IoT*, curso *Desenvolvimento de Sistemas* (tema 2: Estação de Monitoramento Agrícola / Estufa).

> ⚠️ **Simulação:** o hardware roda no simulador **Wokwi**. A lista de materiais para montar de verdade está no relatório (ABNT).

---

## ✨ Funcionalidades

- Leitura do **DHT22** (temperatura e umidade) e de um **LDR** (luz) a cada 2 s.
- Classificação em **3 níveis**: 🟢 OK · 🟡 Atenção · 🔴 Crítico (buzzer intermitente).
- **Histerese** e **confirmação de 2 leituras**: o alarme não fica "piscando" com ruído.
- Detecção de **falha do DHT22** (3 leituras inválidas seguidas → alerta de sensor).
- Envio ao **ThingSpeak** a cada 20 s **e imediatamente quando o estado muda**, com retentativa em *backoff* exponencial.
- **Wi-Fi com reconexão automática**; nada usa `delay()` no laço principal.
- Todos os limites e pinos em **um único arquivo** (`src/config.h`).

## 🧭 Arquitetura

```
 [DHT22] ─┐                                            ┌─> [LEDs verde/amarelo/vermelho]
          ├─> [ESP32] ──(Wi-Fi / HTTP GET)──> [ThingSpeak]
 [LDR]  ──┘      └───────────────────────────────────> [Buzzer]
```

Módulos de software (`src/`):

| Arquivo | Responsabilidade |
|---|---|
| `main.cpp` | Orquestra o laço: sensores → alertas → atuadores → nuvem |
| `config.h` | Pinos, limites, histerese, intervalos |
| `secrets.h` | Wi-Fi e Write API Key (**não vai para o Git**) |
| `types.h` | Tipos compartilhados (`Reading`, `AlertLevel`, `Reason`) |
| `sensors.*` | DHT22 + LDR (oversampling, validação, falha de sensor) |
| `alerts.*` | Regras de negócio, histerese e confirmação |
| `actuators.*` | LEDs e buzzer não bloqueantes |
| `cloud.*` | Wi-Fi, ThingSpeak, backoff |

Mais detalhes em [`docs/ARQUITETURA.md`](docs/ARQUITETURA.md).

## 📁 Estrutura de pastas

```
Wokwi/
├── src/                 código-fonte (módulos acima)
├── docs/                documentação (arquitetura, fiação, ThingSpeak, testes)
├── diagram.json         circuito do Wokwi
├── platformio.ini       build (PlatformIO)
├── wokwi.toml           caminho do firmware para o simulador
├── .gitignore
└── README.md
```

## 🔌 Fiação resumida

| Componente | ESP32 | Lado |
|---|---|---|
| DHT22 (dados) | GPIO 32 | esquerdo (sensores) |
| LDR (AO) | GPIO 34 (ADC1) | esquerdo (sensores) |
| Buzzer | GPIO 18 | direito (atuadores) |
| LED vermelho + 220 Ω | GPIO 17 | direito |
| LED amarelo + 220 Ω | GPIO 16 | direito |
| LED verde + 220 Ω | GPIO 4 | direito |

Código de cores dos fios: vermelho = 3V3 · preto = GND · azul/verde = sinais dos sensores · roxo = buzzer · cor do LED = cor do fio. Veja [`docs/FIACAO.md`](docs/FIACAO.md).

## 🚦 Regras de alerta

| Nível | Condição | Saída |
|---|---|---|
| 🟢 OK | T 18–30 °C, UR 50–80 %, luz ≥ 20 % | LED verde |
| 🟡 Atenção | qualquer valor fora da faixa ideal, pouca luz ou falha do DHT22 | LED amarelo |
| 🔴 Crítico | T < 10 °C ou > 35 °C, UR < 30 % ou > 90 % | LED vermelho + buzzer |

Histerese: T 0,5 °C · UR 2 % · luz 3 %. Valores configuráveis em `src/config.h`.

## ▶️ Como rodar (VS Code + PlatformIO + Wokwi)

1. Instale no VS Code as extensões **PlatformIO IDE** e **Wokwi Simulator** (e ative a licença gratuita do Wokwi).
2. Abra a pasta `Wokwi`.
3. Confira `src/secrets.h` (se não existir, copie `secrets.example.h` e renomeie).
4. **Compile** (`Ctrl+Alt+B`). Isso gera `.pio/build/esp32dev/firmware.bin` — sem isso o Wokwi acusa *"firmware binary not found"*.
5. Abra o `diagram.json` e clique em ▶ (ou `F1` → *Wokwi: Start Simulator*).
6. Abra o monitor serial para ver os logs.

## ☁️ Configurar o ThingSpeak

Resumo: crie um canal com 4 campos, copie a **Write API Key** para `src/secrets.h` e monte o dashboard. Passo a passo em [`docs/THINGSPEAK.md`](docs/THINGSPEAK.md).

| Campo | Conteúdo |
|---|---|
| field1 | Temperatura (°C) |
| field2 | Umidade (%) |
| field3 | Luminosidade (%) |
| field4 | Nível (0 = OK, 1 = Atenção, 2 = Crítico) |

## 🌐 Gerar o link público do Wokwi (exigido na entrega)

O projeto do VS Code não gera link. Para obtê-lo:

1. Em [wokwi.com](https://wokwi.com), crie um projeto **ESP32**.
2. Substitua o `diagram.json` pelo deste repositório.
3. Crie os arquivos de `src/` (o `main.cpp` vira o código principal; adicione os demais `.h`/`.cpp` pelo botão de novo arquivo).
4. No `libraries.txt`, inclua `DHT sensor library` e `Adafruit Unified Sensor`.
5. Salve e copie a URL do projeto para o relatório.

O código foi escrito para compilar tanto no Arduino-ESP32 2.x (PlatformIO) quanto no 3.x (Wokwi web).

## 🧪 Testes

Cenários de teste e resultados esperados em [`docs/TESTES.md`](docs/TESTES.md). Para simular no Wokwi: clique no DHT22 e altere temperatura/umidade; use o controle deslizante do LDR para a luz.

## 🛠️ Solução de problemas

| Sintoma | Causa / solução |
|---|---|
| `firmware.bin not found` | Falta compilar. Rode o build (`Ctrl+Alt+B`). |
| Falha em `ledcSetup`/`ledcAttachPin` | Plataforma acima de 6.5.0 (core 3.x). O código já trata; confirme que está usando o `actuators.cpp` atual. |
| `[WIFI] Tentando reconectar...` repetido | No VS Code o acesso à internet do Wokwi pode exigir o *gateway*; teste no wokwi.com. |
| ThingSpeak responde `0` | Menos de 15 s desde o último envio ou chave inválida. |
| LDR com lógica invertida | Troque `LDR_DARK_IS_HIGH` em `config.h`. |
| DHT22 sempre inválido | Confira o fio de dados no GPIO 32 e a alimentação em 3V3. |

## 🔒 Segurança

- A **Write API Key** fica em `src/secrets.h`, listado no `.gitignore`. Não publique a chave em prints nem no relatório.
- O ThingSpeak gratuito usa HTTP na porta 80 neste projeto (simplicidade no simulador); em produção, use HTTPS/MQTT com TLS.

## 📄 Documentação

- [`docs/ARQUITETURA.md`](docs/ARQUITETURA.md) — módulos, fluxo e decisões de projeto
- [`docs/FIACAO.md`](docs/FIACAO.md) — pinagem e organização dos fios
- [`docs/THINGSPEAK.md`](docs/THINGSPEAK.md) — canal, campos e dashboard
- [`docs/TESTES.md`](docs/TESTES.md) — plano de testes

## 👤 Autoria

Projeto acadêmico — SENAI, curso Desenvolvimento de Sistemas. Autor: Marcos.
