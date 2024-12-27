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


void send_to_vfd() {
    // Prepare the data as three separate arrays (each holding an 8-bit byte or padded byte)
    uint8_t vfd_data1 = 0b00001010;
    uint8_t vfd_data2 = 0b10101010;
    uint8_t vfd_data = 0b10101010;
    uint8_t byte1[1] = {vfd_data1};            // First 8 bits
    uint8_t byte2[1] = {vfd_data2};     // Second 8 bits
    uint8_t byte3[1] = {vfd_data3};    // Last 8 bits, padding remaining bits with zeros

    // Transmit the data via SPI
    spi_write_blocking(SPI_PORT, byte1, 1);  // Send first byte
    spi_write_blocking(SPI_PORT, byte2, 1);  // Send second byte
    spi_write_blocking(SPI_PORT, byte3, 1);  // Send third byte

    // Toggle the Latch pin to transfer the data to the VFD outputs
    gpio_put(SPI_L, 1);
    sleep_us(1);  // Short delay for latch operation
    gpio_put(SPI_L, 0);
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

    // Example: Sending 20 bits of data (replace with your actual data)
    //uint32_t vfd_data = 0b01010101010101010101; // Example data
    //uint8_t bit_count = 20; // Sending 20 bits

    send_to_vfd();

    return 0;
}
