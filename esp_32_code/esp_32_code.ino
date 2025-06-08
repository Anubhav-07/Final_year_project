// ====== Include Libraries and Define Pins ======
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// Motor control pins
#define IN1 15
#define IN2 2
#define IN3 5
#define IN4 18

// Ultrasonic sensor pins
#define TRIG 12
#define ECHO 14

// BLE UUIDs
#define SERVICE_UUID        "12345678-1234-1234-1234-1234567890ab"
#define CMD_CHAR_UUID       "abcdefab-1234-5678-90ab-abcdefabcdef"
#define TEMP_CHAR_UUID      "2a1c"  // Just a placeholder for temperature data

// BLE Characteristics
BLECharacteristic *cmdCharacteristic;
BLECharacteristic *tempCharacteristic;

// To store last command received
char lastCmd = '*';

// ====== Function to Handle Motor Commands ======
void handleCommand(char cmd) {
  switch (cmd) {
    case 'F': // Forward
      Serial.println("Forward");
      digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
      digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
      break;

    case 'B': // Backward
      Serial.println("Backward");
      digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
      digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
      break;

    case 'L': // Left
      Serial.println("Left");
      digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
      digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
      break;

    case 'R': // Right
      Serial.println("Right");
      digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
      digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
      break;

    case 'S': // Stop
      Serial.println("Stop");
      digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
      digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
      break;

    default: // Unknown command
      Serial.println("Unknown Command");
      digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
      digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
      break;
  }
}

// ====== BLE Callback Class ======
class MyCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    String rxValue = pCharacteristic->getValue().c_str();
    if (rxValue.length() > 0) {
      char cmd = rxValue.charAt(0);
      Serial.print("Received Command: ");
      Serial.println(cmd);
      lastCmd = cmd;
      handleCommand(cmd);
    }
  }
};

// ====== Setup Function ======
void setup() {
  Serial.begin(9600);

  // Set pin modes
  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
  pinMode(TRIG, OUTPUT); pinMode(ECHO, INPUT);

  // Initialize BLE
  BLEDevice::init("ESP32_Car_BLE");
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create BLE characteristic for command reception
  cmdCharacteristic = pService->createCharacteristic(
    CMD_CHAR_UUID,
    BLECharacteristic::PROPERTY_WRITE
  );
  cmdCharacteristic->setCallbacks(new MyCallbacks());

  // Create BLE characteristic for sending temperature (placeholder)
  tempCharacteristic = pService->createCharacteristic(
    TEMP_CHAR_UUID,
    BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_READ
  );
  tempCharacteristic->addDescriptor(new BLE2902());
  tempCharacteristic->setValue("Waiting...");

  // Start BLE service
  pService->start();
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->start();

  Serial.println("BLE ready. Waiting for serial temperature data...");
}

// ====== Loop Function ======
void loop() {
  // 1. Send temperature data if available via serial
  if (Serial.available()) {
    String tempStr = Serial.readStringUntil('\n');
    tempStr.trim();
    Serial.print("Sending Temp: ");
    Serial.println(tempStr);
    tempCharacteristic->setValue(tempStr.c_str());
    tempCharacteristic->notify();
  }

  // 2. Obstacle Detection Logic
  long duration, distance;

  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);

  duration = pulseIn(ECHO, HIGH);
  distance = duration * 0.034 / 2;

  if (lastCmd == 'F' && distance > 0 && distance < 15) {
    Serial.println("Obstacle detected! Reversing...");
    handleCommand('B');
    delay(500);
    handleCommand('L');
    delay(500);
    handleCommand('F');
  }

  delay(100);
}
