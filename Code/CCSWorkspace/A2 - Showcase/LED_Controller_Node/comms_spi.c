#include <msp430.h>
#include <stdint.h>
#include <stdlib.h>
#include "comms_spi.h"
#include "pwr_control.h"

/*
 * Sleep level to use
 */
#define SPI_SLEEP_LEVEL  LPM0_bits

/*
 * SPI result variable
 */
#define SPI_RESULT_UNFINISHED  0
#define SPI_RESULT_ERROR       1
#define SPI_RESULT_SUCCESS     2
static uint8_t spi_result = SPI_RESULT_UNFINISHED;

/*
 * SPI next requested command
 */
#define SPI_COMMAND_END    0xA0
#define SPI_COMMAND_READ   0xA1
#define SPI_COMMAND_WRITE  0xA2
static uint8_t spi_next_command = SPI_COMMAND_END;

/*
 * Bytes used in SPI transactions
 */
#define COMMS_SPI_NULL_BYTE 0x00
#define COMMS_SPI_TRANSACTION_REQUEST 0x50
#define COMMS_SPI_TRANSACTION_RESPONSE 0x51

/*
 * SPI state variables
 */
#define SPI_STATE_IDLE                0
#define SPI_STATE_TRANSACT_TYPE       1
#define SPI_STATE_READ_RESPONSE_SIZE  2
#define SPI_STATE_READ_RESPONSE_DATA  3
#define SPI_STATE_WRITE_SIZE          4
#define SPI_STATE_WRITE_DATA          5
#define SPI_STATE_FINISHED            6
static uint8_t spi_state = SPI_STATE_IDLE;
static uint16_t spi_size = 0;
static char *spi_data;
static int error_num;

/*
 * Timeout counter
 */
static int timeout_counter = 0;

void comms_spi_init() {
    // Hold the eUSCI module in software reset:
    CONTROL_REGISTER |= UCSWRST;

    // Disable SPI pins:
    SOMI_SELECT &= ~SOMI_BIT;
    SIMO_SELECT &= ~SIMO_BIT;
    CLK_SELECT &= ~CLK_BIT;

    // Set SPI pins to output
    SOMI_DIR |= SOMI_BIT;
    SIMO_DIR |= SIMO_BIT;
    CLK_DIR |= CLK_BIT;
    IO1_DIR |= IO1_BIT;
    IO2_DIR |= IO2_BIT;

    // Set SPI pins to GND
    SOMI_OUT &= ~SOMI_BIT;
    SIMO_OUT &= ~SIMO_BIT;
    CLK_OUT &= ~CLK_BIT;
    IO1_OUT &= ~IO1_BIT;
    IO2_OUT &= ~IO2_BIT;

    // Setup IO1 interrupt:
    IO1_IES |= IO1_BIT;

    // Update the control register
    CONTROL_REGISTER = UCSWRST | UCCKPH_0 | UCCKPL_1 | UCMSB_1 | UCMST_0 | UCMODE_0 | UCSYNC_1;

    // Release the eUSCI module software reset:
    CONTROL_REGISTER &= ~UCSWRST;
}

uint8_t comms_spi_write(char *data, uint16_t len) {
    // Keep GIE suppressed for now, until ready:
    __bic_SR_register(GIE);

    // First, turn on the power to the comms chip so it can be starting up:
    pwr_control_comms(1);
    __delay_cycles(16000000);

    // Enable output pins:
    SOMI_SELECT |= SOMI_BIT;
    SIMO_SELECT |= SIMO_BIT;
    CLK_SELECT |= CLK_BIT;
    IO1_DIR &= ~IO1_BIT;
    IO2_DIR &= ~IO2_BIT;

    // Setup 1khz timer A0 (to space out measurements):
    TB0R = 0;
    TB0CCR0 = 62500;                        // 1 second interval
    TB0CCTL0 = CCIE;                        // TACCR0 interrupt enabled
    timeout_counter = 10; // seconds before timeout
    TB0CTL = TASSEL__SMCLK | ID_3 | MC__UP;  // SMCLK, UP mode, 500kHz/8 = 62.5Hz

    // Enable interrupts:
    P1IFG &= ~IO1_BIT;
    IO1_IE |= IO1_BIT;
    IFG_REGISTER &= ~UCRXIFG;
    IE_REGISTER |= UCRXIE;

    // Prepare for first SPI byte:
    //  MSP   >> 0x00
    //  COMMS << transaction type
    TXBUF_REGISTER = COMMS_SPI_NULL_BYTE;
    spi_state = SPI_STATE_TRANSACT_TYPE;
    spi_data = data;
    spi_size = len;
    spi_next_command = SPI_COMMAND_WRITE;
    spi_result = SPI_RESULT_UNFINISHED;

    // Enable GIE, go to sleep.
    __bis_SR_register(GIE + SPI_SLEEP_LEVEL);
    __no_operation();
    if (spi_result == SPI_RESULT_ERROR) {
        // If there was an error:
        IO1_IE &= ~IO1_BIT;
        IE_REGISTER &= ~UCRXIFG;
        pwr_control_comms(0);
        TB0CTL &= ~MC__UP; // Stop timer
        TB0CCTL0 &= ~CCIE;
        // Disable SPI pins:
        SOMI_SELECT &= ~SOMI_BIT;
        SIMO_SELECT &= ~SIMO_BIT;
        CLK_SELECT &= ~CLK_BIT;
        // Set SPI pins to output
        SOMI_DIR |= SOMI_BIT;
        SIMO_DIR |= SIMO_BIT;
        CLK_DIR |= CLK_BIT;
        IO1_DIR |= IO1_BIT;
        IO2_DIR |= IO2_BIT;
        // Set SPI pins to GND
        SOMI_OUT &= ~SOMI_BIT;
        SIMO_OUT &= ~SIMO_BIT;
        CLK_OUT &= ~CLK_BIT;
        IO1_OUT &= ~IO1_BIT;
        IO2_OUT &= ~IO2_BIT;
        __bis_SR_register(GIE);
        return 0;
    } else {
        spi_next_command = SPI_COMMAND_END;
        TXBUF_REGISTER = COMMS_SPI_NULL_BYTE;
        spi_state = SPI_STATE_TRANSACT_TYPE;
        __bis_SR_register(GIE + SPI_SLEEP_LEVEL);
        IO1_IE &= ~IO1_BIT;
        IE_REGISTER &= ~UCRXIFG;
        __delay_cycles(1000000);
        pwr_control_comms(0);
        TB0CTL &= ~MC__UP; // Stop timer
        TB0CCTL0 &= ~CCIE;
        // Disable SPI pins:
        SOMI_SELECT &= ~SOMI_BIT;
        SIMO_SELECT &= ~SIMO_BIT;
        CLK_SELECT &= ~CLK_BIT;
        // Set SPI pins to output
        SOMI_DIR |= SOMI_BIT;
        SIMO_DIR |= SIMO_BIT;
        CLK_DIR |= CLK_BIT;
        IO1_DIR |= IO1_BIT;
        IO2_DIR |= IO2_BIT;
        // Set SPI pins to GND
        SOMI_OUT &= ~SOMI_BIT;
        SIMO_OUT &= ~SIMO_BIT;
        CLK_OUT &= ~CLK_BIT;
        IO1_OUT &= ~IO1_BIT;
        IO2_OUT &= ~IO2_BIT;
        __bis_SR_register(GIE);
        return 1;
    }
}

char *comms_spi_write_read(char *write_data, uint16_t write_len, int *response_len) {
    // Keep GIE suppressed for now, until ready:
    __bic_SR_register(GIE);

    // First, turn on the power to the comms chip so it can be starting up:
    pwr_control_comms(1);
    __delay_cycles(16000000);

    // Enable output pins:
    SOMI_SELECT |= SOMI_BIT;
    SIMO_SELECT |= SIMO_BIT;
    CLK_SELECT |= CLK_BIT;
    IO1_DIR &= ~IO1_BIT;
    IO2_DIR &= ~IO2_BIT;

    // Setup 1khz timer A0 (to space out measurements):
    TB0R = 0;
    TB0CCR0 = 62500;                        // 1 second interval
    TB0CCTL0 = CCIE;                        // TACCR0 interrupt enabled
    timeout_counter = 10; // seconds before timeout
    TB0CTL = TASSEL__SMCLK | ID_3 | MC__UP;  // SMCLK, UP mode, 500kHz/8 = 62.5Hz

    // Enable interrupts:
    P1IFG &= ~IO1_BIT;
    IO1_IE |= IO1_BIT;
    IFG_REGISTER &= ~UCRXIFG;
    IE_REGISTER |= UCRXIE;

    // Prepare for first SPI byte:
    //  MSP   >> 0x00
    //  COMMS << transaction type
    TXBUF_REGISTER = COMMS_SPI_NULL_BYTE;
    spi_state = SPI_STATE_TRANSACT_TYPE;
    spi_data = write_data;
    spi_size = write_len;
    spi_next_command = SPI_COMMAND_WRITE;
    spi_result = SPI_RESULT_UNFINISHED;

    // Enable GIE, go to sleep.
    __bis_SR_register(GIE + SPI_SLEEP_LEVEL);
    __no_operation();
    if (spi_result == SPI_RESULT_ERROR) {
        // If there was an error:
        IO1_IE &= ~IO1_BIT;
        IE_REGISTER &= ~UCRXIFG;
        pwr_control_comms(0);
        TB0CTL &= ~MC__UP; // Stop timer
        TB0CCTL0 &= ~CCIE;
        // Disable SPI pins:
        SOMI_SELECT &= ~SOMI_BIT;
        SIMO_SELECT &= ~SIMO_BIT;
        CLK_SELECT &= ~CLK_BIT;
        // Set SPI pins to output
        SOMI_DIR |= SOMI_BIT;
        SIMO_DIR |= SIMO_BIT;
        CLK_DIR |= CLK_BIT;
        IO1_DIR |= IO1_BIT;
        IO2_DIR |= IO2_BIT;
        // Set SPI pins to GND
        SOMI_OUT &= ~SOMI_BIT;
        SIMO_OUT &= ~SIMO_BIT;
        CLK_OUT &= ~CLK_BIT;
        IO1_OUT &= ~IO1_BIT;
        IO2_OUT &= ~IO2_BIT;
        __bis_SR_register(GIE);
        *response_len = error_num;
        return 0;
    } else {
        // Prepare for first SPI byte:
        //  MSP   >> 0x00
        //  COMMS << transaction type
        TXBUF_REGISTER = COMMS_SPI_NULL_BYTE;
        spi_state = SPI_STATE_TRANSACT_TYPE;
        spi_next_command = SPI_COMMAND_READ;
        spi_result = SPI_RESULT_UNFINISHED;
        // Enable GIE, go to sleep.
        __bis_SR_register(GIE + SPI_SLEEP_LEVEL);
        __no_operation();
        if (spi_result == SPI_RESULT_ERROR) {
            // If there was an error:
            IO1_IE &= ~IO1_BIT;
            IE_REGISTER &= ~UCRXIFG;
            pwr_control_comms(0);
            TB0CTL &= ~MC__UP; // Stop timer
            TB0CCTL0 &= ~CCIE;
            // Disable SPI pins:
            SOMI_SELECT &= ~SOMI_BIT;
            SIMO_SELECT &= ~SIMO_BIT;
            CLK_SELECT &= ~CLK_BIT;
            // Set SPI pins to output
            SOMI_DIR |= SOMI_BIT;
            SIMO_DIR |= SIMO_BIT;
            CLK_DIR |= CLK_BIT;
            IO1_DIR |= IO1_BIT;
            IO2_DIR |= IO2_BIT;
            // Set SPI pins to GND
            SOMI_OUT &= ~SOMI_BIT;
            SIMO_OUT &= ~SIMO_BIT;
            CLK_OUT &= ~CLK_BIT;
            IO1_OUT &= ~IO1_BIT;
            IO2_OUT &= ~IO2_BIT;
            __bis_SR_register(GIE);
            *response_len = error_num - 20;
            return 0;
        } else {
            spi_next_command = SPI_COMMAND_END;
            // read request has been sent, go to sleep to await response
            TXBUF_REGISTER = COMMS_SPI_NULL_BYTE;
            spi_state = SPI_STATE_TRANSACT_TYPE;
            __bis_SR_register(GIE + SPI_SLEEP_LEVEL);
            if (spi_result == SPI_RESULT_ERROR) {
                // If there was an error:
                IO1_IE &= ~IO1_BIT;
                IE_REGISTER &= ~UCRXIFG;
                pwr_control_comms(0);
                TB0CTL &= ~MC__UP; // Stop timer
                TB0CCTL0 &= ~CCIE;
                // Disable SPI pins:
                SOMI_SELECT &= ~SOMI_BIT;
                SIMO_SELECT &= ~SIMO_BIT;
                CLK_SELECT &= ~CLK_BIT;
                // Set SPI pins to output
                SOMI_DIR |= SOMI_BIT;
                SIMO_DIR |= SIMO_BIT;
                CLK_DIR |= CLK_BIT;
                IO1_DIR |= IO1_BIT;
                IO2_DIR |= IO2_BIT;
                // Set SPI pins to GND
                SOMI_OUT &= ~SOMI_BIT;
                SIMO_OUT &= ~SIMO_BIT;
                CLK_OUT &= ~CLK_BIT;
                IO1_OUT &= ~IO1_BIT;
                IO2_OUT &= ~IO2_BIT;
                __bis_SR_register(GIE);
                *response_len = error_num -40;
                return 0;
            } else {
                // response received, go to sleep to send END command
                TXBUF_REGISTER = COMMS_SPI_NULL_BYTE;
                spi_state = SPI_STATE_TRANSACT_TYPE;
                __bis_SR_register(GIE + SPI_SLEEP_LEVEL);
                IO1_IE &= ~IO1_BIT;
                IE_REGISTER &= ~UCRXIFG;
                __delay_cycles(16000000);
                pwr_control_comms(0);
                TB0CTL &= ~MC__UP; // Stop timer
                TB0CCTL0 &= ~CCIE;
                // Disable SPI pins:
                SOMI_SELECT &= ~SOMI_BIT;
                SIMO_SELECT &= ~SIMO_BIT;
                CLK_SELECT &= ~CLK_BIT;
                // Set SPI pins to output
                SOMI_DIR |= SOMI_BIT;
                SIMO_DIR |= SIMO_BIT;
                CLK_DIR |= CLK_BIT;
                IO1_DIR |= IO1_BIT;
                IO2_DIR |= IO2_BIT;
                // Set SPI pins to GND
                SOMI_OUT &= ~SOMI_BIT;
                SIMO_OUT &= ~SIMO_BIT;
                CLK_OUT &= ~CLK_BIT;
                IO1_OUT &= ~IO1_BIT;
                IO2_OUT &= ~IO2_BIT;
                __bis_SR_register(GIE);
                *response_len = spi_size;
                return spi_data;
            }
        }
    }
}

// Timer0_B0 interrupt service routine
#pragma vector = TIMER0_B0_VECTOR
__interrupt void Timer0_B0_ISR (void) {
    TB0CCTL0 &= ~CCIFG;
    timeout_counter--;
    if (timeout_counter <= 0) {
        __bic_SR_register_on_exit(LPM0_bits);
        TB0CTL &= ~MC__UP; // Stop timer
        TB0CCTL0 &= ~CCIE;
        spi_result = SPI_RESULT_ERROR;
    }
}

#define on_error() \
    spi_result = SPI_RESULT_ERROR; \
    __bic_SR_register_on_exit(GIE + SPI_SLEEP_LEVEL);

#define on_success() \
    spi_result = SPI_RESULT_SUCCESS; \
    __bic_SR_register_on_exit(GIE + SPI_SLEEP_LEVEL);

#pragma vector = EUSCI_VECTOR
__interrupt void comms_spi_isr() {
    static uint16_t spi_count = 0;
    uint8_t read_data;
    if (IFG_REGISTER & UCRXIFG) {
        read_data = RXBUF_REGISTER;
        switch (spi_state) {
        case SPI_STATE_TRANSACT_TYPE:
            if (read_data == COMMS_SPI_TRANSACTION_REQUEST) {
                TXBUF_REGISTER = spi_next_command;
                if (spi_next_command == SPI_COMMAND_END || spi_next_command == SPI_COMMAND_READ) {
                    spi_state = SPI_STATE_FINISHED;
                } else {
                    spi_state = SPI_STATE_WRITE_SIZE;
                    spi_count = 0;
                }
            } else if (read_data == COMMS_SPI_TRANSACTION_RESPONSE) {
                TXBUF_REGISTER = COMMS_SPI_NULL_BYTE;
                spi_state = SPI_STATE_READ_RESPONSE_SIZE;
                spi_count = 0;
            } else {
                on_error();
                error_num = -1;
            }
            break;
        case SPI_STATE_READ_RESPONSE_SIZE:
            if (spi_count) {
                spi_size = (0x00FF & spi_size) + (read_data << 8);
                spi_state = SPI_STATE_READ_RESPONSE_DATA;
                spi_count = 0;
                spi_data = malloc(spi_size);
            } else {
                spi_size = (0xFF00 & spi_size) + read_data;
                spi_count = 1;
            }
            TXBUF_REGISTER = COMMS_SPI_NULL_BYTE;
            break;
        case SPI_STATE_READ_RESPONSE_DATA:
            spi_data[spi_count++] = read_data;
            if (spi_count >= spi_size) {
                spi_state = SPI_STATE_IDLE;
                on_success();
            } else {
                TXBUF_REGISTER = COMMS_SPI_NULL_BYTE;
            }
            break;
        case SPI_STATE_WRITE_SIZE:
            if (spi_count) {
                TXBUF_REGISTER = 0x00FF & (spi_size >> 8);
                spi_state = SPI_STATE_WRITE_DATA;
                spi_count = 0;
            } else {
                TXBUF_REGISTER = spi_size & 0x00FF;
                spi_count = 1;
            }
            break;
        case SPI_STATE_WRITE_DATA:
            TXBUF_REGISTER = spi_data[spi_count++];
            if (spi_count >= spi_size) {
                spi_state = SPI_STATE_FINISHED;
            }
            break;
        case SPI_STATE_FINISHED:
            spi_state = SPI_STATE_IDLE;
            on_success();
            break;
        default:
            // Unknown state, disable interrupts
            on_error();
            error_num = -3 - spi_state;
            break;
        }
    }
    IFG_REGISTER = 0;
}

#pragma vector = PORT1_VECTOR
__interrupt void comms_spi_error() {
    if (P1IFG & IO1_BIT) {
        // COMMS chip reports an error, end SPI transaction with an error
        on_error();
        error_num = -2;
    }
    // Clear interrupt flags before exit.
    P1IFG = 0;
}
