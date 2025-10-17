/**
 * @file max6921.h
 * @brief MAX6921 VFD controller driver for Raspberry Pi Pico
 *
 * Interface for controlling Russian IV-18 VFD tubes using the Maxim MAX6921 chip
 * on the Raspberry Pi Pico microcontroller.
 *
 * Example:
 * @code
 * vfd_config_t config = vfd_default_config();
 * if (vfd_init(&config) != VFD_OK) {
 *     return 1;
 * }
 * vfd_write_segments(0, 0x3F);
 * vfd_refresh();
 * @endcode
 */

#ifndef MAX6921_H
#define MAX6921_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Error codes returned by library functions */
typedef enum {
    VFD_OK = 0,                    /* Operation successful */
    VFD_ERR_INVALID_PARAM = 1,     /* Invalid parameter provided */
    VFD_ERR_NOT_INITIALIZED = 2,   /* VFD not initialized */
    VFD_ERR_INVALID_GRID = 3,      /* Grid index out of range */
    VFD_ERR_INVALID_SEGMENT = 4,   /* Segment value out of range */
    VFD_ERR_HARDWARE = 5           /* Hardware initialization failed */
} vfd_error_t;

/* VFD configuration structure */
typedef struct {
    uint32_t spi_baudrate;         /* SPI baud rate (default: 2000000) */
    uint8_t pin_spi_tx;            /* MOSI pin (default: 11) */
    uint8_t pin_spi_clk;           /* SCK pin (default: 10) */
    uint8_t pin_latch;             /* Latch/CS pin (default: 13) */
    uint16_t refresh_interval_us;  /* Microseconds between grid refreshes (default: 1500) */
} vfd_config_t;

/* Standard 7-segment digit mappings */
typedef enum {
    VFD_DIGIT_0 = 0b00111111,      /* Segments: A B C D E F */
    VFD_DIGIT_1 = 0b00000110,      /* Segments: B C */
    VFD_DIGIT_2 = 0b01011011,      /* Segments: A B D E G */
    VFD_DIGIT_3 = 0b01001111,      /* Segments: A B C D G */
    VFD_DIGIT_4 = 0b01100110,      /* Segments: B C F G */
    VFD_DIGIT_5 = 0b01101101,      /* Segments: A C D F G */
    VFD_DIGIT_6 = 0b01111101,      /* Segments: A C D E F G */
    VFD_DIGIT_7 = 0b00000111,      /* Segments: A B C */
    VFD_DIGIT_8 = 0b01111111,      /* Segments: A B C D E F G */
    VFD_DIGIT_9 = 0b01101111,      /* Segments: A B C D F G */
    VFD_SYMBOL_DOT = 0b10000000,   /* Decimal point (H) only */
    VFD_SYMBOL_DASH = 0b01000000,  /* Dash/minus (G only) */
    VFD_BLANK = 0b00000000         /* All segments off */
} vfd_segment_pattern_t;

/* Display buffer (one entry per grid) */
typedef uint8_t vfd_display_buffer_t[9];

/* Initialization and Configuration */

/**
 * Get default VFD configuration
 * Returns: SPI 2MHz, MOSI pin 11, SCK pin 10, Latch pin 13, refresh 1500us
 */
vfd_config_t vfd_default_config(void);

/**
 * Initialize the VFD driver
 * Must be called before any other VFD operations
 */
vfd_error_t vfd_init(const vfd_config_t *config);

/**
 * Check if VFD is initialized
 */
bool vfd_is_initialized(void);

/**
 * Deinitialize the VFD driver
 */
vfd_error_t vfd_deinit(void);

/* Display Control */

/**
 * Write segment pattern to a specific grid
 * Grid: 0-8 (left to right)
 * Does not update display until vfd_refresh() is called
 */
vfd_error_t vfd_write_segments(uint8_t grid, uint8_t segments);

/**
 * Read current segment pattern from a grid
 */
vfd_error_t vfd_read_segments(uint8_t grid, uint8_t *segments);

/**
 * Write a digit (0-9) to a grid
 */
vfd_error_t vfd_write_digit(uint8_t grid, uint8_t digit);

/**
 * Clear all display segments
 */
vfd_error_t vfd_clear(void);

/**
 * Refresh the display
 * Updates all grids with values from the buffer
 */
vfd_error_t vfd_refresh(void);

/**
 * Write a string to the display
 * Supported: '0'-'9', '-', '.', ' '
 * Max 9 characters (truncates if longer)
 */
vfd_error_t vfd_write_string(const char *str);

/* Buffer Management */

/**
 * Get pointer to display buffer for direct manipulation
 * Changes applied after vfd_refresh()
 */
vfd_display_buffer_t *vfd_get_buffer(void);

/**
 * Fill entire display buffer with a pattern
 */
vfd_error_t vfd_fill_buffer(uint8_t segments);

/* Custom Commands */

/**
 * Control command type
 * The 20-bit control word uses 3 bits (19-17) for custom command codes (0-7).
 * Users can define their own commands for:
 * - Controlling external logic gates or analog circuits
 * - Triggering digital signals
 * - Implementing backup pins if MAX6921 pins fail
 *
 * Transmission: The command is sent as part of a 3-byte (24-bit) SPI transmission.
 * 4 padding bits are sent first (MSB-first), which shift the 20-bit control word
 * into the MAX6921's shift register at the correct position.
 */
typedef struct {
    uint8_t command;               /* Custom command code (0-7) */
} vfd_control_command_t;

/**
 * Send a custom command
 * Command is encoded in bits 19-17 and sent via SPI as part of a 3-byte transmission.
 * Can be sent standalone (with grid/segment bits = 0) or combined with display data.
 */
vfd_error_t vfd_send_control_command(const vfd_control_command_t *cmd);

/* Utility Functions */

/**
 * Convert segment pattern to human-readable string
 * Example: "A B C D E F"
 * Buffer should be at least 30 bytes
 */
int vfd_segments_to_string(uint8_t segments, char *buffer, int buffer_size);

/**
 * Get descriptive error message
 */
const char *vfd_strerror(vfd_error_t error);

#ifdef __cplusplus
}
#endif

#endif /* MAX6921_H */