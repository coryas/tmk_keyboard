#include <avr/io.h>
#include <avr/power.h>
#include <avr/wdt.h>
#include "lufa.h"
#include "print.h"
#include "sendchar.h"
#include "rn42.h"
#include "rn42_task.h"
#include "serial.h"
#include "keyboard.h"
#include "keycode.h"
#include "action.h"
#include "action_util.h"
#include "wait.h"
#include "host.h"
#include "suart.h"
#include "suspend.h"
#include "power_management.h"
#include "multi_device.h"
#include "rn42_sleep.h"
#include "matrix.h"
#include "config_rn42.h"
#include "timer.h"
#include "debug.h"
#include "wdt_power.h"

static int8_t sendchar_func(uint8_t c)
{
    xmit(c);        // SUART
    sendchar(c);    // LUFA
    return 0;
}

static void SetupHardware(void)
{
    /* Disable watchdog if enabled by bootloader/fuses */
    MCUSR &= ~(1 << WDRF);
    wdt_disable();

    /* Disable clock division */
    clock_prescale_set(clock_div_1);

    // Leonardo needs. Without this USB device is not recognized.
    USB_Disable();

    USB_Init();

    // for Console_Task
    USB_Device_EnableSOFEvents();
#ifdef CONSOLE_ENABLE
    print_set_sendchar(sendchar_func);
#endif

    // SUART PD0:output, PD1:input
    DDRD |= (1<<0);
    PORTD |= (1<<0);
    DDRD &= ~(1<<1);
    PORTD |= (1<<1);
}

int main(void)  __attribute__ ((weak));
int main(void)
{
    SetupHardware();
    sei();

    /* wait for USB startup to get ready for debug output */
    uint8_t timeout = 255;  // timeout when USB is not available(Bluetooth)
    while (timeout-- && USB_DeviceState != DEVICE_STATE_Configured) {
        wait_ms(4);
#if defined(INTERRUPT_CONTROL_ENDPOINT)
        ;
#else
        USB_USBTask();
#endif
    }
    print("\nUSB init\n");

    rn42_init();
    rn42_task_init();
    print("RN-42 init\n");

    /* init modules */
    keyboard_init();
    
    /* Deep Sleep 전원 관리자 초기화 */
    power_mgr_init(&power_manager);
    print("Power Manager init\n");
    
    /* 멀티 디바이스 관리자 초기화 */
    device_mgr_init(&device_manager);
    print("Device Manager init\n");
    
    /* RN-42 Sleep 초기화 */
    rn42_sleep_init();
    print("RN42 Sleep init\n");
    
    /* WDT Power 초기화 */
    wdt_power_init();
    print("WDT Power init\n");

#ifdef SLEEP_LED_ENABLE
    sleep_led_init();
#endif

    print("Keyboard start\n");
    
    /* 타이머 변수를 루프 외부에 선언 */
    uint32_t last_timer = timer_read();
    
    while (1) {
        /* 타이머 업데이트 - 10ms 간격 (모든 상태에서 항상 실행) */
        uint32_t current_timer = timer_read();
        if (current_timer - last_timer >= 10) {
            power_mgr_update_timer(&power_manager, 10);
            last_timer = current_timer;
        }
        
        
        while (rn42_rts() && // RN42 is off
                USB_DeviceState == DEVICE_STATE_Suspended) {
            print("[s]");
            matrix_power_down();
            suspend_power_down();
            suspend_power_down();
            suspend_power_down();
            suspend_power_down();
            suspend_power_down();
            suspend_power_down();
            suspend_power_down();
            if (USB_Device_RemoteWakeupEnabled && suspend_wakeup_condition()) {
                    USB_Device_SendRemoteWakeup();
            }
        }

#ifdef DEEP_SLEEP_ENABLE
        /* Deep Sleep 상태 변수를 먼저 선언 */
        static bool was_sleeping = false;
        bool is_sleeping = power_mgr_is_sleeping(&power_manager);
        
        /* Deep Sleep 상태가 아닐 때만 키보드 처리 */
        if (!is_sleeping) {
#endif
            keyboard_task();
            
#ifdef DEEP_SLEEP_ENABLE
            /* 키 입력 감지 시 타이머 리셋 */
            static uint8_t prev_matrix_state[MATRIX_ROWS] = {0};
            static bool matrix_initialized = false;
            bool key_changed = false;
            
            /* Wake-up 직후이거나 초기화가 필요한 경우 */
            if (!matrix_initialized || (was_sleeping && !is_sleeping)) {
                /* 매트릭스 상태 초기화 */
                for (uint8_t r = 0; r < MATRIX_ROWS; r++) {
                    prev_matrix_state[r] = matrix_get_row(r);
                }
                matrix_initialized = true;
                
                /* Wake-up 후 매트릭스 재초기화 확인 */
            }
            
            /* 실제 키 변경 감지 */
            for (uint8_t r = 0; r < MATRIX_ROWS; r++) {
                matrix_row_t row = matrix_get_row(r);
                if (row != prev_matrix_state[r]) {
                    key_changed = true;
                    prev_matrix_state[r] = row;
                }
            }
            
            if (key_changed) {
                power_mgr_reset_timer(&power_manager);
            }
        }
#endif
        
        
        /* ESC 키 토글 기능 제거됨 - Magic Command + S로만 Deep Sleep 진입 가능 */
        
#ifdef DEEP_SLEEP_ENABLE
        
        /* Deep Sleep 상태에서 Wake-up 처리 */
        /* was_sleeping과 is_sleeping은 위에서 이미 선언됨 */
        
        /* is_sleeping 상태 업데이트 - 매 루프마다 확인 */
        is_sleeping = power_mgr_is_sleeping(&power_manager);
        
        if (is_sleeping) {
            /* 키보드 작업은 건너뛰고 Deep Sleep 처리만 */
            
            /* Deep Sleep 상태에서는 LED 끄기 */
            DDRE |= (1<<6); PORTE |= (1<<6);   // LED 끄기
            
            /* Deep Sleep 상태에서 매트릭스 스캔 주기 최적화 */
            static uint8_t sleep_scan_counter = 0;
            static bool prev_enter_pressed = false;
            static uint8_t deep_sleep_init_count = 0;
            static bool wdt_initialized = false;
            
            /* Deep Sleep에 처음 진입한 경우 초기화 */
            if (!was_sleeping) {
                was_sleeping = true;
                deep_sleep_init_count = 0;
                sleep_scan_counter = 0;
                
                /* Magic + S로 진입한 경우 키 릴리즈 대기 */
                wait_ms(300);  // 300ms 대기하여 키 릴리즈 확실히 처리
                
                /* 모든 키 상태 클리어 */
                clear_keyboard();
                
                /* 첫 번째 Enter 키 상태 읽기 */
                wait_ms(50);  // 추가 안정화 대기
                prev_enter_pressed = matrix_scan_enter_key();
                
                /* 디버깅: 초기 Enter 키 상태 LED 표시 */
                if (prev_enter_pressed) {
                    DDRE |= (1<<6); PORTE &= ~(1<<6);  // LED 켜기 (Enter 눌림)
                    wait_ms(100);
                    PORTE |= (1<<6);  // LED 끄기
                }
                
                /* WDT Power-down 모드 진입 */
                if (!wdt_initialized) {
                    wdt_power_down_enter();
                    wdt_initialized = true;
                    
                    /* WDT 진입 확인 LED 표시 */
                    for (int i = 0; i < 2; i++) {
                        DDRE |= (1<<6); PORTE &= ~(1<<6);  // LED 켜기
                        wait_ms(200);
                        PORTE |= (1<<6);  // LED 끄기
                        wait_ms(200);
                    }
                }
                
                continue;
            }
            
            /* WDT Power-down 모드 사용 시 */
            if (wdt_initialized && wdt_is_power_down()) {
                /* WDT 인터럽트 대기 - CPU는 Power-down 상태로 진입 */
                wdt_power_handle_interrupt();
                
                /* WDT가 Enter 키 체크를 요청하지 않으면 스킵 */
                if (!wdt_should_check_enter()) {
                    continue;  /* 아직 체크 시간이 아님 */
                }
                /* Enter 키 체크 시간이면 아래 Enter 키 스캔 코드로 진행 */
            } else {
                /* WDT 미사용 시 기존 방식 (폴백) */
                if (++sleep_scan_counter < 10) {
                    wait_ms(10);
                    continue;
                }
                sleep_scan_counter = 0;
            }
            
            /* 디버그 모드: 아무 키나 Wake-up 가능 (임시) */
            #define DEBUG_ANY_KEY_WAKEUP 0
            
            #if DEBUG_ANY_KEY_WAKEUP
            /* Deep Sleep 진입 직후에는 디버그 모드에서도 키 무시 */
            if (deep_sleep_init_count >= 10) {
                /* 아무 키나 눌렸는지 확인 (디버그용) */
                bool any_key_pressed = matrix_scan_any_key();
                if (any_key_pressed) {
                    /* 즉시 Wake-up */
                    print("Any key Wake-up!\n");
                    
                    /* LED 깜빡임으로 알림 */
                    for (int i = 0; i < 3; i++) {
                        DDRE |= (1<<6); PORTE &= ~(1<<6);  // LED 켜기
                        wait_ms(50);
                        PORTE |= (1<<6);  // LED 끄기
                        wait_ms(50);
                    }
                    
                    /* Wake-up 실행 */
                    power_mgr_wake_up(&power_manager);
                    was_sleeping = false;
                    continue;
                }
            }
            #endif
            
            /* Enter 키만 빠르게 스캔 (전력 절약) */
            bool enter_pressed = matrix_scan_enter_key();
            
            /* 디버깅: Enter 키 상태를 LED로 표시 */
            if (enter_pressed) {
                DDRE |= (1<<6); PORTE &= ~(1<<6);  // LED 켜기
            } else {
                DDRE |= (1<<6); PORTE |= (1<<6);   // LED 끄기
            }
            
            /* Deep Sleep 진입 직후 10번의 스캔은 무시 (1초) */
            if (deep_sleep_init_count < 10) {
                deep_sleep_init_count++;
                prev_enter_pressed = enter_pressed;  // 상태 업데이트
                continue;
            }
            
            /* Enter 키가 변경될 때 Wake-up (눌림/떼어짐 모두 감지) */
            if (prev_enter_pressed != enter_pressed) {
                /* 디바운싱을 위한 재확인 */
                wait_ms(20);
                bool confirm_pressed = matrix_scan_enter_key();
                
                if (confirm_pressed == enter_pressed && prev_enter_pressed != enter_pressed) {
                    /* Wake-up 시작 */
                    print("Enter key Wake-up! (");
                    if (enter_pressed) {
                        print("pressed");
                    } else {
                        print("released");
                    }
                    print(")\n");
                    
                    /* Wake-up 실행 */
                    power_mgr_wake_up(&power_manager);
                    
                    /* WDT Power-down 모드 해제 */
                    if (wdt_initialized) {
                        wdt_power_down_exit();
                        wdt_initialized = false;
                    }
                    
                    /* 상태 초기화 */
                    was_sleeping = false;
                    
                    /* 다음 루프에서 정상 동작으로 복귀 */
                    continue;
                }
            }
            
            prev_enter_pressed = enter_pressed;
            
            wait_ms(10);
            continue;
        } else {
            /* Deep Sleep 상태가 아닐 때 */
            if (was_sleeping) {
                /* Wake-up 직후 처리 */
                was_sleeping = false;
                
                /* 키보드 재초기화 */
                clear_keyboard();
                wait_ms(100);
            }
        }
#endif

#ifdef DEEP_SLEEP_ENABLE
        /* Deep Sleep 상태가 아닐 때만 USB 및 RN42 작업 처리 */
        if (!power_mgr_is_sleeping(&power_manager)) {
#endif
#if !defined(INTERRUPT_CONTROL_ENDPOINT)
            USB_USBTask();
#endif
            rn42_task();
#ifdef DEEP_SLEEP_ENABLE
        }
#endif
    }
}
