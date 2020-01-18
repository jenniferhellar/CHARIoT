#include <msp430.h>
#include "pwr_control.h"
#include "comms_spi.h"

void pwr_control_setup() {
#ifdef COMMS_SPI_REV_2
    // COMMS Enable pin: P2.7
    P2OUT &= ~BIT7;
    P2DIR |= BIT7;
#endif
#ifdef COMMS_SPI_REV_3
    // COMMS Enable pin: P2.6
    P2OUT &= ~BIT6;
    P2DIR |= BIT6;
#endif
}

void pwr_control_comms(unsigned char state) {
#ifdef COMMS_SPI_REV_2
    if (state) {
        P2OUT |= BIT7;
    } else {
        P2OUT &= ~BIT7;
        __delay_cycles(1000000);
    }
#endif
#ifdef COMMS_SPI_REV_3
    if (state) {
        P2OUT |= BIT6;
    } else {
        P2OUT &= ~BIT6;
        __delay_cycles(1000000);
    }
#endif
}
