# Miniature Smart Greenhouse — Version 1.0 TODO

This file tracks the concrete tasks required to complete **Version 1.0** of the Miniature Smart Greenhouse project.

---

## Documentation

- [x] TODO (1/19)
- [x] Schematic (1/23)
- [x] README (1/22)
- [x] GitHub Project Setup (1/27)

---

## Hardware

- [x] Tank / plant preparation and sourcing (1/18)
- [x] Solder pins to boards (ESP32 / temp + humidity / etc.) (1/19)
- [x] Solder LED strip such that tank is properly covered (1/19)
- [x] 12 V power source → step-down rail setup (1/20)
- [x] System-wide wiring and power-on test (1/26)

---

## Software

### Testing

- [x] Blink example (1/17)
- [x] Initial DS18B20 (soil temperature) tests (1/17)
- [x] Initial PWM tests (fan) (1/21)
- [x] Initial PWM tests (heater) (1/21)
- [x] Initial PWM tests (LED) (1/21)
- [x] Initial CAP-SW-12 (soil moisture) tests (1/25)
- [x] Initial SHT31 tests (1/25)
- [x] MOSFET prep and solder (1/25)

### Development

#### Architecture

- [x] Overall software architecture / layout (1/19)
- [x] Find / add drivers and libraries (1/24)
- [x] File cleanup (1/26)

#### Read Functions

- [x] `float read_soil_temp_c();` (1/24)
- [x] `float read_soil_moisture_percent();` (1/22)
- [x] `float read_air_temp_c();` (1/24)
- [x] `float read_air_humid_percent();` (1/24)
- [x] `rtc_time_t read_current_time();` (1/24)

#### Update Functions

- [x] `time_state_t update_time_state(rtc_time_t current_time);` (1/20)
- [x] `fan_state_t update_fan_state(float temp_c, float humid_percent, fan_state_t current);` (1/19)
- [x] `led_state_t update_led_state(time_state_t current_time, float temp_c);` (1/20)
- [x] `heater_state_t update_heater_state(float temp_c, heater_state_t current);` (1/20)
- [x] `signal_led_state_t update_soil_moisture_led_state(signal_led_state_t current, float soil_moisture_percent);` (1/20)

#### Run Functions

- [x] `void run_fan(fan_state_t current);` (1/21)
- [x] `void run_led(led_state_t current);` (1/21)
- [x] `void run_signal_led(signal_led_state_t current);` (1/21)
- [x] `void run_heater(heater_state_t current);` (1/21)

---

## Blockers & Dependencies

### Current
- None

### Resolved

- [x] Need to order DS1307 (RTC) — 1/20 → 1/20
- [x] Initial SHT31 tests (modules left in Kawataki) — 1/19 → 1/23
- [x] MOSFET prep and solder (parts in Kawataki) — 1/19 → 1/23
- [x] Waiting on ordered DS1307 (parts in Kawataki) — 1/20 → 1/23

---

*EOF*
