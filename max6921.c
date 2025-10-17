/**
 * @file max6921.c
 * @brief MAX6921 VFD controller driver implementation
 */

#include "max6921.h"
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"

/* Internal driver state */
typedef struct {
    bool initialized;
    vfd_config_t config;
    vfd_display_buffer_t display_buffer;
    spi_inst_t *spi_port;
    uint8_t spi_data[3];
} vfd_driver_state_t;

/* Grid control patterns (one grid active at a time) */
static const uint16_t GRID_PATTERNS[9] = {
    0b100000000,  /* Grid 0 */
    0b010000000,  /* Grid 1 */
    0b001000000,  /* Grid 2 */
    0b000100000,  /* Grid 3 */
    0b000010000,  /* Grid 4 */
    0b000001000,  /* Grid 5 */
    0b000000100,  /* Grid 6 */
    0b000000010,  /* Grid 7 */
    0b000000001   /* Grid 8 */
};

/* Digit to segment mapping */
static const uint8_t DIGIT_PATTERNS[13] = {
    VFD_DIGIT_0,
    VFD_DIGIT_1,
    VFD_DIGIT_2,
    VFD_DIGIT_3,
    VFD_DIGIT_4,
    VFD_DIGIT_5,
    VFD_DIGIT_6,
    VFD_DIGIT_7,
    VFD_DIGIT_8,
    VFD_DIGIT_9,
    VFD_SYMBOL_DOT,
    VFD_SYMBOL_DASH,
    VFD_BLANK
};

static vfd_driver_state_t g_vfd_state = {
    .initialized = false,
    .config = {0},
    .display_buffer = {0},
    .spi_port = spi1
};

/* Validate grid index */
static bool _is_valid_grid(uint8_t grid) {
    return grid < 9;
}

/* Validate segment pattern */
static bool _is_valid_segment(uint8_t segment) {
    (void)segment;
    return true;
}

/* Write raw data to VFD chip
 * Constructs the 20-bit control word: [COMMAND(3) | GRID(9) | SEGMENTS(8)]
 * Command bits (19-17) default to 0 for display-only operation
 *
 * The MAX6921 is a 20-bit shift register. Since SPI operates on whole bytes,
 * we transmit 3 bytes (24 bits) total: 4 padding bits + 20-bit control word.
 * The padding bits are transmitted first (MSB-first), shifting the 20-bit word
 * into the correct position in the shift register.
 *
 * Format: [4-bit padding | COMMAND(3) | GRID(9) | SEGMENTS(8)]
 */
static void _write_vfd_raw(uint8_t grid, uint8_t segments) {
    if (!_is_valid_grid(grid)) {
        return;
    }

    uint32_t combined_data = ((uint32_t)GRID_PATTERNS[grid] << 8) | segments;

    g_vfd_state.spi_data[0] = (combined_data >> 16) & 0xFF;
    g_vfd_state.spi_data[1] = (combined_data >> 8) & 0xFF;
    g_vfd_state.spi_data[2] = combined_data & 0xFF;

    spi_write_blocking(g_vfd_state.spi_port, g_vfd_state.spi_data, 3);

    gpio_put(g_vfd_state.config.pin_latch, 1);
    sleep_us(1);
    gpio_put(g_vfd_state.config.pin_latch, 0);
}

/* Initialize GPIO pins */
static vfd_error_t _init_gpio(const vfd_config_t *config) {
    uint actual_baudrate = spi_init(g_vfd_state.spi_port, config->spi_baudrate);
    
    if (actual_baudrate == 0) {
        return VFD_ERR_HARDWARE;
    }

    gpio_set_function(config->pin_spi_clk, GPIO_FUNC_SPI);
    gpio_set_function(config->pin_spi_tx, GPIO_FUNC_SPI);

    gpio_init(config->pin_latch);
    gpio_set_dir(config->pin_latch, GPIO_OUT);
    gpio_put(config->pin_latch, 0);

    return VFD_OK;
}

/* Public API */

vfd_config_t vfd_default_config(void) {
    vfd_config_t config = {
        .spi_baudrate = 2000000,
        .pin_spi_tx = 11,
        .pin_spi_clk = 10,
        .pin_latch = 13,
        .refresh_interval_us = 1500
    };
    return config;
}

vfd_error_t vfd_init(const vfd_config_t *config) {
    if (g_vfd_state.initialized) {
        return VFD_OK;
    }

    if (config == NULL) {
        g_vfd_state.config = vfd_default_config();
    } else {
        if (config->spi_baudrate == 0 || config->refresh_interval_us == 0) {
            return VFD_ERR_INVALID_PARAM;
        }
        g_vfd_state.config = *config;
    }

    stdio_init_all();

    vfd_error_t err = _init_gpio(&g_vfd_state.config);
    if (err != VFD_OK) {
        return err;
    }

    vfd_clear();

    g_vfd_state.initialized = true;
    return VFD_OK;
}

bool vfd_is_initialized(void) {
    return g_vfd_state.initialized;
}

vfd_error_t vfd_deinit(void) {
    if (!g_vfd_state.initialized) {
        return VFD_ERR_NOT_INITIALIZED;
    }

    vfd_clear();
    vfd_refresh();

    spi_deinit(g_vfd_state.spi_port);

    g_vfd_state.initialized = false;
    return VFD_OK;
}

vfd_error_t vfd_write_segments(uint8_t grid, uint8_t segments) {
    if (!g_vfd_state.initialized) {
        return VFD_ERR_NOT_INITIALIZED;
    }

    if (!_is_valid_grid(grid)) {
        return VFD_ERR_INVALID_GRID;
    }

    if (!_is_valid_segment(segments)) {
        return VFD_ERR_INVALID_SEGMENT;
    }

    g_vfd_state.display_buffer[grid] = segments;
    return VFD_OK;
}

vfd_error_t vfd_read_segments(uint8_t grid, uint8_t *segments) {
    if (!g_vfd_state.initialized) {
        return VFD_ERR_NOT_INITIALIZED;
    }

    if (!_is_valid_grid(grid)) {
        return VFD_ERR_INVALID_GRID;
    }

    if (segments == NULL) {
        return VFD_ERR_INVALID_PARAM;
    }

    *segments = g_vfd_state.display_buffer[grid];
    return VFD_OK;
}

vfd_error_t vfd_write_digit(uint8_t grid, uint8_t digit) {
    if (!g_vfd_state.initialized) {
        return VFD_ERR_NOT_INITIALIZED;
    }

    if (!_is_valid_grid(grid)) {
        return VFD_ERR_INVALID_GRID;
    }

    if (digit > 9) {
        return VFD_ERR_INVALID_PARAM;
    }

    g_vfd_state.display_buffer[grid] = DIGIT_PATTERNS[digit];
    return VFD_OK;
}

vfd_error_t vfd_clear(void) {
    memset(g_vfd_state.display_buffer, VFD_BLANK, sizeof(vfd_display_buffer_t));
    return VFD_OK;
}

vfd_error_t vfd_refresh(void) {
    if (!g_vfd_state.initialized) {
        return VFD_ERR_NOT_INITIALIZED;
    }

    for (uint8_t grid = 0; grid < 9; grid++) {
        _write_vfd_raw(grid, g_vfd_state.display_buffer[grid]);
        sleep_us(g_vfd_state.config.refresh_interval_us);
    }

    return VFD_OK;
}

vfd_error_t vfd_write_string(const char *str) {
    if (!g_vfd_state.initialized) {
        return VFD_ERR_NOT_INITIALIZED;
    }

    if (str == NULL) {
        return VFD_ERR_INVALID_PARAM;
    }

    vfd_clear();

    uint8_t grid = 0;
    for (size_t i = 0; str[i] != '\0' && grid < 9; i++) {
        char c = str[i];
        vfd_error_t err;

        if (c >= '0' && c <= '9') {
            err = vfd_write_digit(grid, c - '0');
            if (err != VFD_OK) {
                return err;
            }
            grid++;
        } else if (c == '-') {
            g_vfd_state.display_buffer[grid] = VFD_SYMBOL_DASH;
            grid++;
        } else if (c == '.') {
            if (grid > 0) {
                g_vfd_state.display_buffer[grid - 1] |= VFD_SYMBOL_DOT;
            }
        } else if (c == ' ') {
            g_vfd_state.display_buffer[grid] = VFD_BLANK;
            grid++;
        }
    }

    return VFD_OK;
}

vfd_display_buffer_t *vfd_get_buffer(void) {
    if (!g_vfd_state.initialized) {
        return NULL;
    }
    return &g_vfd_state.display_buffer;
}

vfd_error_t vfd_fill_buffer(uint8_t segments) {
    if (!g_vfd_state.initialized) {
        return VFD_ERR_NOT_INITIALIZED;
    }

    memset(g_vfd_state.display_buffer, segments, sizeof(vfd_display_buffer_t));
    return VFD_OK;
}

vfd_error_t vfd_send_control_command(const vfd_control_command_t *cmd) {
    if (!g_vfd_state.initialized) {
        return VFD_ERR_NOT_INITIALIZED;
    }

    if (cmd == NULL) {
        return VFD_ERR_INVALID_PARAM;
    }

    if (cmd->command > 7) {
        return VFD_ERR_INVALID_PARAM;
    }

    /* Encode command in bits 19-17 and send with zero grid/segment data.
     * User can define what each command code (0-7) does in their application.
     * Transmits 3 bytes (24 bits): 4 padding bits + 20-bit control word.
     * The padding bits position the command in the shift register correctly.
     */
    uint32_t control_word = ((uint32_t)cmd->command << 17);
    
    g_vfd_state.spi_data[0] = (control_word >> 16) & 0xFF;
    g_vfd_state.spi_data[1] = (control_word >> 8) & 0xFF;
    g_vfd_state.spi_data[2] = control_word & 0xFF;

    spi_write_blocking(g_vfd_state.spi_port, g_vfd_state.spi_data, 3);

    gpio_put(g_vfd_state.config.pin_latch, 1);
    sleep_us(1);
    gpio_put(g_vfd_state.config.pin_latch, 0);

    return VFD_OK;
}

int vfd_segments_to_string(uint8_t segments, char *buffer, int buffer_size) {
    if (buffer == NULL || buffer_size < 1) {
        return 0;
    }

    static const char *segment_names[] = {"A", "B", "C", "D", "E", "F", "G", "H"};
    int offset = 0;

    buffer[0] = '\0';

    for (int i = 0; i < 8; i++) {
        if (segments & (1 << i)) {
            if (offset > 0) {
                offset += snprintf(buffer + offset, buffer_size - offset, " ");
            }
            offset += snprintf(buffer + offset, buffer_size - offset, "%s", segment_names[i]);

            if (offset >= buffer_size - 1) {
                break;
            }
        }
    }

    return offset;
}

const char *vfd_strerror(vfd_error_t error) {
    static const char *error_messages[] = {
        "Operation successful",
        "Invalid parameter provided",
        "VFD not initialized",
        "Grid index out of range",
        "Segment value out of range",
        "Hardware initialization failed"
    };

    if (error >= 0 && error < 6) {
        return error_messages[error];
    }

    return "Unknown error";
}