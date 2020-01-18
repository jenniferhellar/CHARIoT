#include <msp430.h> 
#include <stdint.h>
#include "comms_spi.h"

int main(void)
{
    uint8_t *sdep_res;

    // Disable Watchdog Timer:
	WDTCTL = WDTPW | WDTHOLD;

    // Unlock pins:
    PM5CTL0 &= ~LOCKLPM5;
	
    init_comms_spi();

    //__bis_SR_register(GIE);
    comms_sdep_packet(COMMS_SDEP_CMD_INIT, 0, 0, 0);
    /*comms_sdep_msg(COMMS_SDEP_CMD_AT, "AT+FACTORYRESET");
    sdep_res = comms_get_sdep_packet();
    __delay_cycles(5000000);*/
    comms_sdep_msg(COMMS_SDEP_CMD_AT, "AT+GAPDEVNAME=CharIoT Node");
    sdep_res = comms_get_sdep_packet();
    comms_sdep_msg(COMMS_SDEP_CMD_AT, "AT+BLEPOWERLEVEL=-20");
    sdep_res = comms_get_sdep_packet();
    comms_sdep_msg(COMMS_SDEP_CMD_AT, "AT+HWMODELED=BLEUART");
    sdep_res = comms_get_sdep_packet();
    comms_sdep_msg(COMMS_SDEP_CMD_AT, "ATZ");
    sdep_res = comms_get_sdep_packet();

	while (1) {
	    // Main loop
	    //__bis_SR_register(LPM4_bits);

        comms_sdep_msg(COMMS_SDEP_CMD_TX, "TEST");
        sdep_res = comms_get_sdep_packet();
        __delay_cycles(1000000);

	    //comms_sdep_msg(COMMS_SDEP_CMD_AT, "AT+BleKeyboard=ThisIsAReallyLongMessageThatImTestingToSeeIfSomeOfMyFunctionsWork!!!!!!!!!!");
        //__delay_cycles(1000000);
	}
}
