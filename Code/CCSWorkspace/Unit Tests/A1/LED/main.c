//***************************************************************************************
//  Light each debug LED (P6.0-3) in sequence and then turn them off in sequence.
//***************************************************************************************

#include <msp430.h>

void main(void) {
    WDTCTL = WDTPW | WDTHOLD;               // Stop watchdog timer
    PM5CTL0 &= ~LOCKLPM5;                   // Disable the GPIO power-on default high-impedance mode
                                            // to activate previously configured port settings
    P6DIR |= 0x01;                          // Set P6.0-3 to output direction
    P6DIR |= 0x02;
    P6DIR |= 0x04;
    P6DIR |= 0x08;

    for(;;) {
        volatile unsigned int i;            // volatile to prevent optimization

        P6OUT ^= 0x01;                      // Toggle P6.0 using exclusive-OR

        i = 50000;                          // SW Delay
        do i--;
        while(i != 0);

        P6OUT ^= 0x02;                      // Toggle P6.1 using exclusive-OR

        i = 50000;                          // SW Delay
        do i--;
        while(i != 0);

        P6OUT ^= 0x04;                      // Toggle P6.2 using exclusive-OR

        i = 50000;                          // SW Delay
        do i--;
        while(i != 0);

        P6OUT ^= 0x08;                      // Toggle P6.3 using exclusive-OR

        i = 50000;                          // SW Delay
        do i--;
        while(i != 0);

    }
}
