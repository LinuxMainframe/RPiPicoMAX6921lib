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

// Segment control and grid control arrays
uint8_t segment_control[] = {
  0b00111111, // 0: A B C D E F 
  0b00000110, // 1: B C
  0b01011011, // 2: A B D E G
  0b01001111, // 3: A B C D G
  0b01100110, // 4: B C F G
  0b01101101, // 5: A C D F G
  0b01111101, // 6: A C D E F G
  0b00000111, // 7: A B C
  0b01111111, // 8: A B C D E F G
  0b01101111,  // 9: A B C D F G
  0b10000000, // decimal: H (10)
  0b01000000, // dash: G (11)
  0b00000000 //blank (12)
};

uint16_t grid_control[] = {
  0b100000000, //grid 0 (decimal)
  0b010000000, //grid 1
  0b001000000, //grid 2
  0b000100000, //grid 3
  0b000010000, //grid 4
  0b000001000, //grid 5
  0b000000100, //grid 6
  0b000000010, //grid 7
  0b000000001 //grid 8
};

uint8_t state[9] = {
  11,
  1,
  2,
  3,
  4,
  5,
  6,
  7,
  8
};

//define global variables
uint8_t data[3];
uint8_t digit;
uint8_t grid;
uint32_t combined_data;

//function to update the vfd
void write_vfd(uint8_t digit, uint8_t grid) {
  combined_data = (grid_control[grid] << 8) | segment_control[digit];
  //printf("Combined Data(decimal): %u\n", combined_data);
  
  data[0] = (combined_data >> 16) & 0xFF;
  data[1] = (combined_data >> 8) & 0xFF;   
  data[2] = combined_data & 0xFF;  
  
  //printf("Array contents:\n");
  //printf("byte1: %u\n", data[0]);
  //printf("byte2: %u\n", data[1]);
  //printf("byte3: %u\n", data[2]);

  spi_write_blocking(SPI_PORT, data, 3);
  
  gpio_put(SPI_L, 1);
  sleep_us(1);
  gpio_put(SPI_L, 0);
}  
  
int main() {
  stdio_init_all();
  sleep_ms(1000);
  printf("Starting Main\n");
  
  //init SPI pins
  spi_init(SPI_PORT, SPI_BAUDRATE);
  gpio_set_function(SPI_C, GPIO_FUNC_SPI); // CLOCK
  gpio_set_function(SPI_D, GPIO_FUNC_SPI); // DATA
  printf("Initialized SPI. CLK: %d DATA: %d\n", SPI_C, SPI_D);
  
  //init latch pin
  gpio_init(SPI_L);
  gpio_set_dir(SPI_L, GPIO_OUT);
  printf("Initialized Latch Pin: %d\n", SPI_L);
  
  //write_vfd(11, 0);
  
  uint8_t counter = 0;
  while (1) {
    write_vfd(state[counter], counter);
    counter ++;
    if (counter == 9) {
      counter=0;
    }
    sleep_ms(1);
  }
}
