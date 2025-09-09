/*
-------------------------------------------------------
Stepper Motor Control – STM32F429ZI
-------------------------------------------------------
Description:
This program drives a unipolar stepper motor using
an STM32F429ZI Discovery board. It supports:

- Full-step and half-step driving modes
- Adjustable speed (via external buttons)
- Direction control (CW / CCW)
- User profile switching (Sophia / Rushil)
- LCD display updates for profile and step mode

Each profile has its own timing configuration
(seconds per revolution).

-------------------------------------------------------
Motor Angular Resolution:
- Full step: 360° / 48 steps = 7.5° per step
- Half step: 360° / 96 steps = 3.75° per step

Student Profiles:
- Sophia Mokhtari (400479269): 36s per revolution
  Full-step period = 750 ms
  Half-step period = 375 ms

- Rushil (400507143): 43s per revolution
  Full-step period ≈ 896 ms
  Half-step period ≈ 448 ms
-------------------------------------------------------
*/

#include "mbed.h"
#include <chrono>
#include <cstdio>
#include <vector>
#include "DebouncedInterrupt.h"
#include "LCD_DISCO_F429ZI.h"

//-------------------------------------------------------
// LCD Setup
//-------------------------------------------------------
LCD_DISCO_F429ZI LCD;

//-------------------------------------------------------
// Stepper Motor Coil Outputs
// Connected to Red, Gray, Yellow, Black wires
//-------------------------------------------------------
DigitalOut red(PC_15);
DigitalOut gray(PC_13);
DigitalOut yellow(PE_5);
DigitalOut black(PE_3);

//-------------------------------------------------------
// Input Buttons with Interrupts
//-------------------------------------------------------
InterruptIn user_button(PA_0);         // Switch between Sophia and Rushil profiles
DebouncedInterrupt ext_btn_dir(PD_4);  // Toggle motor direction (CW/CCW)
DebouncedInterrupt ext_btn_step(PD_2); // Toggle between full and half-step modes
DebouncedInterrupt inc_speed(PE_6);    // Increase motor speed
DebouncedInterrupt dec_speed(PC_14);   // Decrease motor speed

//-------------------------------------------------------
// Timing and Event Management
//-------------------------------------------------------
Ticker motor;          // Periodic ticker for motor stepping
EventQueue queue;      // Event queue for LCD updates
Thread thread;         // Thread to dispatch LCD events

//-------------------------------------------------------
// Onboard Indicator LED
//-------------------------------------------------------
DigitalOut led3(PG_13); // Toggles when switching user profiles

//-------------------------------------------------------
// Global State Variables
//-------------------------------------------------------
int i = 0;          // Current step index (0–3 for full, 0–7 for half)
int btn_user = 0;   // Active user profile (0 = Sophia, 1 = Rushil)
int btn_dir = 1;    // Motor direction (1 = CW, 0 = CCW)
int btn_step = 0;   // Step mode (0 = full, 1 = half)
int step = 4;       // Number of steps in current pattern

int speed_factor = 0; // User speed adjustment (ms offset)

int freq_1;  // Full-step base period (ms)
int freq_2;  // Half-step base period (ms)
int freq;    // Active step period (ms)

//-------------------------------------------------------
// Step Patterns for Stepper Motor
//-------------------------------------------------------
// Half-step (8-step sequence, 96 steps/rev)
vector<vector<int>> set_up_h = {
    {1, 0, 1, 0}, // Red+Yellow
    {1, 0, 0, 0}, // Red only
    {1, 0, 0, 1}, // Red+Black
    {0, 0, 0, 1}, // Black only
    {0, 1, 0, 1}, // Gray+Black
    {0, 1, 0, 0}, // Gray only
    {0, 1, 1, 0}, // Gray+Yellow
    {0, 0, 1, 0}  // Yellow only
};

// Full-step (4-step sequence, 48 steps/rev)
vector<vector<int>> set_up_f = {
    {1, 0, 1, 0}, // Red+Yellow
    {1, 0, 0, 1}, // Red+Black
    {0, 1, 0, 1}, // Gray+Black
    {0, 1, 1, 0}  // Gray+Yellow
};

//-------------------------------------------------------
// Helper Functions
//-------------------------------------------------------

// Step index updater based on direction
void indexing() {
    if (btn_dir == 0) {  // CCW
        i = (i + 1) % step;
    } else {             // CW
        i = (i - 1 + step) % step;
    }
}

// Perform a full-step and advance index
void rot_step_f() {
    red    = set_up_f[i][0];
    gray   = set_up_f[i][1];
    yellow = set_up_f[i][2];
    black  = set_up_f[i][3];
    indexing();
}

// Perform a half-step and advance index
void rot_step_h() {
    red    = set_up_h[i][0];
    gray   = set_up_h[i][1];
    yellow = set_up_h[i][2];
    black  = set_up_h[i][3];
    indexing();
}

//-------------------------------------------------------
// Interrupt Handlers
//-------------------------------------------------------

// Toggle direction (CW ↔ CCW)
void switch_dir() {
    btn_dir = !btn_dir;
}

// Toggle between full-step and half-step
void switch_step() {
    char stepping[20];

    if (btn_step == 0) { // Full step
        freq = freq_1 + speed_factor;
        step = 4;
        motor.attach(&rot_step_f, std::chrono::milliseconds(freq));
        sprintf(stepping, "Full step");
    } else {             // Half step
        freq = freq_2 + speed_factor;
        step = 8;
        motor.attach(&rot_step_h, std::chrono::milliseconds(freq));
        sprintf(stepping, "Half step");
    }

    // Show mode on LCD
    LCD.DisplayStringAt(0, LINE(11), (uint8_t *)stepping, CENTER_MODE);

    btn_step = !btn_step; // Toggle mode
}

// Increase speed (reduce period by 20 ms)
void increase_speed() {
    speed_factor -= 20;
    btn_step = !btn_step; // Force reapply
    switch_step();
}

// Decrease speed (increase period by 20 ms)
void decrease_speed() {
    speed_factor += 20;
    btn_step = !btn_step; // Force reapply
    switch_step();
}

//-------------------------------------------------------
// LCD Display Functions
//-------------------------------------------------------
void LCD_refresh_sophia() {
    LCD.SetFont(&Font16);
    LCD.SetTextColor(LCD_COLOR_DARKBLUE);
    LCD.DisplayStringAt(0, LINE(5), (uint8_t *)"Sophia Mokhtari", CENTER_MODE);
    LCD.DisplayStringAt(0, LINE(7), (uint8_t *)"400479269", CENTER_MODE);
    LCD.DisplayStringAt(0, LINE(9), (uint8_t *)"36s per revolution", CENTER_MODE);
}

void LCD_refresh_rushil() {
    LCD.SetFont(&Font16);
    LCD.SetTextColor(LCD_COLOR_DARKBLUE);
    LCD.DisplayStringAt(0, LINE(5), (uint8_t *)"Rushil", CENTER_MODE);
    LCD.DisplayStringAt(0, LINE(7), (uint8_t *)"400507143", CENTER_MODE);
    LCD.DisplayStringAt(0, LINE(9), (uint8_t *)"43s per revolution", CENTER_MODE);
}

// Switch between profiles and update motor timing
void switch_display() {
    led3 = !led3; // Blink indicator LED

    if (btn_user == 0) { // Sophia
        freq_1 = 750;  // Full step period
        freq_2 = 375;  // Half step period
        queue.call(&LCD_refresh_sophia);
    } else {           // Rushil
        freq_1 = 896;  // Full step period
        freq_2 = 448;  // Half step period
        queue.call(&LCD_refresh_rushil);
    }

    switch_step();       // Apply timing
    btn_user = !btn_user; // Toggle profile
}

//-------------------------------------------------------
// Main Function
//-------------------------------------------------------
int main() {
    led3 = 1; // LED ON at start

    // Initialize with Sophia's settings
    freq_1 = 750;
    freq_2 = 375;
    freq   = freq_1;

    // Attach interrupt handlers
    ext_btn_step.attach(&switch_step, IRQ_FALL, 20, false);
    ext_btn_dir.attach(&switch_dir, IRQ_FALL, 20, false);
    user_button.fall(&switch_display);
    inc_speed.attach(&increase_speed, IRQ_FALL, 20, false);
    dec_speed.attach(&decrease_speed, IRQ_FALL, 20, false);

    __enable_irq(); // Enable interrupts globally

    // Start event queue thread for LCD
    thread.start(callback(&queue, &EventQueue::dispatch_forever));

    // Main loop stays idle (work is interrupt-driven)
    while (true) {
        ThisThread::sleep_for(1s);
    }
}
