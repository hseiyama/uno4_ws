/**
  ******************************************************************************
  * @file           : drv_uart.c
  * @brief          : UARTドライバー
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "drv.h"
#include "lib.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
#define TX_QUEUE_SIZE		(128)			/* UART送信Queueサイズ			*/
#define RX_QUEUE_SIZE		(128)			/* UART受信Queueサイズ			*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
volatile static uint8_t u8s_UartTxBuffer[TX_QUEUE_SIZE];	/* UART送信Queueデータ			*/
volatile static uint8_t u8s_UartRxBuffer[RX_QUEUE_SIZE];	/* UART受信Queueデータ			*/
volatile static QueueControl sts_UartTxQueue;				/* UART送信Queue情報			*/
volatile static QueueControl sts_UartRxQueue;				/* UART受信Queue情報			*/

/* Private function prototypes -----------------------------------------------*/
static uint8_t setUartTxQueue(const uint8_t u8_Data);		/* UART送信Queueに登録する				*/
static uint8_t getUartTxQueue(uint8_t *pu8_Data);			/* UART送信Queueから取得する			*/
static uint8_t setUartRxQueue(const uint8_t u8_Data);		/* UART受信Queueに登録する				*/
static uint8_t getUartRxQueue(uint8_t *pu8_Data);			/* UART受信Queueから取得する			*/

/* Exported functions --------------------------------------------------------*/

/**
  * @brief  SCI1受信データフル割り込みハンドラ
  * @param  None
  * @retval None
  */
void SCI1_RXI_Handler(void)
{
	/* 割り込み要求フラグ クリア */
	R_ICU->IELSR_b[IRQ_SCI1_RXI].IR = 0;

	/* UART受信Queueに登録する */
	setUartRxQueue(R_SCI1->RDR);
}

/**
  * @brief  SCI1送信データエンプティ割り込みハンドラ
  * @param  None
  * @retval None
  */
void SCI1_TXI_Handler(void)
{
	uint8_t u8_TxData = 0;

	/* 割り込み要求フラグ クリア */
	R_ICU->IELSR_b[IRQ_SCI1_TXI].IR = 0;

	/* UART送信Queueから取得する */
	if (getUartTxQueue(&u8_TxData)) {
		R_SCI1->TDR = u8_TxData;
	}
}

/**
  * @brief  SCI1受信エラー割り込みハンドラ
  * @param  None
  * @retval None
  */
void SCI1_ERI_Handler(void)
{
	/* 割り込み要求フラグ クリア */
	R_ICU->IELSR_b[IRQ_SCI1_ERI].IR = 0;

	/* 文字を出力する */
	uartEchoStr("<SCI1_ERI>");
}

/**
  * @brief  UARTドライバー初期化処理
  * @param  None
  * @retval None
  */
void taskUartDriverInit(void)
{
	mem_set08((uint8_t *)&u8s_UartTxBuffer[0], 0x00, TX_QUEUE_SIZE);
	mem_set08((uint8_t *)&u8s_UartRxBuffer[0], 0x00, RX_QUEUE_SIZE);
	mem_set08((uint8_t *)&sts_UartTxQueue, 0x00, sizeof(sts_UartTxQueue));
	mem_set08((uint8_t *)&sts_UartRxQueue, 0x00, sizeof(sts_UartRxQueue));

	/* ---- ベクターテーブル登録 ---- */
	__disable_irq();
	NVIC_SetVector((IRQn_Type)IRQ_SCI1_RXI, (uint32_t)SCI1_RXI_Handler);
	NVIC_SetVector((IRQn_Type)IRQ_SCI1_TXI, (uint32_t)SCI1_TXI_Handler);
	NVIC_SetVector((IRQn_Type)IRQ_SCI1_ERI, (uint32_t)SCI1_ERI_Handler);
	__enable_irq();

	/* ---- SCI1_RXI 無効 ---- */
	R_ICU->IELSR[IRQ_SCI1_RXI] = 0x00000000;
	/* ---- SCI1_TXI 無効 ---- */
	R_ICU->IELSR[IRQ_SCI1_TXI] = 0x00000000;
	/* ---- SCI1_ERI 無効 ---- */
	R_ICU->IELSR[IRQ_SCI1_ERI] = 0x00000000;

	/* ---- SCI1 モジュールストップ解除 ---- */
	R_MSTP->MSTPCRB_b.MSTPB30 = 0;					// SCI1 ON

	/* ---- SCI 停止 ---- */
	R_SCI1->SCR = 0x00;

	/* ---- 通信条件設定 ---- */
	R_SCI1->SMR = 0x00;								// 8bit, no parity, 1 stop
	R_SCI1->SCMR = 0xF2;							// 通常モード

	/* ---- ボーレート設定 ---- */
	// PCLKA = 48MHz
	// BBR = 48MHz / (64 * 2^(-1) * 9600bps) - 1 = 155.25
	// 前提条件1 [SMR.CKS=00b (n=0)]
	// 前提条件2 [SEMR.ABCS=0b, SEMR.ABCSE=0b, SEMR.BGDM=0b]
	// 9600bps → BRR = 155
	R_SCI1->BRR = 155;

	/* ---- ポート設定 ---- */
	// 書き込みプロテクト解除
	R_BSP_PinAccessEnable();
	// P501 = TXD1, P502 = RXD1
	R_PFS->PORT[5].PIN[1].PmnPFS_b.PSEL = 0b00101;	// SCI1 TX
	R_PFS->PORT[5].PIN[2].PmnPFS_b.PSEL = 0b00101;	// SCI1 RX
	R_PFS->PORT[5].PIN[1].PmnPFS_b.PMR = 1;
	R_PFS->PORT[5].PIN[2].PmnPFS_b.PMR = 1;
	// 書き込みプロテクト施錠
	R_BSP_PinAccessDisable();

	/* ---- 送受信有効 ---- */
	R_SCI1->SCR = 0xF0;								// TIE=1, RIE=1, TE=1, RE=1

	/* ---- ICU → NVIC 割り込み割り当て (SCI1_RXI) ---- */
	R_ICU->IELSR_b[IRQ_SCI1_RXI].IR = 0;			// 割り込み要求フラグ クリア
	R_ICU->IELSR_b[IRQ_SCI1_RXI].IELS = 0x9E;		// SCI1_RXI
	/* ---- ICU → NVIC 割り込み割り当て (SCI1_TXI) ---- */
	R_ICU->IELSR_b[IRQ_SCI1_TXI].IR = 0;			// 割り込み要求フラグ クリア
	R_ICU->IELSR_b[IRQ_SCI1_TXI].IELS = 0x9F;		// SCI1_TXI
	/* ---- ICU → NVIC 割り込み割り当て (SCI1_ERI) ---- */
	R_ICU->IELSR_b[IRQ_SCI1_ERI].IR = 0;			// 割り込み要求フラグ クリア
	R_ICU->IELSR_b[IRQ_SCI1_ERI].IELS = 0xA1;		// SCI1_ERI

	/* ---- NVIC 設定 (SCI1_RXI) ---- */
	NVIC_ClearPendingIRQ((IRQn_Type)IRQ_SCI1_RXI);
	NVIC_SetPriority((IRQn_Type)IRQ_SCI1_RXI, 11);	// 優先度 11
	NVIC_EnableIRQ((IRQn_Type)IRQ_SCI1_RXI);
	/* ---- NVIC 設定 (SCI1_TXI) ---- */
	NVIC_ClearPendingIRQ((IRQn_Type)IRQ_SCI1_TXI);
	NVIC_SetPriority((IRQn_Type)IRQ_SCI1_TXI, 11);	// 優先度 11
	NVIC_EnableIRQ((IRQn_Type)IRQ_SCI1_TXI);
	/* ---- NVIC 設定 (SCI1_ERI) ---- */
	NVIC_ClearPendingIRQ((IRQn_Type)IRQ_SCI1_ERI);
	NVIC_SetPriority((IRQn_Type)IRQ_SCI1_ERI, 11);	// 優先度 11
	NVIC_EnableIRQ((IRQn_Type)IRQ_SCI1_ERI);
}

/**
  * @brief  UARTドライバー入力処理
  * @param  None
  * @retval None
  */
void taskUartDriverInput(void)
{
	/* 処理なし */
}

/**
  * @brief  UARTドライバー出力処理
  * @param  None
  * @retval None
  */
void taskUartDriverOutput(void)
{
	uint8_t u8_TxData = 0;

	/* UART送信Queueデータが存在し、かつSCI1送信データエンプティの場合 */
	if ((sts_UartTxQueue.u16_count > 0) && (R_SCI1->SSR_b.TDRE == 1)) {
		/* UART送信Queueから取得する */
		getUartTxQueue(&u8_TxData);
		R_SCI1->TDR = u8_TxData;
	}
}

/**
  * @brief  UART送信データを登録する
  * @param  pu8_Data: データのポインタ
  * @param  u16_Size: データのサイズ
  * @retval 登録した数
  */
uint16_t uartSetTxData(const uint8_t *pu8_Data, uint16_t u16_Size)
{
	uint16_t RetValue = 0;

	while (u16_Size > 0) {
		/* UART送信Queueに登録する */
		if (setUartTxQueue(pu8_Data[RetValue]) != OK) {
			break;
		}
		RetValue++;
		u16_Size--;
	}
	return RetValue;
}

/**
  * @brief  UART受信データを取得する
  * @param  pu8_Data: データのポインタ
  * @param  u16_Size: データのサイズ
  * @retval 取得した数
  */
uint16_t uartGetRxData(uint8_t *pu8_Data, uint16_t u16_Size)
{
	uint16_t RetValue = 0;

	while (u16_Size > 0) {
		/* UART受信Queueから取得する */
		if (getUartRxQueue(&pu8_Data[RetValue]) != OK) {
			break;
		}
		RetValue++;
		u16_Size--;
	}
	return RetValue;
}

/**
  * @brief  UART受信データの数を取得する
  * @param  None
  * @retval データの数
  */
uint16_t uartGetRxCount(void)
{
	/* UART受信Queueデータの登録数 */
	return sts_UartRxQueue.u16_count;
}

/**
  * @brief  Hex1Byte表示処理
  * @param  u8_Data: データ
  * @retval None
  */
void uartEchoHex8(uint8_t u8_Data) {
	const uint8_t HexTable[] = {
		'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
	};
	setUartTxQueue(HexTable[(u8_Data >> 4) & 0x0F]);
	setUartTxQueue(HexTable[u8_Data & 0x0F]);
}

/**
  * @brief  Hex2Byte表示処理
  * @param  u16_Data: データ
  * @retval None
  */
void uartEchoHex16(uint16_t u16_Data) {
	uartEchoHex8((u16_Data >> 8) & 0xFF);
	uartEchoHex8(u16_Data & 0xFF);
}

/**
  * @brief  Hex4Byte表示処理
  * @param  u32_Data: データ
  * @retval None
  */
void uartEchoHex32(uint32_t u32_Data) {
	uartEchoHex16((u32_Data >> 16) & 0xFFFF);
	uartEchoHex16(u32_Data & 0xFFFF);
}

/**
  * @brief  文字列表示処理
  * @param  pu8_Data: データのポインタ
  * @retval None
  */
void uartEchoStr(const char *ps8_Data) {
	while (*ps8_Data != 0x00) {
		setUartTxQueue(*ps8_Data);
		ps8_Data++;
	}
}

/**
  * @brief  文字列表示処理(改行付き)
  * @param  pu8_Data: データのポインタ
  * @retval None
  */
void uartEchoStrln(const char *ps8_Data) {
	uartEchoStr(ps8_Data);
	uartEchoStr("\r\n");
}

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  UART送信Queueに登録する
  * @param  u8_Data: データ
  * @retval OK/NG
  */
static uint8_t setUartTxQueue(const uint8_t u8_Data)
{
	uint8_t u8_RetCode = NG;

	/* 上限を超えるQueueデータの登録は破棄する */
	if (sts_UartTxQueue.u16_count < TX_QUEUE_SIZE) {
		/* Disable Interrupts */
		__disable_irq();
		u8s_UartTxBuffer[sts_UartTxQueue.u16_head] = u8_Data;
		sts_UartTxQueue.u16_head = (sts_UartTxQueue.u16_head + 1) % TX_QUEUE_SIZE;
		sts_UartTxQueue.u16_count++;
		/* Enable Interrupts */
		__enable_irq();
		u8_RetCode = OK;
	}
	return u8_RetCode;
}

/**
  * @brief  UART送信Queueから取得する
  * @param  pu8_Data: データのポインタ
  * @retval OK/NG
  */
static uint8_t getUartTxQueue(uint8_t *pu8_Data)
{
	uint8_t u8_RetCode = NG;

	/* 登録済のQueueデータが存在する場合 */
	if (sts_UartTxQueue.u16_count > 0) {
		/* Disable Interrupts */
		__disable_irq();
		*pu8_Data = u8s_UartTxBuffer[sts_UartTxQueue.u16_tail];
		sts_UartTxQueue.u16_tail = (sts_UartTxQueue.u16_tail + 1) % TX_QUEUE_SIZE;
		sts_UartTxQueue.u16_count--;
		/* Enable Interrupts */
		__enable_irq();
		u8_RetCode = OK;
	}
	return u8_RetCode;
}

/**
  * @brief  UART受信Queueに登録する
  * @param  u8_Data: データ
  * @retval OK/NG
  */
static uint8_t setUartRxQueue(const uint8_t u8_Data)
{
	/* Disable Interrupts */
	__disable_irq();
	/* 上限を超えるQueueデータの登録は上書きする */
	u8s_UartRxBuffer[sts_UartRxQueue.u16_head] = u8_Data;
	sts_UartRxQueue.u16_head = (sts_UartRxQueue.u16_head + 1) % RX_QUEUE_SIZE;
	sts_UartRxQueue.u16_count++;
	/* Queueデータの上書きが起きる場合 */
	if (sts_UartRxQueue.u16_count > RX_QUEUE_SIZE) {
		sts_UartRxQueue.u16_count = RX_QUEUE_SIZE;
		sts_UartRxQueue.u16_tail = (sts_UartRxQueue.u16_tail + 1) % RX_QUEUE_SIZE;
	}
	/* Enable Interrupts */
	__enable_irq();

	return OK;
}

/**
  * @brief  UART受信Queueから取得する
  * @param  pu8_Data: データのポインタ
  * @retval OK/NG
  */
static uint8_t getUartRxQueue(uint8_t *pu8_Data)
{
	uint8_t u8_RetCode = NG;

	/* 登録済のQueueデータが存在する場合 */
	if (sts_UartRxQueue.u16_count > 0) {
		/* Disable Interrupts */
		__disable_irq();
		*pu8_Data = u8s_UartRxBuffer[sts_UartRxQueue.u16_tail];
		sts_UartRxQueue.u16_tail = (sts_UartRxQueue.u16_tail + 1) % RX_QUEUE_SIZE;
		sts_UartRxQueue.u16_count--;
		/* Enable Interrupts */
		__enable_irq();
		u8_RetCode = OK;
	}
	return u8_RetCode;
}
