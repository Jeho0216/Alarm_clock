/*
 * BUTTON.h
 *
 * Created: 2019-04-10 오후 12:27:04
 *  Author: kccistc
 */ 
#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>

#define SW_PIN PIND
#define SW1 0
#define SW2 1
#define SW3 2

#ifndef BUTTON_H_
#define BUTTON_H_

uint8_t get_button_1_state(void);
uint8_t get_button_2_state(void);

uint8_t button_process(uint8_t *curr, uint8_t *prev, int sw_num);

#endif /* BUTTON_H_ */