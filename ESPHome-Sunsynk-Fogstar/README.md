# ESPHome: Sunsynk 8K-SG05LP1 + Fogstar batteries

Read-only ESPHome config that polls a Sunsynk single-phase hybrid inverter (SYNK-8K-SG05LP1, same register map as the 5 kW/8 kW family) over RS485 Modbus RTU and exposes it to Home Assistant. Register addresses and scaling come from the Sunsynk single-phase register definitions in the open-source `sunsynk` Python package.

## What you get
- **Battery** (as seen by the inverter): SOC, voltage, current, power, temperature, charge/discharge current limits, BMS-reported SOC and cycle count, energy stored (from the selected battery model and pack count).
- **Solar:** PV1/PV2 power, voltage, current, and total PV power.
- **Grid / load / inverter:** grid power, CT power, voltage, frequency, load power, inverter power/voltage/current/frequency, grid connected.
- **Energy:** daily and lifetime battery charge/discharge, grid import/export, load and PV energy (works with the HA Energy dashboard).
- **Health:** inverter state, fault flag, three temperatures, and the BMS protocol currently set on the inverter.

## Fogstar batteries
Models this is set up for: **FE48-10** (10.24 kWh, 51.2 V 200 Ah) and **FE51.2V-16-LVFB** (16.08 kWh, 51.2 V 314 Ah). In Home Assistant choose your model with the **Battery model** select and set **Battery pack count** if you run several; the **Battery capacity** and **Battery energy stored** sensors follow automatically, and the choice is remembered across reboots. Both ship with an RS485/CAN comms cable.

Fogstar batteries are connected to the inverter by the CAN cable, with the inverter's BMS protocol set to **Pylontech (CAN)** per Fogstar's instructions. The inverter then holds the BMS data in its registers, which is what this config reads, so no extra wiring to the battery is needed. The `BMS protocol` entity shows what the inverter is set to, to confirm lithium comms are active.

Per-cell voltages, individual pack temperatures and BMS alarms are **not** available this way. They need a direct connection to the battery's BMS (RS485 or Bluetooth), which depends on the BMS inside your Fogstar model. Fogstar lists Seplos BMS units on some of its batteries and PACE on others, and I could not confirm which is in the FE48-10 or FE51.2V-16-LVFB: check the BMS label or ask Fogstar.

## Hardware
- An ESP32 board.
- A **3.3 V** RS485 transceiver module (e.g. SN65HVD72 or a MAX3485-based board). Many cheap modules are 5 V, which can damage the ESP32 RX pin; use a 3.3 V one or level-shift.
- An RJ45 plug/cable to the inverter's RS485 port.

Wiring (defaults, change in the `substitutions` block):

| ESP32 | RS485 module |
|---|---|
| GPIO17 (TX) | DI |
| GPIO16 (RX) | RO |
| 3V3 / GND | VCC / GND |
| optional GPIO4 | DE+RE tied together (uncomment `flow_control_pin`; auto-direction modules don't need it) |

Inverter side: connect module **A** and **B** to the inverter's RS485 RJ45 port. Solar Assistant's Sunsynk/Deye cable guide lists pin 5 = A and pin 4 = B; some forum posts report pins 7 and 8 on certain units, so **check your inverter manual** for the port on your unit. If nothing responds, swap A and B. On some newer units the port is labelled "BMS 485/CAN" rather than RS485. Keep Modbus on the port intended for it, not the battery's.

Serial settings are 9600 8N1, slave address 1 (the Sunsynk default).

## Setup
1. `cp secrets.yaml.example secrets.yaml` and fill it in (`openssl rand -base64 32` for the API key).
2. Edit `substitutions` in `sunsynk-fogstar.yaml`: board and pins.
   After the device is added to Home Assistant, set **Battery model** and **Battery pack count**.
3. `esphome run sunsynk-fogstar.yaml` (first flash over USB, later over WiFi), then add the device in Home Assistant.

## Notes
- Read-only on purpose. Writing registers (grid charge, time-of-use programs, limits) can change how the inverter treats your battery; add those deliberately and one at a time.
- The Sunsynk convention is signed values: battery power/current and grid power change sign with direction. Check on your unit which direction is positive and adjust the filters if you want the opposite.
- Registers are polled every 10 s; energy, temperature and limit registers every 30-300 s (`skip_updates`).
- This was validated with `esphome config` (valid) but has not been compiled or run against a real inverter.
