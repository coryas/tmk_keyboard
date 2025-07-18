# HHKB Deep Sleep 빌드 지침

## 빌드 환경 설정

### macOS
```bash
# Homebrew로 AVR 툴체인 설치
brew tap osx-cross/avr
brew install avr-gcc
brew install dfu-programmer

# 빌드 도구 확인
avr-gcc --version
dfu-programmer --version
```

### Linux
```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install gcc-avr avr-libc dfu-programmer

# Arch Linux
sudo pacman -S avr-gcc avr-libc dfu-programmer
```

## 빌드 방법

### 1. 소스 코드 준비
```bash
# TMK 저장소 클론
git clone https://github.com/tmk/tmk_keyboard.git
cd tmk_keyboard/keyboard/hhkb
```

### 2. Deep Sleep 기능 활성화
Makefile에서 다음 옵션이 활성화되어 있는지 확인:
```makefile
# Deep Sleep 및 멀티 디바이스 활성화
OPT_DEFS += -DDEEP_SLEEP_ENABLE
OPT_DEFS += -DMULTI_DEVICE_ENABLE
```

### 3. 펌웨어 빌드
```bash
# 기본 빌드 (HHKB Professional 2)
make clean
make KEYMAP=hhkb

# RN-42 블루투스 버전
make -f Makefile.rn42 clean
make -f Makefile.rn42 KEYMAP=hhkb

# 또는 한 줄로
make clean && make KEYMAP=hhkb RN42_ENABLE=yes
```

### 4. 빌드 결과 확인
```bash
# 펌웨어 크기 확인 (32KB 이하여야 함)
avr-size hhkb.elf

# 예상 출력:
#    text    data     bss     dec     hex filename
#   34938     224    1132   36294    8dc6 hhkb.elf
```

## 펌웨어 업로드

### 1. DFU 모드 진입
- 키보드 뒷면의 빨간 리셋 버튼 누르기
- 또는 Fn + Q 키 조합 (bootmagic 활성화 시)

### 2. 펌웨어 업로드
```bash
# dfu-programmer 사용
dfu-programmer atmega32u4 erase --force
dfu-programmer atmega32u4 flash hhkb.hex
dfu-programmer atmega32u4 launch

# 또는 make 명령 사용
make dfu

# RN-42 버전
make -f Makefile.rn42 dfu
```

### 3. 업로드 확인
- LED가 정상적으로 작동하는지 확인
- Magic Command + S로 Deep Sleep 진입 테스트
- Enter 키로 Wake-up 테스트
- 다른 키가 Wake-up하지 않는지 확인

## 문제 해결

### 컴파일 오류
1. **avr-gcc를 찾을 수 없음**
   ```bash
   # PATH에 추가
   export PATH="/usr/local/opt/avr-gcc/bin:$PATH"
   ```

2. **펌웨어 크기 초과**
   ```bash
   # 콘솔 지원 비활성화
   make CONSOLE_ENABLE=no
   ```

### 업로드 오류
1. **장치를 찾을 수 없음**
   - DFU 모드로 제대로 진입했는지 확인
   - USB 케이블 연결 확인
   - 다른 USB 포트 시도

2. **권한 오류 (Linux)**
   ```bash
   # udev 규칙 추가
   sudo echo 'SUBSYSTEM=="usb", ATTR{idVendor}=="03eb", ATTR{idProduct}=="2ff4", MODE="0666"' > /etc/udev/rules.d/50-atmel-dfu.rules
   sudo udevadm control --reload-rules
   ```

## 디버깅

### LED 디버깅 활성화
main.c에서 LED 디버깅 코드가 포함되어 있음:
- PORTE 핀 6 사용 (Caps Lock LED)
- 다양한 패턴으로 상태 표시

### 시리얼 디버깅 (사용 가능한 경우)
```bash
# macOS에서 시리얼 포트 찾기
ls /dev/tty.usb*

# screen으로 연결
screen /dev/tty.usbserial-* 115200
```

## 권장 사항

1. **백업**: 펌웨어 업로드 전 현재 설정 백업
2. **테스트**: 새 기능은 먼저 작은 변경으로 테스트
3. **문서화**: 변경사항은 CHANGELOG에 기록

## 참고 자료
- [TMK 공식 문서](https://github.com/tmk/tmk_keyboard/wiki)
- [AVR 프로그래밍 가이드](http://www.atmel.com/Images/Atmel-2549-8-bit-AVR-Microcontroller-ATmega640-1280-1281-2560-2561_datasheet.pdf)
- [RN-42 데이터시트](http://ww1.microchip.com/downloads/en/DeviceDoc/rn-42-ds-v2.32r.pdf)