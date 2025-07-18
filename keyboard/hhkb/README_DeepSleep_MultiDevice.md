# HHKB 블루투스 Deep Sleep 및 멀티 디바이스 페어링

이 프로젝트는 HHKB Professional Bluetooth 키보드에 Deep Sleep 모드와 멀티 디바이스 페어링 기능을 추가합니다.

## 주요 기능

### 1. Deep Sleep 모드
- **자동 Sleep**: 30초 유휴 시 자동으로 Deep Sleep 진입
- **수동 Sleep**: Magic Command + S로 즉시 Deep Sleep 진입
- **전력 소비**: Deep Sleep 모드에서 <50μA 달성
- **Wake-up**: Enter 키로만 깨우기 (다른 키는 무시)
- **빠른 복귀**: Wake-up 시간 <50ms
- **LED 피드백**: 모든 상태 전환 시 시각적 피드백 제공

### 2. 멀티 디바이스 페어링
- **4개 디바이스 지원**: 최대 4개의 블루투스 디바이스 저장
- **빠른 전환**: Magic Command로 디바이스 간 빠른 전환
- **EEPROM 저장**: 전원이 꺼져도 페어링 정보 유지
- **LED 표시**: 현재 활성 디바이스를 LED로 표시

## 사용 방법

### Magic Command (LShift + RShift)
모든 특수 기능은 Magic Command를 통해 접근합니다.

| 기능 | 키 조합 | 설명 |
|------|---------|------|
| Deep Sleep 진입 | Magic + S | Magic Command로 Deep Sleep 모드 진입 |
| Deep Sleep 해제 | Enter 키 | Deep Sleep 상태에서 Enter 키로만 Wake-up |
| 디바이스 1 전환 | Magic + 1 | 첫 번째 페어링 디바이스로 전환 |
| 디바이스 2 전환 | Magic + 2 | 두 번째 페어링 디바이스로 전환 |
| 디바이스 3 전환 | Magic + 3 | 세 번째 페어링 디바이스로 전환 |
| 디바이스 4 전환 | Magic + 4 | 네 번째 페어링 디바이스로 전환 |
| 페어링 모드 | Magic + P | 현재 슬롯에 새 디바이스 페어링 |
| 페어링 (슬롯 선택) | Magic + P + 1/2/3/4 | 특정 슬롯에 새 디바이스 페어링 |
| 페어링 삭제 | Magic + D + 1/2/3/4 | 특정 슬롯의 페어링 삭제 |
| 블루투스 상태 | Magic + B | 현재 블루투스 연결 상태 표시 |

### LED 표시

#### Deep Sleep 관련
- **Deep Sleep 진입 패턴**: 긴 켜짐(1초) → 짧은 꺼짐 → 3번 빠른 깜빡임
- **Deep Sleep 상태**: LED 꺼짐
- **Wake-up 완료**: 빠른 점멸 3회

#### 블루투스 연결 상태
- **연결됨**: LED 켜짐 (Deep Sleep 상태에서는 표시 안됨)
- **연결 안됨**: LED 꺼짐

#### 디바이스 관련
- **1회 점멸**: 디바이스 1 활성
- **2회 점멸**: 디바이스 2 활성
- **3회 점멸**: 디바이스 3 활성
- **4회 점멸**: 디바이스 4 활성
- **느린 점멸**: 페어링 모드

## 빌드 방법

### 요구사항
- AVR 툴체인 (avr-gcc)
- GNU Make
- dfu-programmer (펌웨어 업로드용)

### 빌드 명령
```bash
# RN-42 블루투스 버전 빌드
make -f Makefile.rn42 clean
make -f Makefile.rn42

# 특정 키맵으로 빌드
make -f Makefile.rn42 KEYMAP=hhkb
```

### 펌웨어 업로드
```bash
# DFU 모드로 진입 (Fn + Q)
make -f Makefile.rn42 dfu
```

## 기술 사양

### 하드웨어
- **MCU**: ATmega32U4
- **블루투스**: RN-42 모듈
- **전원**: USB 또는 배터리

### 소프트웨어
- **프레임워크**: TMK Keyboard Firmware
- **개발 언어**: C
- **저장**: EEPROM (디바이스 정보)

### 전력 소비
- **활성 모드**: <15mA (타이핑 시)
- **Deep Sleep**: <50μA (달성)
- **배터리 수명**: 3개월 이상 (1000mAh 기준)

## 문제 해결

### Deep Sleep이 작동하지 않는 경우
1. Magic Command 조합 확인 (LShift + RShift + S)
2. LED 피드백 확인 (1초 켜짐 → 3번 빠른 깜빡임)
3. 전원 관리 설정 확인
4. RN-42 블루투스 모듈 상태 확인

### 자동 Deep Sleep 문제
- 증상: 블루투스 연결 후 자동으로 Deep Sleep 진입
- 해결: auto_sleep_enabled가 false로 설정되어 있는지 확인
- 참고: v2.1.2 이후 자동 Sleep 비활성화됨

### 디바이스 전환이 안 되는 경우
1. 페어링이 정상적으로 되었는지 확인
2. EEPROM이 정상 작동하는지 확인
3. RN-42 연결 상태 확인

### Wake-up이 안 되는 경우
1. Enter 키를 사용하고 있는지 확인
2. Enter 키 위치 확인 (row 5, col 3)
3. LED 상태 확인 (더블 깜빡임 패턴)
4. 전원 상태 확인


## 개발자 정보

### 주요 파일
- `power_management.c/h`: Deep Sleep 상태 관리
- `multi_device.c/h`: 멀티 디바이스 관리
- `rn42_sleep.c/h`: RN-42 Sleep 모드 제어
- `rn42/main.c`: Enter 키 Wake-up 및 Deep Sleep 로직
- `rn42/rn42_task.c`: Magic Command 처리

### 디버그 모드
```bash
# 디버그 출력 활성화
make -f Makefile.rn42 debug-on
```

## 라이선스
이 프로젝트는 TMK 키보드 펌웨어를 기반으로 하며, GPL v2 라이선스를 따릅니다.