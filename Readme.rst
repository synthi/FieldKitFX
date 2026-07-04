What is this
============

This is a custom fork of FieldKitFX firmware originally released by Koma Elektronik.
The original module is a eurorack effects/looper module based on STM32F303VC.

This fork adds new DSP effects, a long-press UI system for ENV submode selection,
and implements new audio processing modules (Ebb&LFO, with more planned).

Use at your own risk.

Building
========

It's configured as a PlatformIO project. See https://platformio.org/ for details.

.. code-block:: bash

    git clone --recursive https://github.com/synthi/FieldKitFX.git
    cd FieldKitFX
    platformio run

Currently configured to use SWDIO headers, but DFU over USB should also work.

License
=======

GNU General Public License v3.0

UI Overview
===========

The module has two main switches and a multi-color RGB loop button:

**FX Selector (2 positions): LOOP / SHIFTER**
**Rollo Selector (2 positions): SEQ / ENV**

FX Mode (Switch = SHIFTER)
==========================

In this mode, the loop button cycles through DSP effects with a short press.
A **long press (>2s)** enters ENV submode selection (LED blinks).

Available effects (cycled with short press):

+-----------------+-------------+---------------------+---------------------+
| Effect          | LED Color   | Control 1 (CV2)     | Control 2 (CV1)     |
+=================+=============+=====================+=====================+
| Bypass          | Off         | -                   | -                   |
+-----------------+-------------+---------------------+---------------------+
| Frequency       | Blue        | Shift amount        | USB/LSB crossfade   |
| Shifter         |             |                     |                     |
+-----------------+-------------+---------------------+---------------------+
| Thru-Zero       | Pink        | Modulation depth    | Modulation rate     |
| Flanger         |             |                     |                     |
+-----------------+-------------+---------------------+---------------------+
| Phaser          | Green       | Depth (<12h) /      | Allpass coefficient |
|                 |             | Rate (>12h)         |                     |
+-----------------+-------------+---------------------+---------------------+
| Comb Filter     | Orange      | Delay time          | Feedback (+/-)      |
+-----------------+-------------+---------------------+---------------------+
| Decimator       | Red         | Sample rate         | Bit depth           |
|                 |             | reduction           | reduction           |
+-----------------+-------------+---------------------+---------------------+

ENV Submode Selection (Switch = SHIFTER + long press >2s)
==========================================================

When in FX mode, holding the loop button for more than 2 seconds enters
ENV submode selection. The LED blinks to indicate selection mode.
Short press cycles between available submodes. Long press exits.

+-----------------+-------------+---------------------------------------------------+
| Submode         | LED Color   | Description                                       |
+=================+=============+===================================================+
| ADSR            | Green       | Original ADSR envelope (Attack, Decay, Sustain,   |
|                 |             | Release) with Gate/CV threshold mode              |
+-----------------+-------------+---------------------------------------------------+
| Ebb & LFO       | Cyan        | Tides-like slope generator / LFO with Rate,       |
|                 |             | Slope, Shape, Fold. 5 output modes via threshold  |
|                 |             | pot: Gate/Oneshot, Unipolar, Bipolar, EOA, EOR    |
+-----------------+-------------+---------------------------------------------------+

Looper Mode (Switch = LOOP)
============================

The looper records to external SPI SRAM (~3.6 seconds at 48kHz/12bit).

States: ARMED (green) -> RECORD (red) -> PLAYBACK (blue) -> OVERDUB (purple) -> ERASE (white)

- Short press: start/stop recording, toggle overdub
- DSP effects are applied BEFORE the looper (effects are recorded into the loop)

CV Routing Matrix
=================

11 destinations, 5 CV sources (4 physical CV inputs + internal reference).
Each destination can be cycled through sources by pressing the corresponding button.

Destinations: AMOUNT, CONTROL, TRESHOLD, TIME, FBACK, CUTOFF, FBACK (SPRING), VCA1-4

Sequencer (Rollo = SEQ)
=======================

4-step sequencer with CV outputs via DAC.
Each step uses one Roll-O-Deck knob for its CV value.
Threshold pot controls step timing (gate or CV mode).

Release history
===============

1.0.0 (current)
---------------

* Replaced Barberpole Phaser with Frequency Shifter (original effect restored)
* Replaced Chorus with Comb Filter
* Added long-press UI system for ENV submode selection
* Implemented Ebb & LFO (Tides-like slope generator/LFO)
* Fixed PlatformIO board configuration for modern toolchain
* Added stmlib submodule dependency

0.3.1
-----

* Replaced frequency shifter with barberpole phaser effect.
* Added more DSP effects

0.3.0
-----

* DSP effects are independent from looper and can be switch
* "Shift" menu will be used for settings, currenly you can switch effect type here with loop button or disable effect processing
* DSP effect is applied on signal before looper, which means that it would be recored by looper. Second effect slot would be added for wet looper signal later.
* Looper will always run and won't be reset by toggling the other menu

0.2.0
-----

* Rewrote most of the code from C to C++
* Deleted some unused files (ring buffer, debug)
* Fixed DC blocker - previously it wasn't removing DC

0.1.0
-----

* Rewritten all audio processing code to use floating point values
* Increased MCU frequency

0.0.1
-----

Initial version, no new features added - just migrated to platformio project and resolved a few build issues to get everything to work.