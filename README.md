# Simple MAXIM MAX6921AWI Driver Code using Raspberry Pi Pico W

## Overview

This project demonstrates how to drive a VFD (Vacuum Fluorescent Display) using a Raspberry Pi Pico W and the MAXIM MAX6921AWI chip. The display setup includes a 7-segment display with a decimal point and 8 grids, controlled using a 20-bit buffer, leaving room for additional control logic. The VFD of choice this time around is the Soviet (Sovtek) IV-18 a.k.a _ИВ-18 (Совтек)_. 

## Pin Mapping and Layout

### Mapping Layout

- **Display Details**:
  - The display consists of a 7-segment display + 1 dot (decimal point).
  - There are ~~8~~ 9 grids (digits), requiring ~~16~~ 17 pins for control (8 segment pins + ~~8~~ 9 grid pins).
  - Only one grid pin is active high at a time, leaving ~~4~~ 3 unused bits, which can be reserved for future use.

- **Pin Mapping**:
  - **Pins 0–7**: Drive the segments (7-segment + dot).
  - **Pins 8–16**: Control the grids (digits).

- **Buffer Details**:
  - This setup uses 2 bytes + 1 bit (~~16~~ 17 bits), but the MAXIM MAX6921AWI chip requires 20 bits.
  - The ~~4~~ 3 remaining bits can be padded or reserved for control logic.
  - The chip's clear pin can be triggered via the Pico, potentially using the last 4 output pins. 

  **Voltage Note**: Driving the clear pin requires careful consideration of the bus voltage (e.g., 25V for IV-18). A resistor ladder can step down voltages if needed.

---

### VFD Segment Display Matrix

#### Physical VFD Pin Mapping:
```
 --         <- Top segment (Pin 12)
|  |        <- Left (Pin 11) and Right (Pin 10) middle segments
 --         <- Middle segment (Pin 9)
|  |        <- Left (Pin 5) and Right (Pin 4) bottom segments
 -- .       <- Bottom segment (Pin 3) and decimal point (Pin 2)
```

#### Logical Segment Mapping (A–H):
```
 --         <- Segment A (Top)
|  |        <- Segments F (Left) and B (Right)
 --         <- Segment G (Middle)
|  |        <- Segments E (Left) and C (Right)
 -- .       <- Segment D (Bottom) and H (Decimal point)
```
#### Alphanumeric Segment Pinout:
| VFD Pin | Segment |
|---------|---------|
| Pin 12  | A       |
| Pin 11  | F       |
| Pin 10  | B       |
| Pin 9   | G       |
| Pin 5   | E       |
| Pin 4   | C       |
| Pin 3   | D       |
| Pin 2   | H (Decimal Point) |
| Pin 1, 13 | Heating elements (not coded) |

---

## Control Logic and Commands

The remaining ~~4~~ 3 bits of the 20-bit buffer can be used for control logic. Here is the mapping of the control bits:

### Control Bits Command Table

| Decimal | Binary | Hexadecimal | Command Description |
|---------|--------|-------------|---------------------|
| 0       | 000   | 0x0         | N/A                 |
| 1       | 001   | 0x1         | N/A                 |
| 2       | 010   | 0x2         | N/A                 |
| 3       | 011   | 0x3         | N/A                 |
| 4       | 100   | 0x4         | N/A                 |
| 5       | 101   | 0x5         | N/A                 |
| 6       | 110   | 0x6         | N/A                 |
| 7       | 111   | 0x7         | N/A                 |

---

## Example Driver Code

### Sending Data to the VFD
```c
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
```
- Sending data via SPI to the Maxim chip requires the data to be loaded backwards, hence sending the high byte first
- Data separately to that, the data must be sent LSB style via SPI, which was taken care of by swapping the order of the predefined segment and grid code
- With the data flipped appropriately, and the byte order sent backwards, we achieve intercommunication with the VFD driver chip
  - Data segments are 0-9, so you must reverse them 9-0. Grid segments are the same. It should read PADDING + GRIDCONT + SEGMCONT => CONTR-SEQ
  - CONTR-SEQ is split into three bytes, then sent VIA SPI bus.
  - SPI does not have a 3rd wire, but since the maxim chip does not display any output until the latch pin is triggered, we added a 3rd pin for sending a latch signal to. It is pretty fast, so 1 microsecond was used as a buffer between setting it high and then back to low
