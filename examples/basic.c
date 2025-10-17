/**
 * @file basic.c
 * @brief Basic example demonstrating VFD library usage
 *
 * This example shows the simplest way to initialize and use the VFD display.
 * It cycles through digits 0-9 on the first display position.
 */

#include "max6921.h"
#include <stdio.h>
#include "pico/stdlib.h"

int main(void) {
    vfd_error_t err = vfd_init(NULL);
    if (err != VFD_OK) {
        printf("VFD initialization failed: %s\n", vfd_strerror(err));
        return 1;
    }

    printf("VFD initialized successfully\n");

    while (true) {
        for (uint8_t digit = 0; digit < 10; digit++) {
            vfd_write_digit(0, digit);
            vfd_refresh();
            sleep_ms(1000);
        }
    }

    return 0;
}