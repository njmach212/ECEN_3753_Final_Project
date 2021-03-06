/***************************************************************************//**
 * @file
 * @brief Simple LED Blink Demo for SLSTK3402A
 *******************************************************************************
 * # License
 * <b>Copyright 2018 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 ******************************************************************************/

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "em_device.h"
#include "em_chip.h"
#include "em_emu.h"
#include "bsp.h"
#include "main.h"
#include "app.h"
#include "bspconfig.h"
#include "capsense.h"
#include  <bsp_os.h>
#include  "bsp.h"
#include "FIFO.h"
#include "display.h"
#include "retargettextdisplay.h"
#include "textdisplay.h"
#include "displayconfigapp.h"
#include "glib.h"
#include "string.h"
#include "displayls013b7dh03config.h"
#include <math.h>

#include  <cpu/include/cpu.h>
#include  <common/include/common.h>
#include  <kernel/include/os.h>
#include  <kernel/include/os_trace.h>
#include  <kernel/include/os_port_sel.h>

#include  <common/include/lib_def.h>
#include  <common/include/rtos_utils.h>
#include  <common/include/toolchains.h>



/*
*********************************************************************************************************
*********************************************************************************************************
*                                             LOCAL DEFINES
*********************************************************************************************************
*********************************************************************************************************
*/

#define  EX_MAIN_START_TASK_PRIO              21u
#define  EX_MAIN_START_TASK_STK_SIZE         512u

#define  EX_GAIN_TASK_PRIO              22u
#define  EX_GAIN_TASK_STK_SIZE         512u

#define  EX_MOVEMENT_TASK_PRIO              15u
#define  EX_MOVEMENT_TASK_STK_SIZE         512u

#define  EX_LED_OUTPUT_TASK_PRIO              22u
#define  EX_LED_OUTPUT_TASK_STK_SIZE         512u


#define  EX_IDLE_TASK_PRIO              22u
#define  EX_IDLE_TASK_STK_SIZE         512u

#define  EX_PHYSICS_TASK_PRIO              22u
#define  EX_PHYSICS_TASK_STK_SIZE         512u

#define  EX_LCD_DISPLAY_TASK_PRIO             22u
#define  EX_LCD_DISPLAY_TASK_STK_SIZE         512u

/*
*********************************************************************************************************
*********************************************************************************************************
*                                        LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*********************************************************************************************************
*/

                                                                /* Start Task Stack.                                    */
static  CPU_STK  Ex_MainStartTaskStk[EX_MAIN_START_TASK_STK_SIZE];
                                                                /* Start Task TCB.                                      */
static  OS_TCB   Ex_MainStartTaskTCB;

static  CPU_STK  Ex_GAIN_TaskStk[EX_GAIN_TASK_STK_SIZE];
static  OS_TCB   Ex_GAIN_TaskTCB;

static  CPU_STK  Ex_MOVEMENT_TaskStk[EX_MOVEMENT_TASK_STK_SIZE];
static  OS_TCB   Ex_MOVEMENT_TaskTCB;

static  CPU_STK  Ex_IdleTaskStk[EX_IDLE_TASK_STK_SIZE];
static  OS_TCB   Ex_IdleTaskTCB;

static  CPU_STK  Ex_PHYSICS_TaskStk[EX_PHYSICS_TASK_STK_SIZE ];
static  OS_TCB   Ex_PHYSICS_TaskTCB;

static  CPU_STK  Ex_LCD_Display_TaskStk[EX_LCD_DISPLAY_TASK_STK_SIZE];
static  OS_TCB   Ex_LCD_Display_TaskTCB;


static OS_FLAG_GRP 	event_flag;
static OS_FLAG_GRP 	physics_flag;
static OS_SEM		btnsem;
static OS_MUTEX		gainmut;
static OS_MUTEX		movementmut;
static  OS_Q  Message_Q;

enum {
	useless,
	led0,
	led1,
	both,
	none,
};
/*
*********************************************************************************************************
*********************************************************************************************************
*                                       LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*********************************************************************************************************
*/

static  void  Ex_MainStartTask (void  *p_arg);
static void Ex_GAIN_Task(void *p_arg);
static void Ex_MOVEMENT_Task(void *p_arg);
static void Ex_IdleTask(void *p_arg);
static void Ex_LCD_Display_Task(void *p_arg);
static void Ex_PHYSICS_Task(void *p_arg);
static volatile uint32_t msTicks = 0, start = 0;
static  volatile uint32_t sld = 0;
struct node *head = NULL;
struct Physics physics;
static volatile uint32_t gain = 0;
static volatile uint32_t Direction = 0;
GLIB_Context_t context;
enum
{
	Far_Left,
	Middle_Left,
	Middle_Right,
	Far_Right
};

/***************************************************************************//**
 * @brief
 * depending on the slider value change the slider global variables value accordingly
 *
 * @details
 * if the sensor is being pushed top the right and not to the left then set slider global variable to 1
 * if the sensor is being pushed to the left and not to the right then set the slider global variable to 2
 * else set the slider global variable to 0
 *
 * @note
 * does not differentiate what the code does based on far left or right, groups them together so far left and left produce the same result likewise for right and far right
 *
 ******************************************************************************/
static void Sld(void)
{
	if ((CAPSENSE_getPressed(Far_Right))
			&& (!CAPSENSE_getPressed(Middle_Right) || !CAPSENSE_getPressed(Middle_Left) || !CAPSENSE_getPressed(Far_Left)))
	{
		sld = 1;
	}
	else if ((CAPSENSE_getPressed(Middle_Right))
			&& (!CAPSENSE_getPressed(Far_Right) || !CAPSENSE_getPressed(Middle_Left) || !CAPSENSE_getPressed(Far_Left)))
	{
		sld = 2;
	}
	else if ((CAPSENSE_getPressed(Far_Left))
		&& (!CAPSENSE_getPressed(Middle_Left) || !CAPSENSE_getPressed(Middle_Right) || !CAPSENSE_getPressed(Far_Right)))
	{
			sld = 3;
	}
	else if ((CAPSENSE_getPressed(Middle_Left))
		&& (!CAPSENSE_getPressed(Far_Left) || !CAPSENSE_getPressed(Middle_Right) || !CAPSENSE_getPressed(Far_Right)))
	{
			sld = 4;
	}
	else sld = 0;
}


void SysTick_Handler(void)
{
	msTicks++;
	if(msTicks%20 == 0)
	{
		start = msTicks;

	}
	if(msTicks < start + (gain))
	{
    	GPIO_PinOutSet(LED0_port, LED0_pin);
	}
	else
	{
		GPIO_PinOutClear(LED0_port, LED0_pin);
	}
}


/***************************************************************************//**
 * @brief
 * the gpio even irq handler will be called whenever and even gpio interrupt is hit
 *
 * @details
 * if the interrupt mask is the same as the pin for button 0 then clear the interrupts and call the btn0 function
 * else efm_assert false
 *
 * @note
 * if any other gpio even interrupt is triggered we want to assert false because none of them should even be enabled
 *
 ******************************************************************************/
void GPIO_EVEN_IRQHandler(void)
{
	RTOS_ERR  err;
	uint32_t intmask = GPIO_IntGet();
	int state = 0;
	if (intmask & (1 << BSP_GPIO_PB0_PIN))
	{
		GPIO_IntClear(intmask);
		state = GPIO_PinInGet(BSP_GPIO_PB0_PORT, BSP_GPIO_PB0_PIN);
		if(state == 1)
		{
			push(0, 0, &head);
			OSSemPost (&btnsem, OS_OPT_POST_ALL, &err);
		}
	}
	else EFM_ASSERT(false);
}

/***************************************************************************//**
 * @brief
 * the gpio odd irq handler will be called whenever and idd gpio interrupt is hit
 *
 * @details
 * if the interrupt mask is the same as the pin for button 1 then clear the interrupts and call the btn1 function
 * else efm_assert false
 *
 * @note
 * if any other gpio odd interrupt is triggered we want to assert false because none of them should even be enabled
 *
 ******************************************************************************/
void GPIO_ODD_IRQHandler(void)
{
	RTOS_ERR  err;
	uint32_t intmask = GPIO_IntGet();
	int state = 0;
	if (intmask & (1 << BSP_GPIO_PB1_PIN))
	{
		GPIO_IntClear(intmask);
		state = GPIO_PinInGet(BSP_GPIO_PB1_PORT, BSP_GPIO_PB1_PIN);
		if(state == 1)
		{
			push(1, 0, &head);
			OSSemPost(&btnsem, OS_OPT_POST_ALL, &err);
		}
	}
	else EFM_ASSERT(false);
}


int main(void)
{
	  RTOS_ERR  err;
	  SysTick_Config(CMU_ClockFreqGet(cmuClock_CORE) / 1000);
	  BSP_SystemInit();                                           /* Initialize System.                                   */
	  CPU_Init();
	  //while(DISPLAY_Init() != DISPLAY_EMSTATUS_OK);
	  OS_TRACE_INIT();
	  OSInit(&err);                                               /* Initialize the Kernel.                               */
																/*   Check error code.                                  */
	  APP_RTOS_ASSERT_DBG((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), 1);


	  OSTaskCreate(&Ex_MainStartTaskTCB,                          /* Create the Start Task.                               */
				 "Ex Main Start Task",
				  Ex_MainStartTask,
				  DEF_NULL,
				  EX_MAIN_START_TASK_PRIO,
				 &Ex_MainStartTaskStk[0],
				 (EX_MAIN_START_TASK_STK_SIZE / 10u),
				  EX_MAIN_START_TASK_STK_SIZE,
				  0u,
				  0u,
				  DEF_NULL,
				 (OS_OPT_TASK_STK_CLR),
				 &err);

	  OSTaskCreate(&Ex_GAIN_TaskTCB,                          /* Create the Start Task.                               */
				 "Ex Gain Task",
				  Ex_GAIN_Task,
				  DEF_NULL,
				  EX_GAIN_TASK_PRIO,
				 &Ex_GAIN_TaskStk[0],
				 (EX_GAIN_TASK_STK_SIZE / 10u),
				  EX_GAIN_TASK_STK_SIZE,
				  0u,
				  0u,
				  DEF_NULL,
				 (OS_OPT_TASK_STK_CLR),
				 &err);

	  OSTaskCreate(&Ex_MOVEMENT_TaskTCB,                          /* Create the Start Task.                               */
				 "Ex Movement Task",
				  Ex_MOVEMENT_Task,
				  DEF_NULL,
				  EX_MOVEMENT_TASK_PRIO,
				 &Ex_MOVEMENT_TaskStk[0],
				 (EX_MOVEMENT_TASK_STK_SIZE / 10u),
				  EX_MOVEMENT_TASK_STK_SIZE,
				  0u,
				  0u,
				  DEF_NULL,
				 (OS_OPT_TASK_STK_CLR),
				 &err);

	  OSTaskCreate(&Ex_IdleTaskTCB,                          /* Create the Start Task.                               */
				 "Ex Idle Task",
				  Ex_IdleTask,
				  DEF_NULL,
				  EX_IDLE_TASK_PRIO,
				 &Ex_IdleTaskStk[0],
				 (EX_IDLE_TASK_STK_SIZE / 10u),
				  EX_IDLE_TASK_STK_SIZE,
				  0u,
				  0u,
				  DEF_NULL,
				 (OS_OPT_TASK_STK_CLR),
				 &err);

	  OSTaskCreate(&Ex_PHYSICS_TaskTCB,                          /* Create the Start Task.                               */
				 "Ex Physics Task",
				  Ex_PHYSICS_Task,
				  DEF_NULL,
				  EX_PHYSICS_TASK_PRIO,
				 &Ex_PHYSICS_TaskStk[0],
				 (EX_PHYSICS_TASK_STK_SIZE / 10u),
				  EX_PHYSICS_TASK_STK_SIZE,
				  0u,
				  0u,
				  DEF_NULL,
				 (OS_OPT_TASK_STK_CLR),
				 &err);

	  OSTaskCreate(&Ex_LCD_Display_TaskTCB,                          /* Create the Start Task.                               */
				 "Ex LCD Display Task",
				  Ex_LCD_Display_Task,
				  DEF_NULL,
				  EX_LCD_DISPLAY_TASK_PRIO,
				 &Ex_LCD_Display_TaskStk[0],
				 (EX_LCD_DISPLAY_TASK_STK_SIZE / 10u),
				  EX_LCD_DISPLAY_TASK_STK_SIZE,
				  0u,
				  0u,
				  DEF_NULL,
				 (OS_OPT_TASK_STK_CLR),
				 &err);



	  	  OSFlagCreate ((OS_FLAG_GRP  *) &event_flag,
	                      (CPU_CHAR     *)"event Flag",
	                      (OS_FLAGS)      0,
	                      (RTOS_ERR     *)&err);

	  	  OSFlagCreate ((OS_FLAG_GRP  *) &physics_flag,
	                      (CPU_CHAR     *)"Physics Flag",
	                      (OS_FLAGS)      0,
	                      (RTOS_ERR     *)&err);

	  	  OSSemCreate ((OS_SEM      *)&btnsem,
	                     (CPU_CHAR    *)"BTN Semafore",
	                     (OS_SEM_CTR)   0,
	                     (RTOS_ERR    *)&err);

	  	  OSMutexCreate ((OS_MUTEX      *)&movementmut,
	                     (CPU_CHAR    *)"Force Mutex",
	                     (RTOS_ERR    *)&err);

	  	  OSMutexCreate ((OS_MUTEX      *)&gainmut,
	                     (CPU_CHAR    *)"gain Mutex",
	                     (RTOS_ERR    *)&err);


	  	OSQCreate((OS_Q *) &Message_Q,
	  				(CPU_CHAR *)"Message Queue",
	  				(OS_MSG_QTY)4,
	  				(RTOS_ERR *)&err);


																/*   Check error code.                                  */
	  	  APP_RTOS_ASSERT_DBG((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), 1);

	  	  OSStart(&err);                                              /* Start the kernel.                                    */										/*   Check error code.                                  */
	  	  APP_RTOS_ASSERT_DBG((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), 1);




	  	  while (1)
	  	  {
	  	  }
}

static  void  Ex_MainStartTask (void  *p_arg)
{
	  EMU_DCDCInit_TypeDef dcdcInit = EMU_DCDCINIT_DEFAULT;
	  CMU_HFXOInit_TypeDef hfxoInit = CMU_HFXOINIT_DEFAULT;

	  /* Chip errata */
	  CHIP_Init();

	  /* Init DCDC regulator and HFXO with kit specific parameters */
	  /* Init DCDC regulator and HFXO with kit specific parameters */
	  /* Initialize DCDC. Always start in low-noise mode. */
	  EMU_EM23Init_TypeDef em23Init = EMU_EM23INIT_DEFAULT;
	  EMU_DCDCInit(&dcdcInit);
	  em23Init.vScaleEM23Voltage = emuVScaleEM23_LowPower;
	  EMU_EM23Init(&em23Init);
	  CMU_HFXOInit(&hfxoInit);

	  /* Switch HFCLK to HFRCO and disable HFRCO */
	  CMU_OscillatorEnable(cmuOsc_HFRCO, true, true);
	  CMU_ClockSelectSet(cmuClock_HF, cmuSelect_HFRCO);
	  CMU_OscillatorEnable(cmuOsc_HFXO, false, false);

	  app_peripheral_setup();
	  GPIO_IntConfig(BSP_GPIO_PB0_PORT, BSP_GPIO_PB0_PIN, true, true, true);
	  GPIO_IntConfig(BSP_GPIO_PB1_PORT, BSP_GPIO_PB1_PIN, true, true, true);
	  GPIO_PinModeSet(BSP_GPIO_PB0_PORT, BSP_GPIO_PB0_PIN, gpioModeInput, false);
	  GPIO_PinModeSet(BSP_GPIO_PB1_PORT, BSP_GPIO_PB1_PIN, gpioModeInput, false);
	  NVIC_EnableIRQ(GPIO_EVEN_IRQn);
	  NVIC_EnableIRQ(GPIO_ODD_IRQn);
	  RTOS_ERR  err;                                               /* Initialize CPU.                                      */
	  //OSTmrStart(&os_tmr, &err);
	  PP_UNUSED_PARAM(p_arg);                                     /* Prevent compiler warning.                            */

	  Common_Init(&err);                                          /* Call common module initialization example.           */
	  APP_RTOS_ASSERT_CRITICAL(err.Code == RTOS_ERR_NONE, ;);

	  BSP_OS_Init();
	  BSP_LedsInit();
	  while (DEF_ON) {
                                                                /* Delay Start Task execution for                       */
        OSTimeDly( 100,                                        /*   100 OS Ticks                                      */
                   OS_OPT_TIME_DLY,                             /*   from now.                                          */
                  &err);
                                                                /*   Check error code.                                  */
        APP_RTOS_ASSERT_DBG((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), ;);
    }
}


static  void  Ex_GAIN_Task (void  *p_arg)
{
    RTOS_ERR  err;
    PP_UNUSED_PARAM(p_arg);
    int btn, state;
    while (1)
    {
        OSSemPend(&btnsem, 0, OS_OPT_PEND_BLOCKING, (CPU_TS*)0, &err);
        pop(&btn, &state, &head);
        OSMutexPend(&gainmut, 0, OS_OPT_PEND_BLOCKING, (CPU_TS *)0, &err);
        if(btn == 0 && state == 0)
        {
        	if (gain == 20)
        	{
        		gain = gain;
        	}
        	else
        	{
            	gain++;
        	}
        }
        else if(btn == 1 && state == 0)
        {
        	if(gain == 0)
        	{
        		gain = gain;
        	}
        	else
        	{
            	gain--;
        	}

        }
        OSMutexPost(&gainmut, OS_OPT_POST_NONE, &err);
        OSFlagPost(&physics_flag, 0x1, OS_OPT_POST_FLAG_SET, &err);
    }
}

static  void  Ex_MOVEMENT_Task (void  *p_arg)
{
    RTOS_ERR  err;
    PP_UNUSED_PARAM(p_arg);
    CAPSENSE_Init();
    int temp = 0;
    while (1)
    {
       OSTimeDly(50, OS_OPT_TIME_DLY, &err);
       CAPSENSE_Sense();
       temp = sld;
       Sld();
       OSMutexPend(&movementmut, 0, OS_OPT_PEND_BLOCKING, (CPU_TS *)0, &err);
       if (sld == 1)
       {
    	   Direction = 2;
       }
       else if (sld == 2)
       {
    	   Direction = 2;
       }
       else if (sld == 3)
       {
    	   Direction = 1;
       }
       else if (sld == 4)
       {
    	   Direction = 1;
       }
       else
       {
    	   Direction = 0;
       }
       OSMutexPost(&movementmut, OS_OPT_POST_NONE, &err);
       if (temp != 0)
       {
    	   OSFlagPost(&physics_flag, 0x2, OS_OPT_POST_FLAG_SET, &err);
       }

    }
}


static  void  Ex_IdleTask (void  *p_arg)
{
    RTOS_ERR  err;
    PP_UNUSED_PARAM(p_arg);
    while (1)
    {
        OSTimeDly( 1000,                                        /*   100 OS Ticks                                      */
                   OS_OPT_TIME_DLY,                             /*   from now.                                          */
                  &err);
        EMU_EnterEM1();
    }
}

static  void  Ex_PHYSICS_Task (void  *p_arg)
{
    RTOS_ERR  err;
    OS_FLAGS flag;
    int dir = 0;
    physics.Gain = 0;
    physics.st = msTicks;
    physics.ed = 0;
    physics.delta_t = 0;
    physics.Dir = 0;
    physics.gravity = 9.8;
    physics.mass = 0.05;
    physics.length = 20;
    physics.xmin = -64;
    physics.xmax = 64;
    physics.h_velocity = 0;
    physics.v_velocity = 0;
    physics.h_acceleration = 0;
    physics.v_acceleration = 0;
    physics.h_force = 0;
    physics.v_force = 0;
    physics.hb_force = 0;
    physics.vb_force = 0;
    physics.hc_force = 0;
    physics.vc_force = 0;
    physics.h_position = 0;
    physics.v_position = 20;
    physics.theta = 0;
    physics.gm = 0;
    struct lcd_info post_msg;
    PP_UNUSED_PARAM(p_arg);
    while (1)
    {
        if(physics.v_position <= 0 || (physics.h_position*3) <= physics.xmin || (physics.h_position*3) >= physics.xmax)
        {

        	physics.gm = 1;
            GPIO_PinOutSet(LED1_port, LED1_pin);
        	OSMutexPend(&gainmut, 0, OS_OPT_PEND_BLOCKING, (CPU_TS *)0, &err);
        	gain = 0;
        	OSMutexPost(&gainmut, OS_OPT_POST_NONE, &err);
        }
        flag = OSFlagPend(&physics_flag, 0x3, 0, OS_OPT_PEND_FLAG_SET_ANY + OS_OPT_PEND_FLAG_CONSUME + OS_OPT_PEND_NON_BLOCKING, (CPU_TS *)0, &err);
        OSTimeDly( 10,                                        /*   100 OS Ticks                                      */
                   OS_OPT_TIME_DLY,                             /*   from now.                                          */
                  &err);
        if (0x2 == flag || 0x03 == flag)
        {
        	OSMutexPend(&movementmut, 0, OS_OPT_PEND_BLOCKING, (CPU_TS *)0, &err);
        	physics.Dir = Direction;
        	OSMutexPost(&movementmut, OS_OPT_POST_NONE, &err);
        }
        if (0x1 == flag || 0x03 == flag)
        {
        	OSMutexPend(&gainmut, 0, OS_OPT_PEND_BLOCKING, (CPU_TS *)0, &err);
        	physics.Gain = gain;
        	OSMutexPost(&gainmut, OS_OPT_POST_NONE, &err);
        }
        physics.ed = msTicks;
        if(physics.Dir != 0)
        {
        	dir = physics.Dir;
        }
        if(physics.Gain != 0 && physics.theta == 0 && (physics.Dir == 1 || dir == 1))
        {
            physics.theta = 0.0175;
            physics.v_acceleration = 0;
            physics.v_velocity = 0;
        }
        else if (physics.Gain != 0 && physics.theta == 0 && (physics.Dir == 2 || dir == 2))
        {
            physics.theta = -0.0175;
            physics.v_acceleration = 0;
            physics.v_velocity = 0;
        }
        physics.delta_t = (physics.ed - physics.st)/1000;
        physics.vb_force = physics.mass * physics.gravity;
        physics.hb_force = -(physics.vb_force * tan(physics.theta));
        physics.hc_force = physics.Gain*((bool)(physics.Dir))*sin(physics.theta)*sin(physics.theta);
        if(physics.theta == 0)
        {
            physics.vc_force = physics.mass*physics.gravity;
        }
        else
        {
            physics.vc_force = physics.Gain*((bool)(physics.Dir))*sin(physics.theta)*cos(physics.theta);
            if(physics.vc_force < 0)
            {
                physics.vc_force = -physics.vc_force;
            }
        }
        if(physics.theta < 0 && physics.Gain !=0 && physics.Dir == 2)
        {
        	physics.vc_force = 0;
        }
        else if(physics.theta > 0 && physics.Gain !=0 && physics.Dir == 1)
        {
        	physics.vc_force = 0;
        }
        physics.v_force = physics.vc_force - physics.vb_force;
        if(physics.Dir == 1)
        {
            if(physics.hb_force < physics.hc_force)
            {
                physics.h_force = physics.hb_force - physics.hc_force;
            }
            else
            {
                physics.h_force = 0;
            }
        }
        else if(physics.Dir == 2)
        {
            if(physics.hb_force < physics.hc_force)
            {
                physics.h_force = physics.hb_force + physics.hc_force;
            }
            else
            {
                physics.h_force = 0;
            }
        }
        else
        {
            physics.h_force = 0;
        }
        physics.v_acceleration = physics.v_force/physics.mass;
        physics.h_acceleration = physics.h_force/physics.mass;
        physics.v_velocity = physics.v_velocity + physics.v_acceleration*physics.delta_t;
        physics.h_velocity = physics.h_velocity + physics.h_acceleration*physics.delta_t;
        physics.v_position = physics.v_position + physics.v_velocity*physics.delta_t;
        physics.h_position = physics.h_position + physics.h_velocity*physics.delta_t;
        physics.hc_position = physics.h_position - physics.length*sin(physics.theta);
        if (physics.v_position > 20)
        {
        	physics.v_position = 20;
        }
        double horizantal = sqrt((physics.length * physics.length) - (physics.v_position * physics.v_position));
        if (horizantal < 0)
        {
        	horizantal = -(physics.h_position - physics.hc_position);
        }
        if(physics.hc_position < physics.h_position)
        {
            physics.theta = asin(horizantal/physics.length);
        }
        else if(physics.hc_position > physics.h_position)
        {
            physics.theta = -asin(horizantal/physics.length);
        }
        else
        {
            physics.theta = 0;
        }
        physics.st = physics.ed;
        post_msg.c_postion = physics.hc_position;
        post_msg.x_position = physics.h_position;
        post_msg.y_position = physics.v_position;
        post_msg.gm = physics.gm;
		OSQPost(&Message_Q, &post_msg, sizeof(post_msg), OS_OPT_POST_FIFO, &err);
    }
}

static  void  Ex_LCD_Display_Task (void  *p_arg)
{
    RTOS_ERR  err;
    PP_UNUSED_PARAM(p_arg);
	while(DMD_init(0) != DMD_OK);
	while(GLIB_contextInit(&context) != GLIB_OK);
	GLIB_Rectangle_t Rect;
    struct lcd_info *buf;
    double x = 0, y = 0, c = 0;
    int gm = 0;
    char gameover [20];
    strncpy(gameover, "Game Over", 9);
    context.backgroundColor = Black;
    context.foregroundColor = White;
    GLIB_clear(&context);
    GLIB_drawLine(&context,  64,128, 64,68);
    DMD_updateDisplay();
    while (1)
    {
    	buf = OSQPend(&Message_Q, 0 , OS_OPT_PEND_BLOCKING, (OS_MSG_SIZE *)sizeof(struct lcd_info), (CPU_TS *)0, &err);
    	x = buf->x_position;
    	y = buf->y_position;
    	c = buf->c_postion;
    	gm = buf->gm;
    	if(gm == 1)
    	{
    		GLIB_clear(&context);
    		GLIB_drawString(&context, gameover, strlen(gameover), 30, 64, true);
    	}
    	else
    	{
        	c *= 3;
        	x *= 3;
        	y *= 3;
        	y = 128-y;
        	x += 64;
        	c += 64;
            GLIB_clear(&context);
            GLIB_drawLine(&context,  c,128, x,y);
            GLIB_drawCircleFilled(&context, x, y, 5);
            Rect.xMax = c+10;
    		Rect.xMin = c-10;
    		Rect.yMax = 128;
    		Rect.yMin = 118;
            GLIB_drawRectFilled(&context, &Rect);
    	}
        DMD_updateDisplay();
    }
}
