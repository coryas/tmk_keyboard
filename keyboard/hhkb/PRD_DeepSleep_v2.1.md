# Deep Sleep 및 Wake-up 구현 문서 v2.1

## PRD (제품 요구사항 문서)

### 1. 제품 개요
**제품명**: HHKB Professional BT Deep Sleep 기능
**버전**: 2.1
**목적**: 블루투스 키보드의 배터리 수명을 극대화하기 위한 Deep Sleep 모드 구현

### 2. 기능 요구사항

#### 2.1 Deep Sleep 진입
- **FR-001**: Magic Command를 통한 Deep Sleep 진입
  - LShift + RShift + S 조합으로 진입
  - 명령 실행 시 LED 피드백 (7회 점멸)
  - Deep Sleep 진입 확인 LED (긴 켜짐)

#### 2.2 Wake-up 기능
- **FR-002**: Enter 키를 통한 Wake-up
  - Deep Sleep 상태에서 Enter 키로만 깨우기
  - 다른 키는 무시됨
  - 키 릴리즈 시점에 Wake-up 실행
  - Wake-up 시 LED 피드백 (빠른 점멸 3회)

#### 2.3 시각적 피드백
- **FR-003**: 상태 표시 LED
  - Deep Sleep 진입: 긴 켜짐 (500ms)
  - Deep Sleep 상태: 천천히 점멸
  - Wake-up: 빠른 점멸 3회

### 3. 비기능 요구사항
- **NFR-001**: 전력 소비 < 50μA (Deep Sleep 모드)
- **NFR-002**: Wake-up 응답 시간 < 100ms
- **NFR-003**: 안정적인 키 인식 (디바운싱 적용)
- **NFR-004**: Enter 키만 Wake-up 가능 (보안성 향상)

---

## 아키텍처 문서

### 1. 시스템 아키텍처

```
┌─────────────────────────────────────────────────┐
│                 애플리케이션 계층                 │
├─────────────────┬───────────────┬───────────────┤
│  Deep Sleep     │ Magic Command │  LED 제어     │
│   관리자        │    핸들러     │   시스템      │
├─────────────────┴───────────────┴───────────────┤
│             키보드 코어 (TMK)                    │
├─────────────────┬───────────────┬───────────────┤
│ 매트릭스 스캔   │   커맨드      │   호스트      │
│   시스템        │   시스템      │ 인터페이스    │
├─────────────────┴───────────────┴───────────────┤
│           하드웨어 추상화 계층                   │
├─────────────────┬───────────────┬───────────────┤
│  ATmega32U4     │    RN-42      │     LED       │
│   컨트롤러      │  블루투스     │  컨트롤러     │
└─────────────────┴───────────────┴───────────────┘
```

### 2. 주요 컴포넌트

#### 2.1 전원 관리 모듈
- **위치**: `/keyboard/hhkb/power_management.c`
- **책임**: Deep Sleep 상태 관리 및 전환
- **인터페이스**:
  ```c
  void power_mgr_enter_sleep(power_manager_t *mgr);
  void power_mgr_wake_up(power_manager_t *mgr);
  bool power_mgr_is_sleeping(power_manager_t *mgr);
  ```

#### 2.2 메인 제어 루프
- **위치**: `/keyboard/hhkb/rn42/main.c`
- **책임**: 
  - Enter 키 감지 및 Wake-up 처리
  - Deep Sleep 상태에서 매트릭스 스캔
  - 다른 키 입력 무시

#### 2.3 커맨드 핸들러
- **위치**: `/keyboard/hhkb/rn42/rn42_task.c`
- **책임**: Magic Command 처리
- **구현**: `command_extra()` 함수에서 KC_S 처리

### 3. 데이터 흐름

1. **일반 모드 → Deep Sleep**:
   ```
   Magic Command (LShift + RShift + S) → 
   Command Handler → LED 피드백 (7회) → 
   Deep Sleep 진입 → LED 긴 켜짐
   ```

2. **Deep Sleep → Wake-up**:
   ```
   Enter 키 입력 → 매트릭스 스캔 (저전력) → 
   Enter 키 감지 → Wake-up → LED 피드백 (3회) → 일반 모드
   ```

---

## Epic: Deep Sleep 모드 구현

**Epic ID**: HHKB-001
**제목**: HHKB 블루투스 키보드용 Deep Sleep 모드 구현
**목표**: 전력 소비를 줄여 배터리 수명 연장

### 사용자 스토리

1. **사용자로서**, 키보드를 사용하지 않을 때 수동으로 Deep Sleep 모드로 전환하여 배터리를 절약하고 싶다.
   - **인수 조건**:
     - Magic Command + S로 Deep Sleep 진입
     - LED를 통한 시각적 피드백
     - 즉시 Deep Sleep 모드 진입

2. **사용자로서**, Enter 키로 키보드를 깨워서 작업을 재개하고 싶다.
   - **인수 조건**:
     - Enter 키로만 Wake-up 가능
     - 다른 키는 무시됨
     - Wake-up 시간 < 100ms

3. **사용자로서**, 키보드의 전원 상태를 확인하여 Sleep 상태를 알고 싶다.
   - **인수 조건**:
     - LED가 각 상태별로 다른 패턴 표시
     - 모드 간 명확한 시각적 구분

---

## 기술적 세부사항

**주요 변경사항 (v2.1)**:
1. ESC 키 토글 기능 제거
2. Enter 키로만 Wake-up 가능
3. 다른 키 입력 무시 로직 추가
4. 자동 Deep Sleep 기능 (30초 유휴 시)

**구현 하이라이트**:
- 전력 소비: < 50μA 달성
- Wake-up 지연시간: < 50ms
- Enter 키 위치: row 5, col 3
- Magic Command: LShift + RShift + S
- 자동 Sleep: 30초 후 진입

**수정된 파일**:
1. `/keyboard/hhkb/rn42/main.c`: Enter 키 Wake-up 로직
2. `/keyboard/hhkb/rn42/rn42_task.c`: Magic Command 핸들러
3. `/keyboard/hhkb/power_management.c`: 전원 상태 관리