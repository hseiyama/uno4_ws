#include <Arduino.h>

/* IRQ番号の割り当て */
// IRQManager との競合を避けるため、
// 末尾(31)から割り当てるルールとする。
#define IRQ_PORT_IRQ0		(31)					// 外部端子(IRQ0)割り込み
#define IRQ_SCI1_RXI		(30)					// UART受信(SCI1)割り込み
//#define IRQ_SCI1_TXI		(29)					// UART送信(SCI1)割り込み

/* システムタイマー用カウンタ */
volatile uint32_t u32s_SystemTimeCounter = 0;

//volatile uint8_t u8s_TxData = 0x00;

static void sci1_putc(char c);						// 1文字送信
static void sci1_puts(const char *s);				// 文字列送信
static char sci1_getc(void);						// 1文字受信（ブロッキング）

/* SCI1_RXI 割り込みハンドラ */
void SCI1_RXI_Handler(void)
{
    /* 割り込み要求フラグ クリア */
	R_ICU->IELSR_b[IRQ_SCI1_RXI].IR = 0;

	// 1文字送信
	sci1_putc(R_SCI1->RDR);
}

/* SCI1_TXI 割り込みハンドラ */
//void SCI1_TXI_Handler(void)
//{
//    /* 割り込み要求フラグ クリア */
//	R_ICU->IELSR_b[IRQ_SCI1_TXI].IR = 0;
//
//	// 1文字送信
//	R_SCI1->TDR = u8s_TxData;
//	R_SCI1->SCR_b.TIE = 0;							// TIE=0
//}

/*
 * SCI1 UART 初期化
 * 9600bps / 8bit / No parity / 1stop
 */
void sci1_init(void)
{
	/* ---- ベクターテーブル登録 ---- */
	__disable_irq();
	NVIC_SetVector((IRQn_Type)IRQ_SCI1_RXI, (uint32_t)SCI1_RXI_Handler);
//	NVIC_SetVector((IRQn_Type)IRQ_SCI1_TXI, (uint32_t)SCI1_TXI_Handler);
	__enable_irq();

	/* ---- SCI1_RXI 無効 ---- */
	R_ICU->IELSR[IRQ_SCI1_RXI] = 0x00000000;
	/* ---- SCI1_TXI 無効 ---- */
//	R_ICU->IELSR[IRQ_SCI1_TXI] = 0x00000000;

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
//	R_PMISC->PWPR_b.B0WI = 0;
//	R_PMISC->PWPR_b.PFSWE = 1;
	R_BSP_PinAccessEnable();
	// P501 = TXD1, P502 = RXD1
	R_PFS->PORT[5].PIN[1].PmnPFS_b.PSEL = 0b00101;	// SCI1 TX
	R_PFS->PORT[5].PIN[2].PmnPFS_b.PSEL = 0b00101;	// SCI1 RX
	R_PFS->PORT[5].PIN[1].PmnPFS_b.PMR = 1;
	R_PFS->PORT[5].PIN[2].PmnPFS_b.PMR = 1;
	// 書き込みプロテクト施錠
//	R_PMISC->PWPR_b.PFSWE = 0;
//	R_PMISC->PWPR_b.B0WI = 1;
	R_BSP_PinAccessDisable();

	/* ---- 送受信有効 ---- */
	R_SCI1->SCR = 0x70;								// RIE=1, TE=1, RE=1

	/* ---- ICU → NVIC 割り込み割り当て (SCI1_RXI) ---- */
	R_ICU->IELSR_b[IRQ_SCI1_RXI].IR = 0;			// 割り込み要求フラグ クリア
	R_ICU->IELSR_b[IRQ_SCI1_RXI].IELS = 0x9E;		// SCI1_RXI
	/* ---- ICU → NVIC 割り込み割り当て (SCI1_TXI) ---- */
//	R_ICU->IELSR_b[IRQ_SCI1_TXI].IR = 0;			// 割り込み要求フラグ クリア
//	R_ICU->IELSR_b[IRQ_SCI1_TXI].IELS = 0x9F;		// SCI1_TXI

	/* ---- NVIC 設定 (SCI1_RXI) ---- */
	NVIC_ClearPendingIRQ((IRQn_Type)IRQ_SCI1_RXI);
	NVIC_SetPriority((IRQn_Type)IRQ_SCI1_RXI, 11);	// 優先度 11
	NVIC_EnableIRQ((IRQn_Type)IRQ_SCI1_RXI);
	/* ---- NVIC 設定 (SCI1_TXI) ---- */
//	NVIC_ClearPendingIRQ((IRQn_Type)IRQ_SCI1_TXI);
//	NVIC_SetPriority((IRQn_Type)IRQ_SCI1_TXI, 11);	// 優先度 11
//	NVIC_EnableIRQ((IRQn_Type)IRQ_SCI1_TXI);
}

/* 1文字送信 */
static void sci1_putc(char c)
{
	while (!R_SCI1->SSR_b.TDRE);
	R_SCI1->TDR = c;
//	u8s_TxData = c;
//	R_SCI1->SCR_b.TIE = 1;							// TIE=1
}

/* 文字列送信 */
static void sci1_puts(const char *s)
{
	while (*s) {
	    sci1_putc(*s++);
	}
}

/* 1文字受信（ブロッキング） */
static char sci1_getc(void)
{
	while (!R_SCI1->SSR_b.RDRF);
	return R_SCI1->RDR;
}

/* PORT_IRQ0 割り込みハンドラ */
void PORT_IRQ0_Handler(void)
{
    /* 割り込み要求フラグ クリア */
	R_ICU->IELSR_b[IRQ_PORT_IRQ0].IR = 0;

	// 1文字送信
	sci1_putc('e');
}

/* PORT_IRQ0 初期化 */
void port_irq0_init(void)
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

/* SysTick 割り込みハンドラ */
void SysTick_Handler(void)
{
	u32s_SystemTimeCounter++;

	// 割り込みによる1秒周期の確認
	if (u32s_SystemTimeCounter >= 1000) {
		u32s_SystemTimeCounter = 0;

		// 1文字送信
		sci1_putc('t');
	}
}

/* System Timer 初期化 */
void sys_timer_init(void)
{
	/* ---- ベクターテーブル登録 ---- */
	__disable_irq();
	NVIC_SetVector(SysTick_IRQn, (uint32_t)SysTick_Handler);
	__enable_irq();

	/* ---- System Tick Configuration ---- */
	// MPUクロック=48MHz → 1tick=1ms に設定
	SysTick_Config(64000000 / 1000);
}

void setup() {
	// 各ポートの方向設定
	R_PORT1->PDR_b.PDR11 = 1;						// SCK LED(P111): 出力
	R_PORT0->PDR_b.PDR12 = 1;						// TX LED(P012): 出力
	R_PORT0->PDR_b.PDR13 = 1;						// RX LED(P013): 出力

	// SCI1 UART 初期化
	sci1_init();

	// PORT_IRQ0 初期化
	port_irq0_init();

	// System Timer 初期化
	sys_timer_init();
}

void loop() {
	// 各ポートの出力データ設定(1)
	R_PORT1->PODR_b.PODR11 = 1;						// SCK LED(P111): High出力(点灯)
	R_PORT0->PODR_b.PODR12 = 1;						// TX LED(P012): High出力(消灯)
	R_PORT0->PODR_b.PODR13 = 1;						// RX LED(P013): High出力(消灯)
	// 1文字送信
	sci1_putc('1');
	delay(1000);

	// 各ポートの出力データ設定(2)
//	R_PORT1->PODR_b.PODR11 = 0;						// SCK LED(P111): High出力(消灯)
//	R_PORT0->PODR_b.PODR12 = 0;						// TX LED(P012): High出力(点灯)
//	R_PORT0->PODR_b.PODR13 = 1;						// RX LED(P013): High出力(消灯)
	R_PORT1->PORR = 0x0800;
	R_PORT0->PORR = 0x1000;
	R_PORT0->POSR = 0x2000;
	// 1文字送信
	sci1_putc('2');
	delay(1000);

	// 各ポートの出力データ設定(3)
//	R_PORT1->PODR_b.PODR11 = 0;						// SCK LED(P111): High出力(消灯)
//	R_PORT0->PODR_b.PODR12 = 1;						// TX LED(P012): High出力(消灯)
//	R_PORT0->PODR_b.PODR13 = 0;						// RX LED(P013): High出力(点灯)
	R_PORT1->PORR_b.PORR11 = 1;
	R_PORT0->POSR_b.POSR12 = 1;
	R_PORT0->PORR_b.PORR13 = 1;
	// 1文字送信
	sci1_putc('3');
	delay(1000);

	// 1文字送信
	sci1_putc('.');
}
