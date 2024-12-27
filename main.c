#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/spi.h"

// Define pin connections
const uint SPI_D = 11; // MOSI (SPI TX)
const uint SPI_C = 10; // SCK (SPI Clock)
const uint SPI_L = 13; // Latch pin (GPIO)

// SPI configurations
#define SPI_PORT spi1 // Using SPI1 interface
#define SPI_BAUDRATE 5000000 // 5 MHz

// MAPPING LAYOUT
// - The display consists of a 7-segment display + 1 dot (decimal point).
// - There are 8 grids (digits), requiring 16 pins for control (8 segment pins + 8 grid pins).
// - Only one grid pin is active high at a time, meaning 4 bits are left unused (can be used later for special cases).
// - Pin mapping:
//   - Pins 0–7: Drive the segments (7-segment + dot).
//   - Pins 8–15: Control the grids (digits).
// - This setup uses 2 bytes (16 bits), but the Maxim MAX6921AWI chip requires 20 bits. 
//   - The remaining 4 bits will need to be padded or reserved for future functionality.
// - The chip's clear pin can be triggered via the Pico, possibly using the last 4 output pins. 
//   - Note: Driving the clear pin requires careful consideration of the bus voltage (e.g., 25V for IV-18). 
//   - A resistor ladder could be used to step down voltages for the clear pin if necessary.

// VFD SEGMENT DISPLAY MATRIX:
// - Segment-to-pin mapping (physical VFD pins):
//     --         <- Top segment (Pin 12)
//    |  |        <- Left (Pin 11) and Right (Pin 10) middle segments
//     --         <- Middle segment (Pin 9)
//    |  |        <- Left (Pin 5) and Right (Pin 4) bottom segments
//     -- .       <- Bottom segment (Pin 3) and decimal point (Pin 2)

// - Logical segment notation (using letters A–H):
//     --         <- Segment A (Top)
//    |  |        <- Segments F (Left) and B (Right)
//     --         <- Segment G (Middle)
//    |  |        <- Segments E (Left) and C (Right)
//     -- .       <- Segment D (Bottom) and H (Decimal point)

// ALPHANUMERIC SEGMENT PINOUT:
// - VFD pins mapped to segment labels for simplified logic and control:
//     Pin 12 = A                     Pin 1 and Pin 13 = Heating elements (not coded here)
//     Pin 11 = F                     Segment outputs from the Maxim chip are wired to VFD pins.
//     Pin 10 = B                     Logical control is simplified by remapping segments A–H to sequential
//     Pin 9 = G                      outputs on the Maxim chip, ignoring original VFD pinout.
//     Pin 5 = E                      For example, Maxim chip output for Segment A is wired to VFD Pin 12.
//     Pin 4 = C
//     Pin 3 = D
//     Pin 2 = H (Decimal point)hru H, and those outputs
//                                  will be wired to the appropriate VFD pinout.

void send_to_vfd() {
    // Prepare the data as three separate bytes.
    // The MAX6921AWI chip requires 20 bits of data, which must be sent as padded 8-bit frames.
    // Padding ensures proper alignment since the chip reads the least significant bit (LSB) first.
    // To determine the padding required:
    //    n - (x mod n) = padding, where:
    //        n = frame size (8 bits),
    //        x = data size (20 bits in this case).
    // Example: For 20 bits, padding = 8 - (20 mod 8) = 4 bits.
    // This padding must precede the data in the transmission buffer for correct operation.

    uint8_t vfd_data1 = 0b00001010;    // First 8 bits (includes 4 bits of padding).
    uint8_t vfd_data2 = 0b10101010;    // Next 8 bits of the data.
    uint8_t vfd_data3 = 0b10101010;    // Remaining 4 bits padded to a full byte.

    // Transmit the data via SPI.
    spi_write_blocking(SPI_PORT, &vfd_data1, 1);  // Send first byte.
    spi_write_blocking(SPI_PORT, &vfd_data2, 1);  // Send second byte.
    spi_write_blocking(SPI_PORT, &vfd_data3, 1);  // Send third byte.

    // Toggle the latch pin to transfer the data to the VFD outputs.
    gpio_put(SPI_L, 1);  // Set latch pin high.
    sleep_us(1);         // Short delay for latch operation.
    gpio_put(SPI_L, 0);  // Set latch pin low.
}


int main() {
    stdio_init_all();

    // SPI initialization (using SPI1)
    printf("Hello, Max6921! Drawing things on a VFD matrix...\n");

    // Initialize SPI port
    spi_init(SPI_PORT, SPI_BAUDRATE);

    // Set the GPIO functions for SPI
    gpio_set_function(SPI_C, GPIO_FUNC_SPI); // SCK
    gpio_set_function(SPI_D, GPIO_FUNC_SPI); // MOSI

    // Configure the latch pin (GPIO)
    gpio_init(SPI_L);
    gpio_set_dir(SPI_L, GPIO_OUT);

    send_to_vfd();

    return 0;
}
