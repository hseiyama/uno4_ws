/**
  ******************************************************************************
  * @file           : main_app.c
  * @brief          : MAINアプリケーション
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "drv.h"
#include "lib.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
#define TIME_1S				(1000)					/* 1秒判定時間[ms]			*/
#define UART_BUFF_SIZE		(64)					/* UARTバッファサイズ		*/

/* UART命令 */
#define UART_CMD_HELP		(0x08)					/* ヘルプ表示(^H)			*/
#define UART_CMD_RESET		(0x12)					/* リセット(^R)				*/
#define UART_CMD_SLEEP		(0x13)					/* スリープ(^S)				*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static Timer sts_Timer1s;							/* 1秒タイマー				*/
static uint8_t u8s_RcvData[UART_BUFF_SIZE];			/* UART受信データ			*/
static uint16_t u16s_RcvDataSize;					/* UART受信データサイズ		*/

/* Private function prototypes -----------------------------------------------*/
static void port_irq0_init(void);					/* PORT_IRQ0 初期化処理					*/

/* Exported functions --------------------------------------------------------*/

/**
  * @brief  外部端子割り込み0ハンドラ
  * @param  None
  * @retval None
  */
void PORT_IRQ0_Handler(void)
{
    /* 割り込み要求フラグ クリア */
	R_ICU->IELSR_b[IRQ_PORT_IRQ0].IR = 0;

	/* 文字を出力する */
	uartEchoStr("Exti12");
}

/**
  * @brief  初期化関数
  * @param  None
  * @retval None
  */
void setup(void)
{
	mem_set08(&u8s_RcvData[0], 0x00, UART_BUFF_SIZE);
	u16s_RcvDataSize = 0;

	// 各ポートの方向設定
	R_PORT1->PDR_b.PDR11 = 1;						// SCK LED(P111): 出力
	R_PORT0->PDR_b.PDR12 = 1;						// TX LED(P012): 出力
	R_PORT0->PDR_b.PDR13 = 1;						// RX LED(P013): 出力

	/* PORT_IRQ0 初期化処理 */
	port_irq0_init();

	/* タイマーを開始する */
	startTimer(&sts_Timer1s);

	/* プログラム開始メッセージを表示する */
	uartEchoStrln("");
	uartEchoStrln("Start UART/GPIO sample!!");
}

/**
  * @brief  周期処理関数
  * @param  None
  * @retval None
  */
void loop(void)
{
	static uint8_t u8_led_state = 0;

	/* UART受信データを取得する */
	u16s_RcvDataSize = uartGetRxData(&u8s_RcvData[0], UART_BUFF_SIZE);
	/* UART受信データが存在する場合 */
	if (u16s_RcvDataSize > 0) {
		/* UART送信データを登録する */
		uartSetTxData(&u8s_RcvData[0], u16s_RcvDataSize);

		/* UART命令解析 */
		switch (u8s_RcvData[0]) {
		/* ヘルプ表示(^H) */
		case UART_CMD_HELP:
			/* UART命令表示 */
			uartEchoStrln("");
			uartEchoStrln("^H :Help");
			uartEchoStrln("^R :Reset");
			uartEchoStrln("^S :Sleep");
			break;
		/* リセット(^R) */
		case UART_CMD_RESET:
			/* リセット処理 */
			NVIC_SystemReset();
			break;
		/* スリープ(^S) */
		case UART_CMD_SLEEP:
			/* SysTickタイマー停止 */
			LL_SYSTICK_DisableIT();
			/* イベント待機 */
			__WFE();
			__WFE();
			/* SysTickタイマー開始 */
			LL_SYSTICK_EnableIT();
			/* 文字を出力する */
			uartEchoStr("<Wakeup!!>");
			break;
		}
	}

	/* 1秒判定時間が満了した場合 */
	if (checkTimer(&sts_Timer1s, TIME_1S)) {
		/* ユーザーLEDを反転出力する */
		switch (u8_led_state) {
		case 0:
			// 各ポートの出力データ設定(1)
			R_PORT1->PODR_b.PODR11 = 1;				// SCK LED(P111): High出力(点灯)
			R_PORT0->PODR_b.PODR12 = 1;				// TX LED(P012): High出力(消灯)
			R_PORT0->PODR_b.PODR13 = 1;				// RX LED(P013): High出力(消灯)
			break;
		case 1:
			// 各ポートの出力データ設定(2)
//			R_PORT1->PODR_b.PODR11 = 0;				// SCK LED(P111): High出力(消灯)
//			R_PORT0->PODR_b.PODR12 = 0;				// TX LED(P012): High出力(点灯)
//			R_PORT0->PODR_b.PODR13 = 1;				// RX LED(P013): High出力(消灯)
			R_PORT1->PORR = 0x0800;
			R_PORT0->PORR = 0x1000;
			R_PORT0->POSR = 0x2000;
			break;
		case 2:
	// 各ポートの出力データ設定(3)
//			R_PORT1->PODR_b.PODR11 = 0;				// SCK LED(P111): High出力(消灯)
//			R_PORT0->PODR_b.PODR12 = 1;				// TX LED(P012): High出力(消灯)
//			R_PORT0->PODR_b.PODR13 = 0;				// RX LED(P013): High出力(点灯)
			R_PORT1->PORR_b.PORR11 = 1;
			R_PORT0->POSR_b.POSR12 = 1;
			R_PORT0->PORR_b.PORR13 = 1;
			break;
		}
		u8_led_state++;
		u8_led_state = u8_led_state % 3;
		/* 文字を出力する */
		uartEchoStr(".");

		/* タイマーを再開する */
		startTimer(&sts_Timer1s);
	}
}

/**
  * @brief  エラー処理ハンドラ
  * @param  None
  * @retval None
  */
void Error_Handler(void)
{
	__disable_irq();
	while (true) {
		/* SCK LED(P111)を反転出力する */
		R_PORT1->PODR_b.PODR11 = !R_PORT1->PODR_b.PODR11;
		/* 100ms待つ */
		LL_mDelay(100);
	}
}

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  PORT_IRQ0 初期化処理
  * @param  None
  * @retval None
  */
static void port_irq0_init(void)
{
	/* ---- ベクターテーブル登録 ---- */
	__disable_irq();
	NVIC_SetVector((IRQn_Type)IRQ_PORT_IRQ0, (uint32_t)PORT_IRQ0_Handler);
	__enable_irq();

	/* ---- PORT_IRQ0 無効 ---- */
	R_ICU->IELSR[IRQ_PORT_IRQ0] = 0x00000000;
	R_ICU->IRQCR[0] = 0x00;

	/* ---- ポート設定 ---- */
	// 書き込みプロテクト解除
	R_BSP_PinAccessEnable();
	// P105 = IRQ0
	R_PFS->PORT[1].PIN[5].PmnPFS_b.PMR = 0;			// 汎用入出力端子
	R_PFS->PORT[1].PIN[5].PmnPFS_b.PCR = 1;			// 入力プルアップ有効
	R_PFS->PORT[1].PIN[5].PmnPFS_b.ISEL = 1;		// IRQ0入力端子
	// 書き込みプロテクト施錠
	R_BSP_PinAccessDisable();

	/* ---- PORT_IRQ0 設定 ---- */
	R_ICU->IRQCR_b[0].IRQMD = 0;					// 立ち下がりエッジ
	R_ICU->IRQCR_b[0].FCLKSEL = 3;					// PCLKB/64
	R_ICU->IRQCR_b[0].FLTEN = 1;					// デジタルフィルタ有効

	/* ---- ICU → NVIC 割り込み割り当て ---- */
	R_ICU->IELSR_b[IRQ_PORT_IRQ0].IR = 0;			// 割り込み要求フラグ クリア
	R_ICU->IELSR_b[IRQ_PORT_IRQ0].IELS = 0x01;		// PORT_IRQ0

	/* ---- NVIC 設定 ---- */
	NVIC_ClearPendingIRQ((IRQn_Type)IRQ_PORT_IRQ0);
	NVIC_SetPriority((IRQn_Type)IRQ_PORT_IRQ0, 12);	// 優先度 12
	NVIC_EnableIRQ((IRQn_Type)IRQ_PORT_IRQ0);
}

