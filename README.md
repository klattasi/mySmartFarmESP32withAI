# Smart Farm ESP32 with AI - Hardware User Guide

## Overview

This project uses an **ESP32 DevKit V2** microcontroller to build a smart farm monitoring system with AI capabilities. The guide covers hardware connections, sensor configuration, and usage instructions.

---

## Table of Contents

1. [System Specifications](#system-specifications)
2. [Hardware Components](#hardware-components)
3. [GPIO Configuration](#gpio-configuration)
4. [Relay System](#relay-system)
5. [Switch Inputs](#switch-inputs)
6. [Display Connections](#display-connections)
7. [Sensors](#sensors)
8. [Communication Interfaces](#communication-interfaces)
9. [Quick Start Guide](#quick-start-guide)

---

## System Specifications

### Microcontroller Specifications
- **Board**: ESP32 DevKit V2
- **Processor**: Dual-core Xtensa 32-bit CPU
- **RAM**: 520 KB SRAM
- **Flash Memory**: 4 MB (typical)
- **Operating Voltage**: 3.3V

---

## Hardware Components

### LED Status Indicator
- **Function**: Visual indicator of board status
- **GPIO Pin**: GPIO2
- **Type**: Built-in status LED

---

## GPIO Configuration

### Overview
| Component | GPIO | Logic | Function |
|-----------|------|-------|----------|
| Status LED | GPIO2 | Active High | Board status indicator |
| Relay 1 | GPIO4 | Active Low | Control output 1 |
| Relay 2 | GPIO16 | Active Low | Control output 2 |
| Relay 3 | GPIO17 | Active Low | Control output 3 |
| Switch SW1 | GPIO34 | Active Low | User input 1 |
| Switch SW2 | GPIO35 | Active Low | User input 2 |
| Switch SW3 | GPIO32 | Active Low | User input 3 |
| Isolated Input ISO1 | GPIO33 | Active Low | Isolated input 1 (10-24V) |
| Isolated Input ISO2 | GPIO27 | Active Low | Isolated input 2 (10-24V) |
| I2C SDA (OLED) | GPIO21 | - | Display data line |
| I2C SCL (OLED) | GPIO22 | - | Display clock line |
| DS18B20 Data | GPIO14 | - | Temperature sensor (1-Wire) |
| UART0 TX | GPIO1 | - | Serial communication |
| UART0 RX | GPIO3 | - | Serial communication |

---

## Relay System

### Configuration
Three relays are available for controlling external devices:

| Relay | GPIO | Logic Type | Connection |
|-------|------|-----------|-----------|
| Relay 1 | GPIO4 | Active Low | Device 1 |
| Relay 2 | GPIO16 | Active Low | Device 2 |
| Relay 3 | GPIO17 | Active Low | Device 3 |

### Usage
- **Active Low**: LOW signal (0V) activates the relay
- **Active High**: HIGH signal (3.3V) deactivates the relay

---

## Switch Inputs

### Configuration
Three hardware switches with pull-up resistors:

| Switch | GPIO | Pull-up | Logic | Function |
|--------|------|---------|-------|----------|
| SW1 | GPIO34 | 10kΩ | Active Low | Navigation / Mode Select |
| SW2 | GPIO35 | 10kΩ | Active Low | Function 2 |
| SW3 | GPIO32 | 10kΩ | Active Low | Function 3 |

### Usage Notes
- All switches are **Active Low** (pressed = 0V)
- Pull-up resistors already integrate with hardware
- SW1 is used for **OLED page switching** in the current firmware

---

## Display Connections

### OLED 0.96" / 1.3" Display (I2C)

#### Specifications
- **Controller**: SSD1306
- **Resolution**: 128×64 or 128×32 pixels
- **I2C Address**: 0x3C (default) or 0x3D
- **Operating Voltage**: 3.3V (5V NOT recommended)

#### Pin Connection
| OLED Pin | ESP32 DevKit V2 | Function |
|----------|------------------|----------|
| VCC | 3.3V | Power supply |
| GND | GND | Ground |
| SDA | GPIO21 | I2C Data line |
| SCL | GPIO22 | I2C Clock line |
| RST | GPIO5 (optional) | Reset (optional) |

#### Setup Instructions
1. Connect OLED display following the pin table above
2. Ensure I2C address matches (0x3C or 0x3D)
3. Upload firmware and restart ESP32
4. Display will show system information automatically
5. Press **SW1** to switch between display pages

---

## Sensors

### DS18B20 Temperature Sensor (1-Wire Protocol)

#### Specifications
- **Measurement Range**: -55°C to +125°C
- **Accuracy**: ±0.5°C (for -10°C to +85°C)
- **Operating Voltage**: 3.0V - 5.5V (use 3.3V from ESP32)
- **Protocol**: 1-Wire (Dallas/Maxim)
- **Connection Distance**: Up to 100+ meters

#### Pin Connection
| DS18B20 Pin | ESP32 DevKit V2 | Notes |
|-------------|-----------------|-------|
| VCC (Red) | 3.3V | Power supply |
| GND (Black) | GND | Ground |
| Data (Yellow) | GPIO14 | Data line (requires 4.7kΩ pull-up) |

#### Critical Requirements
- **Pull-up Resistor**: **MUST** connect 4.7kΩ resistor between GPIO14 and 3.3V
- **External Power**: Recommended over parasitic power mode
- **Multiple Sensors**: Can connect multiple DS18B20 sensors on the same data line (1-Wire Bus)

#### Usage
- Reads temperature automatically every 2 seconds
- If sensor not found: displays simulated temperature (20-30°C)
- OLED Display: `T:25.5C` (asterisk `*` added if simulated)
- Serial Output:
  - Sensor connected: `DS18B20 Temperature: 25.5 °C`
  - Sensor not found: `Simulated Temperature: 25.5 °C [SENSOR NOT CONNECTED]`

---

### XY-MD03 Temperature & Humidity Sensor (Modbus RTU via RS485)

#### Specifications
- **Type**: Digital temperature & humidity sensor
- **Temperature Range**: -40°C to +125°C
- **Humidity Range**: 0% to 100% RH
- **Temperature Accuracy**: ±0.5°C
- **Humidity Accuracy**: ±3% RH
- **Operating Voltage**: 5-30V DC (use external power supply)
- **Communication Protocol**: Modbus RTU via RS485
- **Default Modbus Slave ID**: 1
- **Default Baud Rate**: 9600 bps

#### Pin Connection
| XY-MD03 Pin | Connection | Function |
|-------------|-----------|----------|
| VCC (Brown) | 5V-24V External Power | Power supply |
| GND (Black) | ESP32 GND | Ground |
| A+ (Yellow) | RS485 Bus A | RS485 signal A |
| B- (Blue) | RS485 Bus B | RS485 signal B |

#### RS485 Interface (MAX13487)

##### Connection Overview
```
XY-MD03 → MAX13487 RS485 Converter → ESP32 Serial0 (GPIO1=TX, GPIO3=RX)
```

##### MAX13487 Connection Table
| ESP32 (UART0) | MAX13487 Converter |
|---------------|-------------------|
| TX0 (GPIO1) | DI (Data Input) |
| RX0 (GPIO3) | RO (Receiver Output) |
| GPIO (RTS control) | DE/RE (Direction Enable) |
| 3.3V | VCC |
| GND | GND |
| MAX13487 A | RS485 Bus A (to sensor) |
| MAX13487 B | RS485 Bus B (to sensor) |

##### Mode Switch Selection
- **RS485 Mode**: Use for XY-MD03 sensor communication
- **RS232/USB Mode**: Use for Serial Monitor and debugging
- **⚠️ Warning**: Serial Monitor will not work in RS485 mode

#### Modbus Registers
| Register | Function | Data Type | Description |
|----------|----------|-----------|-------------|
| 0x0001 | Read Input Register (FC04) | Integer | Temperature (value/10 = °C) |
| 0x0002 | Read Input Register (FC04) | Integer | Humidity (value/10 = %) |

#### Setup Instructions

1. **Switch Configuration**
   - Set the hardware mode switch to **RS485 position**

2. **Wiring**
   - Connect XY-MD03 sensor following the pin table above
   - Ensure proper power supply (5-24V external)
   - Connect RS485 bus to MAX13487

3. **Firmware Upload & Testing**
   - Upload the firmware
   - Restart the ESP32
   - Press **SW1** to navigate to the XY-MD03 page
   - Temperature and humidity readings should appear on OLED

4. **Reading Frequency**
   - Sensor values are read every 3 seconds
   - If sensor not detected: displays simulated values (Temp: 23-30°C, Humidity: 50-80%)

#### OLED Display
The sensor data is displayed on OLED page 2:
```
Status:    Connected (or SIM Mode)
T:26.5 C
H:65.0 %
```

#### Serial Monitor Usage
- If you need to use Serial Monitor while XY-MD03 is connected:
  1. Switch to **RS232 mode**
  2. Comment out XY-MD03 sensor code in firmware
  3. Re-upload and restart

---

## Communication Interfaces

### Serial Communication (UART0)
- **TX Pin**: GPIO1
- **RX Pin**: GPIO3
- **Primary Use**: RS485 via MAX13487 converter for sensor communication
- **Alternative Use**: USB Serial Monitor (when RS232 mode is selected)

### I2C Interface
- **SDA Pin**: GPIO21
- **SCL Pin**: GPIO22
- **Primary Use**: OLED display communication

### 1-Wire Interface
- **Data Pin**: GPIO14
- **Primary Use**: DS18B20 temperature sensor

---

## Isolated Inputs

### Configuration
Two isolated inputs for external digital signals (10-24V TTL compatible):

| Input | GPIO | Voltage Range | Logic | Function |
|-------|------|---------------|-------|----------|
| ISO1 | GPIO33 | 10-24V TTL | Active Low | External input 1 |
| ISO2 | GPIO27 | 10-24V TTL | Active Low | External input 2 |

---

## Quick Start Guide

### Initial Setup
1. **Connect ESP32** to your computer via USB cable
2. **Install PlatformIO** extension in VS Code
3. **Select environment** in PlatformIO (typically `esp32dev`)

### First Test
1. Open `platformio.ini` and verify board settings
2. Connect hardware components as per the pin tables above
3. Build and upload the firmware using PlatformIO
4. Open **Serial Monitor** (set baud rate to 115200) to view debug messages
5. Press **SW1** to navigate display pages

### Hardware Connections Checklist
- [ ] OLED display connected to SDA (GPIO21) and SCL (GPIO22)
- [ ] DS18B20 temperature sensor connected to GPIO14 with 4.7kΩ pull-up
- [ ] XY-MD03 sensor connected via RS485 (if using)
- [ ] Mode switch set to correct position (RS485 for sensors, RS232 for debugging)
- [ ] All power supplies properly connected
- [ ] GND connections verified across all modules

### Troubleshooting

#### OLED Not Displaying
- Verify I2C address (0x3C or 0x3D) matches your display
- Check SDA (GPIO21) and SCL (GPIO22) connections
- Ensure 3.3V power to display (NOT 5V)

#### DS18B20 Shows Simulated Values
- Check GPIO14 connection
- Verify 4.7kΩ pull-up resistor is installed between GPIO14 and 3.3V
- Confirm sensor wiring (Red=3.3V, Black=GND, Yellow=GPIO14)

#### XY-MD03 Not Responding
- Ensure mode switch is in **RS485 position**
- Check RS485 wiring (A+ and B- proper polarity)
- Verify external power supply (5-24V)
- Check baud rate: 9600 bps (Modbus RTU default)
- Verify Modbus ID: 1 (default)

#### Serial Monitor Not Working
- Set mode switch to **RS232 position**
- Comment out XY-MD03 code if active
- Check USB cable connection
- Set baud rate to 115200

---

## Additional Notes

- All GPIO pins use **Active Low logic** for switches and relays (0V = active)
- Power supply should provide adequate current for all devices
- Keep sensor cables as short as possible to minimize noise
- For long-distance sensors, use shielded cables
- Temperature sensors automatically fallback to simulated mode if hardware not found

---

## Document Version
- **Version**: 1.0
- **Last Updated**: March 2026
- **Hardware**: ESP32 DevKit V2 with Smart Farm Configuration

For firmware details, see the `main.cpp` file in the `src/` directory.
