/**
  ******************************************************************************
  * @file           : lld_utils.c
  * @brief          : Low Level Driver ユーティリティ処理
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Max delay can be used in LL_mDelay */
#define LL_MAX_DELAY				(0xFFFFFFFFU)

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Exported functions --------------------------------------------------------*/

/**
  * @brief  時間待ち処理(ms指定)
  * @param  Delay: 待ち時間(ms指定)
  * @retval None
  */
void LL_mDelay(uint32_t Delay)
{
	__IO uint32_t  tmp = SysTick->CTRL;  /* Clear the COUNTFLAG first */
	uint32_t tmpDelay = Delay;

	/* Add this code to indicate that local variable is not used */
	((void)tmp);

	/* Add a period to guaranty minimum wait */
	if(tmpDelay < LL_MAX_DELAY)
	{
		tmpDelay++;
	}

	while (tmpDelay != 0U)
	{
		if((SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) != 0U)
		{
			tmpDelay--;
		}
	}
}

