/**
  ******************************************************************************
  * @file           : lld.h
  * @brief          : Low Level Driver 共通定義
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __LLD_H
#define __LLD_H

/* Includes ------------------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/
#define SET_BIT(REG, BIT)			((REG) |= (BIT))
#define CLEAR_BIT(REG, BIT)			((REG) &= ~(BIT))

/* Exported functions prototypes ---------------------------------------------*/

/* lld_utils.c */
extern void LL_mDelay(uint32_t Delay);										/* 時間待ち処理(ms指定)					*/

/* Exported functions --------------------------------------------------------*/

/**
  * @brief  Enable SysTick exception request
  * @param  None
  * @retval None
  */
static __inline void LL_SYSTICK_EnableIT(void)
{
	SET_BIT(SysTick->CTRL, SysTick_CTRL_TICKINT_Msk);
}

/**
  * @brief  Disable SysTick exception request
  * @param  None
  * @retval None
  */
static __inline void LL_SYSTICK_DisableIT(void)
{
	CLEAR_BIT(SysTick->CTRL, SysTick_CTRL_TICKINT_Msk);
}

#endif /* __LLD_H */
