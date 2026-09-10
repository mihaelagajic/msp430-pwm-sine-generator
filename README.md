# MSP430 PWM Sine Wave Generator

## Overview

This project implements a **PWM generator with sinusoidal modulation** on an MSP430 microcontroller.

The PWM duty cycle is periodically updated according to a sine lookup table. The signal amplitude is controlled using a potentiometer connected to the ADC, while a push button is used to change the sine-wave frequency.

The selected frequency is displayed on a multiplexed two-digit 7-segment display, while the ADC-derived amplitude value is transmitted through UART.

## Features

* PWM generation using **Timer A0**
* Sinusoidal modulation using a **64-point lookup table**
* Adjustable signal amplitude using **ADC12**
* Adjustable sine-wave frequency from **1 Hz to 10 Hz**
* Push-button interrupt for frequency control
* Software button debouncing using **Timer A1**
* UART communication at **9600 baud**
* Two-digit multiplexed **7-segment display**
* Timer-based display multiplexing
* Interrupt-driven peripheral control
* Embedded C implementation using MSP430 registers and peripherals

## System Overview

```text
                 +------------------+
                 |  Potentiometer   |
                 +--------+---------+
                          |
                          v
                    +-----------+
                    |   ADC12   |
                    +-----+-----+
                          |
                          v
                  +---------------+
                  |   Amplitude   |
                  |    Control    |
                  +-------+-------+
                          |
                          v
+----------------+   +----------+   +------------------+
| Sine Lookup    |-->|   PWM    |-->|  PWM Output     |
| Table (64 pts) |   | Generator|   |    TA0.2        |
+-------+--------+   +----------+   +------------------+
        ^
        |
+-------+--------+
|    Timer B0    |
| Frequency Ctrl |
+----------------+

+----------------+
|  Push Button   |
+-------+--------+
        |
        v
+----------------+
| Timer A1       |
| Debouncing     |
+----------------+

+----------------+       +----------------+
| Timer A2       |------>|  7-Segment     |
| Multiplexing   |       |   Display      |
+----------------+       +----------------+

+----------------+
|     UART       |
+-------+--------+
        |
        v
  Amplitude Data
```

## Peripheral Configuration

| Peripheral | Function                                         |
| ---------- | ------------------------------------------------ |
| Timer A0   | PWM generation                                   |
| Timer B0   | Sine lookup table sampling and frequency control |
| ADC12      | Potentiometer / amplitude measurement            |
| Timer A1   | Push-button debouncing                           |
| Timer A2   | 7-segment display multiplexing                   |
| UART       | Transmission of amplitude values                 |
| GPIO       | Button, PWM and display control                  |

## Implementation

### PWM Generation

Timer A0 is configured to generate a PWM signal on the **TA0.2** output.

The PWM period is defined by:

```c
#define PWM_PERIOD (1024)
```

The duty cycle is updated periodically according to the values from the sine lookup table.

The resulting PWM signal has an approximately **1.024 kHz carrier frequency**.

### Sinusoidal Modulation

A **64-point sine lookup table** is used to generate the sinusoidal modulation.

The lookup table contains one complete sine-wave period. Timer B0 periodically triggers an interrupt that:

1. Reads the next value from the lookup table.
2. Applies the selected amplitude.
3. Scales the result to the PWM period.
4. Updates the PWM duty cycle.
5. Advances the lookup-table index.

The lookup-table index wraps around after the 64th sample.

### Amplitude Control

A potentiometer is connected to the **A0 analog input** of the ADC12 peripheral.

The ADC result is used to control the amplitude of the generated sinusoidal waveform.

To reduce unnecessary UART communication, a new amplitude value is transmitted only when the change from the previous value exceeds a defined threshold.

### Frequency Control

The sine-wave frequency can be changed using the **S1 push button**.

Each valid button press increases the frequency:

```text
1 Hz → 2 Hz → 3 Hz → ... → 10 Hz → 1 Hz
```

The button is connected to a GPIO interrupt. A software debounce mechanism based on **Timer A1** prevents multiple frequency changes caused by mechanical button bouncing.

Timer B0 is then reconfigured according to the selected frequency.

### 7-Segment Display

The currently selected sine-wave frequency is displayed using a **two-digit 7-segment display**.

Timer A2 is used to periodically switch between the two digits, creating a multiplexed display.

The display therefore shows the current frequency without requiring a separate timer for each digit.

### UART Communication

UART communication is configured at:

```text
Baud rate: 9600 bit/s
```

The ADC-derived amplitude value is transmitted through UART when a significant change is detected.

The transmitted value is formatted as a decimal number followed by a new line.

## Interrupts

The project uses several interrupt sources:

* **Timer B0 interrupt** – updates the PWM duty cycle according to the sine lookup table
* **Push-button interrupt** – detects a frequency-change request
* **Timer A1 interrupt** – performs button debouncing and changes the frequency
* **ADC12 interrupt** – processes new amplitude measurements
* **Timer A2 interrupt** – multiplexes the 7-segment display

This interrupt-driven architecture allows the peripherals to operate concurrently without requiring continuous polling in the main program.

## Technologies

* **C / Embedded C**
* **MSP430 microcontroller**
* PWM
* ADC12
* Timers
* GPIO interrupts
* UART
* 7-segment display
* Lookup tables
* Interrupt-driven programming

## Project Structure

```text
msp430-pwm-sine-generator/
│
├── README.md
│
└── src/
    ├── main.c
    ├── function.c
    └── function.h
```

### Source Files

**`main.c`**

Contains the main application logic and configuration of:

* PWM
* ADC12
* Timers
* Push button
* UART
* 7-segment display
* Interrupt service routines

**`function.c`**

Contains helper functions and lookup tables used for controlling the 7-segment display.

**`function.h`**

Contains the interface for the 7-segment display helper functions.

## Key Parameters

| Parameter              |       Value |
| ---------------------- | ----------: |
| PWM period             |        1024 |
| PWM carrier frequency  | ≈ 1.024 kHz |
| Sine lookup table size |  64 samples |
| Sine frequency range   |     1–10 Hz |
| UART baud rate         |  9600 bit/s |
| Button debounce period |     ≈ 32 ms |
| ADC resolution used    |       8-bit |

## Academic Context

This project was developed as part of the **Microprocessor Systems** coursework at the **University of Belgrade – School of Electrical Engineering (ETF)**.

The implementation is designed for an MSP430-based laboratory development platform, with peripheral configuration and GPIO assignments corresponding to the target hardware used during the course.

## Skills Demonstrated

This project demonstrates practical experience with:

* Low-level microcontroller programming
* MSP430 peripheral configuration
* Embedded C
* PWM signal generation
* ADC-based control
* Timer configuration
* Interrupt service routines
* GPIO interrupts
* Button debouncing
* UART communication
* 7-segment display control
* Lookup-table based waveform generation
* Real-time interaction between multiple peripherals
