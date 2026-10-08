# Configuração do ThingSpeak

## 1. Criar o canal

1. Crie uma conta gratuita em <https://thingspeak.mathworks.com>.
2. **Channels → New Channel** e preencha:

| Campo | Nome sugerido |
|---|---|
| Field 1 | Temperatura (°C) |
| Field 2 | Umidade (%) |
| Field 3 | Luminosidade (%) |
| Field 4 | Nivel (0=OK, 1=Atencao, 2=Critico) |

3. Marque *Show Status* e salve.

## 2. Copiar a chave

Aba **API Keys → Write API Key**. Cole em `src/secrets.h`:

```cpp
constexpr const char* TS_API_KEY = "SUA_CHAVE_AQUI";
```

> Não compartilhe a chave em prints, relatório ou GitHub.

## 3. Montar o dashboard

Na aba **Private View**, adicione:

| Widget | Campo | Sugestão |
|---|---|---|
| Gráfico de linha | Field 1 | Temperatura ao longo do tempo |
| Gráfico de linha | Field 2 | Umidade ao longo do tempo |
| Gauge | Field 3 | 0–100 % |
| Lamp Indicator | Field 4 | Aceso quando ≥ 1 |
| Status | — | Mostra o motivo (ex.: "CRITICO: temperatura muito alta") |

## 4. Formato da requisição

```
GET http://api.thingspeak.com/update?api_key=<KEY>&field1=25.0&field2=65.0&field3=60&field4=0&status=Condicoes%20ideais
```

A resposta é o número da entrada gravada. `0` = rejeitado (intervalo < 15 s ou chave inválida).

## 5. Para a entrega

Deixe a simulação rodando 2–3 minutos e tire o print do dashboard com os gráficos já preenchidos (cubra a Write API Key, se aparecer).
