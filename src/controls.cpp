#include "controls.h"
#include <Wire.h>
#include <Arduino.h>

void control_init(void) {
    Wire.begin(SDA_PIN, SCL_PIN);
    
    // Configure direct pins with pullups just in case
    pinMode(PIN_L, INPUT_PULLUP);
    pinMode(PIN_R, INPUT_PULLUP);
    pinMode(PIN_MENU, INPUT_PULLUP);
}

void control_read(ControlsState &s) {
    // Read PCF8574
    Wire.requestFrom(PCF_ADDR, 1);
    uint8_t data = 0xFF; // Default to all high (not pressed)
    if (Wire.available()) {
        data = Wire.read();
    }
    
    // Invert data because buttons pull low
    uint8_t pressed = ~data;

    s.up = (pressed >> BIT_UP) & 1;
    s.down = (pressed >> BIT_DOWN) & 1;
    s.left = (pressed >> BIT_LEFT) & 1;
    s.right = (pressed >> BIT_RIGHT) & 1;
    s.a = (pressed >> BIT_A) & 1;
    s.b = (pressed >> BIT_B) & 1;
    
    // We ignore start/select for now as bits are unknown/duplicate
    s.start = false;
    s.select = false;

    // Read direct GPIOs (Active Low)
    s.l = !digitalRead(PIN_L);
    s.r = !digitalRead(PIN_R);
    s.menu = !digitalRead(PIN_MENU);

    // Edge handling
    s.a_pressed = (s.a && !s.a_was_pressed);
    s.a_was_pressed = s.a;

    s.b_pressed = (s.b && !s.b_was_pressed);
    s.b_was_pressed = s.b;
}

