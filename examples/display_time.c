/**
 * @file display_time.c
 * @brief Advanced example: displaying time on IV-18 VFD
 *
 * This example demonstrates:
 * - Custom hardware configuration
 * - Direct buffer manipulation
 * - Formatted display output (HH-MM-SS format)
 */

#include "max6921.h"
#include <stdio.h>
#include <time.h>
#include "pico/stdlib.h"

/**
 * Display formatted time on VFD
 *
 * Layout: [_][H][H][-][M][M][-][S][S]
 * Where _ is blank, H is hour, M is minute, S is second
 * The dash symbols help with readability.
 *
 * @param hours Hours (0-23)
 * @param minutes Minutes (0-59)
 * @param seconds Seconds (0-59)
 */
static void display_time(uint8_t hours, uint8_t minutes, uint8_t seconds) {
    vfd_error_t err;

    vfd_clear();

    if (hours == 0) {
        hours = 12;
    } else if (hours > 12) {
        hours -= 12;
    }

    vfd_write_segments(0, VFD_BLANK);

    if (hours < 10) {
        vfd_write_segments(1, VFD_BLANK);
        vfd_write_digit(2, hours);
    } else {
        vfd_write_digit(1, hours / 10);
        vfd_write_digit(2, hours % 10);
    }

    vfd_write_segments(3, VFD_SYMBOL_DASH);

    vfd_write_digit(4, minutes / 10);
    vfd_write_digit(5, minutes % 10);

    vfd_write_segments(6, VFD_SYMBOL_DASH);

    vfd_write_digit(7, seconds / 10);
    vfd_write_digit(8, seconds % 10);

    vfd_refresh();
}

int main(void) {
    vfd_error_t err = vfd_init(NULL);
    if (err != VFD_OK) {
        printf("VFD initialization failed: %s\n", vfd_strerror(err));
        return 1;
    }

    printf("VFD time display example\n");

    uint32_t seconds = 0;

    while (true) {
        uint8_t hours = (seconds / 3600) % 24;
        uint8_t minutes = (seconds / 60) % 60;
        uint8_t secs = seconds % 60;

        display_time(hours, minutes, secs);

        sleep_ms(1000);
        seconds++;

        if (seconds >= 86400) {
            seconds = 0;
        }
    }

    return 0;
}