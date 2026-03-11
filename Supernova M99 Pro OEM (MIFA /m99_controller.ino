#include <SPI.h>
#include <mcp_can.h>

const int SPI_CS_PIN = 10;
const int BUTTON_PIN = 4;
const int BRAKE_PIN  = 7;   // Bremssensor

MCP_CAN CAN(SPI_CS_PIN);

// CAN Timing
unsigned long lastCanSend = 0;
const unsigned long sendInterval = 100;

// Button Timing
const unsigned long debounceTime = 40;
const unsigned long longPressTime = 700;

bool buttonLast = HIGH;
bool buttonState = HIGH;
unsigned long lastDebounce = 0;
unsigned long pressStart = 0;
bool longPressHandled = false;

// Lichtzustände
bool highBeam = false;

enum LowMode {
  LOW_MIN,
  LOW_MAX
};

LowMode lowMode = LOW_MAX;

// Speed Simulation
const uint16_t speedMin = 500;   // 5 km/h
const uint16_t speedMax = 3000;  // 30 km/h

void sendFrame(unsigned long id, byte len, byte *data) {
  CAN.sendMsgBuf(id, 0, len, data);
}

void sendM99() {
  // 0x400 Light ON
  byte msg400[1] = {0x01};
  sendFrame(0x400, 1, msg400);

  // 0x402 Battery FULL
  byte msg402[4] = {100, 0, 0, 0};
  sendFrame(0x402, 4, msg402);

  // 0x201 Speed
  uint16_t speed = (lowMode == LOW_MIN) ? speedMin : speedMax;

  byte msg201[2] = {
    (byte)(speed & 0xFF),
    (byte)(speed >> 8)
  };
  sendFrame(0x201, 2, msg201);

  // 0x501 Frontlight
  byte frontMode = 0x04; // speed dependent

  if (highBeam) {
    frontMode |= 0x01;
  }

  byte msg501[1] = {frontMode};
  sendFrame(0x501, 1, msg501);

  // 0x503 Brakelight
  // Sensorlogik ggf. hier umdrehen:
  // LOW = aktiv  oder HIGH = aktiv
  bool brakeActive = (digitalRead(BRAKE_PIN) == LOW);

  byte msg503[1] = { brakeActive ? 0x01 : 0x00 };
  sendFrame(0x503, 1, msg503);
}

void shortPress() {
  highBeam = !highBeam;
}

void longPress() {
  if (lowMode == LOW_MIN) {
    lowMode = LOW_MAX;
  } else {
    lowMode = LOW_MIN;
  }
}

void handleButton() {
  bool reading = digitalRead(BUTTON_PIN);

  if (reading != buttonLast) {
    lastDebounce = millis();
    buttonLast = reading;
  }

  if ((millis() - lastDebounce) > debounceTime) {
    if (reading != buttonState) {
      buttonState = reading;

      if (buttonState == LOW) {
        pressStart = millis();
        longPressHandled = false;
      } else {
        unsigned long duration = millis() - pressStart;

        if (!longPressHandled && duration < longPressTime) {
          shortPress();
        }
      }
    }
  }

  if (buttonState == LOW && !longPressHandled) {
    if ((millis() - pressStart) >= longPressTime) {
      longPressHandled = true;
      longPress();
    }
  }
}

void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(BRAKE_PIN, INPUT_PULLUP);

  Serial.begin(115200);

  CAN.begin(MCP_ANY, CAN_250KBPS, MCP_8MHZ);
  CAN.setMode(MCP_NORMAL);
}

void loop() {
  handleButton();

  if (millis() - lastCanSend >= sendInterval) {
    lastCanSend = millis();
    sendM99();
  }
}
