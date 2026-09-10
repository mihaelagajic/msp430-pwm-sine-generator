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

## Source Files

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

## Project Documentation

The complete project report is available in the [`docs`](./docs) directory.

📄 [View the project report](./docs/project_report.docx)

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
