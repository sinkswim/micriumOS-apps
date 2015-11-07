/*
*********************************************************************************************************
*
*                                        Micrium uC/OS-III and
*                                        Freescale Kinetis K64
*                                               on the
*
*                                         Freescale FRDM-K64F
*                                          Evaluation Board
*
*
* Set up an interrupt handler that turns on/off the blue led when SW1 is toggled
* Must set ".config.interrupt = kPortIntFallingEdge" for switch definition in "gpio_pins.c"
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                             INCLUDE FILES
*********************************************************************************************************
*/
#include "fsl_interrupt_manager.h"

#include  <math.h>
#include  <lib_math.h>
#include  <cpu_core.h>

#include  <app_cfg.h>
#include  <os.h>

#include  <fsl_os_abstraction.h>
#include  <system_MK64F12.h>
#include  <board.h>

#include  <bsp_ser.h>

#include <fsl_gpio_common.h>    // externs g_PortBaseAddr needed in ISR

/*
*********************************************************************************************************
*                                                main()
*********************************************************************************************************
*/

// handler associated to SW1 (labeled SW2 on board): turns on/off blue led
void SW1_Intr_Handler(void)
{
  static uint32_t ifsr;          // interrupt flag status register
  uint32_t portBaseAddr = g_portBaseAddr[GPIO_EXTRACT_PORT(kGpioSW1)];

  CPU_CRITICAL_ENTER();         // enter critical section (disable interrupts)

  OSIntEnter();         // notify to scheduler the beginning of an ISR ("This allows �C/OS-III to keep track of interrupt nesting")

  ifsr = PORT_HAL_GetPortIntFlag(portBaseAddr);         // get intr flag reg related to port C

  if( (ifsr & 0x40u) ) // check if kGpioSW1 generated the interrupt [pin 6 -> 7th flag (flags start with index 0)]
  {
      GPIO_DRV_TogglePinOutput(BOARD_GPIO_LED_BLUE);             // turn on/off led

      GPIO_DRV_ClearPinIntFlag( kGpioSW1 );       // clear int flag related to SW1 to avoid recalling the handler
  }

  CPU_CRITICAL_EXIT();  // renable interrupts

  OSIntExit();          /* notify to scheduler the end of an ISR ("determines if a higher priority task is ready-to-run.
                          If so, the interrupt returns to the higher priority task instead of the interrupted task.") */
}


int  main (void)
{
    OS_ERR   err;

#if (CPU_CFG_NAME_EN == DEF_ENABLED)
    CPU_ERR  cpu_err;
#endif
printf("TEST STDOUT");
    hardware_init();
    GPIO_DRV_Init(switchPins, ledPins);


#if (CPU_CFG_NAME_EN == DEF_ENABLED)
    CPU_NameSet((CPU_CHAR *)"MK64FN1M0VMD12",
                (CPU_ERR  *)&cpu_err);
#endif

    OSA_Init();                                                 /* Init uC/OS-III.                                      */

    INT_SYS_InstallHandler(PORTC_IRQn, SW1_Intr_Handler);       // associate ISR with the interrupt source

    OSA_Start();                                                /* Start multitasking (i.e. give control to uC/OS-III). */

    while (DEF_ON) {                                            /* Should Never Get Here                                */
        ;
    }
}
