/*
 * wdt_power.c
 * WDT 기반 Power-down 모드 구현
 * 
 * Phase 2: Watchdog Timer를 사용한 초저전력 Deep Sleep 구현
 */

#include "wdt_power.h"
#include <avr/wdt.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/interrupt.h>
#include "print.h"
#include "matrix.h"
#include "wait.h"
#include "suspend.h"

/* 전역 상태 변수 */
static volatile wdt_power_state_t wdt_state = WDT_POWER_ACTIVE;
static volatile uint8_t wdt_counter = 0;
static volatile bool check_enter_key = false;

/* WDT 인터럽트는 suspend.c에서 이미 사용 중이므로 별도 ISR 없이 플래그만 관리 */
/* 대신 suspend_power_down()을 사용하여 Power-down 모드 진입 */

/* WDT 전력 관리 초기화 */
void wdt_power_init(void) {
    /* WDT 비활성화 */
    cli();
    wdt_reset();
    MCUSR &= ~(1<<WDRF);
    WDTCSR |= (1<<WDCE) | (1<<WDE);
    WDTCSR = 0x00;
    sei();
    
    print("WDT Power Manager initialized\n");
}

/* Power-down 모드 진입 */
void wdt_power_down_enter(void) {
    if (wdt_state != WDT_POWER_ACTIVE) return;
    
    print("Entering WDT Power-down mode\n");
    
    /* 상태 변경 */
    wdt_state = WDT_POWER_DOWN;
    wdt_counter = 0;
    check_enter_key = false;
    
    print("WDT Power-down ready\n");
}

/* Power-down 모드 해제 */
void wdt_power_down_exit(void) {
    if (wdt_state != WDT_POWER_DOWN) return;
    
    print("Exiting WDT Power-down mode\n");
    
    /* 상태 변경 */
    wdt_state = WDT_POWER_ACTIVE;
    check_enter_key = false;
    
    print("WDT Power-down exit complete\n");
}

/* WDT 인터럽트 핸들러에서 호출할 함수 */
void wdt_power_handle_interrupt(void) {
    /* main.c에서 호출하여 Power-down 모드 진입 */
    if (wdt_state == WDT_POWER_DOWN) {
        /* suspend_power_down()을 사용하여 Power-down 모드 진입 */
        /* 이 함수는 WDTO_15MS를 사용하여 15ms마다 Wake-up */
        suspend_power_down();
        
        /* Wake-up 후 카운터 증가 */
        wdt_counter++;
        
        /* 약 100ms마다 Enter 키 체크 (15ms * 7 = 105ms) */
        if (wdt_counter >= 7) {
            wdt_counter = 0;
            check_enter_key = true;
        }
    }
}

/* 현재 Power-down 상태 확인 */
bool wdt_is_power_down(void) {
    return (wdt_state == WDT_POWER_DOWN);
}

/* Enter 키 체크가 필요한지 확인 */
bool wdt_should_check_enter(void) {
    if (check_enter_key) {
        check_enter_key = false;  /* 플래그 클리어 */
        return true;
    }
    return false;
}