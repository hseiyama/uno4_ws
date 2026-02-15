/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : HAL Entry
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "bsp_api.h"
#include "main.h"
#include "drv.h"
#include "lib.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
extern const uint32_t __StackTop;
const uint32_t APPLICATION_VECTOR_TABLE_ADDRESS_RAM = (uint32_t)&__StackTop;

volatile uint32_t *irq_vector_table;

extern const fsp_vector_t __VECTOR_TABLE[];
extern const fsp_vector_t g_vector_table[];

volatile static uint32_t u32s_CycleTimeCounter;		/* 周期時間カウンター			*/

/* Private function prototypes -----------------------------------------------*/
static void arduino_main(void);

/* Exported functions --------------------------------------------------------*/

/**
  * @brief  SysTickタイマ経過コールバック関数
  * @param  None
  * @retval None
  */
void SYSTICK_PeriodElapsed_Callback(void)
{
	u32s_CycleTimeCounter++;
}

/**
  * @brief  hal_entry
  * @param  None
  * @retval None
  */
void hal_entry(void) {
	arduino_main();
}

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  arduino_main
  * @param  None
  * @retval None
  */
static void arduino_main(void)
{
	R_MPU_SPMON->SP[0].CTL = 0;

	__disable_irq();
	irq_vector_table = (volatile uint32_t *)APPLICATION_VECTOR_TABLE_ADDRESS_RAM;
	size_t _i;
	for (_i=0; _i<BSP_CORTEX_VECTOR_TABLE_ENTRIES; _i++) {
		*(irq_vector_table + _i) = (uint32_t)__VECTOR_TABLE[_i];
	}
	for (_i=0; _i<BSP_ICU_VECTOR_MAX_ENTRIES; _i++) {
		*(irq_vector_table + _i +BSP_CORTEX_VECTOR_TABLE_ENTRIES) = (uint32_t)g_vector_table[_i];
	}

	SCB->VTOR = (uint32_t)irq_vector_table;

	__DSB();
	__enable_irq();

	u32s_CycleTimeCounter = 0;
	/* タイマー初期化処理 */
	taskTimerInit();
	/* UARTドライバー初期化処理 */
	taskUartDriverInit();
	/* 初期化関数 */
	setup();
	/* SysTickタイマー開始(1msタイマー割り込み用) */
	LL_SYSTICK_EnableIT();
	/* Infinite loop */
	while (true) {
		/* 周期時間カウンターがシステムの周期時間[ms]に達した場合 */
		if (u32s_CycleTimeCounter >= SYS_CYCLE_TIME) {
			/* Disable Interrupts */
			__disable_irq();
			u32s_CycleTimeCounter = 0;
			/* Enable Interrupts */
			__enable_irq();

			/* タイマー更新処理 */
			taskTimerUpdate();
			/* UARTドライバー入力処理 */
			taskUartDriverInput();
			/* 周期処理関数 */
			loop();
			/* UARTドライバー出力処理 */
			taskUartDriverOutput();
		}
	}
}

