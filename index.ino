#include "ThingSpeak.h"
#include <ESP32Servo.h> // For ESP32 servo motor
#include <WiFi.h>
#include <math.h> // Needed for sin()


#define TRIG 19
#define ECHO 18
#define BUZZER 23
#define RAIN_PIN 35
#define BUTTON_PIN 33
#define SERVO_PIN 15

Servo myServo;

// ----- WIFI -----
const char *ssid = "sneha";
const char *password = "aaaaaaaa";

// ----- THINGSPEAK -----
unsigned long channelID = 3189837;
const char *writeAPIKey = "QS2EXVXCD3OATGGA";

WiFiClient client;

// ----- BASE GPS VALUES -----
double baseLatitude = 28.66467;
double baseLongitude = 77.23246;

// Drift strength (smaller = less movement)
double gpsDelta = 0.00005; // ~5 meters drift

// ---------------- SETUP ----------------
void setup() {
  Serial.begin(115200);

  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  myServo.attach(SERVO_PIN);
  myServo.write(0);

  // WiFi connect
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected!");

  ThingSpeak.begin(client);
}

long getDistance() {
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);

  long duration = pulseIn(ECHO, HIGH, 30000);
  long distance = duration * 0.034 / 2;
  return distance;
}

// ---------------- LOOP ----------------
void loop() {

  int buttonState = digitalRead(BUTTON_PIN);
  long distance = getDistance();
  int rainValue = analogRead(RAIN_PIN);

  int buzzerState = 0;
  int servoState = 0;

  // ------------------------------------------------
  // GPS DATA
  // ------------------------------------------------
  unsigned long t = millis();
  double drift = gpsDelta * sin(t / 20000.0);

  double latitude = baseLatitude + drift;
  double longitude = baseLongitude + drift;

  // ------------------------------------------------
  // SERVO LOGIC (ultrasonic detection)
  // ------------------------------------------------
  if (distance > 0 && distance < 20) {
    myServo.write(90);
    servoState = 1;
  } else {
    myServo.write(0);
    servoState = 0;
  }

  // ------------------------------------------------
  // BUZZER LOGIC (Rain + Button Only)
  // ------------------------------------------------
  if (buttonState == LOW || rainValue < 2000) {
    digitalWrite(BUZZER, HIGH);
    buzzerState = 1;
  } else {
    digitalWrite(BUZZER, LOW);
    buzzerState = 0;
  }

  // ------------------------------------------------
  // SERIAL OUTPUT (every ~200ms)
  // ------------------------------------------------
  Serial.print("LAT: ");
  Serial.println(latitude, 6);
  Serial.print("LON: ");
  Serial.println(longitude, 6);
  Serial.print("Button: ");
  Serial.println(buttonState);
  Serial.print("Distance: ");
  Serial.println(distance);
  Serial.print("Rain: ");
  Serial.println(rainValue);
  Serial.print("Servo: ");
  Serial.println(servoState);
  Serial.print("Buzzer: ");
  Serial.println(buzzerState);
  Serial.println("----------------------------");

  delay(200); // keep serial readable + instant sensor updates

  // ------------------------------------------------
  // SEND TO THINGSPEAK (every 15 seconds)
  // ------------------------------------------------
  ThingSpeak.setField(1, distance);
  ThingSpeak.setField(2, rainValue);
  ThingSpeak.setField(3, buttonState);
  ThingSpeak.setField(4, (float)latitude);
  ThingSpeak.setField(5, (float)longitude);
  ThingSpeak.setField(6, buzzerState);
  ThingSpeak.setField(7, servoState);

  ThingSpeak.writeFields(channelID, writeAPIKey);
}
