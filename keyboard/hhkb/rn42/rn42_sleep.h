/*
 * rn42_sleep.h
 * RN-42 블루투스 모듈 전원 관리
 * 
 * Deep Sleep 모드 및 전원 관리 기능을 제공합니다.
 */

#ifndef RN42_SLEEP_H
#define RN42_SLEEP_H

#include <stdint.h>
#include <stdbool.h>

/* RN-42 전원 상태 */
typedef enum {
    RN42_STATE_ACTIVE,          // 정상 동작 상태
    RN42_STATE_SLEEP_PENDING,   // Sleep 준비 중
    RN42_STATE_DEEP_SLEEP,      // Deep Sleep 상태
    RN42_STATE_WAKING,          // Wake-up 진행 중
    RN42_STATE_ERROR            // 오류 상태
} rn42_power_state_t;

/* RN-42 Sleep 설정 */
typedef struct {
    uint16_t sniff_interval;    // Sniff 모드 간격 (기본값: 0x0320)
    uint16_t deep_sleep_timer;  // Deep Sleep 타이머 (기본값: 0x0320)
    bool auto_connect_enabled;  // Auto-Connect 활성화 여부
    uint8_t retry_count;        // 재시도 횟수
} rn42_sleep_config_t;

/* 초기화 및 설정 */
void rn42_sleep_init(void);
void rn42_sleep_configure(rn42_sleep_config_t* config);

/* Deep Sleep 제어 */
bool rn42_enter_deep_sleep(void);
bool rn42_exit_deep_sleep(void);
bool rn42_wake_up(void);

/* 상태 조회 */
rn42_power_state_t rn42_get_power_state(void);
bool rn42_is_sleeping(void);

/* Auto-Connect 제어 */
bool rn42_set_auto_connect(bool enable);
bool rn42_disable_auto_connect_for_sleep(void);
bool rn42_restore_auto_connect(void);

/* 연결 관리 */
bool rn42_reconnect_last_device(void);
bool rn42_disconnect_current(void);

/* 타이머 설정 */
bool rn42_set_sniff_mode(uint16_t interval);
bool rn42_set_deep_sleep_timer(uint16_t timer);

/* 기본 설정값 */
#define RN42_DEFAULT_SNIFF_INTERVAL    0x0320  /* 800ms */
#define RN42_DEFAULT_SLEEP_TIMER       0x0320  /* 800ms */
#define RN42_COMMAND_TIMEOUT_MS        1000    /* 1초 */
#define RN42_CONNECT_TIMEOUT_MS        5000    /* 5초 */
#define RN42_WAKEUP_DELAY_MS          50      /* 50ms */

/* RN-42 명령어 */
#define RN42_CMD_SLEEP_MODE           "SW,"    /* Deep Sleep 타이머 설정 */
#define RN42_CMD_SNIFF_MODE           "S*,"    /* Sniff 모드 설정 */
#define RN42_CMD_AUTO_CONNECT_MODE    "SM,"    /* Auto-Connect 모드 */
#define RN42_CMD_DISCONNECT           "K,"     /* 연결 종료 */
#define RN42_CMD_CONNECT              "C,"     /* MAC 주소로 연결 */
#define RN42_CMD_SET_REMOTE           "SR,"    /* 원격 주소 설정 */
#define RN42_CMD_GET_REMOTE           "GR"     /* 원격 주소 조회 */

/* Auto-Connect 모드 값 */
#define RN42_MODE_SLAVE               0        /* Slave 모드 */
#define RN42_MODE_MASTER              1        /* Master 모드 */
#define RN42_MODE_DTR                 4        /* DTR 모드 (수동 제어) */

#endif /* RN42_SLEEP_H */