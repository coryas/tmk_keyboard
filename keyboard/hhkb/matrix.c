/*
Copyright 2011 Jun Wako <wakojun@gmail.com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
 * scan matrix
 */
#include <stdint.h>
#include <stdbool.h>
#include <util/delay.h>
#include "print.h"
#include "debug.h"
#include "util.h"
#include "timer.h"
#include "matrix.h"
#include "hhkb_avr.h"
#include <avr/wdt.h>
#include "suspend.h"
#include "lufa.h"
#include "power_management.h"


// matrix power saving
#define MATRIX_POWER_SAVE       10000
static uint32_t matrix_last_modified = 0;

// Deep Sleep 타이머 관리
static uint16_t last_timer_read = 0;
extern power_manager_t power_manager;

// matrix state buffer(1:on, 0:off)
static matrix_row_t *matrix;
static matrix_row_t *matrix_prev;
static matrix_row_t _matrix0[MATRIX_ROWS];
static matrix_row_t _matrix1[MATRIX_ROWS];


void matrix_init(void)
{
#ifdef DEBUG
    debug_enable = true;
    debug_keyboard = true;
#endif

    KEY_INIT();

    // initialize matrix state: all keys off
    for (uint8_t i=0; i < MATRIX_ROWS; i++) _matrix0[i] = 0x00;
    for (uint8_t i=0; i < MATRIX_ROWS; i++) _matrix1[i] = 0x00;
    matrix = _matrix0;
    matrix_prev = _matrix1;
}

uint8_t matrix_scan(void)
{
    uint8_t *tmp;

    tmp = matrix_prev;
    matrix_prev = matrix;
    matrix = tmp;

    // Deep Sleep 타이머 업데이트
    uint16_t current_timer = timer_read();
    uint16_t delta_ms = TIMER_DIFF_16(current_timer, last_timer_read);
    last_timer_read = current_timer;
    
    // 전원 관리자에 타이머 업데이트 전달 (Deep Sleep 상태가 아닐 때만)
    // 단, delta_ms가 0이 아닐 때만 업데이트 (너무 빠른 호출 방지)
    if (!power_mgr_is_sleeping(&power_manager) && delta_ms > 0) {
        power_mgr_update_timer(&power_manager, delta_ms);
    }

    // power on
    if (!KEY_POWER_STATE()) KEY_POWER_ON();
    for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
        for (uint8_t col = 0; col < MATRIX_COLS; col++) {
            KEY_SELECT(row, col);
            _delay_us(5);

            // Not sure this is needed. This just emulates HHKB controller's behaviour.
            if (matrix_prev[row] & (1<<col)) {
                KEY_PREV_ON();
            }
            _delay_us(10);

            // NOTE: KEY_STATE is valid only in 20us after KEY_ENABLE.
            // If V-USB interrupts in this section we could lose 40us or so
            // and would read invalid value from KEY_STATE.
            uint8_t last = TIMER_RAW;

            KEY_ENABLE();

            // Wait for KEY_STATE outputs its value.
            // 1us was ok on one HHKB, but not worked on another.
            // no   wait doesn't work on Teensy++ with pro(1us works)
            // no   wait does    work on tmk PCB(8MHz) with pro2
            // 1us  wait does    work on both of above
            // 1us  wait doesn't work on tmk(16MHz)
            // 5us  wait does    work on tmk(16MHz)
            // 5us  wait does    work on tmk(16MHz/2)
            // 5us  wait does    work on tmk(8MHz)
            // 10us wait does    work on Teensy++ with pro
            // 10us wait does    work on 328p+iwrap with pro
            // 10us wait doesn't work on tmk PCB(8MHz) with pro2(very lagged scan)
            _delay_us(5);

            if (KEY_STATE()) {
                matrix[row] &= ~(1<<col);
            } else {
                matrix[row] |= (1<<col);
            }

            // Ignore if this code region execution time elapses more than 20us.
            // MEMO: 20[us] * (TIMER_RAW_FREQ / 1000000)[count per us]
            // MEMO: then change above using this rule: a/(b/c) = a*1/(b/c) = a*(c/b)
            if (TIMER_DIFF_RAW(TIMER_RAW, last) > 20/(1000000/TIMER_RAW_FREQ)) {
                matrix[row] = matrix_prev[row];
            }

            _delay_us(5);
            KEY_PREV_OFF();
            KEY_UNABLE();

            // NOTE: KEY_STATE keep its state in 20us after KEY_ENABLE.
            // This takes 25us or more to make sure KEY_STATE returns to idle state.
#ifdef HHKB_JP
            // Looks like JP needs faster scan due to its twice larger matrix
            // or it can drop keys in fast key typing
            _delay_us(30);
#else
            _delay_us(75);
#endif
        }
        if (matrix[row] ^ matrix_prev[row]) {
            matrix_last_modified = timer_read32();
            // Deep Sleep 타이머 리셋은 main.c에서 처리
        }
    }
    // power off
    if (KEY_POWER_STATE() &&
            (USB_DeviceState == DEVICE_STATE_Suspended ||
             USB_DeviceState == DEVICE_STATE_Unattached ) &&
            timer_elapsed32(matrix_last_modified) > MATRIX_POWER_SAVE) {
        KEY_POWER_OFF();
        suspend_power_down();
    }
    return 1;
}

inline
matrix_row_t matrix_get_row(uint8_t row)
{
    return matrix[row];
}

void matrix_power_up(void) {
    KEY_POWER_ON();
}
void matrix_power_down(void) {
    KEY_POWER_OFF();
}

/* Deep Sleep 전용 - Enter 키만 빠르게 스캔 */
bool matrix_scan_enter_key(void)
{
    // 키보드 전원 켜기
    if (!KEY_POWER_STATE()) {
        KEY_POWER_ON();
    }
    
    // 안정화 대기
    _delay_us(10);
    
    // Row 5, Col 3 선택 (Enter 키)
    KEY_SELECT(5, 3);
    _delay_us(10);
    
    // 이전 키 상태에 따른 히스테리시스 설정
    // Deep Sleep에서는 단순하게 처리
    KEY_PREV_OFF();  // 기본적으로 OFF로 설정
    _delay_us(10);
    
    // 키 활성화
    KEY_ENABLE();
    
    // 키 상태 읽기 전 안정화 대기
    _delay_us(10);
    
    // Enter 키 상태 읽기 (0 = pressed, 1 = not pressed)
    bool enter_pressed = !KEY_STATE();
    
    // 추가 안정화 대기
    _delay_us(75);
    
    // 키보드 상태 정리
    KEY_UNABLE();
    KEY_PREV_OFF();
    
    // 전원은 켜둔 상태로 유지 (다음 스캔을 위해)
    
    return enter_pressed;
}

/* Deep Sleep 전용 - 아무 키나 눌렸는지 확인 */
bool matrix_scan_any_key(void)
{
    // 키보드 전원 켜기
    if (!KEY_POWER_STATE()) {
        KEY_POWER_ON();
    }
    
    // 모든 행을 빠르게 스캔
    for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
        for (uint8_t col = 0; col < MATRIX_COLS; col++) {
            KEY_SELECT(row, col);
            _delay_us(5);
            
            KEY_PREV_OFF();
            _delay_us(5);
            
            KEY_ENABLE();
            _delay_us(5);
            
            // 키가 눌렸는지 확인
            if (!KEY_STATE()) {
                KEY_UNABLE();
                KEY_PREV_OFF();
                return true;  // 키가 눌림
            }
            
            KEY_UNABLE();
            _delay_us(5);
        }
    }
    
    return false;  // 아무 키도 눌리지 않음
}
