/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "bsp_api.h"
#include "lld.h"

/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/

/* ON/OFF定義 */
#define OFF					(0)
#define ON					(1)

/* OK/NG定義 */
#define NG					(0)
#define OK					(1)

/* LOW/HIGH定義 */
#define LOW					(0)
#define HIGH				(1)

#define SYS_CYCLE_TIME		(5)		/* システムの周期時間[ms]		*/

/* IRQ番号の割り当て */
#define IRQ_SCI1_RXI		(0)		/* SCI1受信データフル割り込み			*/
#define IRQ_SCI1_TXI		(1)		/* SCI1送信データエンプティ割り込み		*/
#define IRQ_SCI1_TEI		(2)		/* SCI1送信終了割り込み					*/
#define IRQ_SCI1_ERI		(3)		/* SCI1受信エラー割り込み				*/
#define IRQ_PORT_IRQ0		(4)		/* 外部端子割り込み0					*/

/* Exported macro ------------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/

/* main_app.c */
extern void setup(void);									/* 初期化関数							*/
extern void loop(void);										/* 周期処理関数							*/
extern void Error_Handler(void);							/* エラー処理ハンドラ					*/

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
