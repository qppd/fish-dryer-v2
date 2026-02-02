// BUTTON_CONFIG.h
// Configuration for 4 tactile buttons with debounce, short press, and long press detection

#ifndef BUTTON_CONFIG_H
#define BUTTON_CONFIG_H

#define NUM_BUTTONS 4
#define DEBOUNCE_DELAY 50      // Debounce time in milliseconds
#define LONG_PRESS_TIME 1000   // Long press threshold in milliseconds

// Button struct to track state
struct Button {
  uint8_t pin;
  bool lastState;
  bool currentState;
  unsigned long lastDebounceTime;
  unsigned long pressStartTime;
  bool isPressed;
  bool longPressTriggered;
};

// Button pins
Button buttons[NUM_BUTTONS] = {
  {32, HIGH, HIGH, 0, 0, false, false}, // Button 1
  {33, HIGH, HIGH, 0, 0, false, false}, // Button 2
  {25, HIGH, HIGH, 0, 0, false, false}, // Button 3
  {26, HIGH, HIGH, 0, 0, false, false}  // Button 4
};

void initButtons() {
  for (int i = 0; i < NUM_BUTTONS; i++) {
    pinMode(buttons[i].pin, INPUT_PULLUP);
  }
  Serial.println("Buttons initialized with INPUT_PULLUP.");
}

void handleShortPress(int buttonIndex) {
  Serial.print("Button ");
  Serial.print(buttonIndex + 1);
  Serial.println(" SHORT PRESS");
  // Add your short press action here
}

void handleLongPress(int buttonIndex) {
  Serial.print("Button ");
  Serial.print(buttonIndex + 1);
  Serial.println(" LONG PRESS");
  // Add your long press action here
}

void updateButtons() {
  unsigned long currentMillis = millis();
  
  for (int i = 0; i < NUM_BUTTONS; i++) {
    int reading = digitalRead(buttons[i].pin);
    
    // Check if button state changed (for debouncing)
    if (reading != buttons[i].lastState) {
      buttons[i].lastDebounceTime = currentMillis;
    }
    
    // If debounce delay has passed, update the current state
    if ((currentMillis - buttons[i].lastDebounceTime) > DEBOUNCE_DELAY) {
      // If state has changed after debounce
      if (reading != buttons[i].currentState) {
        buttons[i].currentState = reading;
        
        // Button pressed (LOW because of INPUT_PULLUP)
        if (buttons[i].currentState == LOW) {
          buttons[i].isPressed = true;
          buttons[i].pressStartTime = currentMillis;
          buttons[i].longPressTriggered = false;
        }
        // Button released
        else {
          if (buttons[i].isPressed && !buttons[i].longPressTriggered) {
            // Short press detected
            handleShortPress(i);
          }
          buttons[i].isPressed = false;
        }
      }
    }
    
    // Check for long press while button is held
    if (buttons[i].isPressed && !buttons[i].longPressTriggered) {
      if ((currentMillis - buttons[i].pressStartTime) >= LONG_PRESS_TIME) {
        buttons[i].longPressTriggered = true;
        handleLongPress(i);
      }
    }
    
    buttons[i].lastState = reading;
  }
}

void readButtons() {
  Serial.println("Button States:");
  for (int i = 0; i < NUM_BUTTONS; i++) {
    Serial.print("  BTN");
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.println(digitalRead(buttons[i].pin) == LOW ? "PRESSED" : "RELEASED");
  }
}

#endif // BUTTON_CONFIG_H
