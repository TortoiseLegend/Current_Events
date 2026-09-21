# Current Events

Current Events is a low-voltage prototype that automatically prioritizes electrical loads within an adjustable current budget.

## Problem

During a power outage, a limited backup supply may not support every desired device simultaneously. Users need a way to prioritize important loads as available capacity changes.

## Solution

Current Events measures the current drawn by electrical loads, compares their combined demand with a user-selected budget, and automatically disconnects lower-priority loads when necessary.

For this demonstration, LEDs represent household devices. Current measurements and switching occur on the physical circuit.

## How it works

- The controller starts with a **12 mA current budget**, adjustable using a rotary encoder.
- Each branch contains an LED and a **1 kΩ resistor**, which both limits current and enables measurement.
- Analog inputs A0–A2 measure the voltage across the resistors. The program converts ADC readings to volts using an assumed 5 V reference.
- Branch currents are calculated using **I = V/R** and added up.
- If total current exceeds the budget, the controller switches off one active load at a time, starting with the lowest priority, and measures again.
- When sufficient capacity becomes available, the controller can restore loads using their remembered operating currents, a waiting period, and an additional current margin.
- An OLED displays measured currents, the budget, and commanded load states real time.

## Load priorities

| Load | Output pin | Measurement pin | Priority |
|---|---|---|---|
| LED 1 | D5 | A0 | Highest |
| LED 2 | D6 | A1 | Medium |
| LED 3 | D7 | A2 | Lowest |

## Prototype scope

The budget applies to the three load branches, not the total consumption of the board and display. It is a software-defined limit; adjusting it does not change the USB supply’s physical capacity.

The prototype demonstrates priority-based control of small DC loads. It does not switch household mains appliances or replace electrical protection. Restoration uses estimated demand, so a restored load is measured again to check whether it fits within the budget.

## Materials and IDE Used in Prototype
- Arduino Uno
- Wires, LEDS, 1 kΩ resistors
- Rotary encoder
- OLED
- Arduino IDE
