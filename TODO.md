# FieldKitFX — Plan de Desarrollo

## Estado Actual (Commit: a54fe8d)

### ✅ Completado

#### 1. Backup del firmware original
- Rama `master-old` creada desde el commit original y pusheada a origin
- Master actual es el fork de desarrollo

#### 2. Efectos FX DSP (6 efectos)
| # | Efecto | Archivo | Ctrl 1 (CV2) | Ctrl 2 (CV1) | Color LED |
|---|---|---|---|---|---|
| 0 | Bypass | `dsp/bypass.h` | - | - | Apagado |
| 1 | **Frequency Shifter** | `dsp/frequency_shifter.h` | Amount (frecuencia desplazamiento) | Crossfade USB/LSB | Azul |
| 2 | Thru-Zero Flanger | `dsp/tz_flanger.h` | Modulation depth | Modulation rate | Rosa |
| 3 | Phaser | `dsp/phaser.h` | Depth (<12h) / Rate (>12h) | Allpass coefficient | Verde |
| 4 | **Comb Filter** | `dsp/comb_filter.h` | Delay time | Feedback (+/-) | Naranja |
| 5 | Decimator | `dsp/decimator.h` | Sample rate reduction | Bit depth | Rojo |

**Nota**: Barberpole Phaser y Chorus fueron eliminados. Frequency Shifter y Comb Filter fueron recuperados.

#### 3. Sistema de UI con long-press para ENV submode selection
- **FX Selector = LOOP**: botón loop controla looper (ARMED → RECORD → PLAYBACK → OVERDUB → ERASE)
- **FX Selector = SHIFTER**:
  - **Short-press**: cicla efectos FX (Bypass → FreqShift → Flanger → Phaser → Comb → Decimator)
  - **Long-press (>2s)**: LED brillante → entra en **modo selección ENV**
    - **Short-press**: cicla entre 8 submódulos ENV
    - **Long-press**: sale del modo, vuelve a FX normal
    - LED parpadea con color del submódulo activo

#### 4. Ebb&LFO (Tides-like slope generator)
- Archivos: `include/engine/ebb_lfo.h`, `src/engine/ebb_lfo.cc`
- Integrado en UI como `ENV_EBB_LFO`

| Control | Parámetro | Rango | Descripción |
|---|---|---|---|
| Rollo 1 | **Rate** | ~0.01Hz - 100Hz | Velocidad del ciclo |
| Rollo 2 | **Slope** | 0-127 | Curva ataque/decay (0=full attack, 64=triangle, 127=full decay) |
| Rollo 3 | **Shape** | 0-127 | Forma de onda (0=sine, 48=triangle, 64=saw, 96=square, 127=PWM) |
| Rollo 4 | **Fold** | 0-127 | Wavefolding amount |
| Threshold pot | **Output mode** | 5 zonas | Ver tabla abajo |
| CV input (matriz) | **Gate o V/Oct** | - | Depende del modo |

**Threshold pot — 5 zonas de output mode:**
| Zona | Modo | Comportamiento |
|---|---|---|
| 0 (CCW) | **Gate/Oneshot** | AD one-shot, CV input = gate/reset |
| 1 | **Unipolar** | LFO libre, salida 0-5V, CV input = V/Oct |
| 2 | **Bipolar** | LFO libre, salida -5V a +5V, CV input = V/Oct |
| 3 | **EOA** | End of Attack gate (0 o 5V), CV input = V/Oct |
| 4 (CW) | **EOR** | End of Rise gate (0 o 5V), CV input = V/Oct |

**Algoritmo interno:**
- Acumulador de fase de 32 bits
- Slope remapea la fase lineal a curva ataque/decay
- Shape morphs entre sine → triangle → saw → square → PWM
- Fold aplica wavefolding (reflexión cuando excede límites)
- Salida escalada por Level (fijo a 100% por ahora)

---

## 📋 PENDIENTE

### 5. VCO — 3 Modelos Plaits

**Archivos a crear:**
- `include/engine/vco_engine.h` — Header con los 3 modelos
- `src/engine/vco_engine.cc` — Implementación

**Modelos:**

#### 5a. VCO Classic (Modelo 1 de Plaits v1.2)
"Classic waveshapes with filter"

| Control | Parámetro | Descripción |
|---|---|---|
| Rollo 1 | **Frequency** | Frecuencia base (rango amplio ~20Hz-8kHz) |
| Rollo 2 | **Fine Tune** | +/- 7 semitonos |
| Rollo 3 | **Timbre** | Filter cutoff del LP/HP |
| Rollo 4 | **Harmonics** | Resonance + filter character (CCW=gentle 24dB, CW=harsh 12dB) |
| Threshold | **Morph** | Waveform selection + sub level |
| CV input | **V/Oct** | Tracking de tono |

**Salida**: LP output (filtro pasabajos resonante)
**Algoritmo**: Oscilador VA clásico con formas de onda (seno, tri, sierra, cuadrada) + filtro SVF multimodo de 2 polos. Anti-aliasing por suma de armónicos limitada por frecuencia.

#### 5b. VCO Dual (Modelo 0 de Plaits v1.2)
"Virtual-analog synthesis of classic waveforms"

| Control | Parámetro | Descripción |
|---|---|---|
| Rollo 1 | **Frequency** | Frecuencia base |
| Rollo 2 | **Fine Tune** | +/- 7 semitonos |
| Rollo 3 | **Timbre** | Variable square: narrow pulse → full square → hardsync formants |
| Rollo 4 | **Harmonics** | Detuning entre los dos osciladores |
| Threshold | **Morph** | Variable saw: triangle → saw with notch → Braids CSAW |
| CV input | **V/Oct** | Tracking de tono |

**Salida**: Main mixed output (suma de los dos osciladores)
**Algoritmo**: Dos osciladores VA detunables. El primero morphs entre triángulo y sierra con notch (CSAW de Braids). El segundo morphs entre pulso estrecho y cuadrado con formants por hardsync.

#### 5c. VCO Wavefolder (Modelo 2 de Plaits v1.2)
"Waveshaping / Wavefolding" — misma cadena que Tides a audio rate

| Control | Parámetro | Descripción |
|---|---|---|
| Rollo 1 | **Frequency** | Frecuencia base |
| Rollo 2 | **Fine Tune** | +/- 7 semitonos |
| Rollo 3 | **Timbre** | Wavefolder amount |
| Rollo 4 | **Harmonics** | Waveshaper waveform |
| Threshold | **Morph** | Waveform asymmetry |
| CV input | **V/Oct** | Tracking de tono |

**Salida**: OUT (waveshaper + wavefolder con curva clásica)
**Algoritmo**: Triángulo asimétrico → waveshaper (modela la forma) → wavefolder (pliega cuando excede límites). Misma cadena que Tides pero a frecuencia de audio.

**Integración en UI:**
- 3 entradas separadas en el enum `EnvSubmode`: `ENV_VCO_CLASSIC`, `ENV_VCO_DUAL`, `ENV_VCO_WAVEFOLDER`
- Cada una con su propio color de LED (amarillo, naranja, naranja-rojo)
- En `UI_tresholdcv_update`, cada caso llama a `vco_engine.updateParams()` con el modelo correspondiente

**Implementación sugerida:**
```cpp
class VcoEngine {
public:
    enum Model { CLASSIC, DUAL, WAVEFOLDER };
    
    void init();
    void reset();
    void setModel(Model model);
    void updateParams(uint16_t cv1, uint16_t cv2, uint16_t cv3, uint16_t cv4,
                      uint16_t threshold_cv, uint8_t gate_in);
    void process();  // Llamado desde timer de audio
    uint16_t getOutput();  // 0-4095 para DAC
    
private:
    uint32_t phase;
    uint32_t phase_increment;
    Model current_model;
    
    // Parámetros
    int16_t pitch;
    int16_t fine_tune;
    int timbre;
    int harmonics;
    int morph;
    
    // Procesamiento por modelo
    void processClassic(float* out);
    void processDual(float* out);
    void processWavefolder(float* out);
};
```

---

### 6. Envelope Follower

**Archivos a crear:**
- `include/engine/env_follower.h`
- `src/engine/env_follower.cc`

| Control | Parámetro | Descripción |
|---|---|---|
| Rollo 1 | **Attack** | Tiempo de ataque del follower (qué tan rápido responde a subidas) |
| Rollo 2 | **Decay** | Tiempo de decay (qué tan rápido cae cuando la señal baja) |
| Rollo 3 | **Sensitivity** | Ganancia de entrada (pre-amplificación) |
| Rollo 4 | - | Sin uso |
| Threshold | **Threshold** | Nivel mínimo para activar la salida |
| Entrada | **Audio codec** | Lee del buffer de audio (`user_audio_in_buffer`) |

**Salida**: DAC (0-4095, 0-5V)

**Algoritmo:**
```cpp
// Envelope follower con tiempos separados de attack/decay
float process(float in) {
    float rectified = fabs(in) * sensitivity;
    if (rectified > envelope) {
        // Attack
        envelope += (rectified - envelope) * attack_coef;
    } else {
        // Decay
        envelope += (rectified - envelope) * decay_coef;
    }
    // Aplicar threshold
    if (envelope < threshold) envelope = 0;
    return envelope;
}
```

**Integración en UI:**
- En `UI_tresholdcv_update`, caso `ENV_FOLLOWER`:
  - Lee los 4 knobs como Attack, Decay, Sensitivity
  - Lee threshold pot como threshold
  - Procesa el envelope follower desde el buffer de audio
  - Escribe salida al DAC

**Consideraciones:**
- El envelope follower debe procesarse en el loop de audio (48kHz), no en UI (100Hz)
- Usar `MagnitudeTracker` existente o implementar nuevo follower
- La salida debe actualizar el DAC periódicamente

---

### 7. Quantizer

**Archivos a crear:**
- `include/engine/quantizer.h`
- `src/engine/quantizer.cc`

| Control | Parámetro | Descripción |
|---|---|---|
| Rollo 1 | **Scale** | Tipo de escala: mayor, menor, pentatónica mayor, pentatónica menor, cromática, whole tone, diminished, etc. |
| Rollo 2 | **Root** | Nota tónica: C, C#, D, D#, E, F, F#, G, G#, A, A#, B |
| Rollo 3 | **Octave** | -2, -1, 0, +1, +2 |
| Rollo 4 | **Transpose** | Semitonos: -12 a +12 |
| Threshold | **CV in** | Voltaje a cuantizar (simula entrada CV) |
| CV input | - | Sin uso (el CV a cuantizar viene del threshold pot) |

**Salida**: DAC (0-4095, cuantizado a la escala seleccionada)

**Escalas predefinidas:**
```cpp
const int scales[8][12] = {
    {0, 2, 4, 5, 7, 9, 11},       // Major
    {0, 2, 3, 5, 7, 8, 10},       // Minor
    {0, 2, 4, 7, 9},              // Pentatonic Major
    {0, 3, 5, 7, 10},             // Pentatonic Minor
    {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}, // Chromatic
    {0, 2, 4, 6, 8, 10},          // Whole Tone
    {0, 3, 6, 9},                 // Diminished
    {0, 2, 3, 5, 7, 8, 10},       // Dorian
};
```

**Algoritmo:**
1. Leer CV in del threshold pot (0-4095, mapeado a ~0-5V)
2. Convertir a nota MIDI (0-127, donde 0=C-2, 60=C4, 127=G8)
3. Encontrar la nota más cercana en la escala seleccionada
4. Aplicar transpose y octave
5. Convertir de vuelta a voltaje (0-4095 para DAC)
6. Salida al DAC

**Integración en UI:**
- En `UI_tresholdcv_update`, caso `ENV_QUANTIZER`:
  - Lee knobs como Scale, Root, Octave, Transpose
  - Lee threshold pot como CV in
  - Procesa cuantización
  - Escribe al DAC

---

### 8. Sample & Hold

**Archivos a crear:**
- `include/engine/sample_hold.h`
- `src/engine/sample_hold.cc`

| Control | Parámetro | Descripción |
|---|---|---|
| Rollo 1 | **Slew** | Suavizado de la salida (tiempo de transición entre muestras) |
| Rollo 2 | **Attenuator** | Atenuación del sample input (0-100%) |
| Rollo 3 | **Quantize** | Cuantización a 1V/oct (semitonos) — on/off |
| Rollo 4 | **Range** | Rango de notas de salida (pre-quant, limita el rango) |
| Threshold | **Rate control** | Sin CV asignado → rate interno. Con CV → threshold/gate (como en sequencer) |
| CV input | **Clock** | Por matriz, para disparar el sample |
| CV4 | **Sample input** | Siempre. Fallback a ruido pink interno si no hay señal |

**Ruido pink interno:**
```cpp
// LFSR 16-bit + LP filter = ruido pink usable
static uint16_t lfsr = 0xACE1;
lfsr = (lfsr >> 1) ^ (-(lfsr & 1) & 0xB400);
float white = (float)(int16_t)lfsr / 32768.0f;
pink = pink * 0.997f + white * 0.03f;  // LP filter
```

**Algoritmo:**
1. Detectar clock (gate rising edge del CV input o rate interno)
2. En cada clock: samplear el valor del sample input (o ruido pink)
3. Aplicar attenuator
4. Aplicar quantize a 1V/oct si está activado
5. Aplicar range (limitar rango de notas)
6. Aplicar slew (filtro pasa-bajos en la salida)
7. Escribir al DAC

**Rate control (threshold pot):**
- Sin CV asignado (threshold pot controla rate interno): el S/H samplea a intervalos regulares
- Con CV asignado (threshold pot > 0): el CV actúa como gate, samplea en rising edge
- Threshold a 0: espera un gate externo

**Integración en UI:**
- En `UI_tresholdcv_update`, caso `ENV_SAMPLE_HOLD`:
  - Lee knobs como Slew, Attenuator, Quantize, Range
  - Lee threshold pot como rate control
  - Lee CV input como clock
  - Procesa sample & hold
  - Escribe al DAC

---

## Resumen de Archivos a Crear

| Archivo | Submódulo | Líneas estimadas |
|---|---|---|
| `include/engine/vco_engine.h` | VCO (3 modelos) | ~120 |
| `src/engine/vco_engine.cc` | VCO (3 modelos) | ~400 |
| `include/engine/env_follower.h` | Envelope Follower | ~50 |
| `src/engine/env_follower.cc` | Envelope Follower | ~100 |
| `include/engine/quantizer.h` | Quantizer | ~60 |
| `src/engine/quantizer.cc` | Quantizer | ~150 |
| `include/engine/sample_hold.h` | Sample & Hold | ~70 |
| `src/engine/sample_hold.cc` | Sample & Hold | ~200 |

## Archivos a Modificar

| Archivo | Cambio |
|---|---|
| `include/ui/ui.h` | Ya incluye los 8 EnvSubmode. Agregar includes para los nuevos headers |
| `src/ui/ui.cc` | Agregar casos en `UI_tresholdcv_update` para cada nuevo submódulo |
| `src/main.cc` | Posiblemente agregar inicialización de los nuevos módulos y procesamiento en el loop de audio |

## Orden de Implementación Sugerido

1. **VCO** (3 modelos) — ~500 líneas, el más complejo pero el que más impacto tiene
2. **Envelope Follower** — ~150 líneas, relativamente simple
3. **Quantizer** — ~200 líneas, algoritmo straightforward
4. **Sample & Hold** — ~250 líneas, el más complejo después del VCO

## Notas Técnicas

- **CPU Budget**: ~50% utilizado actualmente. Los nuevos módulos agregarán ~10-15% adicional. Hay margen.
- **Flash**: ~256KB disponibles. Los nuevos módulos agregarán ~10-15KB de código.
- **RAM**: ~40KB SRAM + 128KB SPI SRAM. Los nuevos módulos usarán ~2-4KB adicionales.
- **DAC**: Salida única de 12 bits. Todos los submódulos ENV comparten la misma salida DAC.
- **Audio**: El VCO podría mezclarse con la salida de audio además del DAC para sonido directamente audible.