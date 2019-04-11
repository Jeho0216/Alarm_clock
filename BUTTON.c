#include "BUTTON.h"

uint8_t get_button_1_state(void){
	static uint8_t state_current = 1;
	static uint8_t state_previous = 1;
	
	return button_process(&state_current, &state_previous, SW1);
}

uint8_t get_button_2_state(void){
	static uint8_t state_current = 1;
	static uint8_t state_previous = 1;
	
	return button_process(&state_current, &state_previous, SW2);
}

uint8_t button_process(uint8_t *curr, uint8_t *prev, int sw_num){
	*curr = (SW_PIN & (0x01 << sw_num)) >> sw_num;
	_delay_ms(10);
	
	if(*curr != *prev){
		*prev = *curr;
		if(*curr == 0)
		return 1;
	}
	return 0;
}