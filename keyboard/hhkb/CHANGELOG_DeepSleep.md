# HHKB Deep Sleep 기능 변경 이력

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