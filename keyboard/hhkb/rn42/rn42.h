#ifndef RN42_H
#define RN42_H

#include <stdbool.h>
#include "host_driver.h"

host_driver_t rn42_driver;
host_driver_t rn42_config_driver;

void rn42_init(void);
int16_t rn42_getc(void);
const char *rn42_gets(uint16_t timeout);
void rn42_putc(uint8_t c);
void rn42_puts(char *s);
bool rn42_autoconnecting(void);
void rn42_autoconnect(void);
void rn42_disconnect(void);
bool rn42_rts(void);
void rn42_cts_hi(void);
void rn42_cts_lo(void);
bool rn42_linked(void);
void rn42_set_leds(uint8_t l);

const char *rn42_send_command(const char *cmd);
void rn42_send_str(const char *str);
void rn42_print_response(void);
#define SEND_STR(str)       rn42_send_str(PSTR(str))
#define SEND_COMMAND(cmd)   rn42_send_command(PSTR(cmd))

/* 추가 함수들 - Deep Sleep 및 멀티 디바이스 지원 */
bool rn42_enter_cmd_mode(void);
bool rn42_exit_cmd_mode(void);
bool rn42_is_connected(void);
bool rn42_enter_pairing_mode(void);
bool rn42_exit_pairing_mode(void);
bool rn42_get_remote_address(char* addr);
bool rn42_connect_to_address(const char* addr);

#endif
