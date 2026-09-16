#include <Adafruit_Fingerprint.h>

HardwareSerial mySerial(2);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

// LEDs
int greenLED = 2;
int redLED = 4;

// Security
int wrongAttempts = 0;
bool systemLocked = false;
unsigned long lockStartTime = 0;

void setup() {
  Serial.begin(115200);
  mySerial.begin(57600, SERIAL_8N1, 16, 17);

  pinMode(greenLED, OUTPUT);
  pinMode(redLED, OUTPUT);

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
    digitalWrite(redLED, HIGH);

    if (millis() - lockStartTime > 15000) {
      systemLocked = false;
      wrongAttempts = 0;
      digitalWrite(redLED, LOW);
      Serial.println("System Unlocked");
    }
    return;
  }

  int id = getFingerprintID();

  // ✅ ACCESS GRANTED
  if (id > 0) {
    Serial.print("Access Granted ID: ");
    Serial.println(id);

    digitalWrite(greenLED, HIGH);
    delay(2000);
    digitalWrite(greenLED, LOW);

    wrongAttempts = 0;
  }

  // ❌ ACCESS DENIED
  else if (id == -1) {
    Serial.println("Access Denied");

    digitalWrite(redLED, HIGH);
    delay(300);
    digitalWrite(redLED, LOW);

    wrongAttempts++;

    Serial.print("Wrong Attempts: ");
    Serial.println(wrongAttempts);

    if (wrongAttempts >= 3) {
      systemLocked = true;
      lockStartTime = millis();
      Serial.println("SYSTEM LOCKED FOR 15 SEC");
    }
  }
}

// Fingerprint function
int getFingerprintID() {
  int p = finger.getImage();

  if (p == FINGERPRINT_NOFINGER) return 0;
  if (p != FINGERPRINT_OK) return -1;

  if (finger.image2Tz() != FINGERPRINT_OK) return -1;
  if (finger.fingerFastSearch() != FINGERPRINT_OK) return -1;

  return finger.fingerID;
}