/*
 * ebb_lfo.h
 *
 * Ebb & LFO - A Tides-like slope generator / LFO
 * Based on Mutable Instruments Tides algorithm
 *
 * Features:
 * - AD (one-shot) and cycling (LFO) modes
 * - Slope, Shape, Fold parameters
 * - Output modes: Unipolar, Bipolar, EOA, EOR
 * - Gate/reset input via CV matrix
 * - V/Oct tracking in LFO mode
 */

#ifndef EBB_LFO_H_
#define EBB_LFO_H_

#include <cstdint>
#include "hardware/adc.h"
#include "hardware/dac.h"
#include "utils/utils.h"

namespace fieldkitfx {

/* Output modes */
enum EbbLfoOutputMode {
    EBB_OUT_UNIPOLAR = 0,
    EBB_OUT_BIPOLAR,
    EBB_OUT_EOA,   // End of Attack gate
    EBB_OUT_EOR,   // End of Rise gate
    EBB_NUM_OUTPUT_MODES
};

/* Threshold zones for output mode selection */
#define EBB_THRESHOLD_ZONE_SIZE (ADC_RESOLUTION_DEZ / 5)

class EbbLfo {
private:
    DISALLOW_COPY_AND_ASSIGN(EbbLfo);

    /* Phase accumulator */
    uint32_t phase;
    uint32_t phase_increment;

    /* Parameters */
    int16_t pitch;       // Frequency (coarse)
    int slope;           // 0-127: attack/decay curve
    int shape;           // 0-127: waveform shape
    int fold;            // 0-127: wavefolding amount
    int level;           // 0-100: output amplitude

    /* Modulated parameters (after CV) */
    int16_t pitch_mod;
    int slope_mod;
    int shape_mod;
    int fold_mod;
    int level_mod;

    /* State */
    bool oneshot_mode;   // true = AD one-shot, false = cycling LFO
    bool oneshot_active; // true when envelope is running in oneshot mode
    EbbLfoOutputMode output_mode;

    /* Gate tracking */
    uint8_t gate_state;
    uint8_t gate_previous;
    uint16_t deadtime_counter;

    /* Output sample */
    struct EbbLfoSample {
        uint16_t unipolar;  // 0-65535
        int16_t bipolar;    // -32767 to 32767
        uint8_t flags;      // FLAG_EOA, FLAG_EOR
    };

    EbbLfoSample sample;

    /* Internal processing */
    void processSample(int s, int sh, int f, uint32_t p, EbbLfoSample* out);
    uint32_t computePhaseIncrement(int16_t pitch);
    int detectThresholdZone(uint16_t threshold_cv);

public:
    EbbLfo();
    void init();
    void reset();

    /* Called from UI at ~100Hz */
    void updateParams(uint16_t cv1, uint16_t cv2, uint16_t cv3, uint16_t cv4,
                      uint16_t threshold_cv, uint8_t gate_in);

    /* Called from audio loop or timer */
    void process();

    /* Get the current output value (0-4095 for DAC) */
    uint16_t getOutput();

    /* Set output mode directly */
    void setOutputMode(EbbLfoOutputMode mode);
    EbbLfoOutputMode getOutputMode() { return output_mode; }

    /* Check if in oneshot mode */
    bool isOneshot() { return oneshot_mode; }
};

extern EbbLfo ebb_lfo;

}

#endif /* EBB_LFO_H_ */