#include <ESP32Servo.h>
#include <Adafruit_Fingerprint.h>
#include <WiFi.h>
#include <HTTPClient.h>

const char* ssid = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";

String botToken = "YOUR_TELEGRAM_BOT_TOKEN";
String chatID = "YOUR_CHAT_ID";

HardwareSerial mySerial(2);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

// Pins
int greenLED = 25;
int redLED = 33;
Servo doorLock;
int servoPin = 18;

// 🔥 Door Sensor
int doorSensor = 27;
bool doorAlertSent = false;
int initialDoorState;   // 🔥 important

// Security
int wrongAttempts = 0;
bool systemLocked = false;
unsigned long lockStartTime = 0;
int lastSecond = -1;

// Alerts
bool alertSent = false;
bool lockAlertSent = false;

void setup() {
  Serial.begin(115200);
  mySerial.begin(57600, SERIAL_8N1, 16, 17);

  WiFi.begin(ssid, password);
  Serial.print("Connecting WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi Connected");

  pinMode(greenLED, OUTPUT);
  pinMode(redLED, OUTPUT);
  pinMode(doorSensor, INPUT);

  doorLock.attach(servoPin);
  doorLock.write(0);

  digitalWrite(greenLED, LOW);
  digitalWrite(redLED, LOW);

  finger.begin(57600);

  if (finger.verifyPassword()) {
    Serial.println("System Ready");
  } else {
    Serial.println("Sensor Error");
    while (1);
  }
}

void loop() {

  // 🔒 LOCK MODE
  if (systemLocked) {

    int elapsed = (millis() - lockStartTime) / 1000;
    int remaining = 15 - elapsed;

    doorLock.write(0);

    // 🔥 DOOR SENSOR LOGIC (FIXED)
    int currentDoorState = digitalRead(doorSensor);

    if (currentDoorState != initialDoorState && !doorAlertSent) {
      Serial.println("🚨 DOOR FORCED OPEN!");
      sendDoorAlert();
      doorAlertSent = true;
    }

    if (lastSecond == -1) {
      Serial.println("SYSTEM LOCKED");
    }

    if (remaining != lastSecond && remaining >= 0) {
      Serial.print("00:");
      if (remaining < 10) Serial.print("0");
      Serial.println(remaining);
      lastSecond = remaining;
    }

    digitalWrite(redLED, HIGH);
    digitalWrite(greenLED, LOW);

    if (!lockAlertSent) {
      sendLockAlert();
      lockAlertSent = true;
    }

    if (millis() - lockStartTime >= 15000) {
      systemLocked = false;
      wrongAttempts = 0;
      lastSecond = -1;
      lockAlertSent = false;
      alertSent = false;
      doorAlertSent = false;

      digitalWrite(redLED, LOW);
      Serial.println("SYSTEM UNLOCKED");
    }

    return;
  }

  int id = getFingerprintID();

  if (id == 0) {
    digitalWrite(greenLED, LOW);
    digitalWrite(redLED, LOW);
    return;
  }

  if (id == -1) {
    wrongAttempts++;

    Serial.print("ACCESS DENIED | Attempt: ");
    Serial.println(wrongAttempts);

    digitalWrite(redLED, HIGH);
    delay(500);
    digitalWrite(redLED, LOW);

    sendTelegramAlert();

    if (wrongAttempts >= 3) {
      systemLocked = true;
      lockStartTime = millis();

      // 🔥 CAPTURE DOOR STATE AT LOCK
      initialDoorState = digitalRead(doorSensor);

      Serial.println("SYSTEM LOCKED");
    }

    return;
  }

  delay(3000);

  Serial.print("ACCESS GRANTED | ID: ");
  Serial.println(id);
  Serial.println("DOOR OPEN");

  digitalWrite(greenLED, HIGH);
  doorLock.write(90);
  delay(3000);

  doorLock.write(0);
  digitalWrite(greenLED, LOW);

  alertSent = false;
}

// 🔐 URL ENCODE
String urlencode(String str) {
  String encoded = "";
  char c;
  char code0;
  char code1;

  for (int i = 0; i < str.length(); i++) {
    c = str.charAt(i);

    if (isalnum(c)) {
      encoded += c;
    } else {
      encoded += '%';
      code0 = (c >> 4) & 0xF;
      code1 = c & 0xF;

      encoded += char(code0 > 9 ? code0 + 'A' - 10 : code0 + '0');
      encoded += char(code1 > 9 ? code1 + 'A' - 10 : code1 + '0');
    }
  }
  return encoded;
}

// 🔥 TELEGRAM ALERT
void sendTelegramAlert() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    String message = "🚨ALERT!!\n Unauthorized Access Attempt\nAttempts: " + String(wrongAttempts);

    String url = "https://api.telegram.org/bot" + botToken +
                 "/sendMessage?chat_id=" + chatID +
                 "&text=" + urlencode(message);

    http.begin(url);
    http.GET();
    http.end();

    Serial.println("Telegram Alert Sent");
  }
}

// 🔥 LOCK ALERT
void sendLockAlert() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    String message = "🚫 SYSTEM LOCKED!!!\nToo many failed attempts!";

    String url = "https://api.telegram.org/bot" + botToken +
                 "/sendMessage?chat_id=" + chatID +
                 "&text=" + urlencode(message);

    http.begin(url);
    http.GET();
    http.end();

    Serial.println("Lock Alert Sent");
  }
}

// 🔥 DOOR ALERT
void sendDoorAlert() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    String message = "🚨 DOOR FORCED OPEN!";

    String url = "https://api.telegram.org/bot" + botToken +
                 "/sendMessage?chat_id=" + chatID +
                 "&text=" + urlencode(message);

    http.begin(url);
    http.GET();
    http.end();

    Serial.println("Door Alert Sent");
  }
}

// 🔥 FINGERPRINT
int getFingerprintID() {
  int p = finger.getImage();

  if (p == FINGERPRINT_NOFINGER) return 0;
  if (p != FINGERPRINT_OK) return -1;

  if (finger.image2Tz() != FINGERPRINT_OK) return -1;
  if (finger.fingerFastSearch() != FINGERPRINT_OK) return -1;

  return finger.fingerID;
}