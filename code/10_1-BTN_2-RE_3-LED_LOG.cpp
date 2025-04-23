// Include necessary libraries for I2C LCD and keypad
#include <Wire.h>                            // for I2C communication
#include <LiquidCrystal_I2C.h>               // for 1602 LCD with I2C
#include <Keypad.h>                          // for 4x4 keypad

LiquidCrystal_I2C lcd(0x27, 16, 2);          // LCD at I2C address 0x27, 16 cols x 2 rows

// Keypad settings
const byte ROWS = 4;
const byte COLS = 4;
char hexaKeys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte rowPins[ROWS] = {9, 8, 7, 6};            // rows connected to digital pins
byte colPins[COLS] = {5, 4, 3, 2};            // columns connected to digital pins
Keypad keypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);

// Pins for buttons and LEDs
const int button1 = A1;                       // left button
const int button2 = A2;                       // right button
const int redLed = 11;
const int yellowLed = 12;
const int greenLed = 10;

// For button timing program
const int sampleSize = 10;
unsigned long leftTimes[sampleSize] = {0};
unsigned long rightTimes[sampleSize] = {0};
int sampleCount = 0;
unsigned long startLeft = 0, startRight = 0;
unsigned long pressLeft = 0, pressRight = 0;
bool leftReleased = false, rightReleased = false;

// Variables for blinking yellow LED
unsigned long lastBlinkTime = 0;
bool yellowBlinkState = false;
bool blinkingYellow = false;

int currentProgram = 0;                        // 0 = menu, 1 = BTN, 2 = React, 3 = LED control

// For reaction game
int expectedNumber = 0;
bool waitingForResponse = false;
int totalResponses = 0;
int correctResponses = 0;
unsigned long responseStartTime = 0;
unsigned long responseEndTime = 0;

// Function declarations
void showMenu();
void resetProgram();
void handleButtonTiming();
void handleLedControl();
void handleReactionGame();
unsigned long calculateAverage(unsigned long arr[]);
void controlLEDs(unsigned long l, unsigned long r);
void waitForAnyKey();

void setup() {
    Serial.begin(9600); // Start Serial connection for USB
  lcd.init();                                 // initialize LCD
  lcd.backlight();                            // turn on LCD backlight

  pinMode(button1, INPUT_PULLUP);             // left button with pull-up
  pinMode(button2, INPUT_PULLUP);             // right button with pull-up
  pinMode(redLed, OUTPUT);
  pinMode(yellowLed, OUTPUT);
  pinMode(greenLed, OUTPUT);

  randomSeed(analogRead(A0));                 // seed random using analog noise

  showMenu();                                 // display menu at startup
}

void loop() {
  char key = keypad.getKey();                 // read key only once per loop

  if (currentProgram == 0) {                  // program selection menu
    if (key == '1') {
      currentProgram = 1;
      lcd.clear(); lcd.print("Program 1: BTN"); delay(1000);
      lcd.clear(); lcd.setCursor(0, 0); lcd.print("1: "); lcd.setCursor(0, 1); lcd.print("2: ");
    } else if (key == '2') {
      currentProgram = 2;
      lcd.clear(); lcd.print("Program 2: React"); delay(1000);
      lcd.clear();
      expectedNumber = random(1, 3);                      // show 1 or 2
      lcd.setCursor(0, 0); lcd.print("Press: "); lcd.print(expectedNumber);
      digitalWrite(yellowLed, HIGH); delay(500); digitalWrite(yellowLed, LOW);
      waitingForResponse = true;
      totalResponses = 0; correctResponses = 0;
    } else if (key == '3') {
      currentProgram = 3;
      lcd.clear(); lcd.print("Program 3: LEDs"); delay(1000); lcd.clear();
    }
  }

  if (currentProgram == 1) {
    handleButtonTiming();
    if (key == '#') resetProgram();           // return to menu
  }

  if (currentProgram == 2) {
    handleReactionGame();
    if (key == '#') resetProgram();           // return to menu
  }

  if (currentProgram == 3) {
    handleLedControl();
    if (key == '#') resetProgram();           // return to menu
    if (key == '0') {
      digitalWrite(redLed, LOW); digitalWrite(yellowLed, LOW); digitalWrite(greenLed, LOW);
      lcd.setCursor(0, 1); lcd.print("LEDs OFF     ");
    }
    if (key == '1') {
      digitalWrite(redLed, HIGH); digitalWrite(yellowLed, LOW); digitalWrite(greenLed, LOW);
      lcd.setCursor(0, 1); lcd.print("RED ON       ");
    }
    if (key == '2') {
      digitalWrite(redLed, LOW); digitalWrite(yellowLed, HIGH); digitalWrite(greenLed, LOW);
      lcd.setCursor(0, 1); lcd.print("YELLOW ON    ");
    }
    if (key == '3') {
      digitalWrite(redLed, LOW); digitalWrite(yellowLed, LOW); digitalWrite(greenLed, HIGH);
      lcd.setCursor(0, 1); lcd.print("GREEN ON     ");
    }
  }
}

// Function for button timing program
void handleButtonTiming() {
  // Blink yellow LED every second if enabled
  if (blinkingYellow) {
    unsigned long now = millis();
    if (now - lastBlinkTime >= 1000) {
      yellowBlinkState = !yellowBlinkState;
      digitalWrite(yellowLed, yellowBlinkState ? HIGH : LOW);
      lastBlinkTime = now;
    }
  }

  // Read button states
  bool leftPressed = (digitalRead(button1) == LOW);
  bool rightPressed = (digitalRead(button2) == LOW);

  // Start timing if left button pressed
  if (leftPressed && startLeft == 0) {
    startLeft = millis(); leftReleased = false;
    blinkingYellow = true;                   // start blinking
    lastBlinkTime = millis();
  }

  // Start timing if right button pressed
  if (rightPressed && startRight == 0) {
    startRight = millis(); rightReleased = false;
    blinkingYellow = true;                   // start blinking
    lastBlinkTime = millis();
  }

  // When left button released
  if (!leftPressed && startLeft > 0 && !leftReleased) {
    pressLeft = millis() - startLeft;
    startLeft = 0; leftReleased = true;
    leftTimes[sampleCount % sampleSize] = pressLeft;     // save time
    lcd.setCursor(3, 0); lcd.print(pressLeft); lcd.print("ms ");

    // (no serial output for individual button presses)
  }

  // When right button released
  if (!rightPressed && startRight > 0 && !rightReleased) {
    pressRight = millis() - startRight;
    startRight = 0; rightReleased = true;
    rightTimes[sampleCount % sampleSize] = pressRight;   // save time
    lcd.setCursor(3, 1); lcd.print(pressRight); lcd.print("ms ");

    // (no serial output for individual button presses)
  }

  // After both buttons released
  if (leftReleased && rightReleased) {
    sampleCount++;
    controlLEDs(pressLeft, pressRight);      // compare times and show LED

    // After 10 samples, show averages
    if (sampleCount % sampleSize == 0) {
      // Send average data to computer with header and timestamp
      // Removed repeated CSV header to avoid extra quotes
      // // Removed repeated CSV header to avoid extra quotes
      // // (removed header line for clean output)
      Serial.print("BTN,"); Serial.print(millis()); Serial.print(",");
      Serial.print(calculateAverage(leftTimes)); Serial.print(","); Serial.println(calculateAverage(rightTimes));
      lcd.clear();
      lcd.setCursor(0, 0); lcd.print("Avg 1: "); lcd.print(calculateAverage(leftTimes)); lcd.print("ms");
      lcd.setCursor(0, 1); lcd.print("Avg 2: "); lcd.print(calculateAverage(rightTimes)); lcd.print("ms");

      blinkingYellow = false;                // stop blinking
      digitalWrite(yellowLed, LOW);

      waitForAnyKey();                       // wait for user to continue
      lcd.clear(); lcd.setCursor(0, 0); lcd.print("1: "); lcd.setCursor(0, 1); lcd.print("2: ");
    }
    leftReleased = false; rightReleased = false;
  }
}

// Function for reaction game
void handleReactionGame() {
  delay(20); // debounce for stability
  bool press1 = digitalRead(button1) == LOW;
  bool press2 = digitalRead(button2) == LOW;

  if (waitingForResponse && (press1 || press2)) {
    int response = press1 ? 1 : 2;            // determine which button pressed
    totalResponses++;

    if (totalResponses == 1) {
      responseStartTime = millis();          // record time for first response
    }

    if (response == expectedNumber) {
      correctResponses++;                    // correct answer
      digitalWrite(greenLed, HIGH);
      digitalWrite(redLed, LOW);
    } else {
      digitalWrite(redLed, HIGH);            // wrong answer
      digitalWrite(greenLed, LOW);
    }

    delay(300);                              // short LED flash
    digitalWrite(redLed, LOW);
    digitalWrite(greenLed, LOW);

    // After 10 responses, show result
    if (totalResponses == 10) {
      responseEndTime = millis();
      float totalTimeSec = (responseEndTime - responseStartTime) / 1000.0;
      int percent = (correctResponses * 100) / 10;

      // Send reaction game result to computer with header and timestamp
      // Removed unused header output for REACT program
      Serial.print("REACT,"); Serial.print(responseStartTime); Serial.print(","); Serial.print(percent); Serial.print(","); Serial.println(totalTimeSec, 1);

      lcd.clear();
      lcd.setCursor(0, 0); lcd.print("Correct: "); lcd.print(percent); lcd.print("%");
      lcd.setCursor(0, 1); lcd.print("Time: "); lcd.print(totalTimeSec, 1); lcd.print("s");

      waitForAnyKey();                        // pause before next round
      lcd.clear();                            // clear result
      correctResponses = 0;
      totalResponses = 0;
    }

    // Next number
    lcd.clear();
    expectedNumber = random(1, 3);
    lcd.setCursor(0, 0); lcd.print("Press: "); lcd.print(expectedNumber);
    digitalWrite(yellowLed, HIGH); delay(500); digitalWrite(yellowLed, LOW);
  }
}

// Function to display LED control options
void handleLedControl() {
  lcd.setCursor(0, 0); lcd.print("1:R 2:Y 3:G #Exit");
}

// Calculate average of last 10 values
unsigned long calculateAverage(unsigned long arr[]) {
  unsigned long sum = 0;
  for (int i = 0; i < sampleSize; i++) {
    sum += arr[i];
  }
  return sum / sampleSize;
}

// Compare times and light LED depending on difference
void controlLEDs(unsigned long l, unsigned long r) {
  unsigned long diff = abs((long)(l - r));
  if (diff < 10) {                            // almost equal
    digitalWrite(redLed, LOW); digitalWrite(greenLed, LOW); digitalWrite(yellowLed, HIGH);
  } else if (l > r) {                         // right faster
    digitalWrite(redLed, HIGH); digitalWrite(yellowLed, LOW); digitalWrite(greenLed, LOW);
  } else {                                    // left faster
    digitalWrite(redLed, LOW); digitalWrite(yellowLed, LOW); digitalWrite(greenLed, HIGH);
  }
}

// Wait until any key is pressed
void waitForAnyKey() {
  while (true) {
    char key = keypad.getKey();
    if (key) break;
  }
}

// Reset to main menu
void resetProgram() {
  currentProgram = 0;
  lcd.clear();
  digitalWrite(redLed, LOW); digitalWrite(yellowLed, LOW); digitalWrite(greenLed, LOW);
  blinkingYellow = false;                    // stop blinking if active
  showMenu();                                // go back to menu
}

// Display program selection menu
void showMenu() {
  lcd.setCursor(0, 0); lcd.print("Select program:");
  lcd.setCursor(0, 1); lcd.print("1-BTN 2-RE 3-LED");
}
