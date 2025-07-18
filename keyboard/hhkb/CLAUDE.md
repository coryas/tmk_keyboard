# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 프로젝트 개요

이것은 HHKB(Happy Hacking Keyboard) Professional 시리즈를 위한 대체 컨트롤러 펌웨어입니다. TMK(Too Many Keys) 프레임워크를 기반으로 합니다.

- **대상 하드웨어**: HHKB Professional, Professional 2, JP, Type-S
- **MCU**: ATmega32U4
- **부트로더**: Atmel DFU (4096 바이트)

## 필수 빌드 명령어

```bash
# 펌웨어 빌드
make -f Makefile.<variant> KEYMAP=<name> clean
make -f Makefile.<variant> KEYMAP=<name>

# 펌웨어 업로드 (컨트롤러의 빨간 리셋 버튼을 먼저 누르세요)
make -f Makefile.<variant> KEYMAP=<name> dfu     # dfu-programmer 사용
make -f Makefile.<variant> KEYMAP=<name> flip    # Atmel FLIP 사용

# 디버그 모드 토글
make debug-on
make debug-off
```

**Makefile 변형**:
- `Makefile` - Pro2/Pro USB 컨트롤러
- `Makefile.jp` - JP 레이아웃
- `Makefile.rn42` - Pro2 Bluetooth (RN-42)
- `Makefile.rn42.jp` - JP Bluetooth
- `Makefile.unimap.*` - 범용 키맵 지원

## 코드 아키텍처

### 핵심 구조
```
keyboard/hhkb/
├── matrix.c          # 키 스캔 매트릭스 구현 (HHKB 특유의 capacitive 스위치)
├── config.h          # 하드웨어 설정 (매트릭스 크기, USB ID, 기능 플래그)
├── config_rn42.h     # Bluetooth RN-42 모듈 설정
├── keymap_*.c        # 키맵 정의 (각 레이어와 키 동작 정의)
├── unimap_*.c        # 범용 키맵 (TMK의 범용 키맵 시스템)
└── rn42/             # Bluetooth 통신 및 명령 처리
```

### 키맵 시스템
- **레이어 기반**: 여러 Fn 키를 통한 다중 레이어 지원
- **액션 매크로**: 키당 복잡한 동작 정의 가능
- **마우스 키**: 키보드로 마우스 제어
- **NKRO**: USB를 통한 N-Key Rollover 지원

새 키맵 생성 시 `keymap_<name>.c` 파일을 만들고 기존 키맵(예: `keymap_hhkb.c`)을 참조하세요.

### 하드웨어 인터페이스
- **매트릭스 스캔**: `matrix.c`에서 HHKB의 특수한 capacitive 스위치 읽기 구현
- **전원 관리**: 키보드가 대기 모드일 때 전력 소모 최소화
- **LED 제어**: Caps Lock 등의 상태 표시

### Bluetooth 지원 (RN-42)
- `rn42/` 디렉토리에 RN-42 모듈과의 통신 구현
- HID 프로파일을 통한 무선 키보드 기능
- 배터리 레벨 모니터링 및 전원 관리

## 개발 팁

### 키맵 테스트
단일 키맵만 빌드하려면:
```bash
make -f Makefile KEYMAP=mymap
```

### 펌웨어 크기 최적화
펌웨어가 너무 크면 `Makefile`에서 다음 기능을 비활성화할 수 있습니다:
- `CONSOLE_ENABLE = no`  # 디버그 콘솔
- `COMMAND_ENABLE = no`  # 매직 커맨드
- `MOUSEKEY_ENABLE = no` # 마우스 키

### 문제 해결
- 펌웨어 업로드 실패 시 컨트롤러를 다시 연결하고 리셋 버튼을 다시 눌러보세요
- 키맵이 작동하지 않으면 `keymap_common.h`의 매크로 정의를 확인하세요
- Bluetooth 연결 문제는 `config_rn42.h`의 설정을 검토하세요