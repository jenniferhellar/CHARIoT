#include <stdint.h>

#ifndef UTIL_H_
#define UTIL_H_

/*
 * Writes the given integer to the given string. The string must be long enough
 * to accept all characters of the integer. To write to a string at a given
 * offset, add the offset to the string pointer before passing it to this function.
 * This function writes the base 10 string representation of the integer.
 *
 * Parameters:
 *     i - The integer to write.
 *     str - A pointer to the start of a place in a string to write the number to.
 *
 * Returns:
 *     The number of characters written to the string.
 */
uint8_t write_int_to_string(int i, char *str);

/*
 * Converts the given integer to a string. This function is a wrapper function
 * for write_int_to_string(). This function handles allocating memory for the
 * string.
 *
 * Parameters:
 *     i - The integer to conver.
 *
 * Returns:
 *     A pointer to the string created containing the base 10 string
 *     representation of the number i.
 */
char *convert_int_to_str(int i);

#endif /* UTIL_H_ */
