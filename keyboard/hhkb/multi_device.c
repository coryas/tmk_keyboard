/*
 * multi_device.c
 * HHKB 멀티 디바이스 페어링 관리 구현
 * 
 * 최대 4개의 블루투스 디바이스 페어링을 관리합니다.
 */

#include "multi_device.h"
#include "rn42/rn42.h"
#include "rn42/rn42_task.h"
#include "rn42/rn42_sleep.h"
#include "print.h"
#include "debug.h"
#include "wait.h"
#include "timer.h"
#include "led.h"
#include "led_control.h"
#include <string.h>
#include <stdio.h>
#include <avr/eeprom.h>

/* 전역 디바이스 관리자 인스턴스 */
device_manager_t device_manager;

/* 내부 함수 선언 */
static void device_mgr_clear_device(bt_device_t* device);
static bool device_mgr_validate_slot(uint8_t slot);
static void device_mgr_update_connection_state(device_manager_t* mgr, uint8_t slot, connection_state_t state);

/* 초기화 함수 */
void device_mgr_init(device_manager_t* mgr) {
    if (!mgr) return;
    
    dprintf("Device Manager Init\n");
    
    /* 설정 초기화 */
    mgr->config.device_count = 0;
    mgr->config.current_device = 0;
    mgr->config.pairing_mode = false;
    mgr->config.pairing_slot = 0;
    
    /* 모든 디바이스 슬롯 초기화 */
    for (uint8_t i = 0; i < MAX_DEVICES; i++) {
        device_mgr_clear_device(&mgr->config.devices[i]);
        mgr->config.devices[i].slot = i;
        mgr->conn_states[i] = CONN_STATE_DISCONNECTED;
    }
    
    /* 연결 관리 초기화 */
    mgr->active_device = 0;
    mgr->switching_in_progress = false;
    mgr->reconnect_attempts = 0;
    mgr->target_device = 0;
    
    /* LED 상태 초기화 */
    mgr->led_blink_count = 0;
    mgr->led_timer = 0;
    
    /* EEPROM에서 설정 로드 */
    device_mgr_load_config(mgr);
}

/* EEPROM에서 설정 로드 */
void device_mgr_load_config(device_manager_t* mgr) {
    if (!mgr) return;
    
    /* 매직 넘버 확인 */
    uint32_t magic = eeprom_read_dword((uint32_t*)EEPROM_MAGIC_ADDR);
    if (magic != EEPROM_MAGIC_NUMBER) {
        dprintf("EEPROM not initialized, using defaults\n");
        
        /* 초기 설정 저장 */
        eeprom_write_dword((uint32_t*)EEPROM_MAGIC_ADDR, EEPROM_MAGIC_NUMBER);
        eeprom_write_dword((uint32_t*)EEPROM_VERSION_ADDR, 0x00010000); /* v1.0.0 */
        device_mgr_save_config(mgr);
        return;
    }
    
    /* 활성 디바이스 로드 */
    mgr->config.current_device = eeprom_read_byte((uint8_t*)EEPROM_ACTIVE_DEVICE_ADDR);
    if (mgr->config.current_device >= MAX_DEVICES) {
        mgr->config.current_device = 0;
    }
    mgr->active_device = mgr->config.current_device;
    
    /* 각 디바이스 정보 로드 */
    mgr->config.device_count = 0;
    for (uint8_t i = 0; i < MAX_DEVICES; i++) {
        uint16_t addr = EEPROM_DEVICE_BASE_ADDR + (i * EEPROM_DEVICE_SIZE);
        bt_device_t* device = &mgr->config.devices[i];
        
        /* MAC 주소 읽기 */
        eeprom_read_block(device->mac_address, (void*)(addr), 6);
        
        /* MAC 주소가 유효한지 확인 (모두 0xFF가 아닌지) */
        bool valid = false;
        for (uint8_t j = 0; j < 6; j++) {
            if (device->mac_address[j] != 0xFF) {
                valid = true;
                break;
            }
        }
        
        if (valid) {
            /* 디바이스 이름 읽기 */
            eeprom_read_block(device->device_name, (void*)(addr + 6), 32);
            device->device_name[31] = '\0';
            
            /* 페어링 상태 및 설정 읽기 */
            device->paired = eeprom_read_byte((uint8_t*)(addr + 38));
            device->auto_reconnect_enabled = eeprom_read_byte((uint8_t*)(addr + 39));
            device->last_connected = eeprom_read_dword((uint32_t*)(addr + 40));
            device->slot = i;
            
            /* MAC 주소 문자열 변환 */
            device_mgr_mac_to_string(device->mac_address, device->mac_address_str);
            
            mgr->config.device_count++;
            dprintf("Loaded device %d: %s\n", i, device->mac_address_str);
        }
    }
    
    dprintf("Loaded %d devices from EEPROM\n", mgr->config.device_count);
}

/* EEPROM에 설정 저장 */
void device_mgr_save_config(device_manager_t* mgr) {
    if (!mgr) return;
    
    /* 활성 디바이스 저장 */
    eeprom_write_byte((uint8_t*)EEPROM_ACTIVE_DEVICE_ADDR, mgr->config.current_device);
    
    /* 각 디바이스 정보 저장 */
    for (uint8_t i = 0; i < MAX_DEVICES; i++) {
        uint16_t addr = EEPROM_DEVICE_BASE_ADDR + (i * EEPROM_DEVICE_SIZE);
        bt_device_t* device = &mgr->config.devices[i];
        
        if (device->paired) {
            /* MAC 주소 저장 */
            eeprom_write_block(device->mac_address, (void*)(addr), 6);
            
            /* 디바이스 이름 저장 */
            eeprom_write_block(device->device_name, (void*)(addr + 6), 32);
            
            /* 페어링 상태 및 설정 저장 */
            eeprom_write_byte((uint8_t*)(addr + 38), device->paired);
            eeprom_write_byte((uint8_t*)(addr + 39), device->auto_reconnect_enabled);
            eeprom_write_dword((uint32_t*)(addr + 40), device->last_connected);
        } else {
            /* 빈 슬롯은 0xFF로 채움 */
            for (uint8_t j = 0; j < EEPROM_DEVICE_SIZE; j++) {
                eeprom_write_byte((uint8_t*)(addr + j), 0xFF);
            }
        }
    }
    
    dprintf("Saved config to EEPROM\n");
}

/* 디바이스 페어링 */
bool device_mgr_pair_device(device_manager_t* mgr, uint8_t slot) {
    if (!mgr || !device_mgr_validate_slot(slot)) return false;
    
    dprintf("Pairing device to slot %d\n", slot);
    
    /* 페어링 모드 진입 */
    if (!device_mgr_enter_pairing_mode(mgr, slot)) {
        return false;
    }
    
    /* RN-42를 페어링 모드로 전환 */
    if (!rn42_enter_pairing_mode()) {
        device_mgr_exit_pairing_mode(mgr);
        return false;
    }
    
    /* 페어링 대기 (타임아웃: 30초) */
    uint32_t start = timer_read32();
    bool paired = false;
    
    while (timer_elapsed32(start) < DEFAULT_PAIRING_TIMEOUT_MS) {
        /* RN-42 상태 확인 */
        if (rn42_is_connected()) {
            /* 연결 성공 - 디바이스 정보 가져오기 */
            char mac_str[18];
            if (rn42_get_remote_address(mac_str)) {
                bt_device_t* device = &mgr->config.devices[slot];
                
                /* MAC 주소 저장 */
                device_mgr_string_to_mac(mac_str, device->mac_address);
                strcpy(device->mac_address_str, mac_str);
                
                /* 디바이스 정보 설정 */
                device->paired = true;
                device->auto_reconnect_enabled = true;
                device->last_connected = timer_read32();
                device->slot = slot;
                snprintf(device->device_name, sizeof(device->device_name), "Device %d", slot + 1);
                
                /* 연결 상태 업데이트 */
                device_mgr_update_connection_state(mgr, slot, CONN_STATE_PAIRED);
                mgr->config.device_count++;
                
                /* 현재 디바이스로 설정 */
                mgr->config.current_device = slot;
                mgr->active_device = slot;
                
                paired = true;
                dprintf("Paired with %s\n", mac_str);
                break;
            }
        }
        
        wait_ms(100);
    }
    
    /* 페어링 모드 종료 */
    device_mgr_exit_pairing_mode(mgr);
    rn42_exit_pairing_mode();
    
    if (paired) {
        /* EEPROM에 저장 */
        device_mgr_save_config(mgr);
        
        /* LED로 성공 표시 */
        device_mgr_show_device_number(mgr, slot);
    }
    
    return paired;
}

/* 디바이스 삭제 */
bool device_mgr_delete_device(device_manager_t* mgr, uint8_t slot) {
    if (!mgr || !device_mgr_validate_slot(slot)) return false;
    
    bt_device_t* device = &mgr->config.devices[slot];
    if (!device->paired) {
        return false;  /* 이미 비어있는 슬롯 */
    }
    
    dprintf("Deleting device from slot %d\n", slot);
    
    /* 현재 연결된 디바이스인 경우 연결 해제 */
    if (mgr->active_device == slot && device_mgr_is_connected(mgr)) {
        device_mgr_disconnect_current(mgr);
    }
    
    /* 디바이스 정보 삭제 */
    device_mgr_clear_device(device);
    device->slot = slot;
    mgr->config.device_count--;
    
    /* 연결 상태 업데이트 */
    device_mgr_update_connection_state(mgr, slot, CONN_STATE_DISCONNECTED);
    
    /* EEPROM에 저장 */
    device_mgr_save_config(mgr);
    
    return true;
}

/* 디바이스 전환 */
bool device_mgr_switch_device(device_manager_t* mgr, uint8_t slot) {
    if (!mgr || !device_mgr_validate_slot(slot)) return false;
    
    bt_device_t* device = &mgr->config.devices[slot];
    if (!device->paired) {
        dprintf("Slot %d is empty\n", slot);
        return false;
    }
    
    /* 이미 활성 디바이스인 경우 */
    if (mgr->active_device == slot && device_mgr_is_connected(mgr)) {
        dprintf("Already connected to device %d\n", slot);
        return true;
    }
    
    dprintf("Switching to device %d: %s\n", slot, device->mac_address_str);
    
    /* 전환 상태 설정 */
    mgr->switching_in_progress = true;
    mgr->target_device = slot;
    
    /* 현재 연결 해제 */
    if (device_mgr_is_connected(mgr)) {
        device_mgr_disconnect_current(mgr);
        wait_ms(500);
    }
    
    /* 새 디바이스로 연결 */
    bool connected = device_mgr_connect_to_device(mgr, slot);
    
    if (connected) {
        /* 활성 디바이스 업데이트 */
        mgr->config.current_device = slot;
        mgr->active_device = slot;
        device->last_connected = timer_read32();
        
        /* EEPROM에 저장 */
        device_mgr_save_config(mgr);
        
        /* LED로 디바이스 번호 표시 */
        device_mgr_show_device_number(mgr, slot);
    }
    
    mgr->switching_in_progress = false;
    return connected;
}

/* 디바이스 연결 */
bool device_mgr_connect_to_device(device_manager_t* mgr, uint8_t slot) {
    if (!mgr || !device_mgr_validate_slot(slot)) return false;
    
    bt_device_t* device = &mgr->config.devices[slot];
    if (!device->paired) return false;
    
    /* 연결 상태 업데이트 */
    device_mgr_update_connection_state(mgr, slot, CONN_STATE_CONNECTING);
    
    /* RN-42로 연결 시도 */
    bool connected = rn42_connect_to_address(device->mac_address_str);
    
    if (connected) {
        device_mgr_update_connection_state(mgr, slot, CONN_STATE_CONNECTED);
        dprintf("Connected to device %d\n", slot);
    } else {
        device_mgr_update_connection_state(mgr, slot, CONN_STATE_DISCONNECTED);
        dprintf("Failed to connect to device %d\n", slot);
    }
    
    return connected;
}

/* 현재 연결 해제 */
bool device_mgr_disconnect_current(device_manager_t* mgr) {
    if (!mgr) return false;
    
    rn42_disconnect();
    
    /* 연결 해제는 항상 성공한다고 가정 */
    device_mgr_update_connection_state(mgr, mgr->active_device, CONN_STATE_DISCONNECTED);
    
    return true;
}

/* 재연결 */
bool device_mgr_reconnect(device_manager_t* mgr) {
    if (!mgr) return false;
    
    uint8_t device = mgr->config.current_device;
    if (!mgr->config.devices[device].paired) {
        return false;
    }
    
    return device_mgr_connect_to_device(mgr, device);
}

/* 연결 이벤트 처리 */
void device_mgr_handle_connection_event(device_manager_t* mgr, bool connected) {
    if (!mgr) return;
    
    if (connected) {
        device_mgr_update_connection_state(mgr, mgr->active_device, CONN_STATE_CONNECTED);
    } else {
        device_mgr_update_connection_state(mgr, mgr->active_device, CONN_STATE_DISCONNECTED);
    }
}

/* 페어링 모드 진입 */
bool device_mgr_enter_pairing_mode(device_manager_t* mgr, uint8_t slot) {
    if (!mgr || !device_mgr_validate_slot(slot)) return false;
    
    mgr->config.pairing_mode = true;
    mgr->config.pairing_slot = slot;
    
    /* LED로 페어링 모드 표시 */
    device_mgr_show_pairing_mode(mgr);
    
    return true;
}

/* 페어링 모드 종료 */
void device_mgr_exit_pairing_mode(device_manager_t* mgr) {
    if (!mgr) return;
    
    mgr->config.pairing_mode = false;
}

/* LED 업데이트 */
void device_mgr_update_led(device_manager_t* mgr, uint16_t delta_ms) {
    if (!mgr) return;
    
    /* LED 타이머 업데이트 */
    if (mgr->led_timer > 0) {
        if (mgr->led_timer > delta_ms) {
            mgr->led_timer -= delta_ms;
        } else {
            mgr->led_timer = 0;
            
            /* LED 점멸 처리 */
            if (mgr->led_blink_count > 0) {
                static bool led_on = false;
                
                if (led_on) {
                    LED_OFF();
                    mgr->led_blink_count--;
                    mgr->led_timer = 200;  /* Off 시간 */
                } else {
                    LED_ON();
                    mgr->led_timer = 200;  /* On 시간 */
                }
                
                led_on = !led_on;
            }
        }
    }
}

/* 디바이스 번호 LED 표시 */
void device_mgr_show_device_number(device_manager_t* mgr, uint8_t device) {
    if (!mgr || device >= MAX_DEVICES) return;
    
    /* 디바이스 번호만큼 LED 점멸 */
    mgr->led_blink_count = (device + 1) * 2;  /* On/Off 쌍 */
    mgr->led_timer = 0;
}

/* 페어링 모드 LED 표시 */
void device_mgr_show_pairing_mode(device_manager_t* mgr) {
    if (!mgr) return;
    
    /* 느린 점멸 */
    mgr->led_blink_count = 20;  /* 10회 점멸 */
    mgr->led_timer = 0;
}

/* 유틸리티 함수 구현 */

/* MAC 주소를 문자열로 변환 */
void device_mgr_mac_to_string(uint8_t* mac, char* str) {
    sprintf(str, "%02X:%02X:%02X:%02X:%02X:%02X",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

/* 문자열을 MAC 주소로 변환 */
bool device_mgr_string_to_mac(const char* str, uint8_t* mac) {
    int values[6];
    if (sscanf(str, "%x:%x:%x:%x:%x:%x",
               &values[0], &values[1], &values[2],
               &values[3], &values[4], &values[5]) == 6) {
        for (int i = 0; i < 6; i++) {
            mac[i] = (uint8_t)values[i];
        }
        return true;
    }
    return false;
}

/* 상태 조회 함수 구현 */

uint8_t device_mgr_get_active_device(device_manager_t* mgr) {
    return mgr ? mgr->active_device : 0;
}

connection_state_t device_mgr_get_connection_state(device_manager_t* mgr, uint8_t slot) {
    if (!mgr || slot >= MAX_DEVICES) return CONN_STATE_DISCONNECTED;
    return mgr->conn_states[slot];
}

uint8_t device_mgr_get_device_count(device_manager_t* mgr) {
    return mgr ? mgr->config.device_count : 0;
}

bool device_mgr_is_connected(device_manager_t* mgr) {
    if (!mgr) return false;
    return (mgr->conn_states[mgr->active_device] == CONN_STATE_CONNECTED ||
            mgr->conn_states[mgr->active_device] == CONN_STATE_PAIRED);
}

bool device_mgr_is_slot_empty(device_manager_t* mgr, uint8_t slot) {
    if (!mgr || slot >= MAX_DEVICES) return true;
    return !mgr->config.devices[slot].paired;
}

bool device_mgr_is_pairing_mode(device_manager_t* mgr) {
    return mgr ? mgr->config.pairing_mode : false;
}

/* 내부 함수 구현 */

/* 디바이스 정보 초기화 */
static void device_mgr_clear_device(bt_device_t* device) {
    memset(device->mac_address, 0, sizeof(device->mac_address));
    memset(device->mac_address_str, 0, sizeof(device->mac_address_str));
    memset(device->device_name, 0, sizeof(device->device_name));
    device->paired = false;
    device->last_connected = 0;
    device->auto_reconnect_enabled = false;
}

/* 슬롯 유효성 검사 */
static bool device_mgr_validate_slot(uint8_t slot) {
    return (slot < MAX_DEVICES);
}

/* 연결 상태 업데이트 */
static void device_mgr_update_connection_state(device_manager_t* mgr, uint8_t slot, connection_state_t state) {
    if (mgr && slot < MAX_DEVICES) {
        mgr->conn_states[slot] = state;
    }
}