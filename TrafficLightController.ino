#include <Keypad.h>

// Keypad setup
const byte ROWS = 4, COLS = 4;
char hexaKeys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte rowPins[ROWS] = {9, 8, 7, 6}; 
byte colPins[COLS] = {5, 4, 3, 2}; 
Keypad customKeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);

// LED and buzzer pins
const int redLight = 10, yellowLight = 11, greenLight = 12, buzzer = 13;

// Default light durations
int redDuration = 24, greenDuration = 20;
const int yellowDuration = 3;

bool settingMode = true, acceptingDigits = false;
int mode = 0, tempDuration = 0;

// State machine and timer variables
enum LightState { RED, GREEN, YELLOW, BUZZER };
volatile LightState currentState = RED;
volatile int timerCounter = 0;
bool flashing = false;

// Timer setup for 1Hz intervals
void setupTimer() {
  cli(); // Disable interrupts
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;
  OCR1A = 7812;  // Set for 1Hz with 1024 prescaler
  TCCR1B |= (1 << WGM12) | (1 << CS12) | (1 << CS10);
  TIMSK1 |= (1 << OCIE1A);  // Enable timer interrupt
  sei(); // Enable interrupts
}

// ISR to handle traffic light timing
ISR(TIMER1_COMPA_vect) {
  timerCounter++;
  handleTrafficLightState();
  switch (currentState) {
    case RED:
      if (timerCounter >= (2*redDuration - 6) && timerCounter < 2*redDuration) {
        flashing = true;
        digitalWrite(buzzer, HIGH);
      } else if (timerCounter >= 2*redDuration) {
        digitalWrite(buzzer, LOW);
        currentState = GREEN;
        timerCounter = 0;
        flashing = false;
      }
      break;
    
    case GREEN:
      if (timerCounter >= 2*greenDuration - 6 && timerCounter < 2*greenDuration) {
        flashing = true;
        digitalWrite(buzzer, HIGH);
      } else if (timerCounter >= 2*greenDuration) {
        digitalWrite(buzzer, LOW);
        currentState = YELLOW;
        timerCounter = 0;
        flashing = false;
      }
      break;
    
    case YELLOW:
      if (timerCounter >= 2*yellowDuration) {
        currentState = RED;
        timerCounter = 0;
      }
      break;
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(redLight, OUTPUT);
  pinMode(yellowLight, OUTPUT);
  pinMode(greenLight, OUTPUT);
  pinMode(buzzer, OUTPUT);
  digitalWrite(buzzer, LOW);
  Serial.println("Enter Red and Green light durations using 'A' and 'B' keys.");
  handleKeypadInput();
  setupTimer();
}

void loop() {}

void handleKeypadInput() {
  while(settingMode) {
    char customKey = customKeypad.getKey();
    if (customKey) {
      Serial.println(customKey);
      if (customKey == 'A') {
        mode = 1;
        tempDuration = 0;
        acceptingDigits = true;
        Serial.println("Enter Red light duration followed by '#'");
      } else if (customKey == 'B') {
        mode = 2;
        tempDuration = 0;
        acceptingDigits = true;
        Serial.println("Enter Green light duration followed by '#'");
      } else if (customKey == '#') {
        if (acceptingDigits) {
          if (mode == 1) {
            redDuration = tempDuration;
            Serial.print("Red Duration Set: ");
            Serial.println(redDuration);
          } else if (mode == 2) {
            greenDuration = tempDuration;
            Serial.print("Green Duration Set: ");
            Serial.println(greenDuration);
          }
          acceptingDigits = false;
          mode = 0;
        }
      } else if (customKey == '*') {
        settingMode = false;
        Serial.println("Starting traffic light cycle...");
      } else if (isdigit(customKey) && acceptingDigits) {
        tempDuration = tempDuration * 10 + (customKey - '0');
        Serial.print("Current Duration: ");
        Serial.println(tempDuration);
      } else if (isdigit(customKey) && !acceptingDigits) {
        Serial.println("Invalid input: Press A or B to set a new duration.");
      }
    }
  }
}

// Function to handle traffic light behavior based on current state
void handleTrafficLightState() {
  switch (currentState) {
    case RED:
      if (flashing) flashLight(redLight);
      else digitalWrite(redLight, HIGH);
      digitalWrite(greenLight, LOW);
      digitalWrite(yellowLight, LOW);
      break;
    
    case GREEN:
      if (flashing) flashLight(greenLight);
      else digitalWrite(greenLight, HIGH);
      digitalWrite(redLight, LOW);
      digitalWrite(yellowLight, LOW);
      break;
    
    case YELLOW:
      digitalWrite(yellowLight, HIGH);
      digitalWrite(redLight, LOW);
      digitalWrite(greenLight, LOW);
      break;
  }
}

// Function to flash the light on/off every 0.5 seconds
void flashLight(int lightPin) {
  static bool isOn = false;
  digitalWrite(lightPin, isOn ? LOW : HIGH);
  isOn = !isOn;
}
