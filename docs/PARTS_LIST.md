# Mini Smart Greenhouse — Parts List (Rev C)

## Controller
- **ESP32-C3 Super Mini**

## Power
- **12 V DC Power Supply** (≥ 3 A recommended, 4–5 A ideal)
- **LM2596 Step-Down (Buck) Converter** (12 V → 5 V)
- **IRLZ44N N-channel MOSFETs**
- **96 °C Thermal Fuse** (for heat mat)

## Sensors
- **DS18B20** Temperature Probe
- **SHT31-D** Temperature / Humidity Sensor
- **CAP-SW-12** Capacitive Soil Moisture Sensor v1.2
- **DS1307** Real Time Clock

## Peripherals
- **USB Plant Heating Pad** (5 V, 2 A, ~8.5 W)
- **FCOB LED Light Strip** (12 V, ~8 W/m, ~0.67 A/m)
- **Ventilation Fans (x2)** (5 V, ~2.85 W each)
- **Status LED** (3 mm, Red)

## Resistors
- **220 Ω** — MOSFET gate
- **330 Ω** — LEDs
- **4.7 kΩ** — DS18B20 pull-up
- **10 kΩ** — MOSFET gate pull-down

## Diodes
- **1N5819** — Flyback diodes for fans

## Capacitors — Ceramic
- **100 nF (0.1 µF)** — Primary decoupling
  - Near ESP32 power pins
  - Near sensors (SHT31, soil sensor)
  - Noise suppression on analog lines

## Capacitors — Electrolytic
- **47 µF (25 V)**
  - 12 V input rail
  - LED strip power branch
  - Fan power branches
  - LM2596 input and output
  - 5 V rail bulk smoothing
