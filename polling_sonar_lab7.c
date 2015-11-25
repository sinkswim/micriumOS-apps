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
* Using POLLING: create a task that receives data from the HC-SR04 ultrasonic sensor and writes on the serial port
* the distance (in cm) of objects.
* NB: Polling has proven not to work on certain HC-SR04 sensors because sampling the value may be too slow
*     with respect to the echo signal (see while loops in the task). For this reason, a solution using
*     interrupts is to be prefered.
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                             ADDITIONAL NOTES
*
* See lab 6 for custom configuration of GPIOs.
* Please refer to any HC-SR04 datasheet for an overview of the sensor's operation and timing diagrams.
* NB: Most HC-SR04 work on 3.3V but others won't behave correctly and need 5V.
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                             INCLUDE FILES
*********************************************************************************************************
*/
#include "fsl_interrupt_manager.h"
#include "fsl_gpio_common.h"

#include "stdint.h"     /* MV */

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

    OSA_Init();                                                 /* Init uC/OS-III.                                   */

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
  OS_ERR os_err;
  CPU_ERR cpu_err;
  CPU_TS64 before;
  CPU_TS64 after;
  char tmp[80];
  float distance;

  (void)p_arg;

  CPU_Init();
  Mem_Init();
  Math_Init();

  BSP_Ser_Init(115200u);

     while (DEF_ON) {
       /* set trigger to high for at least 10 us */
       GPIO_DRV_ClearPinOutput( outPTB23 );
       OSTimeDlyHMSM(0u, 0u, 0u, 1u, OS_OPT_TIME_HMSM_STRICT, &os_err);
       GPIO_DRV_SetPinOutput( outPTB23 );
       /* start polling echo signal */
       while((GPIO_DRV_ReadPinInput( inPTB9 )) == 0);
       before = CPU_TS_Get64();
       while((GPIO_DRV_ReadPinInput( inPTB9 )) == 1);
       after = CPU_TS_Get64();
       /* timestamps received, now compute distance and print it */
       distance = (float)((1000000.0*(after-before))/CPU_TS_TmrFreqGet( &cpu_err ));
       distance = distance/58;
       sprintf( tmp, "Distance measured = %f cm\n\r", distance );
       APP_TRACE_DBG(( tmp ));
       /* wait for at least 60 ms between triggers */
       OSTimeDlyHMSM(0u, 0u, 0u, 90u, OS_OPT_TIME_HMSM_STRICT, &os_err);
    }
}
