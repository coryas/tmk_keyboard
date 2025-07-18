/*
 * led_control.h
 * HHKB LED 제어 매크로
 */

#ifndef LED_CONTROL_H
#define LED_CONTROL_H

#include <avr/io.h>

/* LED 제어 매크로 - HHKB에서는 PD6 사용 */
#define LED_ON()     do { DDRD |= (1<<6); PORTD |= (1<<6); } while(0)
#define LED_OFF()    do { DDRD |= (1<<6); PORTD &= ~(1<<6); } while(0)
#define LED_TOGGLE() do { DDRD |= (1<<6); PORTD ^= (1<<6); } while(0)

#endif /* LED_CONTROL_H */