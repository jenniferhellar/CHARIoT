#ifndef FRAM_H_
#define FRAM_H_

#define FRAM_SIZE 64
extern uint8_t fram_arr[FRAM_SIZE];
extern uint8_t *curr;
extern uint8_t bytes_stored;


/*
 * Take data collected from sensor and write to FRAM
 */
/*
 * Writes data to FRAM
 *
 * Returns:
 *     void
 */

void fram_write(uint8_t *data_ptr, int numbytes);

#endif /* FRAM_H_ */

