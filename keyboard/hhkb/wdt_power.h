/*
 * wdt_power.h
 * WDT 기반 Power-down 모드 구현
 * 
 * Phase 2: Watchdog Timer를 사용한 초저전력 Deep Sleep 구현
 */

#ifndef WDT_POWER_H
#define WDT_POWER_H

#include <stdint.h>
#include <stdbool.h>

/* WDT 타이머 설정 - 약 100ms 간격 */
#define WDT_TIMEOUT_100MS   (0<<WDP3)|(1<<WDP2)|(1<<WDP1)|(0<<WDP0)  /* 0110 = ~125ms @ 5V */

/* Power-down 모드 상태 */
typedef enum {
    WDT_POWER_ACTIVE,
    WDT_POWER_DOWN,
    WDT_POWER_WAKING
} wdt_power_state_t;

/* WDT 전력 관리 초기화 */
void wdt_power_init(void);

/* Power-down 모드 진입 */
void wdt_power_down_enter(void);

/* Power-down 모드 해제 */
void wdt_power_down_exit(void);

/* WDT 인터럽트 핸들러에서 호출할 함수 */
void wdt_power_handle_interrupt(void);

/* 현재 Power-down 상태 확인 */
bool wdt_is_power_down(void);

/* Enter 키 체크가 필요한지 확인 */
bool wdt_should_check_enter(void);

#endif /* WDT_POWER_H */