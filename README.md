# 🎹 ESP32 Synth

Projeto de sintetizador + step sequencer baseado em **ESP32** e **Mozzi**, com foco em **estabilidade de áudio**, **baixo jitter**, **performance** e **controle físico extensivo**.

Este repositório documenta o **hardware**, a **arquitetura do sistema** e serve como base para evolução do firmware.

---

## 📌 Visão Geral

* MCU: **ESP32**
* Áudio: **Mozzi (INTERNAL_DAC)**
* Sequencer: 8 passos com swing
* Interface física rica (pots, encoder, botões)
* Interface visual dupla (OLED + TM1638)

---

## 🔊 Áudio

### Saída

* DAC interno do ESP32
* GPIO **25**
* Mono

### Circuito de saída (simplificado)

```
GPIO25 (DAC)
  |
  +--[100Ω]--+--||---> OUT (Line / Fone)
             |  1µF
            GND
```

### Line Out (P10 mono)

* TIP → OUT
* SLEEVE → GND

### Fone Out (mono, provisório)

* Mesmo OUT
* Potenciômetro como divisor de tensão

> 🔧 Planejado: Op-amp (TL072) + saída estéreo

---

## 🎚️ MUX #1 — CD4051 (Pots contínuos)

### Conexões

* A → GPIO 18
* B → GPIO 19
* C → GPIO 21
* Z → GPIO 34 (ADC)
* VCC → 3.3V
* GND → GND
* INH → GND

### Mapeamento

| Canal | Função    |
| ----- | --------- |
| 0     | Cutoff    |
| 1     | Resonance |
| 2     | Attack    |
| 3     | Decay     |
| 4     | Sustain   |
| 5     | Release   |
| 6     | Swing     |
| 7     | Livre     |

Leitura **round-robin**, 1 canal por tick, smoothing leve.

---

## 🎚️ Pot direto

* GPIO **35**
* Função: **Volume master**

---

## 🔘 MUX #2 — CD4051 (Botões + controles discretos)

### Canais conectados

| Canal | Hardware         | Uso               |
| ----- | ---------------- | ----------------- |
| 0     | Botão táctil     | Função / modo     |
| 1     | Botão táctil     | Função / modo     |
| 2     | Botão táctil     | Função / modo     |
| 3     | Botão táctil     | Função / modo     |
| 4     | Reservado        | Teclado resistivo |
| 5     | Chave SPDT on–on | Toggle            |
| 6     | Potenciômetro    | Macro             |
| 7     | Potenciômetro    | Macro             |

---

## 🔁 Encoder rotativo

* CLK → GPIO 32
* DT → GPIO 33
* SW → GPIO 27

Funções:

* Rotação → altera parâmetro do modo atual
* Clique → troca modo

---

## 🟩 TM1638 (Sequencer físico)

### Conexões

* STB → GPIO 4
* CLK → GPIO 2
* DIO → GPIO 15
* VCC → 3.3V
* GND → GND

### Recursos

* 8 LEDs → passos + cursor
* 8 botões → ativa/desativa passos
* 8 displays 7-seg → BPM / nota / swing

---

## 🖥️ OLED 0.96" (I2C)

* SDA → GPIO 23
* SCL → GPIO 22
* Endereço: **0x3C**
* Clock I2C: **400 kHz**

Mostra:

* BPM
* Step atual
* Nota
* Swing
* Modo
* Barras dos pots

Atualização ~16 Hz (throttled).

---

## ⏱️ Sequencer

* 8 passos
* Cada passo:

  * ativo / inativo
  * nota MIDI
  * accent (reservado)
* BPM real (1 passo = 1 batida)
* Swing aplicado por ticks

---

## 🧩 Arquitetura Geral

```
           +-------------------+
           |       ESP32       |
           |                   |
           |  GPIO25 → DAC OUT |----> Audio Out
           |                   |
           |  GPIO18/19/21     |----> MUX #1 (Pots)
           |  GPIO34 (ADC)     |
           |                   |
           |  ADC (dedicado)   |----> MUX #2 (Btns/Pots)
           |                   |
           |  GPIO32/33/27     |----> Encoder
           |                   |
           |  GPIO23/22 (I2C)  |----> OLED
           |                   |
           |  GPIO4/2/15       |----> TM1638
           +-------------------+
```

---

## ⏱️ Arquitetura do Sequencer (ASCII)

```
 [BPM]
   |
   v
Ticks por Step
   |
   v
+-------------------+
| Tick Counter     |
+-------------------+
          |
          v
+-------------------+
| Step Advance      |----> currentStep
+-------------------+
          |
          v
+-------------------+
| Note Trigger      |----> mtof() -> Oscil
+-------------------+
          |
          v
+-------------------+
| ADSR Envelope     |
+-------------------+
```

---

## 🎹 Teclado Resistivo (Planejado)

```
[Teclas]
   |   |   |
  R1  R2  R3  ...
   \   |   /
    +--+--+
        |
      ADC (MUX #2 CH4)
```

* Uma leitura ADC
* Thresholds fixos
* Quantização direta em notas

---

## 🚀 Próximos Passos

* Integração lógica do MUX #2
* Accent por passo
* Scale / Quantize
* Pattern memory
* Teclado resistivo
* Op-amp + saída estéreo

---

✔ **Estado atual**: hardware completo, áudio estável, arquitetura sólida

