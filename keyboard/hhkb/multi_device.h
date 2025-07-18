/*
 * multi_device.h
 * HHKB 멀티 디바이스 페어링 관리
 * 
 * 최대 4개의 블루투스 디바이스 페어링을 관리합니다.
 */

#ifndef MULTI_DEVICE_H
#define MULTI_DEVICE_H

#include <stdint.h>
#include <stdbool.h>

/* 최대 디바이스 수 */
#define MAX_DEVICES 4

/* 디바이스 연결 상태 */
typedef enum {
    CONN_STATE_DISCONNECTED,    // 연결 안됨
    CONN_STATE_CONNECTING,      // 연결 중
    CONN_STATE_CONNECTED,       // 연결됨
    CONN_STATE_PAIRED          // 페어링됨
} connection_state_t;

/* 블루투스 디바이스 정보 */
typedef struct {
    uint8_t mac_address[6];         // MAC 주소 (바이너리)
    char mac_address_str[18];       // MAC 주소 (문자열: XX:XX:XX:XX:XX:XX)
    char device_name[32];           // 디바이스 이름
    bool paired;                    // 페어링 상태
    uint32_t last_connected;        // 마지막 연결 시간 (타임스탬프)
    bool auto_reconnect_enabled;    // 자동 재연결 활성화
    uint8_t slot;                   // 슬롯 번호 (0-3)
} bt_device_t;

/* 멀티 디바이스 설정 */
typedef struct {
    uint8_t device_count;           // 저장된 디바이스 수
    uint8_t current_device;         // 현재 활성 디바이스 인덱스 (0-3)
    bt_device_t devices[MAX_DEVICES];  // 디바이스 정보 배열
    bool pairing_mode;              // 페어링 모드 활성화
    uint8_t pairing_slot;           // 페어링 대상 슬롯
} multi_device_config_t;

/* 디바이스 관리자 구조체 */
typedef struct {
    // 디바이스 관리
    multi_device_config_t config;
    uint8_t active_device;          // 현재 활성 디바이스
    connection_state_t conn_states[MAX_DEVICES];  // 각 디바이스 연결 상태
    
    // 연결 관리
    bool switching_in_progress;     // 디바이스 전환 중
    uint8_t reconnect_attempts;     // 재연결 시도 횟수
    uint8_t target_device;          // 전환 대상 디바이스
    
    // LED 상태 표시
    uint8_t led_blink_count;        // LED 점멸 횟수
    uint16_t led_timer;             // LED 타이머
} device_manager_t;

/* 전역 디바이스 관리자 인스턴스 */
extern device_manager_t device_manager;

/* 초기화 및 설정 함수 */
void device_mgr_init(device_manager_t* mgr);
void device_mgr_load_config(device_manager_t* mgr);
void device_mgr_save_config(device_manager_t* mgr);

/* 디바이스 관리 함수 */
bool device_mgr_pair_device(device_manager_t* mgr, uint8_t slot);
bool device_mgr_delete_device(device_manager_t* mgr, uint8_t slot);
bool device_mgr_switch_device(device_manager_t* mgr, uint8_t slot);
bool device_mgr_is_slot_empty(device_manager_t* mgr, uint8_t slot);

/* 연결 관리 함수 */
bool device_mgr_connect_to_device(device_manager_t* mgr, uint8_t slot);
bool device_mgr_disconnect_current(device_manager_t* mgr);
bool device_mgr_reconnect(device_manager_t* mgr);
void device_mgr_handle_connection_event(device_manager_t* mgr, bool connected);

/* 상태 조회 함수 */
uint8_t device_mgr_get_active_device(device_manager_t* mgr);
connection_state_t device_mgr_get_connection_state(device_manager_t* mgr, uint8_t slot);
uint8_t device_mgr_get_device_count(device_manager_t* mgr);
bool device_mgr_is_connected(device_manager_t* mgr);

/* 페어링 모드 함수 */
bool device_mgr_enter_pairing_mode(device_manager_t* mgr, uint8_t slot);
void device_mgr_exit_pairing_mode(device_manager_t* mgr);
bool device_mgr_is_pairing_mode(device_manager_t* mgr);

/* LED 상태 표시 함수 */
void device_mgr_update_led(device_manager_t* mgr, uint16_t delta_ms);
void device_mgr_show_device_number(device_manager_t* mgr, uint8_t device);
void device_mgr_show_pairing_mode(device_manager_t* mgr);

/* 유틸리티 함수 */
void device_mgr_mac_to_string(uint8_t* mac, char* str);
bool device_mgr_string_to_mac(const char* str, uint8_t* mac);

/* EEPROM 주소 정의 */
#define EEPROM_MAGIC_ADDR           0x0000  /* 매직 넘버 (4바이트) */
#define EEPROM_VERSION_ADDR         0x0004  /* 버전 정보 (4바이트) */
#define EEPROM_CONFIG_ADDR          0x0008  /* 설정 정보 (8바이트) */
#define EEPROM_ACTIVE_DEVICE_ADDR   0x0010  /* 활성 디바이스 (4바이트) */
#define EEPROM_DEVICE_BASE_ADDR     0x0020  /* 디바이스 정보 시작 주소 */
#define EEPROM_DEVICE_SIZE          64      /* 디바이스당 크기 */

/* 매직 넘버 */
#define EEPROM_MAGIC_NUMBER         0xDEADBEEF

/* 기본값 */
#define DEFAULT_RECONNECT_ATTEMPTS  3
#define DEFAULT_CONNECT_TIMEOUT_MS  5000
#define DEFAULT_PAIRING_TIMEOUT_MS  30000

#endif /* MULTI_DEVICE_H */