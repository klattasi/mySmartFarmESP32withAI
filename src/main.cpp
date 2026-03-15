#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ModbusMaster.h>

// ============================================================
//  OLED Configuration  (SSD1306  128×64  I2C)
//  SDA = GPIO21,  SCL = GPIO22
// ============================================================
#define SCREEN_WIDTH   128
#define SCREEN_HEIGHT   64
#define OLED_RESET      -1      // no hardware reset pin
#define OLED_I2C_ADDR   0x3C
#define SDA_PIN         21
#define SCL_PIN         22

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ============================================================
//  RS485 via MAX13487  (Auto-Direction, no DE/RE GPIO needed)
//  UART0 :  TX0 = GPIO1,  RX0 = GPIO3
// ============================================================
#define RS485_BAUD      9600

// ============================================================
//  PZEM-017 DC Power Meter  —  Modbus RTU
//  Default Slave ID : 0x01
//  Register map (FC 0x04 – Read Input Registers):
//    0x0000  Voltage    LSB = 0.01 V
//    0x0001  Current    LSB = 0.01 A
//    0x0002  Power  LO  ┐
//    0x0003  Power  HI  ┘  (32-bit)  LSB = 0.1 W
//    0x0004  Energy LO  ┐
//    0x0005  Energy HI  ┘  (32-bit)  LSB = 1 Wh
// ============================================================
#define PZEM017_ADDR    0x01

ModbusMaster node;

// ============================================================
//  Sensor Data
// ============================================================
float  g_voltage  = 0.0f;
float  g_current  = 0.0f;
float  g_power    = 0.0f;
float  g_energy   = 0.0f;   // stored in kWh
bool   g_dataOK   = false;

// ============================================================
//  Timing
// ============================================================
unsigned long g_lastRead    = 0;
const uint32_t READ_INTERVAL = 5000;   // ms

// ============================================================
//  Prototypes
// ============================================================
bool readPZEM017();
void updateOLED();
void showSplash();

// ============================================================
//  Setup
// ============================================================
void setup() {
  // UART0 — RS485 (MAX13487 auto-direction)
  Serial.begin(RS485_BAUD, SERIAL_8N1);   // TX=GPIO1, RX=GPIO3

  // OLED
  Wire.begin(SDA_PIN, SCL_PIN);
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDR)) {
    for (;;) delay(1000);   // halt if OLED not found
  }

  showSplash();
  delay(2000);

  // Modbus master — no preTransmission/postTransmission needed
  // because MAX13487 handles bus-direction automatically
  node.begin(PZEM017_ADDR, Serial);

  // Force first read immediately
  g_lastRead = millis() - READ_INTERVAL;
}

// ============================================================
//  Main Loop
// ============================================================
void loop() {
  if (millis() - g_lastRead >= READ_INTERVAL) {
    g_lastRead = millis();
    g_dataOK   = readPZEM017();
    updateOLED();
  }
}

// ============================================================
//  Read PZEM-017 — returns true on success
// ============================================================
bool readPZEM017() {
  // Read 6 input registers starting from 0x0000  (FC 0x04)
  uint8_t status = node.readInputRegisters(0x0000, 6);
  if (status != node.ku8MBSuccess) {
    return false;
  }

  g_voltage = node.getResponseBuffer(0) / 100.0f;          // 0.01 V resolution
  g_current = node.getResponseBuffer(1) / 100.0f;          // 0.01 A resolution

  uint32_t powerRaw  = ((uint32_t)node.getResponseBuffer(3) << 16)
                     |  (uint32_t)node.getResponseBuffer(2);
  g_power = powerRaw / 10.0f;                               // 0.1 W resolution

  uint32_t energyRaw = ((uint32_t)node.getResponseBuffer(5) << 16)
                     |  (uint32_t)node.getResponseBuffer(4);
  g_energy = energyRaw / 1000.0f;                          // Wh → kWh

  return true;
}

// ============================================================
//  OLED Layout  (128 × 64)
//
//   y= 0  │ "PZEM-017  DC METER"   textSize 1
//   y= 9  │ ─────────────────────── (separator line)
//   y=11  │ V: [voltage] V          textSize 2  (label size 1)
//   y=29  │ I: [current] A          textSize 2  (label size 1)
//   y=47  │ P: [power]  W           textSize 1
//   y=56  │ E: [energy] kWh         textSize 1
// ============================================================
void updateOLED() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  // ---- Error Screen ----
  if (!g_dataOK) {
    display.setTextSize(1);
    display.setCursor(22, 0);
    display.print(F("PZEM-017 ERROR"));
    display.drawLine(0, 9, 127, 9, SSD1306_WHITE);
    display.setCursor(4, 20);
    display.print(F("No Sensor Response"));
    display.setCursor(4, 33);
    display.print(F("Check RS485 wiring"));
    display.setCursor(4, 46);
    display.print(F("& 5-30V power supply"));
    display.display();
    return;
  }

  // ---- Title ----
  display.setTextSize(1);
  display.setCursor(14, 0);
  display.print(F("PZEM-017 DC METER"));
  display.drawLine(0, 9, 127, 9, SSD1306_WHITE);

  // ---- Voltage  (TextSize 2) ----
  display.setTextSize(1);
  display.setCursor(0, 13);
  display.print(F("V:"));
  display.setTextSize(2);
  display.setCursor(16, 11);
  display.print(g_voltage, 1);
  display.setTextSize(1);
  display.setCursor(106, 13);
  display.print(F("V"));

  // ---- Current  (TextSize 2) ----
  display.setTextSize(1);
  display.setCursor(0, 31);
  display.print(F("I:"));
  display.setTextSize(2);
  display.setCursor(16, 29);
  display.print(g_current, 2);
  display.setTextSize(1);
  display.setCursor(106, 31);
  display.print(F("A"));

  // ---- Power  (TextSize 1) ----
  display.setTextSize(1);
  display.setCursor(0, 47);
  display.print(F("P:"));
  display.setCursor(14, 47);
  display.print(g_power, 1);
  display.print(F(" W"));

  // ---- Energy  (TextSize 1) ----
  display.setCursor(0, 56);
  display.print(F("E:"));
  display.setCursor(14, 56);
  display.print(g_energy, 3);
  display.print(F(" kWh"));

  display.display();
}

// ============================================================
//  Splash / Startup Screen
// ============================================================
void showSplash() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor(22, 6);
  display.print(F("Smart Farm ESP32"));

  display.setCursor(32, 20);
  display.print(F("PZEM-017"));

  display.setCursor(18, 33);
  display.print(F("DC Power Monitor"));

  display.setCursor(20, 48);
  display.print(F("RS485 Modbus RTU"));

  display.display();
}