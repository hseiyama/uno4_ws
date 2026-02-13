#include "bsp_api.h"

/* default EMPTY interrupt table - it in FLASH at the beginning of the space allowed for the sketch 
   The bootloader is responsible to copy it in RAM and setting the Interrupt vector table to that */
BSP_DONT_REMOVE const fsp_vector_t g_vector_table[BSP_ICU_VECTOR_MAX_ENTRIES] BSP_PLACE_IN_SECTION(BSP_SECTION_APPLICATION_VECTORS) = {NULL};
const bsp_interrupt_event_t g_interrupt_event_link_select[BSP_ICU_VECTOR_MAX_ENTRIES] = {0};

