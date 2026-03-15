#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ─────────────────────────────────────────────────────────────────────────────
// OLED  :  SSD1306 128×64  |  I2C SDA=GPIO21  SCL=GPIO22
// RS485 :  MAX13487 Auto-Direction  |  Serial0  TX=GPIO1  RX=GPIO3
// Sensor:  PZEM-017 DC Power Monitor  |  Modbus RTU  9600 baud
// ─────────────────────────────────────────────────────────────────────────────

// ─── OLED ───────────────────────────────────────────────────────────────────
#define SCREEN_W 128
#define SCREEN_H 64
#define OLED_RESET -1
#define OLED_ADDR 0x3C
#define SDA_PIN 21
#define SCL_PIN 22

Adafruit_SSD1306 display(SCREEN_W, SCREEN_H, &Wire, OLED_RESET);

// ─── RS485 / PZEM-017 ───────────────────────────────────────────────────────
#define RS485_BAUD 9600
#define PZEM_ADDR 0x01        // Default Modbus slave ID
#define PZEM_FN_READ 0x04     // Read Input Registers
#define PZEM_REG_START 0x0000 // First register (Voltage)
#define PZEM_REG_COUNT 0x0006 // V, I, P_L, P_H, E_L, E_H
#define PZEM_RSP_LEN 17       // 3 header + 12 data + 2 CRC
#define PZEM_TIMEOUT_MS 600   // Max wait for response

// ─── Timing ─────────────────────────────────────────────────────────────────
#define UPDATE_INTERVAL_MS 4000

// ─── Data ───────────────────────────────────────────────────────────────────
struct PzemData
{
  float voltage; // V   (0.01 V per LSB)
  float current; // A   (0.01 A per LSB)
  float power;   // W   (0.1  W per LSB, 32-bit)
  float energy;  // Wh  (1    Wh per LSB, 32-bit)
  bool valid;
};

// ─── Modbus CRC-16 ──────────────────────────────────────────────────────────
static uint16_t crc16(const uint8_t *buf, uint16_t len)
{
  uint16_t crc = 0xFFFF;
  for (uint16_t i = 0; i < len; i++)
  {
    crc ^= (uint16_t)buf[i];
    for (uint8_t b = 0; b < 8; b++)
      crc = (crc & 1) ? ((crc >> 1) ^ 0xA001) : (crc >> 1);
  }
  return crc;
}

// ─── Send Read Request ───────────────────────────────────────────────────────
static void sendRequest()
{
  uint8_t req[8];
  req[0] = PZEM_ADDR;
  req[1] = PZEM_FN_READ;
  req[2] = (PZEM_REG_START >> 8) & 0xFF;
  req[3] = PZEM_REG_START & 0xFF;
  req[4] = (PZEM_REG_COUNT >> 8) & 0xFF;
  req[5] = PZEM_REG_COUNT & 0xFF;
  uint16_t c = crc16(req, 6);
  req[6] = c & 0xFF;
  req[7] = (c >> 8) & 0xFF;

  while (Serial.available())
    Serial.read(); // Flush RX
  Serial.write(req, 8);
  Serial.flush(); // Wait TX done (MAX13487 auto-flips direction after TX)
}

// ─── Read + Parse Response ──────────────────────────────────────────────────
static PzemData readResponse()
{
  PzemData d = {};
  d.valid = false;

  uint8_t rsp[PZEM_RSP_LEN];
  uint8_t idx = 0;
  unsigned long t0 = millis();

  while (idx < PZEM_RSP_LEN)
  {
    if (millis() - t0 > PZEM_TIMEOUT_MS)
      break;
    if (Serial.available())
      rsp[idx++] = Serial.read();
  }

  if (idx < PZEM_RSP_LEN)
    return d; // Timeout
  if (rsp[0] != PZEM_ADDR || rsp[1] != PZEM_FN_READ)
    return d; // Wrong frame

  // Verify CRC (last 2 bytes, little-endian)
  uint16_t recvCRC = rsp[PZEM_RSP_LEN - 2] |
                     ((uint16_t)rsp[PZEM_RSP_LEN - 1] << 8);
  if (crc16(rsp, PZEM_RSP_LEN - 2) != recvCRC)
    return d; // CRC error

  // Register bytes start at rsp[3] (big-endian per register)
  uint16_t rawV = ((uint16_t)rsp[3] << 8) | rsp[4];
  uint16_t rawI = ((uint16_t)rsp[5] << 8) | rsp[6];
  uint32_t rawPL = ((uint16_t)rsp[7] << 8) | rsp[8];
  uint32_t rawPH = ((uint16_t)rsp[9] << 8) | rsp[10];
  uint32_t rawEL = ((uint16_t)rsp[11] << 8) | rsp[12];
  uint32_t rawEH = ((uint16_t)rsp[13] << 8) | rsp[14];

  d.voltage = rawV * 0.01f;
  d.current = rawI * 0.01f;
  d.power = (rawPH * 65536UL + rawPL) * 0.1f;
  d.energy = rawEH * 65536UL + rawEL;
  d.valid = true;
  return d;
}

// ─── OLED Display ────────────────────────────────────────────────────────────
//  Layout (128×64, textSize 1 = 6×8 px/char)
//  ┌─────────────────────────────┐
//  │   ===  PZEM-017  DC  ===   │  y= 0
//  ├─────────────────────────────┤  y=10 (line)
//  │ V:     00.00  V             │  y=14
//  │ I:     00.00  A             │  y=26
//  │ P:  00000.0   W             │  y=38
//  │ E:  0000000  Wh             │  y=50
//  └─────────────────────────────┘
static void displayData(const PzemData &d)
{
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

  // ── Title ──
  display.setCursor(13, 0);
  display.print("=== PZEM-017 DC ===");
  display.drawFastHLine(0, 10, 128, SSD1306_WHITE);

  if (!d.valid)
  {
    display.setCursor(20, 20);
    display.print("  No Connection!");
    display.setCursor(4, 34);
    display.print("Check RS485 Mode &");
    display.setCursor(16, 48);
    display.print("Sensor Wiring");
  }
  else
  {
    display.setCursor(0, 14);
    display.printf("V: %8.2f  V", d.voltage);

    display.setCursor(0, 26);
    display.printf("I: %8.2f  A", d.current);

    display.setCursor(0, 38);
    display.printf("P: %8.1f  W", d.power);

    display.setCursor(0, 50);
    display.printf("E: %8.0f Wh", d.energy);
  }

  display.display();
}

// ─── Splash Screen ───────────────────────────────────────────────────────────
static void showSplash()
{
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(10, 4);
  display.print("PZEM-017");
  display.setTextSize(1);
  display.setCursor(22, 28);
  display.print("DC Power Monitor");
  display.setCursor(28, 42);
  display.print("Smart Farm");
  display.setCursor(28, 54);
  display.print("Starting...");
  display.display();
}

// ─── Setup ───────────────────────────────────────────────────────────────────
void setup()
{
  Wire.begin(SDA_PIN, SCL_PIN);

  // OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR))
  {
    while (true)
      delay(1000); // Halt — OLED required
  }
  showSplash();

  // Serial0 → RS485 via MAX13487 (auto-direction, no DE/RE GPIO needed)
  Serial.begin(RS485_BAUD);

  delay(2000);
}

// ─── Loop ────────────────────────────────────────────────────────────────────
void loop()
{
  static unsigned long lastRead = 0;
  static bool firstRun = true;

  if (firstRun || (millis() - lastRead >= UPDATE_INTERVAL_MS))
  {
    firstRun = false;
    lastRead = millis();

    sendRequest();
    delay(150); // Allow MAX13487 to flip direction & response to arrive

    PzemData data = readResponse();
    displayData(data);
  }
}