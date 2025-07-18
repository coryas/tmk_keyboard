/*
 * power_management.h
 * HHKB 블루투스 전원 관리 모듈
 * 
 * Deep Sleep 모드 및 전원 상태 관리를 담당합니다.
 */

#ifndef POWER_MANAGEMENT_H
#define POWER_MANAGEMENT_H

#include <stdint.h>
#include <stdbool.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/interrupt.h>

/* 전원 상태 정의 */
typedef enum {
    POWER_STATE_ACTIVE,         // 정상 동작 상태
    POWER_STATE_IDLE,          // 유휴 상태 (타이머 동작 중)
    POWER_STATE_SLEEP_PENDING, // Sleep 진입 준비 중
    POWER_STATE_DEEP_SLEEP,    // Deep Sleep 상태
    POWER_STATE_WAKING         // Wake-up 진행 중
} power_state_t;

/* Deep Sleep 설정 구조체 */
typedef struct {
    uint16_t idle_timeout_sec;     // 기본값: 30초
    bool auto_sleep_enabled;       // 기본값: true
    uint8_t wakeup_key;            // 깨우기 키 (기본값: KC_ENTER)
    bool auto_reconnect_on_wake;   // 기본값: true
    uint8_t reconnect_delay_ms;    // 재연결 지연 시간 (기본값: 50ms)
    bool use_magic_command;        // Magic Command 사용 여부 (기본값: true)
    bool consume_wakeup_key;       // Wake-up 키 전송 여부 (기본값: false)
} deep_sleep_config_t;

/* 전원 관리자 구조체 */
typedef struct {
    // 상태 관리
    power_state_t current_state;
    uint32_t idle_timer;           // 유휴 시간 카운터 (ms)
    bool wake_pending;             // Wake-up 대기 중
    
    // 설정
    deep_sleep_config_t config;
    
    // Wake-up 설정
    uint8_t wakeup_key;            // 기본값: KC_ENTER
    bool consume_wakeup_key;       // Wake-up 키 소비 여부
    
    // Magic Command 지원
    bool magic_command_enabled;
    
    // 콜백
    void (*on_sleep)(void);        // Sleep 진입 시 콜백
    void (*on_wake)(void);         // Wake-up 시 콜백
} power_manager_t;

/* 전역 전원 관리자 인스턴스 */
extern power_manager_t power_manager;

/* 초기화 및 설정 함수 */
void power_mgr_init(power_manager_t* mgr);
void power_mgr_configure(power_manager_t* mgr, deep_sleep_config_t* config);

/* 상태 제어 함수 */
void power_mgr_reset_timer(power_manager_t* mgr);
void power_mgr_update_timer(power_manager_t* mgr, uint16_t delta_ms);
void power_mgr_enter_sleep(power_manager_t* mgr);
void power_mgr_wake_up(power_manager_t* mgr);

/* 상태 조회 함수 */
power_state_t power_mgr_get_state(power_manager_t* mgr);
uint32_t power_mgr_get_idle_time(power_manager_t* mgr);
bool power_mgr_is_sleeping(power_manager_t* mgr);

/* Magic Command 지원 함수 */
bool power_mgr_is_magic_enabled(power_manager_t* mgr);
void power_mgr_set_magic_enabled(power_manager_t* mgr, bool enabled);

/* Wake-up 키 설정 함수 */
void power_mgr_set_wakeup_key(power_manager_t* mgr, uint8_t key);
uint8_t power_mgr_get_wakeup_key(power_manager_t* mgr);

/* 전원 관리 설정 및 기본값 */
#define DEFAULT_IDLE_TIMEOUT_SEC    300  /* 5분 (300초) */
#define DEFAULT_RECONNECT_DELAY_MS  50

/* Wake-up 키 정의 - TMK의 KC_ENTER와 충돌을 피하기 위해 다른 이름 사용 */
#define PM_WAKEUP_KEY_ENTER  0x28
#define DEFAULT_WAKEUP_KEY   PM_WAKEUP_KEY_ENTER

/* 디버그 매크로 (선택적) */
#ifdef POWER_DEBUG
#define POWER_LOG(msg) print(msg)
#define POWER_LOG_HEX(val) phex(val)
#else
#define POWER_LOG(msg)
#define POWER_LOG_HEX(val)
#endif

#endif /* POWER_MANAGEMENT_H */