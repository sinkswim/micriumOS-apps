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
* Create two tasks: 1) turns on red led if SW1 is pressed 2) turns on green led if SW2 is pressed
* Colors must not overlap (solution makes use of OS_SEM
* Must set ".config.interrupt = kPortIntDisabled" for switch definition in "gpio_pins.c"
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

/*
*********************************************************************************************************
*                                       LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*/

static  OS_TCB       AppTaskStartTCB;
static  CPU_STK      AppTaskStartStk[APP_CFG_TASK_START_STK_SIZE];

static  OS_TCB       TaskRedTCB;
static  CPU_STK      TaskRedStk[APP_CFG_TASK_START_STK_SIZE];

static  OS_TCB       TaskGreenTCB;
static  CPU_STK      TaskGreenStk[APP_CFG_TASK_START_STK_SIZE];

static OS_SEM MySem;

/*
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/

static  void  AppTaskStart (void  *p_arg);
static  void  AppTaskRed (void  *p_arg);      // responds to board SW2
static  void  AppTaskGreen (void  *p_arg);      // responds to board SW3

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

    OSSemCreate(&MySem,           /* Create Switch Semaphore          */
                "Binary semaphore",
                 1,
                &err);

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

static  void  AppTaskStart (void *p_arg)
{
    OS_ERR    os_err;

    (void)p_arg;


    CPU_Init();                                                 /* Initialize the uC/CPU Services.                      */
    Mem_Init();                                                 /* Initialize the Memory Management Module              */
    Math_Init();                                                /* Initialize the Mathematical Module                   */

    BSP_Ser_Init(115200u);

    OSTaskCreate(&TaskRedTCB,                              /* Create the red task                                */
                 "App Task Red",
                  AppTaskRed,
                  0u,
                  APP_CFG_TASK_START_PRIO,
                 &TaskRedStk[0u],
                 (APP_CFG_TASK_START_STK_SIZE / 10u),
                  APP_CFG_TASK_START_STK_SIZE,
                  0u,
                  0u,
                  0u,
                 (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR | OS_OPT_TASK_SAVE_FP),
                 &os_err);

    OSTaskCreate(&TaskGreenTCB,                              /* Create the green task                                */
                 "App Task Green",
                  AppTaskGreen,
                  0u,
                  APP_CFG_TASK_START_PRIO,
                 &TaskGreenStk[0u],
                 (APP_CFG_TASK_START_STK_SIZE / 10u),
                  APP_CFG_TASK_START_STK_SIZE,
                  0u,
                  0u,
                  0u,
                 (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR | OS_OPT_TASK_SAVE_FP),
                 &os_err);


    while (DEF_TRUE) {                                          /* Task body, always written as an infinite loop.       */
    }
}

// Notes on pin usage:
// GPIO_DRV_ReadPinInput(kGpioSW1) == 0 // button is pressed
// GPIO_DRV_SetPinOutput(pinName)       // ex: kGpioLED1, setting means turning pin off
// GPIO_DRV_ClearPinOutput(pinName)     // clearing the led means turning it on on K64F

static  void  AppTaskRed (void *p_arg)
{
    OS_ERR    os_err;
    CPU_TS      ts;

    (void)p_arg;

    APP_TRACE_DBG(("listening on kgpiosw1, outputting red led...\n\r"));

    while (DEF_TRUE) {                                          /* Task body, always written as an infinite loop.       */

      OSSemPend(&MySem,
                         0,
                         OS_OPT_PEND_BLOCKING,
                        &ts,
                        &os_err);

        while(GPIO_DRV_ReadPinInput(kGpioSW1) == 0) // if button is pressed
        {
          GPIO_DRV_ClearPinOutput(kGpioLED2);     // turn on Red LED
        }
          GPIO_DRV_SetPinOutput(kGpioLED2);     // turn off Red LED

        OSSemPost(&MySem,
                         OS_OPT_POST_1 + OS_OPT_POST_NO_SCHED,
                        &os_err);


    }
}


static  void  AppTaskGreen (void *p_arg)
{
    OS_ERR    os_err;
    CPU_TS      ts;

    (void)p_arg;

    APP_TRACE_DBG(("listening on kgpiosw2, outputting green led...\n\r"));

    while (DEF_TRUE) {                                          /* Task body, always written as an infinite loop.       */

        OSSemPend(&MySem,
                         0,
                         OS_OPT_PEND_BLOCKING,
                        &ts,
                        &os_err);

        while(GPIO_DRV_ReadPinInput(kGpioSW2) == 0) // if button is pressed
        {
          GPIO_DRV_ClearPinOutput(kGpioLED1);     // turn on Green LED
        }
          GPIO_DRV_SetPinOutput(kGpioLED1);     // turn off Green LED

        OSSemPost(&MySem,
                         OS_OPT_POST_1 + OS_OPT_POST_NO_SCHED,
                        &os_err);

    }
}
