/*
*********************************************************************************************************
*
*                                         Micrium uC/OS-III on
*                                        Freescale Kinetis K64
*                                               on the
*
*                                         Freescale FRDM-K64F
*                                          Evaluation Board
*
*
*
* Create two tasks: 1) turns on/off red led when SW1 is pressed; 2) turns on/off green led when SW2 is pressed
* Use an ISR to respond to the triggering of the switches and use two distinct semaphores;
* leds can be on at the same time
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

static  OS_TCB       TaskRedTCB;
static  CPU_STK      TaskRedStk[APP_CFG_TASK_START_STK_SIZE];

static  OS_TCB       TaskGreenTCB;
static  CPU_STK      TaskGreenStk[APP_CFG_TASK_START_STK_SIZE];

static OS_SEM MySem1;
static OS_SEM MySem2;
// static CPU_TS ts;
static OS_ERR os_err;

/*
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/
static  void  AppTaskStart (void  *p_arg);
static  void  AppTaskRed (void  *p_arg);
static  void  AppTaskGreen (void  *p_arg);
/*
*********************************************************************************************************
*                                                main()
*********************************************************************************************************
*/
// handler associated to SW1 (labeled SW2 on board)
void SW1_Intr_Handler(void)
{
  static uint32_t c_ifsr;          // port c interrupt flag status register
  uint32_t c_portBaseAddr = g_portBaseAddr[GPIO_EXTRACT_PORT(kGpioSW1)];
  uint32_t portPinMask = (1 << GPIO_EXTRACT_PIN(kGpioSW1));

  CPU_CRITICAL_ENTER();         // enter critical section (disable interrupts)

  OSIntEnter();         // notify to scheduler the beginning of an ISR ("This allows ?C/OS-III to keep track of interrupt nesting")

  c_ifsr = PORT_HAL_GetPortIntFlag(c_portBaseAddr);         // get intr flag reg related to port C

  if( (c_ifsr & portPinMask) ) // check if kGpioSW1 generated the interrupt [pin 6 -> 7th flag (flags start with index 0)]
  {
      //sem_sw1_post
    OSSemPost(&MySem1,
                         OS_OPT_POST_1 + OS_OPT_POST_NO_SCHED,
                        &os_err);
  }

  GPIO_DRV_ClearPinIntFlag( kGpioSW1 );
  CPU_CRITICAL_EXIT();  // renable interrupts

  OSIntExit();          /* notify to scheduler the end of an ISR ("determines if a higher priority task is ready-to-run.
                          If so, the interrupt returns to the higher priority task instead of the interrupted task.") */
}

// handler associated to SW2 (labeled SW3 on board)
void SW2_Intr_Handler(void)
{
  static uint32_t a_ifsr;          // port a interrupt flag status register
  uint32_t a_portBaseAddr = g_portBaseAddr[GPIO_EXTRACT_PORT(kGpioSW2)];
  uint32_t portPinMask = (1 << GPIO_EXTRACT_PIN(kGpioSW2));

  CPU_CRITICAL_ENTER();         // enter critical section (disable interrupts)

  OSIntEnter();         // notify to scheduler the beginning of an ISR ("This allows ?C/OS-III to keep track of interrupt nesting")

  a_ifsr = PORT_HAL_GetPortIntFlag(a_portBaseAddr);             // get intr flag reg related to port A

  if((a_ifsr & portPinMask))
   {
     //sem_sw2_post
    OSSemPost(&MySem2,
                         OS_OPT_POST_1 + OS_OPT_POST_NO_SCHED,
                        &os_err);
  }

  GPIO_DRV_ClearPinIntFlag( kGpioSW2 );
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
    hardware_init();
    GPIO_DRV_Init(switchPins, ledPins);


#if (CPU_CFG_NAME_EN == DEF_ENABLED)
    CPU_NameSet((CPU_CHAR *)"MK64FN1M0VMD12",
                (CPU_ERR  *)&cpu_err);
#endif

    OSA_Init();                                                 /* Init uC/OS-III.                                      */

   OSSemCreate(&MySem1,           /* Create Semaphore 1         */
                "sem 1",
                 0,
                &err);

   OSSemCreate(&MySem2,           /* Create Semaphore 2         */
                "sem 2",
                 0,
                &err);

    INT_SYS_InstallHandler(PORTC_IRQn, SW1_Intr_Handler);       // associate ISR with sw1 intr source
    INT_SYS_InstallHandler(PORTA_IRQn, SW2_Intr_Handler);       // associate ISR with sw2 intr source

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

static  void  AppTaskRed (void *p_arg)
{
    OS_ERR    os_err;
    CPU_TS      ts;

    (void)p_arg;

    APP_TRACE_DBG(("listening on kgpiosw1, toggling red led...\n\r"));

    while (DEF_TRUE) {                                          /* Task body, always written as an infinite loop.       */

     OSSemPend(&MySem1,
                         0,
                         OS_OPT_PEND_BLOCKING,
                        &ts,
                        &os_err);

       GPIO_DRV_TogglePinOutput(BOARD_GPIO_LED_RED);



    }
}


static  void  AppTaskGreen (void *p_arg)
{
    OS_ERR    os_err;
    CPU_TS      ts;

    (void)p_arg;

    APP_TRACE_DBG(("listening on kgpiosw2, toggling green led...\n\r"));

    while (DEF_TRUE) {                                          /* Task body, always written as an infinite loop.       */

        OSSemPend(&MySem2,
                         0,
                         OS_OPT_PEND_BLOCKING,
                        &ts,
                        &os_err);

       GPIO_DRV_TogglePinOutput(BOARD_GPIO_LED_GREEN);

    }
}
