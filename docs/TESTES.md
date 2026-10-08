# Plano de testes

Como testar no Wokwi: clique no **DHT22** para editar temperatura/umidade e use o controle do **LDR** para a luz. Acompanhe o monitor serial.

| # | Cenário | Entrada | Resultado esperado |
|---|---|---|---|
| T1 | Condições ideais | 25 °C · 65 % · luz alta | LED verde, buzzer mudo, `field4 = 0` |
| T2 | Temperatura alta | 32 °C | LED amarelo, `field4 = 1`, motivo "temperatura alta" |
| T3 | Temperatura crítica | 38 °C | LED vermelho + buzzer intermitente, `field4 = 2` |
| T4 | Umidade crítica | 25 % | LED vermelho + buzzer |
| T5 | Pouca luz | luz < 20 % | LED amarelo, motivo "pouca luz" |
| T6 | Confirmação (debounce) | Mudar um valor e voltar em < 4 s | Estado não muda |
| T7 | Histerese | T crítica → 34,8 °C | Continua CRÍTICO; só sai abaixo de 34,5 °C |
| T8 | Histerese (atenção) | T alta → 29,8 °C | Continua ATENÇÃO; só volta a OK abaixo de 29,5 °C |
| T9 | Envio periódico | Deixar rodando | Nova entrada a cada ≈ 20 s no ThingSpeak |
| T10 | Envio por mudança | Mudar de OK para CRÍTICO | Envio logo após ≥ 16 s do envio anterior |
| T11 | Perda de Wi-Fi | Sem rede | Log "Conexao perdida" e reconexão a cada 10 s; LEDs seguem funcionando |
| T12 | Retomada | Rede volta | "Conectado" e envio retomado |

## Registro dos resultados

| # | Passou? | Observação / print |
|---|---|---|
| T1 | ☐ | |
| T2 | ☐ | |
| T3 | ☐ | |
| T4 | ☐ | |
| T5 | ☐ | |
| T6 | ☐ | |
| T7 | ☐ | |
| T8 | ☐ | |
| T9 | ☐ | |
| T10 | ☐ | |
| T11 | ☐ | |
| T12 | ☐ | |

> A lógica de alertas (T2–T8) foi verificada em teste de mesa no computador, fora do simulador; os demais cenários dependem da simulação.
