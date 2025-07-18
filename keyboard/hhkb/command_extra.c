/*
 * command_extra.c
 * HHKB Magic Command 확장
 * 
 * Deep Sleep 및 멀티 디바이스 제어를 위한 추가 명령을 제공합니다.
 */

#include <stdint.h>
#include <stdbool.h>
#include "keycode.h"
#include "command.h"
#include "print.h"
#include "debug.h"
#include "power_management.h"
#include "multi_device.h"
#include "led.h"

/* 외부 변수 */
extern power_manager_t power_manager;
extern device_manager_t device_manager;

/* Magic Command 키 정의 */
#define MAGIC_KEY_SLEEP         KC_S
#define MAGIC_KEY_DEVICE_1      KC_1
#define MAGIC_KEY_DEVICE_2      KC_2
#define MAGIC_KEY_DEVICE_3      KC_3
#define MAGIC_KEY_DEVICE_4      KC_4
#define MAGIC_KEY_PAIR          KC_P
#define MAGIC_KEY_DELETE        KC_D
#define MAGIC_KEY_BT_STATUS     KC_B
#define MAGIC_KEY_WAKE_ONLY     KC_W

/* command_extra 구현 */
bool command_extra(uint8_t code) {
    static uint8_t sub_command = 0;
    
    switch (code) {
        /* Deep Sleep 제어 */
        case MAGIC_KEY_SLEEP:
            print("Manual Deep Sleep\n");
            power_mgr_enter_sleep(&power_manager);
            return true;
            
        /* Wake-up 키 설정 */
        case MAGIC_KEY_WAKE_ONLY:
            print("Setting Wake-up key\n");
            /* 다음 키 입력을 Wake-up 키로 설정 */
            /* TODO: Wake-up 키 변경 구현 */
            return true;
            
        /* 멀티 디바이스 전환 */
        case MAGIC_KEY_DEVICE_1:
        case MAGIC_KEY_DEVICE_2:
        case MAGIC_KEY_DEVICE_3:
        case MAGIC_KEY_DEVICE_4:
            {
                uint8_t device = code - MAGIC_KEY_DEVICE_1;
                
                if (sub_command == MAGIC_KEY_PAIR) {
                    /* Magic + P + 1/2/3/4: 특정 슬롯에 페어링 */
                    print("Pairing to slot "); phex(device); print("\n");
                    device_mgr_pair_device(&device_manager, device);
                    sub_command = 0;
                } else if (sub_command == MAGIC_KEY_DELETE) {
                    /* Magic + D + 1/2/3/4: 특정 슬롯 삭제 */
                    print("Deleting slot "); phex(device); print("\n");
                    device_mgr_delete_device(&device_manager, device);
                    sub_command = 0;
                } else {
                    /* Magic + 1/2/3/4: 디바이스 전환 */
                    print("Switching to device "); phex(device); print("\n");
                    device_mgr_switch_device(&device_manager, device);
                }
            }
            return true;
            
        /* 페어링 모드 */
        case MAGIC_KEY_PAIR:
            sub_command = MAGIC_KEY_PAIR;
            print("Pairing mode - press device number\n");
            /* 현재 슬롯에 페어링 시작 */
            device_mgr_pair_device(&device_manager, device_manager.active_device);
            return true;
            
        /* 페어링 삭제 */
        case MAGIC_KEY_DELETE:
            sub_command = MAGIC_KEY_DELETE;
            print("Delete mode - press device number\n");
            return true;
            
        /* 블루투스 상태 */
        case MAGIC_KEY_BT_STATUS:
            print("Bluetooth Status:\n");
            print("Active device: "); phex(device_manager.active_device); print("\n");
            print("Device count: "); phex(device_mgr_get_device_count(&device_manager)); print("\n");
            print("Connected: "); 
            if (device_mgr_is_connected(&device_manager)) {
                print("Yes\n");
            } else {
                print("No\n");
            }
            print("Deep Sleep: ");
            if (power_mgr_is_sleeping(&power_manager)) {
                print("Yes\n");
            } else {
                print("No\n");
            }
            
            /* LED로 현재 디바이스 표시 */
            device_mgr_show_device_number(&device_manager, device_manager.active_device);
            return true;
            
        default:
            /* 다른 명령은 기본 처리기로 전달 */
            sub_command = 0;
            return false;
    }
}

/* console 모드 추가 명령 (옵션) */
bool command_console_extra(uint8_t code) {
    switch (code) {
        case KC_S:
            print("Sleep timer: "); 
            print_dec(power_mgr_get_idle_time(&power_manager) / 1000); 
            print(" sec\n");
            return true;
            
        case KC_D:
            print("Device info:\n");
            for (uint8_t i = 0; i < MAX_DEVICES; i++) {
                if (!device_mgr_is_slot_empty(&device_manager, i)) {
                    print("Slot "); phex(i); print(": ");
                    xputs(device_manager.config.devices[i].device_name);
                    print(" ("); 
                    xputs(device_manager.config.devices[i].mac_address_str); 
                    print(")\n");
                }
            }
            return true;
            
        default:
            return false;
    }
}