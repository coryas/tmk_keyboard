#include <avr/io.h>
#include <avr/pgmspace.h>
#include <string.h>
#include "host.h"
#include "host_driver.h"
#include "serial.h"
#include "rn42.h"
#include "print.h"
#include "timer.h"
#include "wait.h"


/* Host driver */
static uint8_t keyboard_leds(void);
static void send_keyboard(report_keyboard_t *report);
static void send_mouse(report_mouse_t *report);
static void send_system(uint16_t data);
static void send_consumer(uint16_t data);

host_driver_t rn42_driver = {
    keyboard_leds,
    send_keyboard,
    send_mouse,
    send_system,
    send_consumer
};


void rn42_init(void)
{
    // JTAG disable for PORT F. write JTD bit twice within four cycles.
    MCUCR |= (1<<JTD);
    MCUCR |= (1<<JTD);

    // PF7: BT connection control(high: connect, low: disconnect)
    rn42_autoconnect();

    // PF6: linked(input without pull-up)
    DDRF  &= ~(1<<6);
    PORTF |=  (1<<6);

    // PF1: RTS(low: allowed to send, high: not allowed)
    DDRF &= ~(1<<1);
    PORTF &= ~(1<<1);

    // PD5: CTS(low: allow to send, high:not allow)
    DDRD |= (1<<5);
    PORTD &= ~(1<<5);

    serial_init();
}

int16_t rn42_getc(void)
{
    return serial_recv2();
}

const char *rn42_gets(uint16_t timeout)
{
    static char s[24];
    uint16_t t = timer_read();
    uint8_t i = 0;
    int16_t c;
    while (i < 23 && timer_elapsed(t) < timeout) {
        if ((c = rn42_getc()) != -1) {
            if ((char)c == '\r') continue;
            if ((char)c == '\n') break;
            s[i++] = c;
        }
    }
    s[i] = '\0';
    return s;
}

void rn42_putc(uint8_t c)
{
    serial_send(c);
}

void rn42_puts(char *s)
{
    while (*s)
	serial_send(*s++);
}

bool rn42_autoconnecting(void)
{
    // GPIO6 for control connection(high: auto connect, low: disconnect)
    // Note that this needs config: SM,4(Auto-Connect DTR Mode)
    return (PORTF & (1<<7) ? true : false);
}

void rn42_autoconnect(void)
{
    // hi to auto connect
    DDRF |= (1<<7);
    PORTF |= (1<<7);
}

void rn42_disconnect(void)
{
    // low to disconnect
    DDRF |= (1<<7);
    PORTF &= ~(1<<7);
}

bool rn42_rts(void)
{
    // low when RN-42 is powered and ready to receive
    return PINF&(1<<1);
}

void rn42_cts_hi(void)
{
    // not allow to send
    PORTD |= (1<<5);
}

void rn42_cts_lo(void)
{
    // allow to send
    PORTD &= ~(1<<5);
}

bool rn42_linked(void)
{
    // RN-42 GPIO2
    //   Hi-Z:  Not powered
    //   High:  Linked
    //   Low:   Connecting
    return PINF&(1<<6);
}


static uint8_t leds = 0;
static uint8_t keyboard_leds(void) { return leds; }
void rn42_set_leds(uint8_t l) { leds = l; }


void rn42_send_str(const char *str)
{
    uint8_t c;
    while ((c = pgm_read_byte(str++)))
        rn42_putc(c);
}

const char *rn42_send_command(const char *cmd)
{
    static const char *s;
    rn42_send_str(cmd);
    wait_ms(500);
    s = rn42_gets(100);
    xprintf("%s\r\n", s);
    rn42_print_response();
    return s;
}

void rn42_print_response(void)
{
    int16_t c;
    while ((c = rn42_getc()) != -1) {
        xprintf("%c", c);
    }
}


static void send_keyboard(report_keyboard_t *report)
{
    // wake from deep sleep
/*
    PORTD |= (1<<5);    // high
    wait_ms(5);
    PORTD &= ~(1<<5);   // low
*/

    serial_send(0xFD);  // Raw report mode
    serial_send(9);     // length
    serial_send(1);     // descriptor type
    serial_send(report->mods);
    serial_send(0x00);
    serial_send(report->keys[0]);
    serial_send(report->keys[1]);
    serial_send(report->keys[2]);
    serial_send(report->keys[3]);
    serial_send(report->keys[4]);
    serial_send(report->keys[5]);
}

static void send_mouse(report_mouse_t *report)
{
    // wake from deep sleep
/*
    PORTD |= (1<<5);    // high
    wait_ms(5);
    PORTD &= ~(1<<5);   // low
*/

    serial_send(0xFD);  // Raw report mode
    serial_send(5);     // length
    serial_send(2);     // descriptor type
    serial_send(report->buttons);
    serial_send(report->x);
    serial_send(report->y);
    serial_send(report->v);
}

static void send_system(uint16_t data)
{
    // Table 5-6 of RN-BT-DATA-UB
    // 81,82,83 scan codes can be used?
}


static uint16_t usage2bits(uint16_t usage)
{
    switch (usage) {
        case APPCONTROL_HOME:         return 0x01;
        case APPLAUNCH_EMAIL:         return 0x02;
        case APPCONTROL_SEARCH:       return 0x04;
        //case AL_KBD_LAYOUT:         return 0x08;  // Apple virtual keybaord toggle
        case AUDIO_VOL_UP:            return 0x10;
        case AUDIO_VOL_DOWN:          return 0x20;
        case AUDIO_MUTE:              return 0x40;
        case TRANSPORT_PLAY_PAUSE:    return 0x80;
        case TRANSPORT_NEXT_TRACK:    return 0x100;
        case TRANSPORT_PREV_TRACK:    return 0x200;
        case TRANSPORT_STOP:          return 0x400;
        case TRANSPORT_STOP_EJECT:    return 0x800;
        case TRANSPORT_FAST_FORWARD:  return 0x1000;
        case TRANSPORT_REWIND:        return 0x2000;
        //case return 0x4000;   // Stop/eject
        //case return 0x8000;   // Internet browser
    };
    return 0;
}

static void send_consumer(uint16_t data)
{
    uint16_t bits = usage2bits(data);
    serial_send(0xFD);  // Raw report mode
    serial_send(3);     // length
    serial_send(3);     // descriptor type
    serial_send(bits&0xFF);
    serial_send((bits>>8)&0xFF);
}


/* Null driver for config_mode */
static uint8_t config_keyboard_leds(void);
static void config_send_keyboard(report_keyboard_t *report);
static void config_send_mouse(report_mouse_t *report);
static void config_send_system(uint16_t data);
static void config_send_consumer(uint16_t data);

host_driver_t rn42_config_driver = {
    config_keyboard_leds,
    config_send_keyboard,
    config_send_mouse,
    config_send_system,
    config_send_consumer
};

static uint8_t config_keyboard_leds(void) { return leds; }
static void config_send_keyboard(report_keyboard_t *report) {}
static void config_send_mouse(report_mouse_t *report) {}
static void config_send_system(uint16_t data) {}
static void config_send_consumer(uint16_t data) {}

/* 추가 함수 구현 - Deep Sleep 및 멀티 디바이스 지원 */

bool rn42_enter_cmd_mode(void) {
    // Deep Sleep 구현에서는 RN-42 명령 모드 진입 비활성화
    print("RN42 command mode disabled for Deep Sleep implementation\n");
    return false;
}

bool rn42_exit_cmd_mode(void) {
    // ---를 전송하여 명령 모드 종료
    SEND_STR("---\r\n");
    wait_ms(100);
    const char* response = rn42_gets(500);
    return (response && strstr(response, "END"));
}

bool rn42_is_connected(void) {
    return rn42_linked();
}

bool rn42_enter_pairing_mode(void) {
    if (!rn42_enter_cmd_mode()) return false;
    
    // 검색 가능하게 설정
    SEND_STR("SI,0200\r\n");  // Inquiry scan interval
    wait_ms(100);
    SEND_STR("SJ,0200\r\n");  // Page scan interval
    wait_ms(100);
    SEND_STR("I,60\r\n");     // 60초간 검색 가능
    wait_ms(100);
    
    return rn42_exit_cmd_mode();
}

bool rn42_exit_pairing_mode(void) {
    // 검색 불가능하게 설정
    if (!rn42_enter_cmd_mode()) return false;
    SEND_STR("Q,0\r\n");  // Quiet mode
    wait_ms(100);
    return rn42_exit_cmd_mode();
}

bool rn42_get_remote_address(char* addr) {
    if (!rn42_enter_cmd_mode()) return false;
    
    SEND_STR("GR\r\n");
    const char* response = rn42_gets(1000);
    
    if (response && strlen(response) >= 17) {
        // MAC 주소 형식: XX:XX:XX:XX:XX:XX
        strncpy(addr, response, 17);
        addr[17] = '\0';
        rn42_exit_cmd_mode();
        return true;
    }
    
    rn42_exit_cmd_mode();
    return false;
}

bool rn42_connect_to_address(const char* addr) {
    if (!rn42_enter_cmd_mode()) return false;
    
    // 연결 명령 전송
    rn42_send_str("C,");
    rn42_send_str(addr);
    rn42_send_str("\r\n");
    
    // 연결 응답 대기 (최대 5초)
    const char* response = rn42_gets(5000);
    
    rn42_exit_cmd_mode();
    return (response && strstr(response, "CONNECT"));
}
