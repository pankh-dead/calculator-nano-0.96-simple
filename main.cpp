/*
  Arduino Nano Calculator
  
  HARDWARE CONNECTIONS:
  ---------------------
  0.96" OLED Display (I2C):
    VCC -> 5V
    GND -> GND
    SDA -> A4 (Nano)
    SCL -> A5 (Nano)

  4x4 Matrix Keypad:
    Row 0 -> D9
    Row 1 -> D8
    Row 2 -> D7
    Row 3 -> D6
    Col 0 -> D5
    Col 1 -> D4
    Col 2 -> D3
    Col 3 -> D2

  KEYPAD CONTROLS:
  ----------------
  [0-9] : Enter Number
  [A]   : Add (+)
  [B]   : Subtract (-)
  [C]   : Multiply (x)
  [D]   : Divide (/)
  [*]   : Clear All
  [#]   : Calculate (=)

  REQUIRED LIBRARIES (Install via Library Manager):
  1. Adafruit GFX Library
  2. Adafruit SSD1306
  3. Keypad by Mark Stanley, Alexander Brevig
*/

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Keypad.h>

// --- OLED CONFIGURATION ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C // See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// --- KEYPAD CONFIGURATION ---
const byte ROWS = 4; 
const byte COLS = 4; 

char hexaKeys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

// Connections as requested
byte rowPins[ROWS] = {9, 8, 7, 6}; // D9, D8, D7, D6
byte colPins[COLS] = {5, 4, 3, 2}; // D5, D4, D3, D2

Keypad customKeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);

// --- CALCULATOR VARIABLES ---
String currentInput = "";
String previousInput = "";
char operation = 0;
bool newCalculation = false;

void setup() {
  Serial.begin(9600);

  // Initialize OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }

  // --- FANCY STARTUP ANIMATION (Approx 1.5s) ---
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  
  // Sequence 1: Growing circle (0.5s)
  for(int i=0; i<40; i+=2) {
    display.clearDisplay();
    display.drawCircle(64, 32, i, SSD1306_WHITE);
    display.display();
    delay(15);
  }
  
  // Sequence 2: Text typewriter effect (0.5s)
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(10, 25);
  String text = "ARDUINO";
  for(int i=0; i<text.length(); i++) {
    display.print(text[i]);
    display.display();
    delay(70);
  }
  
  // Sequence 3: Blink inverse (0.5s)
  delay(200);
  display.invertDisplay(true);
  delay(100);
  display.invertDisplay(false);
  delay(100);
  display.invertDisplay(true);
  delay(100);
  display.invertDisplay(false);
  
  // Clear and prepare for calculator
  display.clearDisplay();
  display.display();
}

void loop() {
  char customKey = customKeypad.getKey();
  
  if (customKey) {
    handleKeyInput(customKey);
    updateDisplay();
  }
}

void handleKeyInput(char key) {
  // Clear screen on new calc start if number pressed
  if (newCalculation && isDigit(key)) {
    currentInput = "";
    previousInput = "";
    operation = 0;
    newCalculation = false;
  }

  if (key >= '0' && key <= '9') {
    // Number input
    if (currentInput.length() < 10) { // Limit digits
      currentInput += key;
    }
  } 
  else if (key == 'A') { // Add
    setOperation('+');
  } 
  else if (key == 'B') { // Subtract
    setOperation('-');
  } 
  else if (key == 'C') { // Multiply
    setOperation('*');
  } 
  else if (key == 'D') { // Divide
    setOperation('/');
  } 
  else if (key == '#') { // Equals
    calculateResult();
  } 
  else if (key == '*') { // Clear
    currentInput = "";
    previousInput = "";
    operation = 0;
  }
}

void setOperation(char op) {
  if (currentInput.length() > 0 || previousInput.length() > 0) {
    if (currentInput.length() > 0) {
        // If we have a current input, move it to previous
        // If chaining operations (e.g. 1+1+), calculate first
        if (previousInput.length() > 0 && operation != 0) {
           calculateResult();
           previousInput = currentInput;
           currentInput = "";
        } else {
           previousInput = currentInput;
           currentInput = "";
        }
    }
    operation = op;
    newCalculation = false;
  }
}

void calculateResult() {
  if (previousInput.length() > 0 && currentInput.length() > 0 && operation != 0) {
    float num1 = previousInput.toFloat();
    float num2 = currentInput.toFloat();
    float result = 0;
    
    switch(operation) {
      case '+': result = num1 + num2; break;
      case '-': result = num1 - num2; break;
      case '*': result = num1 * num2; break;
      case '/': 
        if (num2 != 0) result = num1 / num2; 
        else result = 0; // Basic Error handling
        break;
    }
    
    // Format result to remove trailing decimal zeros if integer
    currentInput = String(result);
    if (currentInput.endsWith(".00")) currentInput.remove(currentInput.length()-3);
    else if (currentInput.endsWith(".0")) currentInput.remove(currentInput.length()-2);
    
    previousInput = "";
    operation = 0;
    newCalculation = true;
  }
}

void updateDisplay() {
  display.clearDisplay();
  
  // Draw Top Bar (Previous Input & Op)
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(previousInput);
  if (operation != 0) {
    display.print(" ");
    display.print(operation);
  }
  
  // Draw Main Input (Right Aligned approx)
  display.setTextSize(2);
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(currentInput, 0, 20, &x1, &y1, &w, &h);
  
  // Right align logic
  int xPos = 128 - w - 2;
  if (xPos < 0) xPos = 0;
  
  display.setCursor(xPos, 25);
  display.print(currentInput);
  
  display.display();
}
