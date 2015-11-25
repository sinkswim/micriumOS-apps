/*
*********************************************************************************************************
*
*                                        Micrium uC/OS-III for
*                                        Freescale Kinetis K64
*                                               on the
*
*                                         Freescale FRDM-K64F
*                                          Evaluation Board
*
* Using INTERRUPTS: create a task that receives data from the HC-SR04 ultrasonic sensor and writes on the serial port
* the distance (in cm) of objects.
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

#include <stdint.h>

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
static  OS_TCB       TaskPTB9TCB;
static  CPU_STK      TaskPTB9Stk[APP_CFG_TASK_START_STK_SIZE];

static  OS_SEM  Sem1;
static  OS_SEM  Sem2;

static  uint32_t  old_value = 0;                /* Stores old value of PTB9 line */



/*
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/

static  void  AppTaskStart (void  *p_arg);
static  void  TaskPTB9 (void  *p_arg);
static  void  BSP_PTB9_int_hdlr( void );

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

    INT_SYS_InstallHandler(PORTB_IRQn, BSP_PTB9_int_hdlr);

    OSSemCreate( &Sem1, "Semaphore 1", 0, &err );
    OSSemCreate( &Sem2, "Semaphore 2", 0, &err );


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

    OSTaskCreate(&TaskPTB9TCB,                              /* Create the start task                                */
                 "App Task ptb9",
                  TaskPTB9,
                  0u,
                  APP_CFG_TASK_START_PRIO,
                 &TaskPTB9Stk[0u],
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
*                                           TASKS
*********************************************************************************************************
*/

static  void  AppTaskStart (void *p_arg)
{
    OS_ERR      err;


    (void)p_arg;


    CPU_Init();                                                 /* Initialize the uC/CPU Services.                      */
    Mem_Init();                                                 /* Initialize the Memory Management Module              */
    Math_Init();                                                /* Initialize the Mathematical Module                   */

    BSP_Ser_Init(115200u);



    while (DEF_ON) {
      /* send trigger signals to sensor */
      GPIO_DRV_ClearPinOutput( outPTB23 );
      OSTimeDlyHMSM(0u, 0u, 0u, 5u, OS_OPT_TIME_HMSM_STRICT, &err);
      GPIO_DRV_SetPinOutput( outPTB23 );
      OSTimeDlyHMSM(0u, 0u, 0u, 100u, OS_OPT_TIME_HMSM_STRICT, &err);

    }

}

/* gets echo signal timestamps and computes object distance */
static void TaskPTB9 (void  *p_arg)
{
    OS_ERR      os_err;
    CPU_TS      os_ts;
    CPU_ERR     cpu_err;
    CPU_TS64    before, after;
    char        tmp[80];

    (void)p_arg;

    while (DEF_ON) {

        OSSemPend(&Sem1, 0,OS_OPT_PEND_BLOCKING,&os_ts, &os_err);
        before = CPU_TS_Get64();

        OSSemPend(&Sem2, 0,OS_OPT_PEND_BLOCKING,&os_ts, &os_err);

        after = CPU_TS_Get64();

        /* compute distance, refer to datasheet */
        sprintf( tmp, "Distance  = %f cm \n\r", (float)(((1000000.0*(after-before))/CPU_TS_TmrFreqGet( &cpu_err ))/58) );
        APP_TRACE_DBG(( tmp ));

    }

}


/* ISR for inPTB9, which is sensitive to either edge  */
static void BSP_PTB9_int_hdlr( void )
{

  uint32_t new_value;
  OS_ERR   os_err;
  uint32_t ifsr;         /* interrupt flag status register */
  uint32_t portBaseAddr = g_portBaseAddr[GPIO_EXTRACT_PORT(inPTB9)];
  uint32_t portPin = (1 << GPIO_EXTRACT_PIN(inPTB9));

  CPU_CRITICAL_ENTER();
  OSIntEnter();                                               /* Tell the OS that we are starting an ISR              */

  ifsr = PORT_HAL_GetPortIntFlag(portBaseAddr);
  new_value = GPIO_DRV_ReadPinInput( inPTB9 );                 /* acquire a sample of the current value */
  if( (ifsr & portPin) )                                         /* Check if the pending interrupt is for inPTB9 */
  {

        if ( new_value != old_value && new_value == 1) {
          old_value = new_value;
          OSSemPost( &Sem1, OS_OPT_POST_1+OS_OPT_POST_NO_SCHED, &os_err );
        }
        else if ( new_value != old_value && new_value == 0) {
            old_value = new_value;
            OSSemPost( &Sem2, OS_OPT_POST_1+OS_OPT_POST_NO_SCHED, &os_err );
        }

      GPIO_DRV_ClearPinIntFlag( inPTB9 );
  }

  CPU_CRITICAL_EXIT();

  OSIntExit();
}
