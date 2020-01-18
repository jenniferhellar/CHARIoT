#include <msp430.h>
#include <stdint.h>
#include "comms.h"
#include "pwr_control.h"

void comms_setup() {
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

    /*
     * Turn on Bluefruit and allow it to power up while setting up SPI:
     */
    pwr_control_comms(1);

    /*
     * Setup eUSCI_B1 module for SPI communications:
     */

    // Hold eUSCI module in reset state.
    UCB1CTLW0 |= UCSWRST;

    // Setup eUSCI module.
    UCB1CTLW0 = UCCKPH | UCMSB | UCMST | UCSYNC | UCSSEL | UCSWRST;

    // Assign P5.0-2 to the eUSCI module.
    P5SEL0 |= BIT0 | BIT1 | BIT2;

    // Output: CS, RST
    P4DIR |= BIT1 | BIT3;

    // Input: IRQ
    P4DIR &= ~BIT2;

    // CS, RST should start out high.
    P4OUT |= BIT1 | BIT3;

    // Enable IRQ interrupt:
    /*
    P4IFG &= ~BIT2;
    P4IES &= ~BIT2;
    P4IE |= BIT2;
    */

    // Enable eUSCI module.
    UCB1CTLW0 &= ~UCSWRST;

    /*
     * Communicate with Bluefruit and initialize it:
     */
    //__delay_cycles(1000000);
    comms_sdep_packet(COMMS_SDEP_CMD_INIT, 0, 0, 0);
    __delay_cycles(1000000);
    comms_sdep_msg(COMMS_SDEP_CMD_AT, "AT+GAPDEVNAME=CharIoT Node");
    __delay_cycles(1000000);
    comms_sdep_msg(COMMS_SDEP_CMD_AT, "AT+BLEPOWERLEVEL=4");
    __delay_cycles(1000000);
    comms_sdep_msg(COMMS_SDEP_CMD_AT, "AT+HWMODELED=MANUAL,OFF");
    __delay_cycles(1000000);
    comms_sdep_msg(COMMS_SDEP_CMD_AT, "ATZ");
    __delay_cycles(1000000);

    /*
     * Turn off Bluefruit:
     */
    pwr_control_comms(0);
}

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
            comms_sdep_packet(cmd_type, 1, COMMS_SDEP_MAX_PAYLOAD, start);
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
    while (comms_spi_byte(SDEP_MSGTYPE_CMD, 1) == SPI_IGNORED_BYTE) {
        P4OUT |= BIT1; // release select
        __delay_cycles(50);
        P4OUT &= ~BIT1; // pull select low
    }

    // Command bytes (lower byte first):
    comms_spi_byte(cmd_type & 0x00FF, 0);
    comms_spi_byte((cmd_type & 0xFF00) >> 8, 0);

    // Size byte, including the more_data flag and the payload_size:
    b = (payload_size > 16) ? 16 : payload_size;
    if (more_data) {
        comms_spi_byte(b | 0x80, 0);
    } else {
        comms_spi_byte(b, 0);
    }

    // Payload:
    for (i = 0; i < b; i++) {
        comms_spi_byte(*(payload + i), 0);
    }

    P4OUT |= BIT1; // release select
    __delay_cycles(100000); // Delay required for Bluefruit to work.
}

// Max response is 20 bytes, use 21 to guarantee string is NUL-terminated.
static char sdep_response[21] = {0};

char *comms_get_sdep_packet() {
    unsigned int i, j;

    // Clear response memory
    for (i = 20; i > 0;) {
        i--;
        sdep_response[i] = 0;
    }

    // Check that IRQ is asserted.
    if (!(P4IN & BIT2)) {
        return sdep_response;
    }

    // pull select low
    P4OUT &= ~BIT1;

    // Get first byte:
    sdep_response[0] = comms_spi_byte(SDEP_MSGTYPE_GETRESPONSE, 1);
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
    sdep_response[1] = comms_spi_byte(SDEP_MSGTYPE_GETRESPONSE, 1);
    sdep_response[2] = comms_spi_byte(SDEP_MSGTYPE_GETRESPONSE, 1);

    // Get payload size:
    sdep_response[3] = comms_spi_byte(SDEP_MSGTYPE_GETRESPONSE, 1);

    // Get the rest of the payload:
    j = sdep_response[3] & 0x7F; // exclude more data bit
    j = ((j > COMMS_SDEP_MAX_PAYLOAD) ? COMMS_SDEP_MAX_PAYLOAD : j) + 4;
    for (i = 4; i < j; i++) {
        sdep_response[i] = comms_spi_byte(SDEP_MSGTYPE_GETRESPONSE, 1);
    }

    // release select
    P4OUT |= BIT1;

    return sdep_response;
}

#define COMMS_SPI_STATE_IDLE 0
#define COMMS_SPI_STATE_TX 1
#define COMMS_SPI_STATE_RX 2
static uint8_t state = COMMS_SPI_STATE_IDLE;
static uint8_t tx_byte = 0x00;
static uint8_t wait_for_rx = 0;

uint8_t comms_spi_byte(uint8_t b, uint8_t wait_for_response) {
    // Clear any pending receive flag:
    UCB1IFG &= ~UCRXIFG;

    // Temporarily disable interrupts:
    __bic_SR_register(GIE);

    if (UCB1IFG & UCTXIFG) {
        // If SPI TX is ready now, just write the byte:
        UCB1TXBUF = b;
        if (wait_for_response) {
            // Enable receive interrupt:
            UCB1IE |= UCRXIE;
            // Set ISR state to RX mode:
            state = COMMS_SPI_STATE_RX;
        } else {
            // Don't wait for response, return now.
            __bis_SR_register(GIE);
            return 0;
        }
    } else {
        // SPI TX not ready now, set ISR state to TX mode:
        state = COMMS_SPI_STATE_TX;
        tx_byte = b;
        wait_for_rx = wait_for_response;
        // Enable transmit interrupt:
        UCB1IE |= UCTXIE;
    }

    // Globally enable interrupts and drop into LPM0:
    __bis_SR_register(LPM0_bits | GIE);

    if (wait_for_response) {
        return UCB1RXBUF;
    } else {
        return 0;
    }
}

#pragma vector = EUSCI_B1_VECTOR
__interrupt void comms_spi_isr() {
    switch (state) {
        case COMMS_SPI_STATE_TX:
            if (UCB1IFG & UCTXIFG) {
                // TX now ready, write the byte:
                UCB1TXBUF = tx_byte;
                // Disable transmit interrupt:
                UCB1IE &= ~UCTXIE;
                if (wait_for_rx) {
                    // Enable receive interrupt:
                    UCB1IE |= UCRXIE;
                    // Set ISR state to RX mode:
                    state = COMMS_SPI_STATE_RX;
                } else {
                    // Don't wait for response, wake up CPU now.
                    state = COMMS_SPI_STATE_IDLE;
                    __bic_SR_register_on_exit(LPM0_bits);
                }
            } else {
                // Control should not reach here. If it does, something is
                // wrong.
                UCB1IE &= ~UCTXIE;
                UCB1IE &= ~UCRXIE;
                state = COMMS_SPI_STATE_IDLE;
            }
            break;
        case COMMS_SPI_STATE_RX:
            // TX/RX cycle finished, wake up CPU
            UCB1IE &= ~UCRXIE;
            state = COMMS_SPI_STATE_IDLE;
            __bic_SR_register_on_exit(LPM0_bits);
            break;
        default:
            // Module interrupting when it should not, disable interrupts.
            UCB1IE = 0;
            break;
    }
}
