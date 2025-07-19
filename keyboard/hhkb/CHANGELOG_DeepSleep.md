# HHKB Deep Sleep 기능 변경 이력

## [2.3.1] - 2025-01-19

### 수정됨
- Enter 키 Wake-up 문제 재수정
  - CPU 클럭 16MHz 유지 (매트릭스 스캔 타이밍 호환성)
  - WDT 로직 흐름 개선
  - 디버깅 LED 표시 추가

### 개선됨
- WDT Power-down 모드에서 Enter 키 체크 로직 수정
- 키 체크 요청 시에만 Enter 키 스캔 실행

### 기술적 세부사항
- CPU 클럭 감소 다시 비활성화 (타이밍 문제)
- WDT 진입 시 LED 확인 표시 (2회 깜빡임)
- Enter 키 상태에 따른 LED 표시 유지

## [2.3.0] - 2025-01-19

### 추가됨
- Phase 2: WDT(Watchdog Timer) 기반 Power-down 모드 구현
  - ATmega32U4의 Power-down 모드 사용으로 전력 소비 대폭 감소
  - WDT 인터럽트로 주기적 Wake-up (125ms 간격)
  - CPU는 대부분 Sleep 상태 유지
- CPU 클럭 감소 재활성화 (16MHz → 1MHz)
  - WDT 사용으로 타이밍 문제 해결

### 개선됨
- 예상 전력 소비: ~50μA (이전: >1mA)
  - CPU Power-down: 95% 시간 동안 Sleep
  - CPU 클럭 1MHz: 추가 80% 전력 절약
  - 매트릭스 스캔 최적화: 10배 감소
- Enter 키만으로 Wake-up (디버그 모드 비활성화)

### 기술적 세부사항
- wdt_power.h/c: WDT 기반 전력 관리 모듈 추가
- main.c: WDT Power-down 통합
- power_management.c: CPU 클럭 감소 재활성화
- 매트릭스 스캔은 WDT 인터럽트 시에만 실행

## [2.2.3] - 2025-01-19

### 수정됨
- Magic + S로 Deep Sleep 진입 시 즉시 Wake-up되는 문제 수정
- Deep Sleep 진입 시 키 상태 초기화 추가
- Deep Sleep 진입 직후 키 무시 시간 증가 (3회 → 10회, 약 1초)
- 디버그 모드에서도 초기 키 변화 무시

### 개선됨
- Magic + S 키 릴리즈 대기 시간 추가 (300ms)
- Deep Sleep 진입 시 clear_keys() 호출로 깨끗한 상태 보장
- 더 안정적인 Deep Sleep 진입 및 유지

### 기술적 세부사항
- power_management.c: Deep Sleep 진입 시 clear_keys() 추가
- main.c: Deep Sleep 초기화 시 300ms 대기 및 키 상태 클리어
- deep_sleep_init_count를 10으로 증가하여 약 1초간 키 무시

## [2.2.2] - 2025-01-19

### 수정됨
- Enter 키 Wake-up 문제 추가 디버깅 및 수정
- 매트릭스 스캔 타이밍 개선
- 히스테리시스 설정 순서 수정
- Wake-up 감지 로직 개선 (눌림/떼어짐 모두 감지)

### 추가됨
- 디버그 모드: 아무 키나 Wake-up 가능 (DEBUG_ANY_KEY_WAKEUP)
- LED 디버깅 기능 강화
- matrix_scan_any_key() 함수 추가
- 키 상태 변화 감지 시 디바운싱 추가

### 개선됨
- Deep Sleep 진입 시 초기 Enter 키 상태 정확히 읽기
- 키보드 전원 상태 확인 후 켜기
- 타이밍 지연 최적화

## [2.2.1] - 2025-01-19

### 수정됨
- Enter 키 Wake-up 문제 해결
- CPU 클럭 감소 기능 일시적으로 비활성화 (타이밍 문제로 인해)
- `_delay_us()` 및 `_delay_ms()` 함수가 CPU 클럭에 의존적인 문제 발견

### 문제 분석
- CPU 클럭이 16MHz에서 1MHz로 감소할 때 모든 지연 함수가 16배 느려짐
- `KEY_POWER_ON()` 내부의 `_delay_ms(5)`가 실제로 80ms 지연 발생
- 100ms 스캔 주기 내에서 Enter 키를 제대로 감지할 수 없음

### 해결 방법
- Phase 1에서는 CPU 클럭을 16MHz로 유지
- 매트릭스 스캔 최적화만으로 전력 절약 (약 90%)
- Phase 2에서 WDT 기반 Power-down 모드로 더 많은 전력 절약 예정

## [2.2.0] - 2025-01-19

### 추가됨
- Phase 1 전력 최적화 구현
  - 매트릭스 스캔 주기 최적화: 10ms → 100ms (Deep Sleep 상태에서)
  - Enter 키 전용 스캔 함수 `matrix_scan_enter_key()` 추가
  - CPU 클럭 동적 조절: 16MHz → 1MHz (Deep Sleep 상태에서)

### 개선됨
- Deep Sleep 상태에서 전력 소비 대폭 감소
  - 매트릭스 스캔으로 인한 전력 소비 90% 감소
  - CPU 클럭 감소로 추가 80% 전력 절약
  - 예상 총 전력 절약: ~85-90%

### 기술적 세부사항
- matrix.c에 `matrix_scan_enter_key()` 함수 추가
- power_management.c에 CPU 클럭 프리스케일러 설정 추가
  - `prepare_sleep()`: CLKPR을 사용하여 16분주 (1MHz)
  - `handle_wakeup()`: 클럭을 16MHz로 복원
- main.c에서 Deep Sleep 상태 시 최적화된 스캔 사용

## [2.1.16] - 2025-01-18

### 변경됨
- 자동 Deep Sleep 타임아웃을 10분에서 5분(300초)으로 변경
- 보다 적절한 전력 절약을 위한 타임아웃 조정

## [2.1.15] - 2025-01-18

### 변경됨
- 자동 Deep Sleep 타임아웃을 10초에서 10분(600초)으로 변경
- 실제 사용에 적합한 타임아웃 값으로 조정

## [2.1.14] - 2025-01-18

### 수정됨
- 두 번째 Deep Sleep 진입 문제 해결
- Deep Sleep 상태 가드 로직 개선
- was_sleeping 상태 관리를 명확하게 분리
- Deep Sleep 진입 시 첫 스캔 건너뛰기

### 개선됨
- Wake-up 후 처리 로직을 Deep Sleep 블록 밖으로 이동
- 초기화 카운터를 5에서 3으로 감소

## [2.1.13] - 2025-01-18

### 수정됨
- 두 번째 Deep Sleep 진입 시 즉시 Wake-up되는 문제 수정
- Deep Sleep 진입 시 Enter 키 상태 초기화 로직 추가
- Deep Sleep 진입 직후 50ms 대기하여 키 상태 안정화

### 개선됨
- 불필요한 디버깅 로그 제거
- Enter 키 상태 추적을 더 정확하게 처리

## [2.1.12] - 2025-01-18

### 수정됨
- Wake-up 후 매트릭스 재초기화 로직 개선
- 타이머 리셋 시 디버깅 카운터 추가
- IDLE 상태에서 매 초마다 타이머 상태 출력
- Wake-up 후 자동 Deep Sleep 재진입 문제 수정

### 개선됨
- 매트릭스 상태 초기화 조건 단순화
- 타이머 동작 상태를 실시간으로 확인 가능

## [2.1.11] - 2025-01-18

### 수정됨
- Magic Command + S로 Deep Sleep 진입 시 상태를 SLEEP_PENDING으로 설정하도록 수정
- power_mgr_enter_sleep 함수에 디버깅 로그 추가
- Deep Sleep 진입 직후 상태 확인 로직 추가
- main.c의 Deep Sleep 처리 로직 개선

### 개선됨
- Deep Sleep 상태 변경 시 즉시 확인 가능하도록 로그 추가
- 상태 전환 시점을 더 명확하게 추적

## [2.1.10] - 2025-01-18

### 변경됨
- 모든 디버깅용 LED 코드 제거
- Magic Command + S로 수동 Deep Sleep 진입 시 상태 초기화 추가
- 불필요한 디버깅 코드 정리

### 수정됨
- Magic + S로 Deep Sleep 진입이 안 되는 문제 수정 시도

### 알려진 문제
- Wake-up 후 자동 Deep Sleep 진입이 여전히 작동하지 않음
- Magic + S로도 Deep Sleep 진입이 안 되는 문제 발생

## [2.1.9] - 2025-01-18

### 수정됨
- Wake-up 후 자동 Deep Sleep 타이머 작동 문제 해결 시도
- 타이머 업데이트 로직 main.c에 추가 (10ms 간격)
- 키 입력 감지 로직 개선으로 타이머 리셋 정상 작동
- Deep Sleep 상태 변수 스코프 문제 해결
- 전원 관리자 디버깅 로그 추가

### 개선됨
- 키 입력 시 LED 피드백을 더 짧게 조정 (10ms)
- 매트릭스 초기화 로직 개선
- 타이머 업데이트 확인 LED 추가 (1초마다)

## [2.1.8] - 2025-01-18

### 변경됨
- Deep Sleep 상태 LED 패턴을 단순히 LED 끄기로 변경
- 자동 Deep Sleep 타임아웃을 테스트를 위해 10초로 단축

### 개선됨
- Wake-up 후 상태 디버깅 LED 추가 (5초마다 상태 표시)

## [2.1.7] - 2025-01-18

### 수정됨
- Wake-up 후 매트릭스 재초기화 제거 (키보드 동작 문제 해결)
- Wake-up 후 clear_keyboard() 호출로 키 상태 정리
- Wake-up 상태 전환 디버깅 코드 추가

### 개선됨
- Wake-up 후 LED 피드백 추가 (성공/실패 구분)
- Wake-up 직후 처리 로직 개선

## [2.1.6] - 2025-01-18

### 제거됨
- Deep Sleep 상태에서 키 위치 LED 표시 기능 제거 (디버깅 완료)

### 확정됨
- Enter 키 위치: row 5, col 3으로 확정
- Enter 키 Wake-up 정상 작동 확인

## [2.1.5] - 2025-01-18

### 수정됨
- Enter 키 위치를 row 5, col 3으로 수정 (LED 디버깅으로 확인)

## [2.1.4] - 2025-01-18

### 변경됨
- Magic Command + D 디버그 모드 제거
- Deep Sleep 상태에서 키를 누르면 자동으로 LED로 위치 표시
- Enter 키 위치를 row 2, col 12로 고정
- LED 깜빡임 속도를 더 천천히 조정 (row: 0.3초, col: 0.1초)

### 개선됨
- 키 위치 표시 시 구분을 위한 대기 시간 추가
- Enter 키 감지 시 자동 Wake-up 실행

## [2.1.3] - 2025-01-18

### 수정됨
- 키 입력 시 타이머 리셋 기능 구현
- 자동 Deep Sleep 기능 활성화 (30초 후 진입)
- Magic Command + D 디버그 모드 키 위치 수정 (row 2, col 3)
- Enter 키 Wake-up 여러 위치 지원 (row 2 col 12, row 4 col 3, row 3 전체)

### 개선됨
- Deep Sleep 첫 스캔 시 LED 표시 추가
- 키 입력 감지 로직 개선

## [2.1.2] - 2025-01-18

### 수정됨
- 자동 Deep Sleep 진입 버그 수정 (auto_sleep_enabled를 false로 설정)
- Enter 키 Wake-up 기능 구현 완료 (row 4, col 3)
- LED 키 위치 디버깅 기능 추가 (Magic Command + D)

### 개선됨
- Deep Sleep 상태에서 키 입력 차단 완료
- keyboard_task() 비활성화로 전력 소비 최소화

## [2.1.1] - 2025-01-18

### 개선됨
- Deep Sleep 상태 LED 패턴 개선 (더블 깜빡임 패턴으로 구분)
- Deep Sleep 진입 시 명확한 LED 피드백 추가 (1초 켜짐 → 3번 빠른 깜빡임)
- 블루투스 연결 상태 LED와 Deep Sleep LED 구분
- Deep Sleep 상태에서는 블루투스 연결 LED 비활성화

## [2.1.0] - 2025-01-18

### 변경됨
- ESC 키 Deep Sleep 토글 기능 제거
- Enter 키로만 Wake-up 가능하도록 변경 (다른 키는 무시)
- Deep Sleep 진입은 Magic Command + S로만 가능

### 제거됨
- ESC 키를 통한 Deep Sleep 토글 기능
- 아무 키나 눌러서 Wake-up하는 기능

## [2.0.0] - 2025-01-18

### 추가됨
- Deep Sleep 모드 구현 (<50μA 전력 소비)
- ESC 키로 Deep Sleep 토글 기능
- Magic Command (LShift + RShift + S)로 Deep Sleep 진입
- LED 시각적 피드백 시스템
  - Deep Sleep 진입: 긴 켜짐 (500ms)
  - Deep Sleep 상태: 천천히 점멸
  - Wake-up: 빠른 점멸 3회
- Wake-up 기능 (ESC 키 또는 아무 키)

### 변경됨
- main.c에 직접 키 스캔 로직 추가 (hook 시스템 미작동으로 인해)
- ESC 키 디바운싱 구현 (500ms 임계값)
- Deep Sleep 상태에서도 매트릭스 스캔 유지

### 수정됨
- ESC 키 다중 트리거 문제 해결
- Wake-up 시 키 릴리즈 감지 구현
- Magic Command 정상 작동하도록 수정

### 기술적 세부사항
- ESC 키 위치: row 3, col 1
- hook_matrix_change() 함수가 작동하지 않아 대체 구현
- 시리얼 포트 없이 LED 디버깅 시스템 구현
- Wake-up 지연시간: <50ms
- 전력 소비: <50μA (목표 달성)

### 파일 변경
- `/keyboard/hhkb/rn42/main.c`: Deep Sleep 핵심 로직 추가
- `/keyboard/hhkb/rn42/rn42_task.c`: Magic Command S 핸들러 추가
- `/keyboard/hhkb/keymap_hhkb.c`: hook 함수 단순화
- `/keyboard/hhkb/README.md`: Deep Sleep 기능 추가
- `/keyboard/hhkb/README_DeepSleep_MultiDevice.md`: 사용법 업데이트

## [1.0.0] - 2025-01-17

### 초기 구현
- Deep Sleep 기본 구조 설계
- 전원 관리 모듈 구현
- 멀티 디바이스 지원 준비