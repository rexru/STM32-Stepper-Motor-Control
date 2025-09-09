/*

(full step) angular resolution: 360/48 = 7.5 degrees/step;
(half step) angular resolution: 360/96 = 3.75 degrees/step;

Calculation for the angular resolution of the motor:
Angular resolution = 360 degrees / Number of steps per revolution
                   = 360 degrees / 48 steps per revolution
                   ≈ 7.5 degrees per step

Student 1:
Name: Sophia Mokhtari
Number: 400479269
Time period between two steps for half-stepping: 0.375 seconds
Time period between two steps for full-stepping: 0.75 seconds
Seconds per revolution: 36 seconds (from 69 - 33, per lab rules)

Student 2:
Name: Rushil
Number: 400507143
Time period between two steps for half-stepping: ~0.448 seconds
Time period between two steps for full-stepping: ~0.896 seconds
Seconds per revolution: 43 seconds (43 as is, per lab rules)
*/

#include "mbed.h"
#include <chrono>
#include <cstdio>
#include <vector>
#include "DebouncedInterrupt.h"
#include "LCD_DISCO_F429ZI.h"

// Initialize LCD object for displaying student info and step mode
LCD_DISCO_F429ZI LCD;

// Digital outputs for stepper motor coils (connected to red, gray, yellow, black wires)
DigitalOut red(PC_15);
DigitalOut gray(PC_13);
DigitalOut yellow(PE_5);
DigitalOut black(PE_3);

// Input buttons with interrupts for user interaction
InterruptIn user_button(PA_0);         // Toggles between Sophia and Rushil's settings
DebouncedInterrupt ext_btn_dir(PD_4);  // Changes motor direction (CW/CCW)
DebouncedInterrupt ext_btn_step(PD_2); // Switches between full and half-step modes
DebouncedInterrupt inc_speed(PE_6);    // Increases motor speed (decreases step period)
DebouncedInterrupt dec_speed(PC_14);   // Decreases motor speed (increases step period)

// Ticker objects for periodic tasks
Ticker motor;          // Controls motor stepping timing
Ticker periodicTicker; // Not used in this code, but included in original
EventQueue queue;      // Manages asynchronous LCD updates
Thread thread;         // Runs the event queue in a separate thread

DigitalOut led3(PG_13); // LED indicator (toggles when switching profiles)

// Global variables
int i = 0;          // Current step index in the step pattern (0 to 3 for full, 0 to 7 for half)
int btn_user = 0;   // Profile selector (0 = Sophia, 1 = Rushil)
int btn_dir = 1;    // Direction flag (1 = CW, 0 = CCW)
int btn_step = 0;   // Step mode (0 = full step, 1 = half step)
int step = 4;       // Number of steps in current pattern (4 for full, 8 for half)

int speed_factor = 0; // Speed adjustment in ms (added to base step period)

int freq_1;  // Full-step period in ms (time between steps)
int freq_2;  // Half-step period in ms (time between steps)
int freq;    // Current step period in ms (freq_1 or freq_2 + speed_factor)

// 2D vectors to store step patterns for motor coils
vector<vector<int>> set_up_h; // Half-step pattern (8 steps)
vector<vector<int>> set_up_f; // Full-step pattern (4 steps)

// Updates step index based on direction (CW or CCW)
void indexing() {
    if (btn_dir == 0) { // Counterclockwise
        i = i + 1;
    } else {            // Clockwise
        i = i - 1;
    }
    // Keep i within bounds (0 to step-1), handling negative values
    i = i % step + (i < 0) * step;
}

// Applies full-step pattern to motor coils and advances index
void rot_step_f() {
    red   = set_up_f[i][0];
    gray  = set_up_f[i][1];
    yellow= set_up_f[i][2];
    black = set_up_f[i][3];
    indexing();
}

// Applies half-step pattern to motor coils and advances index
void rot_step_h() {
    red   = set_up_h[i][0];
    gray  = set_up_h[i][1];
    yellow= set_up_h[i][2];
    black = set_up_h[i][3];
    indexing();
}

// Toggles motor direction (CW <-> CCW)
void switch_dir() {
    btn_dir = !btn_dir;
}

// Switches between full and half-step modes, updates motor timing
void switch_step() {
    char stepping[20]; // Buffer for LCD display text
    if (btn_step == 0) { // Switch to full step
        freq = freq_1 + speed_factor; // Set period for full step
        step = 4;                     // 4 steps per cycle
        motor.attach(&rot_step_f, (std::chrono::milliseconds)freq); // Start ticker
        sprintf(stepping, "Full step");
    } else { // Switch to half step
        freq = freq_2 + speed_factor; // Set period for half step
        step = 8;                     // 8 steps per cycle
        motor.attach(&rot_step_h, (std::chrono::milliseconds)freq); // Start ticker
        sprintf(stepping, "Half step");
    }
    // Display current step mode on LCD
    LCD.DisplayStringAt(0, LINE(11), (uint8_t *)stepping, CENTER_MODE);
    btn_step = !btn_step; // Toggle mode flag
}

// Increases motor speed by reducing step period by 20ms
void increase_speed() {
    speed_factor += 20;   // Increase period (slower)
    btn_step = !btn_step; // Force mode toggle to update timing
    switch_step();
}

// Decreases motor speed by increasing step period by 20ms
void decrease_speed() {
    speed_factor -= 20;   // Decrease period (faster)
    btn_step = !btn_step; // Force mode toggle to update timing
    switch_step();
}

// Displays Sophia's info on LCD
void LCD_refresh_sophia() {
    LCD.SetFont(&Font16);         // Set font size
    LCD.SetTextColor(LCD_COLOR_DARKBLUE); // Set text color
    char name[] = "Sophia Mokhtari";
    char number[] = "400479269";
    char time[] = "36s per revolution";

    // Display name, number, and time on LCD at specific lines
    LCD.DisplayStringAt(0, LINE(5), (uint8_t *)name, CENTER_MODE);
    LCD.DisplayStringAt(0, LINE(7), (uint8_t *)number, CENTER_MODE);
    LCD.DisplayStringAt(0, LINE(9), (uint8_t *)time, CENTER_MODE);
}

// Displays Rushil's info on LCD
void LCD_refresh_rushil() {
    LCD.SetFont(&Font16);
    LCD.SetTextColor(LCD_COLOR_DARKBLUE);
    char name[] = "Rushil";
    char number[] = "400507143";
    char time[] = "43s per revolution";

    LCD.DisplayStringAt(0, LINE(5), (uint8_t *)name, CENTER_MODE);
    LCD.DisplayStringAt(0, LINE(7), (uint8_t *)number, CENTER_MODE);
    LCD.DisplayStringAt(0, LINE(9), (uint8_t *)time, CENTER_MODE);
}

// Toggles between Sophia and Rushil's profiles and updates motor timing
void switch_display() {
    led3 = !led3; // Toggle LED to indicate profile switch
    if (btn_user == 0) { // Switch to Sophia
        freq_1 = 750;    // Full step: 36s / 48 = 750ms/step
        freq_2 = 375;    // Half step: 36s / 96 = 375ms/step
        queue.call(&LCD_refresh_sophia); // Update LCD asynchronously
    } else { // Switch to Rushil
        freq_1 = 896;    // Full step: 43s / 48 ≈ 896ms/step
        freq_2 = 448;    // Half step: 43s / 96 ≈ 448ms/step
        queue.call(&LCD_refresh_rushil);
    }
    switch_step(); // Apply new timing to motor
    btn_user = !btn_user; // Toggle profile flag
}

int main() {
    led3 = 1; // Turn on LED initially

    // Define full-step pattern (4 steps, repeated 12 times = 48 steps/rev)
    set_up_f = {
        {1, 0, 1, 0}, // Red+Yellow
        {1, 0, 0, 1}, // Red+Black
        {0, 1, 0, 1}, // Gray+Black
        {0, 1, 1, 0}  // Gray+Yellow
    };

    // Define half-step pattern (8 steps, repeated 12 times = 96 steps/rev)
    set_up_h = {
        {1, 0, 1, 0}, // Full step
        {1, 0, 0, 0}, // Red only
        {1, 0, 0, 1}, // Full step
        {0, 0, 0, 1}, // Black only
        {0, 1, 0, 1}, // Full step
        {0, 1, 0, 0}, // Gray only
        {0, 1, 1, 0}, // Full step
        {0, 0, 1, 0}  // Yellow only
    };

    // Initialize with Sophia's settings
    freq_1 = 750;  // Full step period in ms
    freq_2 = 375;  // Half step period in ms
    freq = freq_1; // Start with full step

    // Attach interrupt handlers to buttons
    ext_btn_step.attach(&switch_step, IRQ_FALL, 20, false); // Step mode toggle
    ext_btn_dir.attach(&switch_dir, IRQ_FALL, 20, false);   // Direction toggle
    user_button.fall(&switch_display);                      // Profile switch
    inc_speed.attach(&increase_speed, IRQ_FALL, 20, false); // Speed up
    dec_speed.attach(&decrease_speed, IRQ_FALL, 20, false); // Speed down

    __enable_irq(); // Enable interrupts globally

    // Start thread to handle LCD updates via event queue
    thread.start(callback(&queue, &EventQueue::dispatch_forever));

    while(true) {
        // Main loop is empty; motor control is handled by tickers and interrupts
    }
}