#include <Servo.h>
#include <LiquidCrystal.h>

#define LED_PIN 6
#define HEAT_FAN_PIN 13
#define COOL_FAN_PIN 11
#define SERVO_PIN 10

#define TEMP_SENSOR A1
#define MOISTURE_SENSOR A0
#define CONTRAST_PIN 9

LiquidCrystal lcd1(12, 7, 5, 4, 3, 2);
LiquidCrystal lcd2(12, 8, 5, 4, 3, 2);

Servo valveServo;

bool servoActive = false;
unsigned long servoStartTime = 0;
String lastStatus = "";
String inputBuffer = "";

void setup() {
  Serial.begin(9600);
  pinMode(LED_PIN, OUTPUT);
  pinMode(HEAT_FAN_PIN, OUTPUT);
  pinMode(COOL_FAN_PIN, OUTPUT);
  pinMode(CONTRAST_PIN, OUTPUT);
  analogWrite(CONTRAST_PIN, 50);

  lcd1.begin(16, 2);
  lcd2.begin(16, 2);
  lcd1.display();
  lcd2.display();

  valveServo.attach(SERVO_PIN);
  valveServo.write(90);

  lcd1.setCursor(0, 0);
  lcd1.print("System Ready");
  lcd2.setCursor(0, 0);
  lcd2.print("Last status:");
  lcd2.setCursor(0, 1);
  lcd2.print("Idle");
  delay(1000);
}

void loop() {
  // === Temperature ===
  int tempReading = analogRead(TEMP_SENSOR);
  float voltage = tempReading * (5.0 / 1023.0);
  float temperatureF = (voltage - 0.5) * 100.0 * 9.0 / 5.0 + 32;

  // === Moisture ===
  int moisture = analogRead(MOISTURE_SENSOR);

  // === Send data as JSON ===
    Serial.print("{");
  Serial.print("\"temperature\":");
  Serial.print(temperatureF);
  Serial.print(",\"moisture\":");
  Serial.print(moisture);
  Serial.println("}");

  // === Update LCD ===
  lcd1.setCursor(0, 0);
  lcd1.print("Temp: ");
  lcd1.print(temperatureF, 1);
  lcd1.print(" F   ");

  lcd1.setCursor(0, 1);
  lcd1.print("Moist: ");
  lcd1.print(moisture);
  lcd1.print("   ");

  // === Fan logic ===
  static bool heatFanOn = false;
  static bool coolFanOn = false;

  if (temperatureF < 60 && !heatFanOn) {
    digitalWrite(HEAT_FAN_PIN, HIGH);
    digitalWrite(COOL_FAN_PIN, LOW);
    heatFanOn = true;
    coolFanOn = false;
    showLCD2Status("Heat Fan ON");
  } else if (temperatureF > 80 && !coolFanOn) {
    digitalWrite(COOL_FAN_PIN, HIGH);
    digitalWrite(HEAT_FAN_PIN, LOW);
    coolFanOn = true;
    heatFanOn = false;
    showLCD2Status("Cool Fan ON");
  } else if (temperatureF >= 60 && temperatureF <= 80) {
    if (heatFanOn || coolFanOn) {
      digitalWrite(HEAT_FAN_PIN, LOW);
      digitalWrite(COOL_FAN_PIN, LOW);
      heatFanOn = false;
      coolFanOn = false;
      showLCD2Status("Fans OFF");
    }
  }

  // === Serial input JSON command ===
  while (Serial.available()) {
    char inChar = Serial.read();
    if (inChar == '\n') {
      handleCommand(inputBuffer);
      inputBuffer = "";
    } else {
      inputBuffer += inChar;
    }
  }

  // === Servo timeout ===
  if (servoActive && millis() - servoStartTime >= 5000) {
    valveServo.write(90);
    servoActive = false;
    showLCD2Status("Watering OFF");
  }

  delay(300);
}

void showLCD2Status(String status) {
  if (status != lastStatus) {
    lcd2.clear();
    lcd2.setCursor(0, 0);
    lcd2.print("Last status:");
    lcd2.setCursor(0, 1);
    lcd2.print(status);
    lastStatus = status;
  }
}

void handleCommand(String cmd) {
  if (cmd.indexOf("\"device\":\"coolfan\"") >= 0) {
    if (cmd.indexOf("\"action\":\"on\"") >= 0) {
      digitalWrite(COOL_FAN_PIN, HIGH);
      showLCD2Status("Fan ON");
    } else if (cmd.indexOf("\"action\":\"off\"") >= 0) {
      digitalWrite(COOL_FAN_PIN, LOW);
      showLCD2Status("Fan OFF");
    }
    }
  else if (cmd.indexOf("\"device\":\"heatfan\"") >= 0) {
    if (cmd.indexOf("\"action\":\"on\"") >= 0) {
      digitalWrite(HEAT_FAN_PIN, HIGH);
      showLCD2Status("Fan ON");
    } else if (cmd.indexOf("\"action\":\"off\"") >= 0) {
      digitalWrite(HEAT_FAN_PIN, LOW);
      showLCD2Status("Fan OFF");
    }
  } else if (cmd.indexOf("\"device\":\"led\"") >= 0) {
    if (cmd.indexOf("\"action\":\"on\"") >= 0) {
      digitalWrite(LED_PIN, HIGH);
      showLCD2Status("LED ON");
    } else if (cmd.indexOf("\"action\":\"off\"") >= 0) {
      digitalWrite(LED_PIN, LOW);
      showLCD2Status("LED OFF");
    }
  } else if (cmd.indexOf("\"device\":\"water\"") >= 0) {
    if (!servoActive) {
      valveServo.write(0); // open valve
      servoStartTime = millis();
      servoActive = true;
      showLCD2Status("Watering ON");
    }
  }

}
