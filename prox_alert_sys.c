/* Robert Margelli - 224854 */

/* includes */
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

/* macros and typedefs */
#define lptmr_start() (LPTMR0->CSR |= (1 << 0))         /* enable timer (starts counting), sets TEN bit */
#define disable_timer() (LPTMR0->CSR &= 0xFFFFFFFEu)    /* disable timer (), unsets TEN bit */
typedef enum {red, blue, green} color;          /* simple enum for LED color */

/* Task resources */
static  OS_TCB       AppTaskStartTCB;
static  CPU_STK      AppTaskStartStk[APP_CFG_TASK_START_STK_SIZE];
static  OS_TCB       MainTaskTCB;
static  CPU_STK      MainTaskStk[APP_CFG_TASK_START_STK_SIZE];
static  OS_TCB       BlinkerTCB;
static  CPU_STK      BlinkerStk[APP_CFG_TASK_START_STK_SIZE];

/* Global variables */
color led_color = red;          /* stores value of LED to turn on */
uint32_t half_period = 0u;      /* 0u means keep the LED on */
uint16_t counter = 0;           /* stores timer counter register (CNR) value */

/* Function prototypes */
static  void  AppTaskStart (void  *p_arg);
static  void  MainTask (void  *p_arg);
static void BlinkerTask (void *p_arg);
static void ptb9_handler(void);
void LPTMR_init(void);
uint32_t get_counter_value(void);
void os_err_check(OS_ERR os_err);


/* Main: initializes OS and creates AppTaskStart */
int  main (void)
{
    OS_ERR   os_err;
    
#if (CPU_CFG_NAME_EN == DEF_ENABLED)
    CPU_ERR  cpu_err;
#endif
    
    hardware_init();
    GPIO_DRV_Init(switchPins, ledPins);
    
#if (CPU_CFG_NAME_EN == DEF_ENABLED)
    CPU_NameSet((CPU_CHAR *)"MK64FN1M0VMD12",
                (CPU_ERR  *)&cpu_err);
    if (cpu_err != CPU_ERR_NONE)
    {
        APP_TRACE_DBG(( "CPU error." ));
    }
#endif
    OSA_Init();                                                 /* Init uC/OS-III */
    
    INT_SYS_InstallHandler(PORTB_IRQn, ptb9_handler);           /* installs ISR for PTB9 */
    
    BSP_Ser_Init(115200u);              /* useful for debugging purposes to output to serial  */
    
    OSTaskCreate(&AppTaskStartTCB,                              /* Create the start task */
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
                 &os_err);
    os_err_check(os_err);
    
    OSA_Start();                                                /* Start multitasking (i.e. give control to uC/OS-III). */
    
    while (DEF_ON) {                                            /* Should Never Get Here */
        ;
    }
}


/* AppTaskStart: initializes services, modules, creates the main task (MainTask) and then dies */
static  void  AppTaskStart (void *p_arg)
{
    OS_ERR      os_err;
    (void)p_arg;
    
    CPU_Init();                                                 /* Initialize the uC/CPU Services. */
    Mem_Init();                                                 /* Initialize the Memory Management Module */
    Math_Init();                                                /* Initialize the Mathematical Module */
    
    /* hardware inits for lptmr */
    MCG->C2 |= 1;       /* select fast internal reference clock (irc) (4 MHz) - MCG Control 2 Register */
    MCG->SC |= 0x04u;   /* divide irc by 4 (get 1 MHz) -  MCG Control and Status Register */
    LPTMR_init();       /* initialize lptmr registers */
    
    OSTaskCreate(&MainTaskTCB,                              /* Create the MainTask */
                 "MainTask: responsible for all operations",
                 MainTask,
                 0u,
                 APP_CFG_TASK_START_PRIO,
                 &MainTaskStk[0u],
                 (APP_CFG_TASK_START_STK_SIZE / 10u),
                 APP_CFG_TASK_START_STK_SIZE,
                 0u,
                 0u,
                 0u,
                 (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR | OS_OPT_TASK_SAVE_FP),
                 &os_err);
    os_err_check(os_err);
    
    
    OSTaskCreate(&BlinkerTCB,                              /* Create the BlinkerTask */
                 "BlinkerTask: blinks LED",
                 BlinkerTask,
                 0u,
                 APP_CFG_TASK_START_PRIO,
                 &BlinkerStk[0u],
                 (APP_CFG_TASK_START_STK_SIZE / 10u),
                 APP_CFG_TASK_START_STK_SIZE,
                 0u,
                 0u,
                 0u,
                 (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR | OS_OPT_TASK_SAVE_FP),
                 &os_err);
    os_err_check(os_err);
    
    
    OSTaskDel((OS_TCB *)0, &os_err);       /* delete this task */
    os_err_check(os_err);
    
}


/* sends trigger signal, computes distance and wakes up BlinkerTask if distance is in a new range */
static  void  MainTask (void  *p_arg)
{
    OS_ERR      os_err;
    char tmp[80];           /* used for debugging */
    float distance;                             /* stores distance value */
    uint8_t range = 0;                   /* keeps track of current range */
    /* lbs[0] and ubs[0] are used only at first run since no previous range is available for comparison*/
    float lbs[12] = {500.0, 0.0, 10.0, 25.0, 50.0, 75.0, 100.0, 120.0, 140.0, 160.0, 180.0, 200.0};
    float ubs[12] = {0.0, 10.0, 25.0, 50.0, 75.0, 100.0, 120.0, 140.0, 160.0, 180.0, 200.0, 500.0};
    
    (void)p_arg;
    
    while (DEF_ON) {
        /* send trigger signal to ultrasonic sensor */
        GPIO_DRV_ClearPinOutput( outPTB23 );             /* set PTB23 (trigger) to high */
        OSTimeDlyHMSM(0u, 0u, 0u, 5u, OS_OPT_TIME_HMSM_STRICT, &os_err);
        os_err_check(os_err);
        GPIO_DRV_SetPinOutput( outPTB23 );               /* set PTB23 (trigger) to low */
        OSTimeDlyHMSM(0u, 0u, 0u, 65u, OS_OPT_TIME_HMSM_STRICT, &os_err);      /* must wait 60us between triggerings, this provides a safe margin */
        os_err_check(os_err);
        
        /* compute distance and check if in a new range */
        distance = ((float)(1.0 * counter)/(58));        /* update distance to new value */
        if(distance < lbs[range] || distance >= ubs[range])  /* new distance is in another range */
        {
            /* <if else> cascade to find distance range and call blink function with parameters related to range */
            if(distance < 10.0)
            {
                range = 1;
                led_color = red;
                half_period = 0u;
            }
            else if(distance >= 10.0 && distance < 25.0)
            {
                range = 2;
                led_color = red;
                half_period = 200u;
            }
            else if(distance >= 25.0 && distance < 50.0)
            {
                range = 3;
                led_color = red;
                half_period = 300u;
            }
            else if(distance >= 50.0 && distance < 75.0)
            {
                range = 4;
                led_color = red;
                half_period = 400u;
            }
            else if(distance >= 75.0 && distance < 100.0)
            {
                range = 5;
                led_color = red;
                half_period = 500u;
            }
            else if(distance >= 100.0 && distance < 120.0)
            {
                range = 6;
                led_color = blue;
                half_period = 100u;
            }
            else if(distance >= 120.0 && distance < 140.0)
            {
                range = 7;
                led_color = blue;
                half_period = 200u;
            }
            else if(distance >= 140.0 && distance < 160.0)
            {
                range = 8;
                led_color = blue;
                half_period = 300u;
            }
            else if(distance >= 160.0 && distance < 180.0)
            {
                range = 9;
                led_color = blue;
                half_period = 400u;
            }
            else if(distance >= 180.0 && distance < 200.0)
            {
                range = 10;
                led_color = blue;
                half_period = 500u;
            }
            else
            {
                range = 11;
                led_color = green;
                half_period = 1000u;
            }
            OSTimeDlyResume(&BlinkerTCB, &os_err);            /* wake up blinker */
        }
        /* debugging, prints distance to serial */
         sprintf(tmp, "Measured distance = %f cm \n\r", distance);
         APP_TRACE_DBG(( tmp ));
         
    }
}


/* blink an LED with a specific color and frequency */
void BlinkerTask(void *p_arg)
{
    OS_ERR      os_err;
    (void)p_arg;
    
    while (DEF_ON) {
        if(half_period == 0u)   /* keep RED LED on */
        {
            GPIO_DRV_ClearPinOutput( BOARD_GPIO_LED_RED );
        }
        else
        {
            /* turn off any ON LED */
            GPIO_DRV_SetPinOutput( BOARD_GPIO_LED_RED );
            GPIO_DRV_SetPinOutput( BOARD_GPIO_LED_BLUE );
            GPIO_DRV_SetPinOutput( BOARD_GPIO_LED_GREEN );
            /* blink  */
            if(led_color == red)
            {
                GPIO_DRV_ClearPinOutput(BOARD_GPIO_LED_RED);
                OSTimeDlyHMSM(0u, 0u, 0u, half_period, OS_OPT_TIME_HMSM_STRICT, &os_err);
                GPIO_DRV_SetPinOutput(BOARD_GPIO_LED_RED);
                OSTimeDlyHMSM(0u, 0u, 0u, half_period, OS_OPT_TIME_HMSM_STRICT, &os_err);
            }
            else if(led_color == blue)
            {
                GPIO_DRV_ClearPinOutput(BOARD_GPIO_LED_BLUE);
                OSTimeDlyHMSM(0u, 0u, 0u, half_period, OS_OPT_TIME_HMSM_STRICT, &os_err);
                GPIO_DRV_SetPinOutput(BOARD_GPIO_LED_BLUE);
                OSTimeDlyHMSM(0u, 0u, 0u, half_period, OS_OPT_TIME_HMSM_STRICT, &os_err);
            }
            else if (led_color == green)
            {
                GPIO_DRV_ClearPinOutput(BOARD_GPIO_LED_GREEN);
                OSTimeDlyHMSM(0u, 0u, 0u, half_period, OS_OPT_TIME_HMSM_STRICT, &os_err);
                GPIO_DRV_SetPinOutput(BOARD_GPIO_LED_GREEN);
                OSTimeDlyHMSM(0u, 0u, 0u, half_period, OS_OPT_TIME_HMSM_STRICT, &os_err);
            }
        }
    }
}


/* ISR for PTB9 GPIO pin, which is sensitive to either edge */
static void ptb9_handler(void)
{
    static  uint32_t  old_level = 0;                /* stores old value of PTB9 line */
    uint32_t new_level;
    uint32_t ifsr;         /* interrupt flag status register */
    uint32_t portBaseAddr = g_portBaseAddr[GPIO_EXTRACT_PORT(inPTB9)];
    uint32_t portPin = (1 << GPIO_EXTRACT_PIN(inPTB9));
    
    CPU_CRITICAL_ENTER();
    OSIntEnter();                                               /* Tell the OS that we are starting an ISR              */
    
    ifsr = PORT_HAL_GetPortIntFlag(portBaseAddr);
    new_level = GPIO_DRV_ReadPinInput( inPTB9 );                 /*  */
    if( (ifsr & portPin) )                                         /* Check if the pending interrupt is for inPTB9 */
    {
        if(new_level != old_level)     /* edge on echo signal occured */
        {
            /* rising edge of echo signal, start counting */
            if (new_level == 1) {
                old_level = new_level;
                lptmr_start();      /* start counting */
            }
            /* falling edge of echo signal, read counter value and stop counting*/
            else if (new_level == 0)
            {
                old_level = new_level;
                counter = get_counter_value();      /* store CNR reg value */
                disable_timer();            /* stop the timer */
            }
        }
        GPIO_DRV_ClearPinIntFlag( inPTB9 );
    }
    
    CPU_CRITICAL_EXIT();
    OSIntExit();
}

/* LPTMR initialization */
void LPTMR_init(void)
{
    SIM_SCGC5 |= (1 << 0);            /* enable clock software access to LPTMR - System Clock Gating Control Register 5 */
    
    /* reset lptmr registers */
    LPTMR0->CSR = 0;
    LPTMR0->PSR = 0;
    
    LPTMR0->CSR |= (1 << 2);        /* free-running mode: CNR is reset on overflow  */
    LPTMR0->PSR |= (1 << 2);        /* bypass LPTMR prescaler (use 1 MHz irc for LPTMR) */
}

/* returns CNR register value */
uint32_t get_counter_value(void)
{
    LPTMR0->CNR = 1;     /* looks strange but we must write (any value) in CNR register right before reading its value */
    uint16_t lower_half_mask = 0xFFFFu;
    return (LPTMR0->CNR & lower_half_mask);      /* return CNR value (only lower 16 bits are used to count) */
}


/* simple error check */
void os_err_check(OS_ERR os_err)
{
    if (os_err != OS_ERR_NONE) 
    {
        APP_TRACE_DBG(( "OS Error." ));
    }
}
