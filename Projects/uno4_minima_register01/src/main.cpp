#include <Arduino.h>

void setup() {
	// 各ポートの方向設定
	R_PORT1->PDR_b.PDR11 = 1;		// SCK LED(P111): 出力
	R_PORT0->PDR_b.PDR12 = 1;		// TX LED(P012): 出力
	R_PORT0->PDR_b.PDR13 = 1;		// RX LED(P013): 出力
}

void loop() {
	// 各ポートの出力データ設定(1)
	R_PORT1->PODR_b.PODR11 = 1;		// SCK LED(P111): High出力(点灯)
	R_PORT0->PODR_b.PODR12 = 1;		// TX LED(P012): High出力(消灯)
	R_PORT0->PODR_b.PODR13 = 1;		// RX LED(P013): High出力(消灯)
	delay(1000);
	// 各ポートの出力データ設定(2)
	R_PORT1->PODR_b.PODR11 = 0;		// SCK LED(P111): High出力(消灯)
	R_PORT0->PODR_b.PODR12 = 0;		// TX LED(P012): High出力(点灯)
	R_PORT0->PODR_b.PODR13 = 1;		// RX LED(P013): High出力(消灯)
	delay(1000);
	// 各ポートの出力データ設定(3)
	R_PORT1->PODR_b.PODR11 = 0;		// SCK LED(P111): High出力(消灯)
	R_PORT0->PODR_b.PODR12 = 1;		// TX LED(P012): High出力(消灯)
	R_PORT0->PODR_b.PODR13 = 0;		// RX LED(P013): High出力(点灯)
	delay(1000);
}
