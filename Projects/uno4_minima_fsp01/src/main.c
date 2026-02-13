/* Standard C library includes */
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include "bsp_api.h"

void setup(void);
void loop(void);


/* TODO: make me configurable by the variant */
extern const uint32_t __StackTop;
const uint32_t APPLICATION_VECTOR_TABLE_ADDRESS_RAM = (uint32_t)&__StackTop;

volatile uint32_t *irq_vector_table;

extern const fsp_vector_t __VECTOR_TABLE[];
extern const fsp_vector_t g_vector_table[];


void arduino_main(void)
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

	setup();
	while (1)
	{
		loop();
	}
}

void hal_entry(void) {
	arduino_main();
}

