# micriumOS-apps
A collection of apps for Micrium uC/OS-III for the FRDM-K64F platform by Freescale.
The following have been developed during lab sessions of a course on operating systems for embedded systems.

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
