/*
*********************************************************************************************************
*
*
*                                        Micrium uC/OS-III for
*                                        Freescale Kinetis K64
*                                               on the
*
*                                         Freescale FRDM-K64F
*                                          Evaluation Board
*
*
*
* Define two custom GPIO pins: 1) PORTB.23 as digital output; 2) PORTB.9 as digital input
* A task should drive PORTB.23 with a freq. of 5Hz
* Also: calculate the period a positive pulse is present on PORTB.9 and print it on the serial port
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                             ADDITIONAL NOTES
* In order to define custom GPIOs a few modifications to some configuration files must be made.
*
* 1) In:  Micrium\Examples\Freescale\KSDK\boards\frdmk64f120m\gpio_pins.c
* Add to ledPins[] the entry:
  {
  .pinName = outPTB23,
  .config.outputLogic = 1,
  .config.slewRate = kPortSlowSlewRate,
  .config.isOpenDrainEnabled = false,
  .config.driveStrength = kPortLowDriveStrength,
  .config.interrupt = kPortIntDisabled
  },

* Add to switchPins[] the entry:
  {
  .pinName = inPTB9,
  .config.isPullEnable = true,
  .config.pullSelect = kPortPullDown,
  .config.isPassiveFilterEnabled = false,
  .config.interrupt = kPortIntEitherEdge
  },
*
* 2) In:  \Micrium\Examples\Freescale\KSDK\boards\frdmk64f120m\gpio_pins.h
* Add to pinNames the entries:
  outPTB23 = GPIO_MAKE_PIN(HW_GPIOB, 23U),
  inPTB9 = GPIO_MAKE_PIN(HW_GPIOB, 9U),
*
* 3) In:  Micrium\Examples\Freescale\KSDK\boards\frdmk64f120m\pin_mux.c
* Add to configure_gpio_pins in the HW_PORTB case the entries:
  PORT_HAL_SetMuxMode(PORTB_BASE,9u,kPortMuxAsGpio);
  PORT_HAL_SetMuxMode(PORTB_BASE,23u,kPortMuxAsGpio);
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                             INCLUDE FILES
*********************************************************************************************************
*/
#include "fsl_interrupt_manager.h"
#include "fsl_gpio_common.h"

#include "stdint.h"

#include  <math.h>
#include  <lib_math.h>
#include  <cpu_core.h>

#include  <app_cfg.h>
#include  <os.h>

#include  <fsl_os_abstraction.h>
#include  <system_MK64F12.h>
#include  <board.h>

#include  <bsp_ser.h>


/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                       LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*/

static  OS_TCB       AppTaskStartTCB;
static  CPU_STK      AppTaskStartStk[APP_CFG_TASK_START_STK_SIZE];

/*
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/

static  void  AppTaskStart (void  *p_arg);


/*
*********************************************************************************************************
*                                                main()
*********************************************************************************************************
*/

int  main (void)
{
    OS_ERR   err;

#if (CPU_CFG_NAME_EN == DEF_ENABLED)
    CPU_ERR  cpu_err;
#endif

    hardware_init();

    GPIO_DRV_Init(switchPins, ledPins);

#if (CPU_CFG_NAME_EN == DEF_ENABLED)
    CPU_NameSet((CPU_CHAR *)"MK64FN1M0VMD12",
                (CPU_ERR  *)&cpu_err);
#endif

    OSA_Init();                                                 /* Init uC/OS-III.                                      */

    OSTaskCreate(&AppTaskStartTCB,                              /* Create the start task                                */
                 "App Task Start",
                  AppTaskStart,
                  0u,
                  APP_CFG_TASK_START_PRIO,
                 &AppTaskStartStk[0u],
                 (APP_CFG_TASK_START_STK_SIZE / 10u),
                  APP_CFG_TASK_START_STK_SIZE,
                  0u,
                  0u,
                  0u,
                 (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR | OS_OPT_TASK_SAVE_FP),
                 &err);

    OSA_Start();                                                /* Start multitasking (i.e. give control to uC/OS-III). */

    while (DEF_ON) {                                            /* Should Never Get Here                                */
        ;
    }
}


/*
*********************************************************************************************************
*                                          TASKS
*********************************************************************************************************
*/

static void AppTaskStart (void *p_arg)
{
  OS_ERR err;
  CPU_ERR cpu_err;
  uint32_t value;
  static uint32_t pulse_flag = 0;
  CPU_TS64 before, after;
  char tmp[80];
    
  (void)p_arg;

  CPU_Init();
  Mem_Init();
  Math_Init();

  BSP_Ser_Init(115200u);

    while (DEF_ON) {
        GPIO_DRV_TogglePinOutput( outPTB23 );
        value = GPIO_DRV_ReadPinInput( inPTB9 );
        if( value ){
            if(pulse_flag == 0)
                before = CPU_TS_Get64();    // in cpu_cfg.h must set: #define  CPU_CFG_TS_64_EN DEF_ENABLED
            pulse_flag = 1;
            GPIO_DRV_ClearPinOutput( BOARD_GPIO_LED_BLUE );
        }
        else{
            GPIO_DRV_SetPinOutput( BOARD_GPIO_LED_BLUE );
            if(pulse_flag == 1)
            {
                after = CPU_TS_Get64();     // see comment above
                sprintf( tmp, "Elapsed time in Task R = %f s\n\r", (float)((1.0*(after-before))/CPU_TS_TmrFreqGet( &cpu_err )) );
                APP_TRACE_DBG(( tmp ));
            }
            pulse_flag = 0;
        }
        OSTimeDlyHMSM(0u, 0u, 0u, 200u, OS_OPT_TIME_HMSM_STRICT, &err);
        
    }
}


