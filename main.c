/*
* Alarm_clock.c
*
* Created: 2019-04-10 오후 4:22:03
* Author : 이제호
*/
#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include "UART0.h"
#include "CLCD.h"
#include "BUTTON.h"

FILE OUTPUT = FDEV_SETUP_STREAM(UART0_transmit, NULL, _FDEV_SETUP_WRITE);
FILE INPUT = FDEV_SETUP_STREAM(NULL, UART0_receive, _FDEV_SETUP_READ);

typedef struct alarm{
	uint8_t hh;
	uint8_t mm;
	uint8_t ss;	
}alarm;

volatile int count = 0;		//타이머/카운터 인터럽트 발생 횟수 카운트.
volatile uint8_t hh = 12, mm = 0, ss = 0;		//시계의 시, 분, 초 변수
volatile uint8_t alarm_flag = 0, alarm_set_flag = 0;		//알람 발생여부 결정, 알람 설정 여부 결정. 0 : OFF, 1 : ON
volatile uint8_t position_cur = 1;
volatile alarm alarm_1;

void PORT_init(void);
void INT_init(void);
void print_time(uint8_t alarm_hh, uint8_t alarm_mm, uint8_t alarm_ss);
void print_alarm(uint8_t alarm_hh, uint8_t alarm_mm, uint8_t alarm_ss);
void check_alarm(uint8_t alarm_hh, uint8_t alarm_mm, uint8_t alarm_ss);
void alarm_set(void);

ISR(TIMER0_COMP_vect){
	count++;
	if(count >= 1000){
		ss++;
		if(ss > 60){
			ss = 0;
			mm++;
			if(mm > 60){
				mm = 0;
				hh++;
				if(hh > 24)
					hh = 0;
			}
		}
		count = 0;
	}
	if((alarm_flag == 1) && ((count % 250) == 0)){
		PORTD ^= 0xF0;
	}
}

ISR(INT1_vect){
	if(alarm_set_flag == 1){		//알람 화면일 경우, 커서 위치 결정.
		printf("move time\n");
		position_cur += 3;
		if(position_cur > 7)
			position_cur = 1;
	}
}

ISR(INT2_vect){
	if(alarm_flag == 1){
		PORTD &= ~(0xF0);
		alarm_flag = 0;
	}
	else{
		if(alarm_set_flag == 1){
			eeprom_update_block((void *)&alarm_1, (void *)0, sizeof(alarm));		//알람화면에서 3번 버튼을 누르면, 현재 알람시간이 저장됨.
		}
		alarm_set_flag ^= 0x01;
	}
}

int main(void){
	
	int addressAlarm = 0;
	
	stdout = &OUTPUT;
	stdin = &INPUT;
	
	PORT_init();
	INT_init();
	UART0_init();
	LCD_init();
	
	eeprom_read_block((void *)&alarm_1, (void *)addressAlarm, sizeof(alarm));		//프로그램 시작할 때, EEPROM에 저장된 알람 값을 읽어옴.
	
	if((alarm_1.hh == 255) | (alarm_1.mm == 255) | (alarm_1.ss == 255)){		//초기값은 255이므로 이를 처리해줌.
		alarm_1.hh = 0;
		alarm_1.mm = 0;
		alarm_1.ss = 0;
	}
	
	while (1){
		//알람, 시간 선택 출력부분.
		if(alarm_set_flag == 1){
			LCD_clear();
			print_alarm(alarm_1.hh, alarm_1.mm, alarm_1.ss);		//알람 시간 출력.
			LCD_write_command(0x0F);
			while(alarm_set_flag == 1){
				LCD_goto_XY(1, position_cur);
				alarm_set();		//커서 위치에 따라 알람 값 증가를 위한 함수.
			}
			LCD_clear();			//화면 전환시, clear수행.
		}
		else{
			print_time(alarm_1.hh, alarm_1.mm, alarm_1.ss);
			if(alarm_flag != 1){
				PORTD = 0x00;
				check_alarm(alarm_1.hh, alarm_1.mm, alarm_1.ss);		//알람시간 체크.
			}
		}
		//알람 시간 체크부분.
	}
}

void PORT_init(void){
	DDRD |= 0xF0;
	PORTD &= ~(0xF0);
}

void INT_init(void){
	TCCR0 = (1 << CS02) | (0 << CS01) | (0 << CS00);		//분주비 64
	OCR0 = 250;												//0.001초마다 비교일치 인터럽트 발생.
	TIMSK |= (1 << OCIE0);
	
	EIMSK = (1 << INT1) | (1 << INT2);		//버튼인터럽트 허용
	EICRA = (1 << ISC11) |(1 << ISC21);		//하강엣지에서 인터럽트 발생.
	
	sei();		//인터럽트 허용
}

void print_time(uint8_t alarm_hh, uint8_t alarm_mm, uint8_t alarm_ss){		//시간 화면 출력
	char buff[20];
	LCD_goto_XY(0, 0);
	LCD_write_string("TIME  - ");
	LCD_goto_XY(0, 8);
	sprintf(buff, "%02d:%02d:%02d", hh, mm, ss);
	LCD_write_string(buff);
	LCD_goto_XY(1, 0);
	LCD_write_string("ALRAM - ");
	sprintf(buff, "%02d:%02d:%02d", alarm_hh, alarm_mm, alarm_ss);
	LCD_write_string(buff);
	
}

void print_alarm(uint8_t alarm_hh, uint8_t alarm_mm, uint8_t alarm_ss){		//알람 화면 출력
	char buff[20];
	LCD_goto_XY(0, 0);
	LCD_write_string("ALARM SETTING");
	LCD_goto_XY(1, 0);
	sprintf(buff, "%02d:%02d:%02d", alarm_hh, alarm_mm, alarm_ss);
	LCD_goto_XY(1, 0);
	LCD_write_string(buff);
}

void check_alarm(uint8_t alarm_hh, uint8_t alarm_mm, uint8_t alarm_ss){
	if((alarm_hh == hh) && (alarm_mm == mm) && (alarm_ss == ss)){		//알람시간과 현재시간이 일치할 경우,
		printf("alarm! alarm! alarm!\n");
		alarm_flag = 1;
	}
}

void alarm_set(){
	if(get_button_1_state()){
		if(alarm_set_flag == 1){
			printf("incresing time\n");
			switch(position_cur){		//커서위치에 따라 변경할 값을 결정.
				case 1:
					alarm_1.hh++;
					if(alarm_1.hh > 12)
						alarm_1.hh = 0;
				break;
				case 4:
					alarm_1.mm++;
					if(alarm_1.mm > 60)
						alarm_1.mm = 0;
				break;
				case 7:
					alarm_1.ss++;
					if(alarm_1.ss > 60)
						alarm_1.ss = 0;
				break;
			}
		}
		print_alarm(alarm_1.hh, alarm_1.mm, alarm_1.ss);
	}
}