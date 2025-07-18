# 제품 요구사항 문서 (PRD)
## HHKB 블루투스 Deep Sleep 모드 및 멀티 디바이스 페어링 기능

### 문서 정보
- **버전**: 1.1
- **작성일**: 2025-01-17
- **대상 제품**: HHKB Professional Bluetooth Controller (RN-42 기반)
- **프로젝트 코드**: HHKB-BT-PWR-001

---

## 1. 개요

### 1.1 배경
현재 HHKB 블루투스 컨트롤러는 배터리 수명 최적화와 다중 기기 연결 관리에 제한이 있습니다. 사용자들은 더 긴 배터리 수명과 여러 기기 간 편리한 전환을 요구하고 있습니다.

### 1.2 목적
- **Deep Sleep 모드**: 30초간 입력이 없을 시 자동으로 저전력 모드로 진입하여 배터리 수명 연장
- **멀티 디바이스 페어링**: 최대 4개 기기와 페어링을 유지하고 빠르게 전환

### 1.3 범위
- RN-42 블루투스 모듈을 사용하는 HHKB 컨트롤러
- TMK 펌웨어 프레임워크 기반 구현
- 기존 키맵 시스템과의 통합

---

## 2. 기능 요구사항

### 2.1 Deep Sleep 모드

#### 2.1.1 기능 설명
- **진입 조건**: 
  - 블루투스 연결 상태에서 30초간 키 입력 없음
  - 사용자가 수동으로 활성화 (Magic + S)
  
- **동작**:
  - MCU를 Power-down 모드로 전환
  - RN-42 모듈을 Deep Sleep 모드로 설정
  - 전력 소비를 최소화 (목표: <50μA)

- **복귀 조건**:
  - Enter 키 입력 시에만 복귀 (다른 키는 무시)
  - 복귀 시간: <100ms
  - 복귀 후 마지막 연결된 디바이스로 자동 재연결
  - Enter 키는 재연결 후 전송되지 않음 (Wake-up 전용)

#### 2.1.2 기술 사양
```c
// Deep Sleep 설정 구조체
typedef struct {
    uint16_t idle_timeout_sec;     // 기본값: 30초
    bool auto_sleep_enabled;       // 기본값: true
    uint8_t wakeup_key_mask;       // 깨우기 키 설정
    bool auto_reconnect_on_wake;   // 기본값: true
    uint8_t reconnect_delay_ms;    // 재연결 지연 시간 (기본값: 50ms)
    bool use_magic_command;        // Magic Command 사용 여부 (기본값: true)
} deep_sleep_config_t;

// RN-42 Deep Sleep 명령
// S*,0320 - Sniff 모드 설정 (20ms 간격)
// SW,0320 - Deep Sleep 타이머 설정
// SM,4 - Auto-Connect 모드 설정 (DTR 모드)
```

#### 2.1.3 Wake-up 및 자동 재연결
- **Wake-up 메커니즘**:
  - 오직 Enter 키만 Wake-up 트리거로 동작
  - 다른 모든 키 입력은 Deep Sleep 상태에서 무시
  - Enter 키는 Wake-up 전용으로 사용되며 전송되지 않음
  - Magic + W로 임시 다른 키를 Wake-up 키로 설정 가능

- **자동 재연결 프로세스**:
  1. Wake-up 신호 감지 (Enter 키)
  2. MCU 및 RN-42 모듈 활성화
  3. 50ms 안정화 대기
  4. 마지막 연결 디바이스 정보 확인
  5. 재연결 시도 (최대 3회, 각 2초 타임아웃)
  6. 연결 성공 시 정상 동작 모드로 전환
  7. 연결 실패 시 대기 모드로 전환

- **Auto-Connect 충돌 방지**:
  - RN-42의 Auto-Connect를 DTR 모드(SM,4)로 설정
  - Deep Sleep 진입 시 Auto-Connect 비활성화
  - Wake-up 시 수동 연결 제어로 충돌 방지
  - 연결 우선순위: 수동 제어 > Auto-Connect

### 2.2 멀티 디바이스 페어링

#### 2.2.1 기능 설명
- **페어링 슬롯**: 4개 디바이스 저장 가능
- **디바이스 전환**: 
  - Magic + 1/2/3/4 키로 디바이스 선택
  - LED 표시로 현재 연결 상태 확인
  
- **페어링 관리**:
  - 새 디바이스 페어링: Magic + P + 슬롯 번호
  - 페어링 삭제: Magic + D + 슬롯 번호
  - 자동 재연결: 마지막 연결 디바이스 우선

#### 2.2.2 기술 사양
```c
// 멀티 디바이스 설정 구조체
typedef struct {
    uint8_t device_count;          // 최대 4
    uint8_t current_device;        // 현재 활성 디바이스
    bt_device_t devices[4];        // 디바이스 정보 배열
} multi_device_config_t;

// 디바이스 정보 구조체
typedef struct {
    uint8_t mac_address[6];        // 블루투스 MAC 주소
    char device_name[32];          // 디바이스 이름
    bool paired;                   // 페어링 상태
    uint32_t last_connected;       // 마지막 연결 시간
    bool auto_reconnect_enabled;   // 자동 재연결 활성화
} bt_device_t;

// Wake-up 설정
typedef struct {
    bool enter_pressed;            // Enter 키 누름 상태
    uint8_t custom_wakeup_key;     // 사용자 정의 Wake-up 키
    bool use_custom_key;           // 사용자 정의 키 사용 여부
} wakeup_config_t;
```

---

## 3. 사용자 인터페이스

### 3.1 키 조합

#### 3.1.1 Magic Command 키 조합
TMK의 Magic Command (LShift + RShift)를 활용하여 직관적인 키 조합을 제공합니다.

| 기능 | 키 조합 | 설명 |
|------|---------|------|
| Magic Command 활성화 | LShift + RShift | Magic Command 모드 진입 |
| Deep Sleep 진입 | Magic + S | 수동으로 Deep Sleep 모드 진입 |
| Wake-up 키 변경 | Magic + W | 임시로 다른 키를 Wake-up 키로 설정 |
| 디바이스 1 연결 | Magic + 1 | 첫 번째 페어링 디바이스로 전환 |
| 디바이스 2 연결 | Magic + 2 | 두 번째 페어링 디바이스로 전환 |
| 디바이스 3 연결 | Magic + 3 | 세 번째 페어링 디바이스로 전환 |
| 디바이스 4 연결 | Magic + 4 | 네 번째 페어링 디바이스로 전환 |
| 새 페어링 모드 | Magic + P | 현재 디바이스 슬롯에 새 페어링 |
| 페어링 모드 (슬롯 선택) | Magic + P + 1/2/3/4 | 선택한 슬롯에 새 디바이스 페어링 |
| 페어링 삭제 | Magic + D + 1/2/3/4 | 선택한 슬롯의 페어링 삭제 |
| 블루투스 상태 표시 | Magic + B | 현재 블루투스 연결 상태 LED 표시 |

#### 3.1.2 키 조합 사용 예시

- **디바이스 1로 전환**: 양쪽 Shift 키를 동시에 누른 상태에서 1을 누름
- **새 디바이스 페어링 (슬롯 2)**: 양쪽 Shift 누른 상태에서 P를 누르고, 이어서 2를 누름
- **Deep Sleep 진입**: 양쪽 Shift 누른 상태에서 S를 누름

### 3.2 LED 표시
| LED 패턴 | 의미 |
|----------|------|
| 1회 점멸 | 디바이스 1 활성 |
| 2회 점멸 | 디바이스 2 활성 |
| 3회 점멸 | 디바이스 3 활성 |
| 4회 점멸 | 디바이스 4 활성 |
| 느린 점멸 | 페어링 모드 |
| 빠른 점멸 | Deep Sleep 진입 중 |

---

## 4. 구현 계획

### 4.1 Magic Command 통합

#### 4.1.1 Magic Command 확장
```c
// command.c 확장
void command_extra(uint8_t code) {
    switch (code) {
        // Deep Sleep 및 멀티 디바이스 명령
        case KC_S:  // Magic + S: Deep Sleep
            enter_deep_sleep();
            break;
        case KC_1:  // Magic + 1: Device 1
        case KC_2:  // Magic + 2: Device 2
        case KC_3:  // Magic + 3: Device 3
        case KC_4:  // Magic + 4: Device 4
            switch_device(code - KC_1);
            break;
        case KC_P:  // Magic + P: Pairing mode
            enter_pairing_mode();
            break;
        case KC_D:  // Magic + D: Delete pairing
            delete_pairing_prompt();
            break;
        case KC_B:  // Magic + B: Bluetooth status
            show_bluetooth_status();
            break;
        case KC_W:  // Magic + W: Wake-only key
            set_wake_only_flag();
            break;
    }
}
```

#### 4.1.2 키 코드 매핑
```c
// Magic Command로 사용할 키 정의
#define MAGIC_KEY_SLEEP         KC_S
#define MAGIC_KEY_DEVICE_1      KC_1
#define MAGIC_KEY_DEVICE_2      KC_2
#define MAGIC_KEY_DEVICE_3      KC_3
#define MAGIC_KEY_DEVICE_4      KC_4
#define MAGIC_KEY_PAIR          KC_P
#define MAGIC_KEY_DELETE        KC_D
#define MAGIC_KEY_BT_STATUS     KC_B
#define MAGIC_KEY_WAKE_ONLY     KC_W
```

### 4.2 파일 구조
```
keyboard/hhkb/
├── power_management.c     # Deep Sleep 구현
├── power_management.h     # Deep Sleep 헤더
├── multi_device.c         # 멀티 디바이스 구현
├── multi_device.h         # 멀티 디바이스 헤더
├── rn42/
│   ├── rn42_sleep.c      # RN-42 Sleep 명령
│   └── rn42_multi.c      # RN-42 멀티 연결
└── config_rn42.h         # 설정 추가
```

### 4.2 주요 구현 사항

#### 4.2.1 Deep Sleep 구현
1. **타이머 인터럽트 설정**
   - 30초 idle 타이머 구현
   - 키 입력 시 타이머 리셋

2. **전원 관리**
   ```c
   void enter_deep_sleep(void) {
       // RN-42를 Deep Sleep 모드로
       rn42_command("SW,0320");
       
       // MCU Power-down 모드
       set_sleep_mode(SLEEP_MODE_PWR_DOWN);
       sleep_enable();
       sei();
       sleep_cpu();
       
       // 깨어난 후
       sleep_disable();
       rn42_wakeup();
       
       // 자동 재연결 처리
       if (config.auto_reconnect_on_wake) {
           handle_auto_reconnect();
       }
   }
   ```

3. **Wake-up 처리**
   ```c
   // Wake-up 인터럽트 핸들러
   ISR(INT6_vect) {
       uint8_t key = scan_key();
       if (key == config.wakeup_key) {  // Enter 키만 처리
           wake_up_flag = true;
       }
       // 다른 키는 무시하고 Deep Sleep 유지
   }
   ```
   - Enter 키만 외부 인터럽트로 감지
   - 빠른 복귀를 위한 초기화 최적화
   - Wake-up 키는 소비하고 전송하지 않음

4. **자동 재연결 구현**
   ```c
   void handle_auto_reconnect(void) {
       // Auto-Connect 비활성화 상태 확인
       rn42_command("SM,4");
       
       // 마지막 연결 디바이스 정보 로드
       uint8_t last_device = load_last_connected_device();
       
       // 재연결 시도
       for (int i = 0; i < 3; i++) {
           if (rn42_connect_to_address(devices[last_device].mac_address)) {
               // 연결 성공 - 버퍼된 키 전송
               send_buffered_keys();
               break;
           }
           _delay_ms(2000);
       }
   }
   ```

#### 4.2.2 멀티 디바이스 구현
1. **EEPROM 저장**
   - 4개 디바이스 정보 저장
   - 전원 차단 후에도 유지

2. **연결 관리**
   ```c
   bool switch_device(uint8_t device_num) {
       // 현재 연결 종료
       rn42_disconnect();
       
       // 새 디바이스 MAC 주소 설정
       rn42_set_remote_address(devices[device_num].mac_address);
       
       // 연결 시도
       return rn42_connect();
   }
   ```

3. **페어링 프로세스**
   - 페어링 모드 진입
   - 디바이스 검색 및 연결
   - 정보 저장

---

## 5. 테스트 계획

### 5.1 Deep Sleep 테스트
- [ ] 30초 타이머 정확도 검증
- [ ] 전력 소비 측정 (<50μA 목표)
- [ ] Wake-up 응답 시간 측정 (<100ms)
- [ ] 다양한 시나리오에서 안정성 테스트

### 5.2 멀티 디바이스 테스트
- [ ] 4개 디바이스 페어링 및 저장
- [ ] 디바이스 전환 속도 측정 (<2초)
- [ ] 자동 재연결 기능 검증
- [ ] 페어링 삭제 및 재페어링

### 5.3 통합 테스트
- [ ] Deep Sleep 중 디바이스 전환
- [ ] 배터리 수명 측정 (목표: 3개월 이상)
- [ ] 사용자 시나리오 테스트

---

## 6. 성능 목표

### 6.1 전력 효율성
- **대기 전류**: <50μA (Deep Sleep 모드)
- **활성 전류**: <15mA (타이핑 시)
- **배터리 수명**: 3개월 이상 (1000mAh 기준)

### 6.2 응답성
- **Wake-up 시간**: <100ms
- **디바이스 전환**: <2초
- **페어링 시간**: <10초

---

## 7. 위험 요소 및 대응 방안

### 7.1 기술적 위험
| 위험 요소 | 영향도 | 대응 방안 |
|-----------|--------|-----------|
| RN-42 호환성 문제 | 높음 | 펌웨어 버전별 테스트 |
| 전력 소비 목표 미달성 | 중간 | 추가 최적화 및 하드웨어 개선 |
| 메모리 부족 | 낮음 | 코드 최적화 및 기능 우선순위 조정 |

### 7.2 사용성 위험
| 위험 요소 | 영향도 | 대응 방안 |
|-----------|--------|-----------|
| 복잡한 키 조합 | 중간 | 직관적인 키 매핑 및 문서화 |
| 예기치 않은 Sleep 진입 | 낮음 | 설정 가능한 타이머 제공 |
| Auto-Connect 충돌 | 중간 | DTR 모드 사용 및 수동 제어 |
| Wake-up 키 손실 | 낮음 | 키 버퍼링 메커니즘 구현 |

---

## 8. 향후 개선 사항

### 8.1 단계별 출시 계획
- **Phase 1** (v1.0): Deep Sleep 기본 기능
- **Phase 2** (v1.1): 멀티 디바이스 4개 지원
- **Phase 3** (v2.0): 고급 전원 관리 및 6개 디바이스 지원

### 8.2 추가 기능 검토
- 배터리 잔량 표시
- 적응형 Sleep 타이머
- 디바이스별 커스텀 설정
- OTA 펌웨어 업데이트

---

## 9. 참고 자료

### 9.1 기술 문서
- [RN-42 Advanced User Guide](http://ww1.microchip.com/downloads/en/DeviceDoc/rn-42-ds-v2.32r.pdf)
- [ATmega32U4 Power Management](http://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-7766-8-bit-AVR-ATmega16U4-32U4_Datasheet.pdf)
- [TMK Keyboard Documentation](https://github.com/tmk/tmk_keyboard/wiki)

### 9.2 관련 프로젝트
- TMK Bluetooth 구현 예제
- 다른 키보드의 멀티 디바이스 구현 사례

---

## 10. 승인

| 역할 | 이름 | 서명 | 날짜 |
|------|------|------|------|
| 제품 관리자 | | | |
| 개발 리드 | | | |
| QA 리드 | | | |
| 프로젝트 매니저 | | | |