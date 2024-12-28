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


// Define the 7-segment display encodings (using only 8 bits for each digit)
uint8_t segment_control[] = {
    0b00111111, // 0: A B C D E F (All segments except G)
    0b00000110, // 1: B C
    0b01011011, // 2: A B D E G
    0b01001111, // 3: A B C D G
    0b01100110, // 4: B C F G
    0b01101101, // 5: A C D F G
    0b01111101, // 6: A C D E F G
    0b00000111, // 7: A B C
    0b01111111, // 8: A B C D E F G (All segments)
    0b01101111  // 9: A B C D F G
};

uint8_t grid_control[] = {
    0b00000001, // grid 1
    0b00000010, // grid 2
    0b00000100, // grid 3
    0b00001000, // grid 4
    0b00010000, // grid 5
    0b00100000, // grid 6
    0b01000000, // grid 7
    0b10000000  // grid 8
}


// Function to combine grid control and segment control into a 16-bit value
uint16_t combine_grid_segment(uint8_t digit, uint8_t grid) {
    // Ensure the digit and grid are within valid bounds
    if (digit > 9 || grid > 7) {
        printf("Invalid digit or grid value\n");
        return 0;
    }

    // Shift the grid control to the high byte (left by 8 bits) and combine with segment control
    uint16_t combined_value = (grid_control[grid] << 8) | segment_control[digit];
    return combined_value;
}

void latch_data() {
    // Toggle the latch pin to transfer the data to the VFD outputs
    gpio_put(SPI_L,1);  // Set latch pin high.
    sleep_us(1);        // Short delay for latch operation... plenty of time to wait.
    gpio_put(SPI_L, 0); // Set latch pin low.
}

// Function to split 20-bit data into three padded bytes
void prepare_vfd_data(uint32_t control_sequence, uint8_t *frame1, uint8_t *frame2, uint8_t *frame3) {

    uint32_t padded_seq = (uint32_t) (control_sequence << 4); //pad the Least Sig Bits by 4 since adding them at the end will cause overwriting
                                                              // essentially we will have to overwrite, so we put the padding first so we overwrite the padding with our desired bits.
    *frame1 = (uint8_t) (padded_seq & 0xFF);         //select the first 8 bits
    *frame2 = (uint8_t) ((padded_seq >> 8) & 0xFF);  //shift the next 8 least significant bits right, and then select them
    *frame3 = (uint8_t) ((padded_seq >> 16) & 0xFF); //shift the entire bit seq to the right 16 bits to grab the last 8 bits
}

// Function to send 20-bit data to the VFD via SPI
void send_to_vfd(uint32_t control_sequence) {
    uint8_t vfd_data1, vfd_data2, vfd_data3;

    // Prepare the data frames
    prepare_vfd_data(control_sequence, &vfd_data1, &vfd_data2, &vfd_data3);

    // Transmit the data via SPI
    spi_write_blocking(SPI_PORT, &vfd_data1, 1);  // Send first byte
    spi_write_blocking(SPI_PORT, &vfd_data2, 1);  // Send second byte
    spi_write_blocking(SPI_PORT, &vfd_data3, 1);  // Send third byte

    latch_data();
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

    // Example control sequence (20 bits)
    // The lower 8 bits control segments, and the next 8 bits control grids.
    // Ensure only one grid is active high at a time.
    uint32_t control_sequence = 0b00000001111000000101; // Example sequence

    // Send the sequence to the VFD
    send_to_vfd(combine_grid_segment(3,3));

    return 0;
}
