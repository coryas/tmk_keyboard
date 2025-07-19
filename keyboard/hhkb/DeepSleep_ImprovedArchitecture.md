# HHKB Deep Sleep 개선 아키텍처 설계

## 개요

현재 Deep Sleep의 문제점을 해결하기 위한 실용적인 개선 아키텍처를 제안합니다. 이 설계는 하드웨어 변경 없이 소프트웨어만으로 구현 가능하며, 단계적으로 적용할 수 있습니다.

## Phase 1: 즉시 적용 가능한 개선 (1-2일)

### 1.1 매트릭스 스캔 주기 최적화

```c
// main.c - Deep Sleep 상태에서의 스캔 주기 조정
if (is_sleeping) {
    static uint16_t sleep_scan_counter = 0;
    
    // 100ms마다 한 번만 스캔 (10배 절약)
    if (++sleep_scan_counter >= 10) {
        sleep_scan_counter = 0;
        
        // Enter 키만 빠르게 체크
        matrix_scan_single_key(5, 3);  // Row 5, Col 3
        
        if (enter_key_pressed()) {
            power_mgr_wake_up(&power_manager);
        }
    }
    
    wait_ms(10);
    continue;
}
```

### 1.2 CPU 클럭 동적 조정

```c
// power_management.c - Deep Sleep 진입 시
void power_mgr_enter_sleep(power_manager_t* mgr) {
    // CPU 클럭을 1MHz로 감소 (전력 80% 절약)
    clock_prescale_set(clock_div_16);  // 16MHz → 1MHz
    
    // 타이머 보정
    timer_prescale_adjust(16);
}

// Wake-up 시
void power_mgr_wake_up(power_manager_t* mgr) {
    // CPU 클럭 복원
    clock_prescale_set(clock_div_1);  // 1MHz → 16MHz
    
    // 타이머 복원
    timer_prescale_adjust(1);
}
```

### 예상 효과
- MCU 전력: 10-15mA → 2-3mA
- 매트릭스 스캔: 1-2mA → 0.1-0.2mA
- **총 절약**: ~13mA (약 35% 개선)

## Phase 2: WDT 기반 Power-down 구현 (1주일)

### 2.1 아키텍처 개요

```
┌─────────────────────────────────────────────────────┐
│                 Deep Sleep 상태 머신                  │
├─────────────────────────────────────────────────────┤
│                                                      │
│  ACTIVE ──5분──> IDLE ──즉시──> POWER_DOWN         │
│     ↑                               │                │
│     │                               │ WDT 인터럽트   │
│     │                               ↓ (250ms)       │
│     │                          CHECK_WAKEUP         │
│     │                               │                │
│     │                      Enter 키? │ No            │
│     └────────── Yes ────────────────┘               │
│                                     │ Yes            │
│                                     ↓                │
│                                POWER_DOWN           │
│                                                      │
└─────────────────────────────────────────────────────┘
```

### 2.2 WDT 설정

```c
// wdt_sleep.c - 새로운 모듈
#include <avr/wdt.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>

// WDT 인터럽트 핸들러
ISR(WDT_vect) {
    // WDT 인터럽트 플래그만 설정
    // 실제 처리는 메인 루프에서
}

void wdt_sleep_init(void) {
    // WDT를 인터럽트 모드로 설정 (리셋 안함)
    MCUSR &= ~(1 << WDRF);
    WDTCSR |= (1 << WDCE) | (1 << WDE);
    // 250ms 타임아웃, 인터럽트만 활성화
    WDTCSR = (1 << WDIE) | (0 << WDP3) | (1 << WDP2) | (0 << WDP1) | (1 << WDP0);
}

void enter_power_down_with_wdt(void) {
    // 주변 장치 비활성화
    PRR0 = 0xFF;  // 모든 주변장치 클럭 정지
    PRR1 = 0xFF;
    
    // Power-down 모드 설정
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sleep_enable();
    
    // BOD(Brown-out Detector) 비활성화로 추가 절약
    sleep_bod_disable();
    
    // Sleep 진입
    sei();  // 인터럽트 활성화
    sleep_cpu();
    
    // Wake-up 후
    sleep_disable();
    
    // 주변 장치 재활성화
    PRR0 = 0x00;
    PRR1 = 0x00;
}
```

### 2.3 메인 루프 수정

```c
// main.c
if (is_sleeping) {
    static bool wdt_wakeup = false;
    
    if (!wdt_wakeup) {
        // Power-down 진입
        enter_power_down_with_wdt();
        wdt_wakeup = true;  // WDT에 의해 깨어남
    }
    
    // Enter 키만 체크
    if (check_enter_key_fast()) {
        power_mgr_wake_up(&power_manager);
        wdt_wakeup = false;
    } else {
        // 다시 Sleep으로
        wdt_wakeup = false;
        continue;
    }
}
```

### 예상 효과
- MCU 전력: 10-15mA → <0.1mA (Power-down 모드)
- Wake-up 주기: 250ms (조정 가능)
- **총 절약**: ~15mA (약 40% 개선)

## Phase 3: RN-42 Sniff 모드 통합 (2주일)

### 3.1 RN-42 Sniff 모드 설정

```c
// rn42_sleep.c 개선
bool rn42_enter_sniff_mode(void) {
    // Command 모드 진입
    rn42_send_command("$$$");
    wait_ms(100);
    
    // Sniff 모드 파라미터 설정
    // SW,<interval>,<window>,<tries>,<timeout>
    // interval: 500ms, window: 10ms, tries: 1, timeout: 0
    rn42_send_command("SW,0320,000A,0001,0000\r");
    wait_ms(100);
    
    // Sniff 모드 활성화
    rn42_send_command("S|,0100\r");  // Sniff 모드 비트 설정
    wait_ms(100);
    
    // 설정 저장 및 재부팅
    rn42_send_command("R,1\r");
    wait_ms(2000);  // 재부팅 대기
    
    return true;
}

bool rn42_exit_sniff_mode(void) {
    // 데이터 전송으로 자동 Wake-up
    rn42_putc(0x00);  // NULL 문자 전송
    wait_ms(50);
    
    return true;
}
```

### 3.2 통합 전력 관리

```c
// power_management.c
void power_mgr_enter_deep_sleep(power_manager_t* mgr) {
    // 1. RN-42를 Sniff 모드로
    rn42_enter_sniff_mode();
    
    // 2. CPU 클럭 감소
    clock_prescale_set(clock_div_16);
    
    // 3. WDT 설정
    wdt_sleep_init();
    
    // 4. 상태 변경
    mgr->current_state = POWER_STATE_DEEP_SLEEP;
}
```

### 예상 효과
- RN-42 전력: 25-40mA → 3-5mA (Sniff 모드)
- **총 절약**: ~30mA (약 75% 개선)

## Phase 4: 최적화된 Wake-up 메커니즘 (선택적)

### 4.1 Enter 키 전용 빠른 스캔

```c
// matrix_fast.c - 최적화된 단일 키 스캔
bool check_enter_key_fast(void) {
    // 전원 켜기
    KEY_POWER_ON();
    _delay_us(5);
    
    // Row 5 선택
    KEY_SELECT_ROW(5);
    _delay_us(5);
    
    // Col 3 읽기
    bool pressed = KEY_READ_COL(3);
    
    // 전원 끄기
    KEY_POWER_OFF();
    
    return pressed;
}
```

### 4.2 인터럽트 기반 부분 Wake-up

```c
// PCINT를 사용한 Enter 키 감지
void setup_enter_key_interrupt(void) {
    // PC3를 PCINT19로 설정 (가정)
    PCICR |= (1 << PCIE2);   // PCINT16-23 활성화
    PCMSK2 |= (1 << PCINT19); // PCINT19만 활성화
}

ISR(PCINT2_vect) {
    // Enter 키 변화 감지
    enter_key_changed = true;
}
```

## 구현 로드맵

### Week 1: Phase 1 구현
- [ ] 매트릭스 스캔 주기 조정
- [ ] CPU 클럭 동적 제어
- [ ] 테스트 및 전력 측정

### Week 2: Phase 2 구현
- [ ] WDT 인터럽트 설정
- [ ] Power-down 모드 통합
- [ ] Wake-up 로직 구현
- [ ] 안정성 테스트

### Week 3: Phase 3 구현
- [ ] RN-42 Sniff 모드 명령
- [ ] 블루투스 연결 유지 테스트
- [ ] 통합 전력 관리

### Week 4: 최적화 및 문서화
- [ ] 전력 소비 측정
- [ ] 사용자 매뉴얼 작성
- [ ] 릴리즈 준비

## 예상 최종 결과

| 구성 요소 | 현재 | Phase 1 | Phase 2 | Phase 3 |
|----------|------|---------|---------|---------|
| MCU | 10-15mA | 2-3mA | <0.1mA | <0.1mA |
| RN-42 | 25-40mA | 25-40mA | 25-40mA | 3-5mA |
| 매트릭스 | 1-2mA | 0.1mA | 0mA | 0mA |
| **총계** | **36-57mA** | **27-43mA** | **25-40mA** | **3-5mA** |
| **개선율** | - | 25% | 30% | **90%** |

## 위험 요소 및 대응

1. **WDT Wake-up 지연**
   - 위험: 250ms 지연으로 사용자 경험 저하
   - 대응: 첫 키 입력은 Wake-up 전용으로 처리

2. **RN-42 연결 끊김**
   - 위험: Sniff 모드에서 연결 불안정
   - 대응: Sniff 파라미터 조정, 폴백 메커니즘

3. **타이밍 문제**
   - 위험: 클럭 변경 시 타이머 오차
   - 대응: 타이머 보정 로직 추가

## 결론

이 개선된 아키텍처를 통해 현재 대비 90% 이상의 전력 절약이 가능하며, 배터리 수명을 10배 이상 연장할 수 있습니다. 단계적 구현으로 리스크를 최소화하면서 점진적인 개선이 가능합니다.