// diag.c - Trinamic stepper driver

#include "diag.h"

#ifdef _DIAG


#include <avr/pgmspace.h>
#include "uart.h"
#include "abtn3.h"
#include "tmc2130.h"
#include "main.h"
#include "tmc2130.h"


int8_t cmd_uart_bridge(FILE *inout)
{
    fprintf_P(inout, PSTR("UART bridge started, press any button to stop\n"));
    int c0;
    int c1;
    while (1) {
        c0 = getc(uart0io);
        if (c0 >= 0) {
            putc(c0, uart1io);
        }
        //      if (c0 >= 0) printf_P(PSTR("%d\n"), c0);
        c1 = getc(uart1io);
        //      if (c1 >= 0) putc(c1, uart0io);
        if (c1 >= 0) {
            printf_P(PSTR("%d\n"), c1);
        }
        process_signals();
        if (abtn_state) {
            break;
        }
    }
    fprintf_P(inout, PSTR("UART bridge terminated\n"));
    return 0;
}

int8_t cmd_diag_uart1(FILE *inout)
{
    if (inout == uart1io) {
        return -1;
    }
    fprintf_P(inout, PSTR("UART1 diag\n"));
    fprintf_P(inout, PSTR("connect loopback jumper and press enter..\n"));
    fflush(inout);
    while (getc(inout) >= 0); //clear rx buffer
    int c = 0;
    while (((c = (char)getc(inout)) != '\n') && (c != '\r')) {
        process_signals();    //wait enter
    }
    fflush(uart0io);
    while (getc(uart1io) >= 0); //clear rx buffer uart1
    uint16_t err_tmo = 0;
    uint16_t err_chr = 0;
    for (c = 0; c < 256; c++) {
        putc(c, uart1io); //send char
        uint8_t tmo = 100; //timeout
        int cr = 0; //received char
        while (((cr = getc(uart1io)) < 0) && (tmo--)) {
            process_signals();    //wait char
        }
        if (cr == c) { //char equal = OK
            fprintf_P(inout, PSTR(" 0x%02x OK\n"), c);
        } else if (cr < 0) { //timeout = NG
            fprintf_P(inout, PSTR(" 0x%02x NG! timeout\n"), c);
            err_tmo++;
        } else { //char not equal = NG
            fprintf_P(inout, PSTR(" 0x%02x NG! received 0x%02x\n"), c, cr);
            err_chr++;
        }
    }
    if ((err_tmo == 0) && (err_chr == 0)) {
        fprintf_P(inout, PSTR("SUCCED\n"));
    } else {
        fprintf_P(inout, PSTR("ok=%d\n"), 256 - (err_chr + err_tmo));
        fprintf_P(inout, PSTR("ne=%d\n"), err_chr);
        fprintf_P(inout, PSTR("to=%d\n"), err_tmo);
        fprintf_P(inout, PSTR("FAILED\n"));
    }
    return 0;
}

int8_t cmd_diag_tmc(FILE *inout, uint8_t axis)
{
    fprintf_P(inout, PSTR("TMC2130, axis %d diag\n"), axis);
    uint8_t check = tmc2130_check_axis(axis);
    const char *_ok = PSTR("OK");
    const char *_ng = PSTR("NG");
    fprintf_P(inout, PSTR(" SPI: %S\n"), (check & TMC2130_CHECK_SPI) ? _ok : _ng);
    fprintf_P(inout, PSTR(" MOT: %S\n"), (check & TMC2130_CHECK_SPI) ? _ok : _ng);
    fprintf_P(inout, PSTR(" STP: %S\n"), (check & TMC2130_CHECK_STP) ? _ok : _ng);
    fprintf_P(inout, PSTR(" DIR: %S\n"), (check & TMC2130_CHECK_STP) ? _ok : _ng);
    fprintf_P(inout, PSTR(" ENA: %S\n"), (check & TMC2130_CHECK_STP) ? _ok : _ng);
    return 0;
}

#endif //_DIAG
