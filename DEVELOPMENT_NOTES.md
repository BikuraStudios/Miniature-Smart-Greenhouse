# Development Notes (WIP)

This document records concrete issues encountered during development of the Miniature Smart Greenhouse project, along with suspected root causes and mitigations.
This is **not** an exhaustive list and will be expanded as development continues.

---

## Hardware

### Inconsistent ESP32-C3 Board Behavior

**Symptom**  
Identical ESP32-C3 Super Mini boards showed inconsistent flashing and USB enumeration behavior.

**Suspected Cause**  
Marginal USB-C connectors or onboard USB circuitry; typical quality variance for low-cost boards.

**Mitigation**  
- Switched to a known-good board for active development  
- Verified power rails and continuity  
- Purchased backup board from a more reputable vendor

---

### Board Pinout Orientation Error

**Symptom**  
Multiple sensors failed to register or produced inconsistent readings during initial wiring.

**Root Cause**  
GPIO connections were made assuming schematic orientation rather than verifying the ESP32-C3 Super Mini’s actual board pinout as populated on the breadboard.

**Mitigation**  
- Re-verified pin numbers against the board’s physical pinout  
- Corrected wiring to match GPIO numbering rather than schematic visual orientation  
- Adopted a policy of validating pinouts against the board itself before first power-on

---

### PWM Output Appeared Non-Functional (LED Test)

**Symptom**  
LED connected to a GPIO using the LEDC example showed no visible output.

**Root Cause**  
On the ESP32-C3 Super Mini variant used, PWM outputs below ~1000 Hz did not produce reliable visible output during testing.

**Mitigation**  
- Reduced test case to a single LED and resistor  
- Validated GPIO output separately  
- Set all PWM frequencies to a minimum of 1000 Hz

---

### MOSFET Package Size Mismatch

**Symptom**  
AO3400 MOSFETs proved impractically small to hand-solder.

**Root Cause**  
Underestimation of SMD package size during part selection.

**Mitigation**  
Replaced with IRLZ44N N-channel MOSFETs suitable for through-hole soldering.

---

## Toolchain & Environment

### USB / Driver Friction on Linux

**Symptom**  
Intermittent flashing failures and unreliable device detection under Linux.

**Root Cause**  
Combination of libusb / permission issues and low-quality USB hardware.

**Mitigation**  
- Temporarily switched to Windows for ESP-IDF flashing   
- Issue not observed to affect runtime behavior once flashing was successful

---

### Multi-Location Development Constraints

**Symptom**  
Hardware, parts, and tools split across home, school, and storage locations.

**Root Cause**  
Physical and logistical constraints.

**Mitigation**  
Structured development so that:
- Software architecture  
- State logic  
- API design  

could proceed independently of missing hardware.

---

## Software Architecture

### Sensor Integration Staging

**Symptom**  
Some sensors (RTC, SHT31) were unavailable during early integration.

**Root Cause**  
Parts delivery timing and physical access constraints.

**Mitigation**  
Defined clear **read / update / run** interfaces early, allowing sensor drivers to be integrated incrementally without refactoring control logic.

---

## System Behavior Observations

### RTC Time Offset Affecting Light Schedule

**Symptom**  
Observed LED light intensity changes occurred earlier than expected relative to wall-clock time.

**Suspected Cause**  
RTC time offset or initialization discrepancy leading to an incorrect internal time state.

**Mitigation**  
- Logged time state transitions alongside RTC values  
- Deferred correction pending longer-term observation and validation

---

## Ruled-Out Issues

- No evidence of I2C bus contention at current bus speed and wiring length
- No observable ADC instability once soil sensor pull-up configuration was finalized

---

## Known Future Risk Areas

- Thermal behavior once enclosure is sealed
- Long-term soil moisture sensor drift and recalibration needs


---

## General Lessons

- Cheap hardware variability should be assumed and planned for.
- Development processes should tolerate missing hardware and partial system availability.
- Verifying physical pinouts is more reliable than trusting schematic orientation alone.
- Early architectural boundaries reduce rework later.

*(Additional lessons to be added as the project evolves.)*

