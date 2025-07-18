/*
 * power_management.c
 * HHKB 블루투스 전원 관리 모듈 구현
 * 
 * Deep Sleep 모드 및 전원 상태 관리 기능을 제공합니다.
 */

#include "power_management.h"
#include "timer.h"
#include "print.h"
#include "debug.h"
#include "wait.h"
#include "host.h"
#include "keyboard.h"
#include "matrix.h"
#include "action_util.h"
#include "rn42/rn42_sleep.h"
#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/interrupt.h>

/* 전역 전원 관리자 인스턴스 */
power_manager_t power_manager;

/* 내부 함수 선언 */
static void power_mgr_setup_interrupts(void);
static void power_mgr_prepare_sleep(power_manager_t* mgr);
static void power_mgr_execute_sleep(void);
static void power_mgr_handle_wakeup(power_manager_t* mgr);

/* 초기화 함수 */
void power_mgr_init(power_manager_t* mgr) {
    if (!mgr) return;
    
    POWER_LOG("Power Manager Init\n");
    
    /* 상태 초기화 */
    mgr->current_state = POWER_STATE_ACTIVE;
    mgr->idle_timer = 0;
    mgr->wake_pending = false;
    
    /* 기본 설정 적용 */
    mgr->config.idle_timeout_sec = DEFAULT_IDLE_TIMEOUT_SEC;
    mgr->config.auto_sleep_enabled = true;  /* 5분 후 자동 Sleep 활성화 */
    mgr->config.wakeup_key = DEFAULT_WAKEUP_KEY;
    mgr->config.auto_reconnect_on_wake = true;
    mgr->config.reconnect_delay_ms = DEFAULT_RECONNECT_DELAY_MS;
    mgr->config.use_magic_command = true;
    mgr->config.consume_wakeup_key = false;
    
    POWER_LOG("Init: auto_sleep=");
    if (mgr->config.auto_sleep_enabled) {
        POWER_LOG("ON");
    } else {
        POWER_LOG("OFF");
    }
    POWER_LOG(", timeout=");
    POWER_LOG_HEX(mgr->config.idle_timeout_sec);
    POWER_LOG("\n");
    
    /* Wake-up 설정 */
    mgr->wakeup_key = DEFAULT_WAKEUP_KEY;
    mgr->consume_wakeup_key = false;
    mgr->magic_command_enabled = true;
    
    /* 콜백 초기화 */
    mgr->on_sleep = NULL;
    mgr->on_wake = NULL;
    
    /* 인터럽트 설정 */
    power_mgr_setup_interrupts();
    
    POWER_LOG("Power Manager Init Complete\n");
}

/* 설정 함수 */
void power_mgr_configure(power_manager_t* mgr, deep_sleep_config_t* config) {
    if (!mgr || !config) return;
    
    /* 설정 복사 */
    mgr->config = *config;
    
    /* Wake-up 키 업데이트 */
    mgr->wakeup_key = config->wakeup_key;
    mgr->consume_wakeup_key = config->consume_wakeup_key;
    mgr->magic_command_enabled = config->use_magic_command;
    
    POWER_LOG("Power Manager Configured\n");
}

/* 타이머 리셋 */
void power_mgr_reset_timer(power_manager_t* mgr) {
    if (!mgr) return;
    
    mgr->idle_timer = 0;
    
    /* IDLE 상태에서 ACTIVE로 전환 */
    if (mgr->current_state == POWER_STATE_IDLE) {
        mgr->current_state = POWER_STATE_ACTIVE;
        POWER_LOG("State: IDLE -> ACTIVE\n");
    }
    
    /* 디버깅: 타이머 리셋 확인 - 제거 */
}

/* 타이머 업데이트 */
void power_mgr_update_timer(power_manager_t* mgr, uint16_t delta_ms) {
    if (!mgr || !mgr->config.auto_sleep_enabled) return;
    
    switch (mgr->current_state) {
        case POWER_STATE_ACTIVE:
            /* 1초 후 IDLE 상태로 전환 */
            mgr->idle_timer += delta_ms;
            if (mgr->idle_timer >= 1000) {
                mgr->current_state = POWER_STATE_IDLE;
                POWER_LOG("State: ACTIVE -> IDLE\n");
            }
            break;
            
        case POWER_STATE_IDLE:
            /* 설정된 시간 후 Sleep 준비 */
            mgr->idle_timer += delta_ms;
            
            /* 매 초마다 상태 확인 - 제거 */
            
            if (mgr->idle_timer >= (uint32_t)mgr->config.idle_timeout_sec * 1000) {
                mgr->current_state = POWER_STATE_SLEEP_PENDING;
                POWER_LOG("State: IDLE -> SLEEP_PENDING (");
                POWER_LOG_HEX(mgr->idle_timer);
                POWER_LOG("ms)\n");
            }
            break;
            
        case POWER_STATE_SLEEP_PENDING:
            /* 다음 타이머 틱에서 실제 Sleep 진입 */
            POWER_LOG("SLEEP_PENDING -> Entering sleep\n");
            power_mgr_enter_sleep(mgr);
            break;
            
        default:
            break;
    }
}

/* Deep Sleep 진입 */
void power_mgr_enter_sleep(power_manager_t* mgr) {
    if (!mgr) return;
    
    /* 이미 Sleep 상태인 경우 무시 */
    if (mgr->current_state == POWER_STATE_DEEP_SLEEP) {
        POWER_LOG("Already in Deep Sleep\n");
        return;
    }
    
    POWER_LOG("Entering Deep Sleep from state: ");
    POWER_LOG_HEX(mgr->current_state);
    POWER_LOG("\n");
    
    /* Sleep 준비 */
    power_mgr_prepare_sleep(mgr);
    
    /* 상태 변경 */
    mgr->current_state = POWER_STATE_DEEP_SLEEP;
    mgr->idle_timer = 0;  /* 타이머 리셋 */
    
    POWER_LOG("State changed to DEEP_SLEEP\n");
    
    /* Sleep 콜백 호출 */
    if (mgr->on_sleep) {
        mgr->on_sleep();
    }
    
    /* HHKB에서는 실제 Sleep 모드 사용 안함 */
    /* MCU Sleep 모드 진입 */
    /* power_mgr_execute_sleep(); */
    
    /* Wake-up 후 처리는 main.c에서 Enter 키 감지 시 처리 */
    /* power_mgr_handle_wakeup(mgr); */
}

/* Wake-up 처리 */
void power_mgr_wake_up(power_manager_t* mgr) {
    if (!mgr) return;
    
    /* Deep Sleep 상태가 아니면 무시 */
    if (mgr->current_state != POWER_STATE_DEEP_SLEEP) {
        return;
    }
    
    POWER_LOG("Waking up from Deep Sleep\n");
    
    /* Wake-up 후 처리 */
    power_mgr_handle_wakeup(mgr);
    
    /* 상태 변경 */
    mgr->current_state = POWER_STATE_ACTIVE;
    mgr->idle_timer = 0;
    mgr->wake_pending = false;
    
    /* auto_sleep_enabled가 변경되지 않도록 보장 */
    mgr->config.auto_sleep_enabled = true;
    mgr->config.idle_timeout_sec = DEFAULT_IDLE_TIMEOUT_SEC;
    
    POWER_LOG("Wake-up: auto_sleep=");
    if (mgr->config.auto_sleep_enabled) {
        POWER_LOG("ON");
    } else {
        POWER_LOG("OFF");
    }
    POWER_LOG(", timeout=");
    POWER_LOG_HEX(mgr->config.idle_timeout_sec);
    POWER_LOG(", state=");
    POWER_LOG_HEX(mgr->current_state);
    POWER_LOG("\n");
    
    /* Wake 콜백 호출 */
    if (mgr->on_wake) {
        mgr->on_wake();
    }
    
    POWER_LOG("Wake-up complete\n");
}

/* 상태 조회 함수 */
power_state_t power_mgr_get_state(power_manager_t* mgr) {
    return mgr ? mgr->current_state : POWER_STATE_ACTIVE;
}

uint32_t power_mgr_get_idle_time(power_manager_t* mgr) {
    return mgr ? mgr->idle_timer : 0;
}

bool power_mgr_is_sleeping(power_manager_t* mgr) {
    return mgr && (mgr->current_state == POWER_STATE_DEEP_SLEEP);
}

/* Magic Command 지원 함수 */
bool power_mgr_is_magic_enabled(power_manager_t* mgr) {
    return mgr ? mgr->magic_command_enabled : false;
}

void power_mgr_set_magic_enabled(power_manager_t* mgr, bool enabled) {
    if (mgr) {
        mgr->magic_command_enabled = enabled;
    }
}

/* Wake-up 키 설정 함수 */
void power_mgr_set_wakeup_key(power_manager_t* mgr, uint8_t key) {
    if (mgr) {
        mgr->wakeup_key = key;
        mgr->config.wakeup_key = key;
    }
}

uint8_t power_mgr_get_wakeup_key(power_manager_t* mgr) {
    return mgr ? mgr->wakeup_key : DEFAULT_WAKEUP_KEY;
}

/* 내부 함수 구현 */

/* 인터럽트 설정 - 매트릭스 스캔 방식에서는 불필요 */
static void power_mgr_setup_interrupts(void) {
    /* HHKB는 매트릭스 스캔 방식을 사용하므로 인터럽트 설정 불필요 */
    /* Wake-up은 매트릭스 스캔으로 처리 */
}

/* Sleep 준비 */
static void power_mgr_prepare_sleep(power_manager_t* mgr) {
    /* RN-42 상태만 변경 (연결 유지) */
    rn42_enter_deep_sleep();
    
    /* 키보드 스캔은 유지해서 Wake-up 감지 */
    /* keyboard_protocol은 유지 */
    
    /* 불필요한 주변장치 비활성화 */
    power_adc_disable();
    power_spi_disable();
    power_twi_disable();
    power_timer1_disable();
    power_timer3_disable();
    
    /* 매트릭스 스캔을 위한 타이머 유지 */
    /* Timer0은 매트릭스 스캔을 위해 유지 */
}

/* Sleep 실행 */
static void power_mgr_execute_sleep(void) {
    /* HHKB는 매트릭스 스캔 방식이므로 Sleep 모드 사용 안함 */
    /* 대신 CPU 클럭만 낮춰서 전력 절약 */
    /* 실제 Sleep 모드는 매트릭스 스캔과 호환되지 않음 */
    
    /* 불필요한 주변장치는 이미 power_mgr_prepare_sleep에서 비활성화됨 */
    /* 여기서는 단순히 리턴해서 main loop에서 매트릭스 스캔 계속 */
    
    /* Deep Sleep 상태는 main.c에서 매트릭스 스캔으로 처리 */
}

/* Wake-up 후 처리 */
static void power_mgr_handle_wakeup(power_manager_t* mgr) {
    /* 주변장치 재활성화 */
    power_all_enable();
    
    /* 키보드 스캔 재개 - 이미 진행 중이므로 생략 */
    /* keyboard_protocol = 1; */
    
    /* 매트릭스 초기화는 하지 않음 - 이미 작동 중 */
    /* matrix_init(); */
    
    /* RN-42 Wake-up (상태만 변경) */
    rn42_wake_up();
    
    /* 키 상태 클리어 */
    clear_keys();
    
    POWER_LOG("Hardware reinitialized\n");
}

/* Wake-up 인터럽트 핸들러 - 매트릭스 스캔 방식에서는 불필요 */
/* HHKB는 매트릭스 스캔을 사용하므로 인터럽트 방식 Wake-up 불가능 */