# max6921 - VFD Driver Library for Raspberry Pi Pico

Clean C library for controlling Russian IV-18 VFD displays using the Maxim MAX6921 driver chip on Raspberry Pi Pico.

## How It Works

### The MAX6921 Architecture

The MAX6921 is a 20-bit serial VFD controller that uses **shift-register serial communication** to control Russian IV-18 vacuum fluorescent displays. It uses a compact 3-wire SPI interface requiring minimal GPIO pins.

**20-Bit Control Word:**
```
Bit 19 18 17 | 16 15 14 13 12 11 10 9 8 | 7 6 5 4 3 2 1 0
[   CMD    |        Grid Select      | Segment Pattern]
 3 bits    |         9 bits          |     8 bits
```

- **Bits 19-17**: Command bits (user-defined control codes 0-7)
- **Bits 16-8**: Grid selection (9 bits, one bit per digit position 0-8)
- **Bits 7-0**: Segment pattern (8 bits controlling display segments A-F, G, DP)

### Communication

The library communicates with the MAX6921 using a 3-wire SPI interface:

**Pins (default GPIO):**
- **GPIO 11 (MOSI)** - Serial data input
- **GPIO 10 (SCK)** - Serial clock
- **GPIO 13 (Latch)** - Output latch pulse

**Protocol:**
1. Construct a 20-bit control word: [COMMAND(3) | GRID(9) | SEGMENTS(8)]
2. Prefix with 4 padding bits for byte-aligned transmission
3. Shift via SPI (transmitted as 3 bytes, MSB first): [4-bit padding | 20-bit control word]
4. Pulse the latch pin to apply the output
5. Wait before next transmission

**Example: Display digit 5 on grid 0**
```
Bits 19-17: 000 (command = 0, display only)
Bits 16-8:  100000000 (grid 0 active)
Bits 7-0:   01101101 (segments for digit 5)
Result: 0x08D (20-bit value)
```

### Custom Commands

The 3 command bits (19-17) can be used for custom functionality. Users can define their own 8 command codes (0-7) to:
- Control external logic gates or analog circuits
- Trigger digital signals via separate pins
- Implement backup functionality if MAX6921 pins fail
- Send custom control signals alongside display updates

The command bits are independent of display data and can be sent standalone or combined with any grid/segment pattern.

### Display Multiplexing

The IV-18 display has **9 grids** (digit positions) and requires sequential addressing:
1. Send 20-bit word activating grid 0 with its segment pattern
2. Wait ~1.5ms for display to stabilize
3. Send 20-bit word activating grid 1 with its segment pattern
4. Repeat for all 9 grids

This creates a **multiplexed display** where only one grid is active at a time, but they cycle quickly enough (~13.5ms per complete refresh) to appear as all digits lit simultaneously.

### Data Flow

```
Application Layer
      ↓
vfd_write_digit/string → Display Buffer (9 bytes)
      ↓
vfd_refresh()
      ↓
For each grid 0-8:
  - Construct 20-bit word (command + grid + segments)
  - Transmit via SPI
  - Pulse latch pin
  - Wait refresh_interval_us
```

## Hardware

**Required:**
- Raspberry Pi Pico or Pico W
- Maxim MAX6921AWI driver chip
- Russian IV-18 VFD display (or compatible)
- 3-wire SPI: MOSI (GPIO 11), SCK (GPIO 10), Latch (GPIO 13)
- Power supply: 12V for filament, 5V logic

**Display Specs:**
- 9 grids (digit positions)
- 7 segments + decimal point per grid
- 20-bit control word (3 command bits + 9 grid bits + 8 segment bits)

## Quick Start

### Installation

Add to your project:
```bash
git submodule add https://github.com/yourusername/libpico_vfd6921.git max6921
```

Update CMakeLists.txt:
```cmake
add_subdirectory(max6921)
target_link_libraries(your_app max6921)
```

### Basic Usage

```c
#include "max6921.h"
#include "pico/stdlib.h"

int main(void) {
    // Initialize with default settings
    vfd_error_t err = vfd_init(NULL);
    if (err != VFD_OK) {
        return 1;
    }

    // Write string to display buffer
    vfd_write_string("123456789");
    
    // Serialize and transmit via SPI
    vfd_refresh();

    while (1) {
        sleep_ms(1000);
    }
    return 0;
}
```

## API Reference

### Initialization

```c
vfd_config_t vfd_default_config(void);
vfd_error_t vfd_init(const vfd_config_t *config);
bool vfd_is_initialized(void);
vfd_error_t vfd_deinit(void);
```

Initialize the library before any display operations. `vfd_init(NULL)` uses default GPIO pins (11, 10, 13) and 2MHz SPI.

### Display Control

```c
vfd_error_t vfd_write_digit(uint8_t grid, uint8_t digit);
vfd_error_t vfd_write_segments(uint8_t grid, uint8_t segments);
vfd_error_t vfd_read_segments(uint8_t grid, uint8_t *segments);
vfd_error_t vfd_write_string(const char *str);
vfd_error_t vfd_refresh(void);
vfd_error_t vfd_clear(void);
```

Write operations modify the internal 9-byte display buffer. Call `vfd_refresh()` to serialize and transmit via SPI.

### Buffer Management

```c
vfd_display_buffer_t *vfd_get_buffer(void);
vfd_error_t vfd_fill_buffer(uint8_t segments);
```

Direct buffer access for advanced usage. Buffer changes take effect after `vfd_refresh()`.

### Custom Commands

```c
vfd_error_t vfd_send_control_command(const vfd_control_command_t *cmd);
```

Send custom commands (0-7) via the 3 command bits. Users can implement their own command handling via callbacks or external logic.

### Utilities

```c
const char *vfd_strerror(vfd_error_t error);
int vfd_segments_to_string(uint8_t segments, char *buffer, int buffer_size);
```

## Configuration

Customize hardware pins and SPI timing:

```c
vfd_config_t config = vfd_default_config();
config.pin_spi_tx = 11;           // MOSI pin (data input)
config.pin_spi_clk = 10;          // SCK pin (serial clock)
config.pin_latch = 13;            // Latch pin (output enable)
config.spi_baudrate = 2000000;    // 2 MHz serial clock
config.refresh_interval_us = 1500; // Microseconds between grid updates

vfd_init(&config);
```

**Timing Notes:**
- Lower `refresh_interval_us` = faster refresh, but higher CPU usage
- Minimum recommended: 1000 µs (9ms full refresh)
- Default: 1500 µs (13.5ms full refresh)
- Higher values may cause flicker

## Error Codes

```c
VFD_OK                  /* Operation successful */
VFD_ERR_INVALID_PARAM   /* Invalid parameter provided */
VFD_ERR_NOT_INITIALIZED /* VFD not initialized */
VFD_ERR_INVALID_GRID    /* Grid index out of range (0-8) */
VFD_ERR_INVALID_SEGMENT /* Segment value out of range */
VFD_ERR_HARDWARE        /* Hardware initialization failed */
```

## Supported Display Characters

- **0-9**: Numeric digits
- **-**: Dash/minus symbol
- **.**: Decimal point (applies to previous digit)
- **space**: Blank position

Example:
```c
vfd_write_string("123.45");  // Displays: 123.45
vfd_write_string("12 34");   // Displays: 12 34 (space between)
vfd_write_string("-99");     // Displays: -99
```

## Segment Layout

Each digit has 8 controllable segments:

```
 --A--
|     |
F     B
|--G--|
E     C
|--D--|
  dp H
```

Access segments directly for custom patterns:

```c
uint8_t custom_pattern = 0b01111111;  // All segments on except DP
vfd_write_segments(0, custom_pattern);
vfd_refresh();
```

## Implementation Details

The MAX6921 is a 20-bit shift register. Since SPI communication uses whole bytes, the library transmits 3 bytes (24 bits) total:
- **4 prefix bits** (padding, sent first)
- **20-bit control word** (shifted into position by the padding bits)

The 4 padding bits are transmitted first via MSB-first SPI protocol, which shifts them through the shift register and positions the 20-bit control word correctly for latching.

```c
// Build 20-bit word: [command (3) | grid (9) | segments (8)]
uint32_t control_word = (command << 17) | (grid_pattern << 8) | segments;

// Prepare 3 bytes for transmission (24 bits total)
// Format: [4-bit padding | 20-bit control word]
spi_data[0] = (control_word >> 16) & 0xFF;  // Byte 1: includes 4 padding bits
spi_data[1] = (control_word >> 8) & 0xFF;   // Byte 2
spi_data[2] = control_word & 0xFF;          // Byte 3

// Transmit via SPI (3 bytes, MSB first)
spi_write_blocking(spi_port, spi_data, 3);

// Pulse latch pin to apply
gpio_put(latch_pin, 1);
sleep_us(1);
gpio_put(latch_pin, 0);
```

By default, the command bits are set to 0 (display only). Custom command codes can be ORed with display data for flexible integration.

## Examples

See the `examples/` directory for complete working examples:
- `basic.c` - Simple digit cycling
- `display_time.c` - Display formatted numbers with timing

## Testing

Use the TESTING.md file in examples/ directory for comprehensive test procedures and verification steps.

## Building

Standard CMake build:

```bash
mkdir build
cd build
cmake -DPICO_SDK_PATH=/path/to/pico-sdk ..
make
```

## Performance

- **Full display refresh**: ~13.5ms (9 grids × 1.5ms each)
- **SPI clock**: 2 MHz (default, configurable)
- **Memory usage**: ~50 bytes driver state + 9 bytes display buffer
- **Latency**: <1µs from vfd_write_* to buffer update; 13.5ms to display

## Compatibility

- C11 standard
- C++ compatible (extern "C" wrapper)
- Raspberry Pi Pico and Pico W
- Other platforms with similar SPI and GPIO (partial porting needed)

## License

See LICENSE file for details.

## Support

For issues or questions, refer to CHANGELOG.md for recent changes and known issues.