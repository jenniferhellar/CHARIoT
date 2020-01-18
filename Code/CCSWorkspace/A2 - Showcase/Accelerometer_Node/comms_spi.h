#include <stdint.h>

#ifndef COMMS_SPI_H_
#define COMMS_SPI_H_

// Define which board revision to use:
//#define COMMS_SPI_REV_2
#define COMMS_SPI_REV_3

/*
 * Function definitions:
 */
void comms_spi_init();
uint8_t comms_spi_write(char *data, uint16_t len);
char *comms_spi_write_read(char *write_data, uint16_t write_len, int *response_len);

/*
 * Board-revision-specific configurations
 */
#ifdef COMMS_SPI_REV_2

// Define what the eUSCI control and interrupt registers are:
#define CONTROL_REGISTER  UCB1CTLW0
#define IE_REGISTER       UCB1IE
#define IFG_REGISTER      UCB1IFG
#define TXBUF_REGISTER    UCB1TXBUF
#define RXBUF_REGISTER    UCB1RXBUF
#define EUSCI_VECTOR      USCI_B1_VECTOR

/*
 * SOMI: P5.1
 * SIMO: P5.0
 * CLK:  P5.2
 * IO1:  P1.5
 * IO2:  P1.4
 */

// Define the pin bits
#define SOMI_BIT          BIT1
#define SIMO_BIT          BIT0
#define CLK_BIT           BIT2
#define IO1_BIT           BIT5
#define IO2_BIT           BIT4

// Define the select registers for SPI
#define SOMI_SELECT       P5SEL0
#define SIMO_SELECT       P5SEL0
#define CLK_SELECT        P5SEL0
#define SOMI_DIR          P5DIR
#define SIMO_DIR          P5DIR
#define CLK_DIR           P5DIR
#define IO1_DIR           P1DIR
#define IO2_DIR           P1DIR
#define SOMI_OUT          P5OUT
#define SIMO_OUT          P5OUT
#define CLK_OUT           P5OUT
#define IO1_OUT           P1OUT
#define IO2_OUT           P1OUT
#define IO1_IE            P1IE
#define IO1_IES           P1IES

#endif
#ifdef COMMS_SPI_REV_3

// Define what the eUSCI control and interrupt registers are:
#define CONTROL_REGISTER  UCA0CTLW0
#define IE_REGISTER       UCA0IE
#define IFG_REGISTER      UCA0IFG
#define TXBUF_REGISTER    UCA0TXBUF
#define RXBUF_REGISTER    UCA0RXBUF
#define EUSCI_VECTOR      EUSCI_A0_VECTOR

/*
 * SOMI: P2.1
 * SIMO: P2.0
 * CLK:  P1.5
 * IO1:  P1.3
 * IO2:  P1.4
 */

// Define the pin bits
#define SOMI_BIT          BIT1
#define SIMO_BIT          BIT0
#define CLK_BIT           BIT5
#define IO1_BIT           BIT3
#define IO2_BIT           BIT4

// Define the select registers for SPI
#define SOMI_SELECT       P2SEL1
#define SIMO_SELECT       P2SEL1
#define CLK_SELECT        P1SEL1
#define SOMI_DIR          P2DIR
#define SIMO_DIR          P2DIR
#define CLK_DIR           P1DIR
#define IO1_DIR           P1DIR
#define IO2_DIR           P1DIR
#define SOMI_OUT          P2OUT
#define SIMO_OUT          P2OUT
#define CLK_OUT           P1OUT
#define IO1_OUT           P1OUT
#define IO2_OUT           P1OUT
#define IO1_IE            P1IE
#define IO1_IES           P1IES

#endif

#endif /* COMMS_SPI_H_ */
