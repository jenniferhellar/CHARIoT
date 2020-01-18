#include "pwr_control.h"

void pwr_control_setup() {

}

void pwr_control_comms(unsigned char state) {
    if (state) {
        // TODO: Ask power controller to turn comms power on.
    } else {
        // TODO: Ask power controller to turn comms power off.
    }
}

void pwr_control_sensor(unsigned char state) {
    if (state) {
        // TODO: Ask power controller to turn sensor power on.
    } else {
        // TODO: Ask power controller to turn sensor power off.
    }
}
