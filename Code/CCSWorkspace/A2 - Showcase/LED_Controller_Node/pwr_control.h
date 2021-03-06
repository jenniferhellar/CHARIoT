#ifndef PWR_CONTROL_H_
#define PWR_CONTROL_H_

/*
 * Code to initialize the power system.
 */
void pwr_control_setup();

/*
 * This function controls the power to the comms board.
 *
 * Inputs:
 *     state - zero for off, non-zero for on
 */
void pwr_control_comms(unsigned char state);

#endif /* PWR_CONTROL_H_ */
