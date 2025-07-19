RN42_DIR = rn42

SRC +=  serial_uart.c \
	rn42/suart.S \
	rn42/rn42.c \
	rn42/rn42_task.c \
	rn42/battery.c \
	rn42/main.c \
	rn42/rn42_sleep.c \
	power_management.c \
	multi_device.c \
	wdt_power.c

OPT_DEFS += -DPROTOCOL_RN42

# Deep Sleep 및 멀티 디바이스 기능 활성화
OPT_DEFS += -DDEEP_SLEEP_ENABLE
OPT_DEFS += -DMULTI_DEVICE_ENABLE
OPT_DEFS += -DPOWER_DEBUG

VPATH += $(RN42_DIR)
