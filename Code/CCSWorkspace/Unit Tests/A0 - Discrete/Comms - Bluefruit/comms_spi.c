#include <msp430.h>
#include <stdint.h>
#include "comms_spi.h"

#define SDEP_MSGTYPE_CMD 0x10
#define SDEP_MSGTYPE_RESPONSE 0x20
#define SDEP_MSGTYPE_ALERT 0x40
#define SDEP_MSGTYPE_ERROR 0x80
#define SDEP_MSGTYPE_GETRESPONSE 0xFF
#define SPI_IGNORED_BYTE 0xFE
#define SPI_OVERREAD_BYTE 0xFF

static uint8_t sdep_response[20] = {0};

void comms_sdep_msg(uint16_t cmd_type, char *msg) {
    uint8_t *start, *ptr;

    start = (uint8_t *)msg;
    ptr = (uint8_t *)msg;

    while (1) {
        if (*(ptr) == '\0') {
            if (start != ptr) {
                comms_sdep_packet(cmd_type, 0, ptr - start, start);
            }
            break;
        }
        if (ptr - start >= COMMS_SDEP_MAX_PAYLOAD) {
            comms_sdep_packet(cmd_type, 1, ptr - start, start);
            start = ptr;
        } else {
            ptr++;
        }
    }
}

void comms_sdep_packet(uint16_t cmd_type, uint8_t more_data,
                       uint8_t payload_size, uint8_t *payload) {
    int i;
    uint8_t b;

    P4OUT &= ~BIT1; // pull select low

    // Command header:
    while (comms_spi_byte(SDEP_MSGTYPE_CMD) == SPI_IGNORED_BYTE) {
        P4OUT |= BIT1; // release select
        // TODO: delay properly
        __delay_cycles(50);
        P4OUT &= ~BIT1; // pull select low
    }

    // Command bytes (lower byte first):
    comms_spi_byte(cmd_type & 0x00FF);
    comms_spi_byte((cmd_type & 0xFF00) >> 8);

    // Size byte, including the more_data flag and the payload_size:
    b = (payload_size > 16) ? 16 : payload_size;
    if (more_data) {
        comms_spi_byte(b | 0x80);
    } else {
        comms_spi_byte(b);
    }

    // Payload:
    for (i = 0; i < b; i++) {
        comms_spi_byte(*(payload + i));
    }

    P4OUT |= BIT1; // release select
}

void comms_reset() {
    P4OUT &= ~BIT3;
    // TODO: use something other than delay cycles?
    __delay_cycles(10);
    P4OUT |= BIT3;
}

uint8_t *comms_get_sdep_packet() {
    unsigned int i, j;

    // Clear response memory
    for (i = 0; i < 20; i++) {
        sdep_response[i] = 0;
    }

    // Check that IRQ is asserted.
    if (!(P4IN & BIT2)) {
        return sdep_response;
    }

    // pull select low
    P4OUT &= ~BIT1;

    // Get first byte:
    sdep_response[0] = comms_spi_byte(SDEP_MSGTYPE_GETRESPONSE);
    while (sdep_response[0] == SPI_IGNORED_BYTE) {
        // release select
        P4OUT |= BIT1;
        // wait a short amount of time
        __delay_cycles(10);
        // pull select low
        P4OUT &= ~BIT1;
    }
    if (sdep_response[0] == SPI_OVERREAD_BYTE) {
        return sdep_response;
    }

    // Get command id:
    sdep_response[1] = comms_spi_byte(SDEP_MSGTYPE_GETRESPONSE);
    sdep_response[2] = comms_spi_byte(SDEP_MSGTYPE_GETRESPONSE);

    // Get payload size:
    sdep_response[3] = comms_spi_byte(SDEP_MSGTYPE_GETRESPONSE);

    // Get the rest of the payload:
    j = sdep_response[3] & 0x7F; // exclude more data bit
    j = ((j > COMMS_SDEP_MAX_PAYLOAD) ? COMMS_SDEP_MAX_PAYLOAD : j) + 4;
    for (i = 4; i < j; i++) {
        sdep_response[i] = comms_spi_byte(SDEP_MSGTYPE_GETRESPONSE);
    }

    // release select
    P4OUT |= BIT1;

    return sdep_response;
}

void init_comms_spi() {
    /*
     * Ports Used:
     *
     * P5.0 - UCB1SIMO
     * P5.1 - UCB1SOMI
     * P5.2 - UCB1CLK
     * P4.1 - GPIO Output - CS
     * P4.2 - GPIO Input - IRQ
     * P4.3 - GPIO Output - RST
     */
    UCB1CTLW0 |= UCSWRST; // Hold eUSCI module in reset state.
    UCB1CTLW0 = UCCKPH | UCMSB | UCMST | UCSYNC | UCSSEL | UCSWRST; // Setup eUSCI module.
    //UCB1BRW = 4;
    P5SEL0 |= BIT0 | BIT1 | BIT2; // Assign P5.0-2 to the eUSCI module.
    P4DIR |= BIT1 | BIT3; // Output: CS, RST
    P4DIR &= ~BIT2; // Input: IRQ
    P4OUT |= BIT1 | BIT3; // CS, RST should start out high.
    // Enable IRQ interrupt:
    P4IFG &= ~BIT2;
    P4IES &= ~BIT2;
    P4IE |= BIT2;
    UCB1CTLW0 &= ~UCSWRST; // Enable eUSCI module.
}

uint8_t comms_spi_byte(uint8_t b) {
    UCB1TXBUF = b;

    // TODO: make this use interrupts, not polling:
    while (UCB1STATW & UCBUSY);

    return UCB1RXBUF;
}
