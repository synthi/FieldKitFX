/*
 * ebb_lfo.cc
 *
 * Ebb & LFO - A Tides-like slope generator / LFO
 * Based on Mutable Instruments Tides algorithm
 */

#include "engine/ebb_lfo.h"
#include <cmath>

namespace fieldkitfx {

EbbLfo ebb_lfo;

/* Flags for EOA and EOR */
#define FLAG_EOA 1
#define FLAG_EOR 2

/* Constants */
#define EBB_ADC_RESOLUTION 1023.0f
#define EBB_DEADTIME 50

EbbLfo::EbbLfo()
    : phase(0)
    , phase_increment(0)
    , pitch(-3 * 12 * 128)  // ~0.5 Hz default
    , slope(64)
    , shape(48)  // triangle default
    , fold(0)
    , level(100)
    , pitch_mod(0)
    , slope_mod(64)
    , shape_mod(48)
    , fold_mod(0)
    , level_mod(1000)
    , oneshot_mode(false)
    , oneshot_active(false)
    , output_mode(EBB_OUT_UNIPOLAR)
    , gate_state(0)
    , gate_previous(0)
    , deadtime_counter(0)
{
    sample.unipolar = 0;
    sample.bipolar = 0;
    sample.flags = 0;
}

void EbbLfo::init() {
    reset();
}

void EbbLfo::reset() {
    phase = 0;
    oneshot_active = true;
    sample.unipolar = 0;
    sample.bipolar = 0;
    sample.flags = 0;
}

uint32_t EbbLfo::computePhaseIncrement(int16_t pitch) {
    /*
     * Convert pitch (in MIDI-like units, 128 per semitone) to phase increment.
     * pitch = 0 -> ~1 Hz at 48kHz sample rate
     * Each 128 units = 1 semitone = * 2^(1/12)
     * We use a simplified exponential mapping
     */
    float freq;
    if (pitch >= 0) {
        freq = 0.1f + (float)pitch * 0.0005f;
    } else {
        freq = 0.1f / (1.0f + (float)(-pitch) * 0.0005f);
    }
    // Clamp frequency
    if (freq < 0.01f) freq = 0.01f;
    if (freq > 100.0f) freq = 100.0f;

    // Convert to phase increment at 48kHz
    // phase is 32-bit, so full range = 2^32
    // phase_increment = freq * 2^32 / sample_rate
    const float increment = freq * 4294967296.0f / 48000.0f;
    return (uint32_t)increment;
}

void EbbLfo::processSample(int s, int sh, int f, uint32_t p, EbbLfoSample* out) {
    /*
     * Tides-like sample computation
     *
     * s = slope (0-65535): controls the attack/decay balance
     *     s=0: full attack (saw up), s=32768: triangle, s=65535: full decay (saw down)
     *
     * sh = shape (0-65535): controls the waveform shape
     *     0: sine, ~16384: triangle, ~32768: saw, ~49152: square/pulse
     *
     * f = fold (0-32767): wavefolding amount
     *
     * p = phase (0-0xFFFFFFFF): current phase
     */

    // Normalize phase to 0.0-1.0
    float t = (float)p / 4294967296.0f;

    // Apply slope: remap phase based on slope parameter
    // s=0: t stays linear (saw up)
    // s=32768: t becomes triangle (up then down)
    // s=65535: t becomes 1-t (saw down)
    float slope_norm = (float)s / 65535.0f;
    float phase_out;
    if (t < slope_norm) {
        // Rising phase
        phase_out = t / slope_norm;
    } else {
        // Falling phase
        phase_out = (1.0f - t) / (1.0f - slope_norm);
    }

    // Apply shape: waveshape the phase_out
    float shape_norm = (float)sh / 65535.0f;
    float shaped;

    if (shape_norm < 0.25f) {
        // Sine-like (through to triangle)
        float morph = shape_norm * 4.0f;
        float sine_val = std::sin(phase_out * 3.14159265f);
        float tri_val = 2.0f * (phase_out < 0.5f ? phase_out : 1.0f - phase_out);
        shaped = tri_val * morph + sine_val * (1.0f - morph);
    } else if (shape_norm < 0.5f) {
        // Triangle to saw
        float morph = (shape_norm - 0.25f) * 4.0f;
        float tri_val = 2.0f * (phase_out < 0.5f ? phase_out : 1.0f - phase_out);
        float saw_val = phase_out;
        shaped = saw_val * morph + tri_val * (1.0f - morph);
    } else if (shape_norm < 0.75f) {
        // Saw to square/pulse
        float morph = (shape_norm - 0.5f) * 4.0f;
        float saw_val = phase_out;
        float sqr_val = phase_out < 0.5f ? 1.0f : 0.0f;
        shaped = sqr_val * morph + saw_val * (1.0f - morph);
    } else {
        // Pulse width modulation
        float morph = (shape_norm - 0.75f) * 4.0f;
        float pulse_width = 0.1f + morph * 0.8f;
        shaped = phase_out < pulse_width ? 1.0f : 0.0f;
    }

    // Apply fold (wavefolding)
    float fold_norm = (float)f / 32767.0f;
    if (fold_norm > 0.0f) {
        // Simple wavefolding: reflect when exceeding bounds
        shaped = shaped * 2.0f - 1.0f;  // Map to -1..1
        shaped = shaped * (1.0f + fold_norm * 2.0f);
        // Fold
        if (shaped > 1.0f) {
            shaped = 2.0f - shaped;
        } else if (shaped < -1.0f) {
            shaped = -2.0f - shaped;
        }
        shaped = (shaped + 1.0f) * 0.5f;  // Map back to 0..1
        if (shaped < 0.0f) shaped = 0.0f;
        if (shaped > 1.0f) shaped = 1.0f;
    }

    // Compute unipolar output (0-65535)
    out->unipolar = (uint16_t)(shaped * 65535.0f);

    // Compute bipolar output (-32767 to 32767)
    out->bipolar = (int16_t)((shaped - 0.5f) * 65535.0f);

    // Compute flags
    out->flags = 0;
    float eoa_threshold = slope_norm * 0.95f;
    float eor_threshold = 0.05f;
    if (t >= eoa_threshold && t < eoa_threshold + 0.01f) {
        out->flags |= FLAG_EOA;
    }
    if (t >= 1.0f - eor_threshold) {
        out->flags |= FLAG_EOR;
    }
}

int EbbLfo::detectThresholdZone(uint16_t threshold_cv) {
    if (threshold_cv < EBB_THRESHOLD_ZONE_SIZE) {
        return 0;  // Gate mode
    } else if (threshold_cv < EBB_THRESHOLD_ZONE_SIZE * 2) {
        return 1;  // Unipolar
    } else if (threshold_cv < EBB_THRESHOLD_ZONE_SIZE * 3) {
        return 2;  // Bipolar
    } else if (threshold_cv < EBB_THRESHOLD_ZONE_SIZE * 4) {
        return 3;  // EOA
    } else {
        return 4;  // EOR
    }
}

void EbbLfo::updateParams(uint16_t cv1, uint16_t cv2, uint16_t cv3, uint16_t cv4,
                          uint16_t threshold_cv, uint8_t gate_in) {
    /*
     * cv1 = Rate (Rollo 1)
     * cv2 = Slope (Rollo 2)
     * cv3 = Shape (Rollo 3)
     * cv4 = Fold (Rollo 4)
     * threshold_cv = Output mode selection (5 zones)
     * gate_in = Gate signal from CV matrix
     */

    // Map CVs to parameters (CV is 0-1023 after >> 2)
    const float cv_scale = 1.0f / EBB_ADC_RESOLUTION;

    // Rate: map to pitch range
    float rate_norm = (float)cv1 * cv_scale;
    pitch = (int16_t)((rate_norm - 0.5f) * 6000.0f);

    // Slope: 0-127
    slope = (int)((float)cv2 * cv_scale * 127.0f);
    if (slope < 0) slope = 0;
    if (slope > 127) slope = 127;

    // Shape: 0-127
    shape = (int)((float)cv3 * cv_scale * 127.0f);
    if (shape < 0) shape = 0;
    if (shape > 127) shape = 127;

    // Fold: 0-127
    fold = (int)((float)cv4 * cv_scale * 127.0f);
    if (fold < 0) fold = 0;
    if (fold > 127) fold = 127;

    // Threshold: detect zone for output mode
    int zone = detectThresholdZone(threshold_cv);
    switch (zone) {
    case 0:
        // Gate mode: oneshot, CV input acts as gate
        oneshot_mode = true;
        gate_state = gate_in;
        break;
    case 1:
        oneshot_mode = false;
        output_mode = EBB_OUT_UNIPOLAR;
        break;
    case 2:
        oneshot_mode = false;
        output_mode = EBB_OUT_BIPOLAR;
        break;
    case 3:
        oneshot_mode = false;
        output_mode = EBB_OUT_EOA;
        break;
    case 4:
        oneshot_mode = false;
        output_mode = EBB_OUT_EOR;
        break;
    }

    // Apply modulated parameters
    pitch_mod = pitch;
    slope_mod = slope;
    shape_mod = shape;
    fold_mod = fold;
    level_mod = level * 10;  // 0-1000
}

void EbbLfo::process() {
    // Handle gate in oneshot mode
    if (oneshot_mode) {
        gate_previous = gate_state;
        // Gate detection with deadtime
        if (gate_state && !gate_previous) {
            if (deadtime_counter > EBB_DEADTIME) {
                // Rising edge: reset phase
                phase = 0;
                oneshot_active = true;
                deadtime_counter = 0;
            }
        }
        if (!gate_state && gate_previous) {
            deadtime_counter = 0;
        }
        if (deadtime_counter < 65535) {
            deadtime_counter++;
        }

        if (!oneshot_active) {
            sample.unipolar = 0;
            sample.bipolar = 0;
            sample.flags = 0;
            return;
        }
    }

    // Compute phase increment
    phase_increment = computePhaseIncrement(pitch_mod);

    // Store old phase for rollover detection
    uint32_t old_phase = phase;

    // Advance phase
    phase += phase_increment;

    // Check for rollover in oneshot mode
    if (phase < old_phase && oneshot_mode) {
        phase = 0;
        oneshot_active = false;
        sample.unipolar = 0;
        sample.bipolar = 0;
        sample.flags = 0;
        return;
    }

    // Process sample
    int s = slope_mod * 65535 / 127;
    int sh = shape_mod * 65535 / 127;
    int f = fold_mod * 32767 / 127;

    processSample(s, sh, f, phase, &sample);
}

uint16_t EbbLfo::getOutput() {
    // Scale output based on level and output mode
    float scaled;

    switch (output_mode) {
    case EBB_OUT_UNIPOLAR:
        scaled = (float)sample.unipolar / 65535.0f * (float)level_mod / 1000.0f;
        return (uint16_t)(scaled * 4095.0f);

    case EBB_OUT_BIPOLAR:
        scaled = (float)sample.bipolar / 32767.0f * (float)level_mod / 1000.0f;
        // Map bipolar (-1..1) to DAC range (0-4095)
        return (uint16_t)((scaled + 1.0f) * 0.5f * 4095.0f);

    case EBB_OUT_EOA:
        return (sample.flags & FLAG_EOA) ? 4095 : 0;

    case EBB_OUT_EOR:
        return (sample.flags & FLAG_EOR) ? 4095 : 0;

    default:
        return 0;
    }
}

void EbbLfo::setOutputMode(EbbLfoOutputMode mode) {
    output_mode = mode;
}

}