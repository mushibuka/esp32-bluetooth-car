/*
  Bluetooth Car — ESP32 port
  Uses NimBLE-Arduino to advertise a Nordic UART Service (NUS),
  which is what the Adafruit Bluefruit Control Pad app expects to connect to.

  Motor Controller pin config (TB6612FNG-style driver):
    PWMA - GPIO 25   AIN2 - GPIO 26   AIN1 - GPIO 27
    BIN1 - GPIO 14   BIN2 - GPIO 12   PWMB - GPIO 13
    VCC  - 3v3       Standby - 3v3    VM - Li+

  Adjust the GPIO numbers below to match your actual breadboard wiring —
  these are just common "safe" GPIOs on most ESP32 dev boards (avoid strapping
  pins like GPIO 0, 2, 15 and input-only pins like GPIO 34-39 for outputs).
*/

#include <NimBLEDevice.h>

// ---- Nordic UART Service UUIDs (what Bluefruit Control Pad connects to) ----
#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E" // app writes here
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E" // notify (optional)

// ---- Motor driver pins ----
const int PWMA = 25;
const int AIN2 = 26;
const int AIN1 = 27;
const int BIN1 = 14;
const int BIN2 = 12;
const int PWMB = 13;
const int LED_PIN = 2;   // onboard LED on most ESP32 dev boards

const int SPEED = 200;   // 0-255 PWM duty cycle

NimBLEServer* pServer = nullptr;
bool deviceConnected = false;

// ---------------- Motor control functions ----------------
void stopMotors() {
  analogWrite(PWMA, 0);
  analogWrite(PWMB, 0);
}

void driveForward() {
  digitalWrite(AIN1, HIGH);
  digitalWrite(AIN2, LOW);
  digitalWrite(BIN1, HIGH);
  digitalWrite(BIN2, LOW);
  analogWrite(PWMA, SPEED);
  analogWrite(PWMB, SPEED);
}

void driveBackward() {
  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, HIGH);
  digitalWrite(BIN1, LOW);
  digitalWrite(BIN2, HIGH);
  analogWrite(PWMA, SPEED);
  analogWrite(PWMB, SPEED);
}

void turnLeft() {
  // pivot turn: left motor backward, right motor forward
  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, HIGH);
  digitalWrite(BIN1, HIGH);
  digitalWrite(BIN2, LOW);
  analogWrite(PWMA, SPEED);
  analogWrite(PWMB, SPEED);
}

void turnRight() {
  // pivot turn: left motor forward, right motor backward
  digitalWrite(AIN1, HIGH);
  digitalWrite(AIN2, LOW);
  digitalWrite(BIN1, LOW);
  digitalWrite(BIN2, HIGH);
  analogWrite(PWMA, SPEED);
  analogWrite(PWMB, SPEED);
}

// ---------------- Bluefruit packet parsing ----------------
// Packet format from Adafruit Bluefruit Control Pad:
//   ['!']['B'][button][state][CRC]
//   button: '1'-'4' = numbered buttons, '5'-'8' = Up/Down/Left/Right
//   state:  '1' = pressed, '0' = released
void onDataReceived(const uint8_t* data, size_t len) {
  if (len < 4) return;
  if (data[0] != '!' || data[1] != 'B') return;

  char button = data[2];
  char state  = data[3];
  bool pressed = (state == '1');

  switch (button) {
    case '1': // Button 1 -> LED toggle placeholder
      digitalWrite(LED_PIN, pressed ? HIGH : LOW);
      break;

    case '5': // Up
      if (pressed) driveForward();
      else stopMotors();
      break;

    case '6': // Down
      if (pressed) driveBackward();
      else stopMotors();
      break;

    case '7': // Left
      if (pressed) turnLeft();
      else stopMotors();
      break;

    case '8': // Right
      if (pressed) turnRight();
      else stopMotors();
      break;

    // '2', '3', '4' left open — add your own actions here
    default:
      break;
  }
}

// ---------------- BLE callbacks ----------------
class ServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) override {
    deviceConnected = true;
    Serial.println("BLE client connected");
  }
  void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) override {
    deviceConnected = false;
    stopMotors();
    Serial.println("BLE client disconnected — restarting advertising");
    NimBLEDevice::startAdvertising();
  }
};

class RxCallbacks : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
    NimBLEAttValue rxValue = pCharacteristic->getValue();
    if (rxValue.length() > 0) {
      onDataReceived(rxValue.data(), rxValue.length());
    }
  }
};

// ---------------- Setup ----------------
void setup() {
  Serial.begin(115200);

  pinMode(PWMA, OUTPUT);
  pinMode(AIN1, OUTPUT);
  pinMode(AIN2, OUTPUT);
  pinMode(BIN1, OUTPUT);
  pinMode(BIN2, OUTPUT);
  pinMode(PWMB, OUTPUT);
  pinMode(LED_PIN, OUTPUT);

  NimBLEDevice::init("ESP32 BLE Car");

  pServer = NimBLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());

  NimBLEService* pService = pServer->createService(SERVICE_UUID);

  NimBLECharacteristic* pTxCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID_TX,
      NIMBLE_PROPERTY::NOTIFY
  );

  NimBLECharacteristic* pRxCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID_RX,
      NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR
  );
  pRxCharacteristic->setCallbacks(new RxCallbacks());

  pService->start();

  NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->start();

  Serial.println("Waiting for a BLE client connection (open Bluefruit Connect app)...");
}

void loop() {
  // Nothing needed here — motor control happens in the BLE write callback.
}
