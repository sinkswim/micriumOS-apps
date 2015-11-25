# micriumOS-apps
A collection of apps for Micrium uC/OS-III on the FRDM-K64F platform by Freescale developed during lab sessions of a course on operating systems for embedded systems.

# running
Download the OS with the BSP for K64F from http://micrium.com/download/frdm-k64f_os3-ksdk/ .
To run a program place a file in "Micrium/Examples/Freescale/FRDM-K64F" and rename it "app.c".


## blueredgreen_sem_lab2
Create three tasks: 1) blinks blue led at 5Hz; 2) blinks red led at 2Hz; 3) blinks green led at 1Hz.
Colors must not overlap (solution makes use of OS_SEM).

## blueredgreen_mutex_lab2
Create three tasks: 1) blinks blue led at 5Hz; 2) blinks red led at 2Hz; 3) blinks green led at 1Hz.
Colors must not overlap (solution makes use of OS_MUTEX).

## redsw_greensw_lab3
Create two tasks: 1) turns on red led if SW1 is pressed; 2) turns on green led if SW2 is pressed.
Colors must not overlap (solution makes use of OS_SEM).

## sw1_interrupt_lab4
Set up an interrupt handler that turns on/off the blue led when SW1 is toggled.

## sw1sw2_interrupts_lab5
Create two tasks: 1) turns on/off red led when SW1 is pressed; 2) turns on/off green led when SW2 is pressed.
Use two ISRs to respond to the triggering of the switches and use two distinct semaphores as a signalling system;
leds can be on at the same time.

## custom_gpios_lab6.c
Define two custom GPIO pins: 1) PORTB.23 as digital output; 2) PORTB.9 as digital input
A task should drive PORTB.23 with a freq. of 5Hz
Also: calculate the period a positive pulse is present on PORTB.9 and print it on the serial port

## polling_sonar_lab7.c
Using POLLING: create a task that receives data from the HC-SR04 ultrasonic sensor and writes on the serial port the
distance (in cm) of objects.

## interrupt_sonar_lab7.c
Using INTERRUPTS: create a task that receives data from the HC-SR04 ultrasonic sensor and writes on the serial port
the distance (in cm) of objects.
