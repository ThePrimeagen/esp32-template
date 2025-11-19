#ifndef BUTTONS_H
#define BUTTONS_H

#include <stdbool.h>
#include <stdint.h>

// --- PCF8574 I2C keypad (active-low) ---
#define PCF_ADDR   0x20
#define SDA_PIN    21
#define SCL_PIN    22

// Replace with your discovered bits:
#define BIT_UP      2
#define BIT_DOWN    3
#define BIT_LEFT    4
#define BIT_RIGHT   5
#define BIT_A       6
#define BIT_B       7
#define BIT_START   1 // I DONT KNOW THE NUMBER HERE
#define BIT_SELECT  1 // I DONT KNOW THE NUMBER HERE

// --- Direct GPIO buttons ---
#define PIN_L    36
#define PIN_R    34
#define PIN_MENU 35

typedef struct {
    bool up, down, left, right;
    bool a, b, start, select;
    bool l, r, menu;

    bool a_was_pressed, b_was_pressed;
    bool a_pressed, b_pressed;
} ControlsState;

void control_init(void);
void control_read(ControlsState &s);

#endif // BUTTONS_H


