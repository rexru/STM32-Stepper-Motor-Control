# Stepper Motor Control on STM32F429ZI

This project demonstrates **stepper motor control with full-step and half-step modes** on the STM32F429ZI Discovery board. It uses the onboard LCD to display student profiles, motor parameters, and step mode, while buttons and interrupts allow for real-time control of direction, speed, and stepping method.

---

## ✨ Features
- Full-step (7.5°/step) and Half-step (3.75°/step) modes  
- User profiles: **Sophia Mokhtari** and **Rushil Saha** (different motor timing values per lab rules)  
- Motor direction control (Clockwise / Counterclockwise)  
- Adjustable speed control (increase/decrease step delay in 20 ms increments)  
- LCD display showing student info and active stepping mode  
- Interrupt-driven inputs (no polling, all controls are event-based)  
- EventQueue + Thread for smooth LCD updates without blocking motor control  

---

## 📐 Motor Calculations
- **Full-step angular resolution**:  
  ```
  360° / 48 = 7.5° per step
  ```
- **Half-step angular resolution**:  
  ```
  360° / 96 = 3.75° per step
  ```

### Sophia Mokhtari (400479269)
- Full-step: 0.75 s/step  
- Half-step: 0.375 s/step  
- Revolution time: 36 s  

### Rushil (400507143)
- Full-step: ~0.896 s/step  
- Half-step: ~0.448 s/step  
- Revolution time: 43 s  

---

## 🛠️ Hardware Setup
- **STM32F429ZI Discovery Board**  
- **Stepper motor (4-wire, unipolar/bipolar)**  
- **Connections**:  
  - `PC_15` → Red coil  
  - `PC_13` → Gray coil  
  - `PE_5`  → Yellow coil  
  - `PE_3`  → Black coil  

- **Inputs (buttons)**:  
  - `PA_0` → Switch between Sophia/Rushil profiles  
  - `PD_4` → Toggle motor direction (CW/CCW)  
  - `PD_2` → Switch full-step / half-step  
  - `PE_6` → Increase speed  
  - `PC_14` → Decrease speed  

- **LED Indicator**:  
  - `PG_13` toggles on profile switch  

---

## 💻 Software Setup
1. Open [Keil Studio Cloud](https://studio.keil.arm.com/) or an Mbed CLI project workspace.  
2. Import the code into an Mbed project for **STM32F429ZI**.  
3. Compile and flash the program to the Discovery board.  
4. Connect the stepper motor coils as per the pin mapping.  
5. Use buttons to:  
   - Switch between **Sophia/Rushil profiles**  
   - Change **direction**  
   - Toggle **stepping mode**  
   - Adjust **speed**  

---

## 🚀 How It Works
- The **motor stepping** is controlled by `Ticker` interrupts that periodically energize coils according to step patterns.  
- **Profiles** load different step timings to match lab rules (36s/rev for Sophia, 43s/rev for Rushil).  
- **EventQueue and Threads** handle LCD updates asynchronously so motor control is never blocked.  
- **DebouncedInterrupts** prevent bouncing from mechanical button inputs.  

---

## 📸 Demo
_Add images or GIFs of the motor running and LCD display here._  

---

## 📂 File Structure
```
StepperMotorProject/
│── main.cpp        # Main program with stepper control logic
│── README.md       # Documentation
```

---

## 👨‍🎓 Authors
- **Sophia Mokhtari** – 400479269  
- **Rushil** – 400507143  
