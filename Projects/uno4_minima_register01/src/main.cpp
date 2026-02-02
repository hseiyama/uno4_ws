#include <Arduino.h>

#define FIXED_IRQ_NUM		(16)					// Arm Cortex-M4 コア用の固定IRQ数
/* IRQ番号の割り当て */
// IRQManager との競合を避けるため、
// 末尾(31)から割り当てるルールとする。
#define IRQ_PORT_IRQ0		(31)					// 外部端子割り込み(0)

/*
 * SCI1 UART 初期化
 * 9600bps / 8bit / No parity / 1stop
 */
void sci1_init(void)
{
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
	R_SCI1->SCR = 0x30;								// TE=1, RE=1
}

/* 1文字送信 */
void sci1_putc(char c)
{
	while (!R_SCI1->SSR_b.TDRE);
	R_SCI1->TDR = c;
}

/* 文字列送信 */
void sci1_puts(const char *s)
{
	while (*s) {
	    sci1_putc(*s++);
	}
}

/* 1文字受信（ブロッキング） */
char sci1_getc(void)
{
	while (!R_SCI1->SSR_b.RDRF);
	return R_SCI1->RDR;
}

#if 0
/* 外部割込み処理 */
static void sw_push(void)
{
	// 1文字送信
	sci1_putc('e');
}
#endif

/* IRQ0 割り込みハンドラ */
void IRQ0_Handler(void)
{
    /* 割り込み要求フラグ クリア */
	R_ICU->IELSR_b[IRQ_PORT_IRQ0].IR = 0;

	// 1文字送信
	sci1_putc('e');
}

/* IRQ0 初期化 */
void irq0_init(void)
{
	/* ---- ベクターテーブル登録 ---- */
	volatile uint32_t *irq_ptr = (volatile uint32_t *)SCB->VTOR;
	irq_ptr += FIXED_IRQ_NUM;
	__disable_irq();
	*(irq_ptr + IRQ_PORT_IRQ0) = (uint32_t)IRQ0_Handler;
	__enable_irq();

	/* ---- IRQ0 無効 ---- */
	R_ICU->IELSR[IRQ_PORT_IRQ0] = 0x00000000;
	R_ICU->IRQCR[IRQ_PORT_IRQ0] = 0x00;

	/* ---- ポート設定 ---- */
	// 書き込みプロテクト解除
	R_BSP_PinAccessEnable();
	// P105 = IRQ0
	R_PFS->PORT[1].PIN[5].PmnPFS_b.PMR = 0;			// 汎用入出力端子
	R_PFS->PORT[1].PIN[5].PmnPFS_b.PCR = 1;			// 入力プルアップ有効
	R_PFS->PORT[1].PIN[5].PmnPFS_b.ISEL = 1;		// IRQ0入力端子
	// 書き込みプロテクト施錠
	R_BSP_PinAccessDisable();

	/* ---- IRQ0 設定 ---- */
	R_ICU->IRQCR_b[IRQ_PORT_IRQ0].IRQMD = 0;		// 立ち下がりエッジ
	R_ICU->IRQCR_b[IRQ_PORT_IRQ0].FCLKSEL = 3;		// PCLKB/64
	R_ICU->IRQCR_b[IRQ_PORT_IRQ0].FLTEN = 1;		// デジタルフィルタ有効

	/* ---- ICU → NVIC 割り込み割り当て ---- */
	R_ICU->IELSR_b[IRQ_PORT_IRQ0].IR = 0;			// 割り込み要求フラグ クリア
	R_ICU->IELSR_b[IRQ_PORT_IRQ0].IELS = 0x01;		// PORT_IRQ0

	/* ---- NVIC 設定 ---- */
	NVIC_ClearPendingIRQ((IRQn_Type)IRQ_PORT_IRQ0);
	NVIC_SetPriority((IRQn_Type)IRQ_PORT_IRQ0, 12);
	NVIC_EnableIRQ((IRQn_Type)IRQ_PORT_IRQ0);
}

void setup() {
	// 各ポートの方向設定
	R_PORT1->PDR_b.PDR11 = 1;		// SCK LED(P111): 出力
	R_PORT0->PDR_b.PDR12 = 1;		// TX LED(P012): 出力
	R_PORT0->PDR_b.PDR13 = 1;		// RX LED(P013): 出力

	// SCI1 UART 初期化
	sci1_init();

#if 0
	// 外部割込みの準備
	pinMode(2, INPUT_PULLUP);
	attachInterrupt(2, sw_push, FALLING);
#endif
	// IRQ0 初期化
	irq0_init();
}

void loop() {
	// 各ポートの出力データ設定(1)
	R_PORT1->PODR_b.PODR11 = 1;		// SCK LED(P111): High出力(点灯)
	R_PORT0->PODR_b.PODR12 = 1;		// TX LED(P012): High出力(消灯)
	R_PORT0->PODR_b.PODR13 = 1;		// RX LED(P013): High出力(消灯)
	// 1文字送信
	sci1_putc('1');
	delay(1000);

	// 各ポートの出力データ設定(2)
//	R_PORT1->PODR_b.PODR11 = 0;		// SCK LED(P111): High出力(消灯)
//	R_PORT0->PODR_b.PODR12 = 0;		// TX LED(P012): High出力(点灯)
//	R_PORT0->PODR_b.PODR13 = 1;		// RX LED(P013): High出力(消灯)
	R_PORT1->PORR = 0x0800;
	R_PORT0->PORR = 0x1000;
	R_PORT0->POSR = 0x2000;
	// 1文字送信
	sci1_putc('2');
	delay(1000);

	// 各ポートの出力データ設定(3)
//	R_PORT1->PODR_b.PODR11 = 0;		// SCK LED(P111): High出力(消灯)
//	R_PORT0->PODR_b.PODR12 = 1;		// TX LED(P012): High出力(消灯)
//	R_PORT0->PODR_b.PODR13 = 0;		// RX LED(P013): High出力(点灯)
	R_PORT1->PORR_b.PORR11 = 1;
	R_PORT0->POSR_b.POSR12 = 1;
	R_PORT0->PORR_b.PORR13 = 1;
	// 1文字送信
	sci1_putc('3');
	delay(1000);

	// 1文字送信
	sci1_putc('.');
}
