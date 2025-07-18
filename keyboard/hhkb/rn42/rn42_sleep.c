/*
 * rn42_sleep.c
 * RN-42 블루투스 모듈 전원 관리 구현
 * 
 * Deep Sleep 모드 및 전원 관리 기능을 제공합니다.
 */

#include "rn42_sleep.h"
#include "rn42.h"
#include "rn42_task.h"
#include "serial.h"
#include "print.h"
#include "debug.h"
#include "wait.h"
#include "timer.h"
#include <string.h>
#include <stdio.h>
#include <avr/pgmspace.h>

/* 내부 상태 관리 */
static rn42_power_state_t current_state = RN42_STATE_ACTIVE;
static rn42_sleep_config_t sleep_config = {
    .sniff_interval = RN42_DEFAULT_SNIFF_INTERVAL,
    .deep_sleep_timer = RN42_DEFAULT_SLEEP_TIMER,
    .auto_connect_enabled = true,
    .retry_count = 3
};

/* 마지막 연결 디바이스 주소 */
static char last_device_address[18] = {0};  /* XX:XX:XX:XX:XX:XX 형식 */

/* 내부 함수 선언 */
static bool rn42_send_command_with_response(const char* cmd, const char* expected, uint16_t timeout_ms);
static bool rn42_get_last_device_address(void);
static void rn42_clear_buffer(void);

/* 초기화 */
void rn42_sleep_init(void) {
    print("RN42 Sleep Init\n");
    
    /* 기본 설정만 적용, RN-42 모듈 설정은 건드리지 않음 */
    current_state = RN42_STATE_ACTIVE;
    
    /* RN-42 모듈은 기본 설정 그대로 사용 */
    print("RN42 Sleep Init Complete (no module configuration)\n");
}

/* 설정 */
void rn42_sleep_configure(rn42_sleep_config_t* config) {
    if (!config) return;
    
    sleep_config = *config;
    
    /* 새 설정 적용 */
    rn42_set_sniff_mode(config->sniff_interval);
    rn42_set_deep_sleep_timer(config->deep_sleep_timer);
    rn42_set_auto_connect(config->auto_connect_enabled);
}

/* Deep Sleep 진입 */
bool rn42_enter_deep_sleep(void) {
    print("RN42 Enter Deep Sleep\n");
    
    if (current_state == RN42_STATE_DEEP_SLEEP) {
        return true;  /* 이미 Sleep 상태 */
    }
    
    /* RN-42 모듈과 상호작용하지 않고 단순히 상태만 변경 */
    /* 블루투스 연결 유지, MCU만 Sleep 모드로 진입 */
    current_state = RN42_STATE_DEEP_SLEEP;
    print("RN42 Deep Sleep state set (connection maintained)\n");
    return true;
}

/* Deep Sleep 종료 */
bool rn42_exit_deep_sleep(void) {
    return rn42_wake_up();
}

/* Wake-up */
bool rn42_wake_up(void) {
    print("RN42 Wake Up\n");
    
    if (current_state != RN42_STATE_DEEP_SLEEP) {
        return true;  /* 이미 활성 상태 */
    }
    
    /* 블루투스 연결이 유지되었으므로 단순히 상태만 변경 */
    current_state = RN42_STATE_ACTIVE;
    print("RN42 Wake Up complete (connection maintained)\n");
    return true;
}

/* 상태 조회 */
rn42_power_state_t rn42_get_power_state(void) {
    return current_state;
}

bool rn42_is_sleeping(void) {
    return (current_state == RN42_STATE_DEEP_SLEEP);
}

/* Auto-Connect 제어 */
bool rn42_set_auto_connect(bool enable) {
    char cmd[16];
    
    if (enable) {
        /* Master 모드로 설정 (Auto-Connect 활성화) */
        snprintf(cmd, sizeof(cmd), "%s%d", RN42_CMD_AUTO_CONNECT_MODE, RN42_MODE_MASTER);
    } else {
        /* DTR 모드로 설정 (수동 제어) */
        snprintf(cmd, sizeof(cmd), "%s%d", RN42_CMD_AUTO_CONNECT_MODE, RN42_MODE_DTR);
    }
    
    return rn42_send_command_with_response(cmd, "AOK", RN42_COMMAND_TIMEOUT_MS);
}

bool rn42_disable_auto_connect_for_sleep(void) {
    /* DTR 모드로 설정하여 Auto-Connect 비활성화 */
    char cmd[16];
    snprintf(cmd, sizeof(cmd), "%s%d", RN42_CMD_AUTO_CONNECT_MODE, RN42_MODE_DTR);
    return rn42_send_command_with_response(cmd, "AOK", RN42_COMMAND_TIMEOUT_MS);
}

bool rn42_restore_auto_connect(void) {
    return rn42_set_auto_connect(sleep_config.auto_connect_enabled);
}

/* 연결 관리 */
bool rn42_reconnect_last_device(void) {
    if (strlen(last_device_address) == 0) {
        dprintf("No last device to reconnect\n");
        return false;
    }
    
    dprintf("Reconnecting to: %s\n", last_device_address);
    
    /* 연결 명령 전송 */
    char cmd[32];
    snprintf(cmd, sizeof(cmd), "%s%s", RN42_CMD_CONNECT, last_device_address);
    
    for (uint8_t i = 0; i < sleep_config.retry_count; i++) {
        if (rn42_send_command_with_response(cmd, "CONNECT", RN42_CONNECT_TIMEOUT_MS)) {
            dprintf("Reconnected successfully\n");
            return true;
        }
        wait_ms(1000);
    }
    
    dprintf("Reconnection failed\n");
    return false;
}

bool rn42_disconnect_current(void) {
    return rn42_send_command_with_response(RN42_CMD_DISCONNECT, "DISCONNECT", RN42_COMMAND_TIMEOUT_MS);
}

/* 타이머 설정 */
bool rn42_set_sniff_mode(uint16_t interval) {
    char cmd[16];
    snprintf(cmd, sizeof(cmd), "%s%04X", RN42_CMD_SNIFF_MODE, interval);
    return rn42_send_command_with_response(cmd, "AOK", RN42_COMMAND_TIMEOUT_MS);
}

bool rn42_set_deep_sleep_timer(uint16_t timer) {
    char cmd[16];
    snprintf(cmd, sizeof(cmd), "%s%04X", RN42_CMD_SLEEP_MODE, timer);
    return rn42_send_command_with_response(cmd, "AOK", RN42_COMMAND_TIMEOUT_MS);
}

/* 내부 함수 구현 */

/* RN-42 명령 전송 및 응답 확인 - 비활성화됨 */
static bool rn42_send_command_with_response(const char* cmd, const char* expected, uint16_t timeout_ms) {
    /* Deep Sleep 구현에서는 RN-42 명령 모드를 사용하지 않음 */
    print("RN42 command mode disabled for Deep Sleep\n");
    return false;
}

/* 마지막 연결 디바이스 주소 가져오기 - 비활성화됨 */
static bool rn42_get_last_device_address(void) {
    /* Deep Sleep 구현에서는 주소 조회를 하지 않음 */
    print("Device address query disabled for Deep Sleep\n");
    return false;
}

/* 버퍼 클리어 */
static void rn42_clear_buffer(void) {
    while (rn42_getc() >= 0) {
        /* 버퍼의 모든 데이터를 읽어서 비움 */
    }
}