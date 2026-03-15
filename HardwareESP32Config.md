
# ESP32 DevKit V2 Hardware Configuration

## Overview
This document outlines the hardware configuration for the ESP32 DevKit V2 microcontroller.

## Specifications
- **Microcontroller**: ESP32
- **Board**: DevKit V2
- **Processor**: Dual-core Xtensa 32-bit CPU
- **RAM**: 520 KB SRAM
- **Flash**: 4 MB (typically)


## LED Status Indicator
- **LED**: Status LED on board
- **GPIO**: GPIO2
- **Function**: Indicates board status during various events


## Relay Configuration

| Relay | GPIO | Logic |
|-------|------|-------|
| Relay 1 | GPIO4 | Active Low |
| Relay 2 | GPIO16 | Active Low |
| Relay 3 | GPIO17 | Active Low |


## Switch Configuration

| Switch | GPIO | Pull-up Resistor | Logic |
|--------|------|------------------|-------|
| SW1 | GPIO34 | R10k | Active Low |
| SW2 | GPIO35 | R10k | Active Low |
| SW3 | GPIO32 | R10k | Active Low |


## Isolated Input Configuration

| Input | GPIO | Voltage Range | Logic |
|-------|------|---------------|-------|
| ISO1 | GPIO33 | 10-24V TTL | Active Low |
| ISO2 | GPIO27 | 10-24V TTL | Active Low |


## OLED (I2C) Configuration

### สรุป
- ประเภทที่พบบ่อย: OLED 0.96" / 1.3" driven by SSD1306
- ความละเอียดตัวอย่าง: 128x64 (หรือ 128x32)
- I2C Address ทั่วไป: 0x3C (บางรุ่น 0x3D)
- แรงดัน: 3.3V (ห้ามจ่าย 5V ถ้าจอไม่รองรับ)

### การต่อสาย (ตัวอย่าง)
| OLED Pin | ESP32 DevKit V2 |
|----------|------------------|
| VCC      | 3.3V             |
| GND      | GND              |
| SDA      | GPIO21 (I2C SDA) |
| SCL      | GPIO22 (I2C SCL) |
| RST      | GPIO5 (optional) |


## RS485 (Serial0) — MAX13487, Auto Direction และสวิตช์ RS232/RS485

### สรุปการต่อ
- ใช้ UART0 (Serial0) ของ ESP32 (TX0 = GPIO1, RX0 = GPIO3) ต่อกับ MAX13487 เพื่อเชื่อมบัส RS485
- MAX13487 ทำงานแบบ half‑duplex (A/B) และควบคุมทิศทางด้วย DE/RE (Auto‑Direction โดยให้ MCU/RTS ควบคุม)
- บนบอร์ดมีสวิตช์แบบ SPDT สลับโหมด RS232 <-> RS485 (สลับการเชื่อมต่อ TX/RX ไปยังตัวแปลงที่เหมาะสม)

### ตัวอย่างการต่อสาย
| ESP32 (UART0) | MAX13487 |
|---------------|----------|
| TX0 (GPIO1)   | DI       |
| RX0 (GPIO3)   | RO       |
| control GPIO / UART RTS | DE (และควบคุม /RE) |
| GND           | GND      |
| MAX13487 A    | บัส A    |
| MAX13487 B    | บัส B    |
| VCC           | 3.3V     |


## XY-MD03 Temperature & Humidity Sensor (Modbus RTU via RS485)

### สรุป
- เซ็นเซอร์วัดอุณหภูมิและความชื้นแบบดิจิทัล XY-MD03
- สื่อสารผ่านโปรโตคอล Modbus RTU (RS485)
- ช่วงการวัดอุณหภูมิ: -40°C ถึง +125°C
- ช่วงการวัดความชื้น: 0% ถึง 100% RH
- ความแม่นยำอุณหภูมิ: ±0.5°C
- ความแม่นยำความชื้น: ±3% RH
- แรงดัน: 5-30V DC
- Modbus Slave ID: 1 (default)
- Baud Rate: 9600 (default)

### การต่อสาย
| XY-MD03 Pin | ESP32 DevKit V2 | หมายเหตุ |
|-------------|-----------------|----------|
| VCC (Brown) | 5V - 24V        | แรงดันจ่าย (ใช้ external power) |
| GND (Black) | GND             | กราวด์ (ต่อเข้า ESP32 GND) |
| A+ (Yellow) | RS485 A         | ต่อผ่าน MAX13487 บัส A |
| B- (Blue)   | RS485 B         | ต่อผ่าน MAX13487 บัส B |

### การเชื่อมต่อ
```
XY-MD03 → MAX13487 (RS485) → ESP32 Serial0 (GPIO1=TX, GPIO3=RX)
```

### Switch Mode (RS232/RS485)
- **RS485 Mode**: ใช้สำหรับเชื่อมต่อกับ XY-MD03 Sensor
- **RS232 Mode (USB)**: ใช้สำหรับ Serial Monitor/Debug

⚠️ **คำเตือน**: เมื่อสลับเป็นโหมด RS485 Serial Monitor จะไม่ทำงาน

### Modbus Registers
| Register | Function | Description | Format |
|----------|----------|-------------|--------|
| 0x0001   | Read Input Register (FC04) | Temperature | Value/10 (°C) |
| 0x0002   | Read Input Register (FC04) | Humidity | Value/10 (%) |

### การทำงานของโค้ด
- อ่านค่าทุก 3 วินาที
- หากไม่พบเซ็นเซอร์: แสดงค่า Simulated (Temp: 23-30°C, Humidity: 50-80%)
- แสดงบน OLED หน้าที่ 2:
  - Status: `Connected` หรือ `SIM Mode`
  - Temperature: `T:26.5 C`
  - Humidity: `H:65.0 %`
- สลับหน้าจอด้วยปุ่ม SW1

### วิธีการใช้งาน
1. สลับ Switch เป็นโหมด **RS485**
2. ต่อสาย XY-MD03 ตามตารางข้างต้น
3. Upload โค้ดและรีสตาร์ท ESP32
4. กดปุ่ม SW1 เพื่อสลับไปหน้า XY-MD03
5. ถ้าต้องการใช้ Serial Monitor: สลับกลับเป็นโหมด **RS232** และ comment โค้ด XY-MD03


## DS18B20 Temperature Sensor (1-Wire)

### สรุป
- เซ็นเซอร์วัดอุณหภูมิแบบดิจิทัล DS18B20
- สื่อสารผ่านโปรโตคอล 1-Wire (Dallas/Maxim)
- ช่วงการวัด: -55°C ถึง +125°C
- ความแม่นยำ: ±0.5°C (-10°C ถึง +85°C)
- แรงดัน: 3.0V - 5.5V (ใช้ 3.3V จาก ESP32)
- ต้องใช้ Pull-up Resistor 4.7kΩ ระหว่าง Data กับ VCC

### การต่อสาย
| DS18B20 Pin | ESP32 DevKit V2 | หมายเหตุ |
|-------------|-----------------|----------|
| VCC (Red)   | 3.3V            | แรงดันจ่าย |
| GND (Black) | GND             | กราวด์ |
| DATA (Yellow) | GPIO14        | สาย Data (ต้องใช้ Pull-up 4.7kΩ) |

### คำแนะนำ
- **Pull-up Resistor**: ต้องต่อ Resistor 4.7kΩ ระหว่าง DATA (GPIO14) กับ VCC (3.3V)
- **Parasitic Power**: สามารถใช้โหมด Parasitic Power ได้ (ต่อ VCC กับ GND เข้าด้วยกัน) แต่แนะนำใช้ External Power
- **Multiple Sensors**: สามารถต่อ DS18B20 หลายตัวบนสาย Data เส้นเดียวกันได้ (1-Wire Bus)
- **Distance**: สามารถต่อสายได้ไกลถึง 100+ เมตร (ขึ้นกับคุณภาพสาย)

### การทำงานของโค้ด
- อ่านค่าอุณหภูมิทุก 2 วินาที
- หากไม่พบเซ็นเซอร์: แสดงค่า Simulated Temperature (จำลอง 20-30°C)
- แสดงบน OLED: `T:25.5C` (ถ้า Simulated จะมี `*` ต่อท้าย เช่น `T:25.5C*`)
- แสดงบน Serial Monitor: 
  - เจอเซ็นเซอร์: `DS18B20 Temperature: 25.5 °C`
  - ไม่เจอเซ็นเซอร์: `Simulated Temperature: 25.5 °C [SENSOR NOT CONNECTED]`

















