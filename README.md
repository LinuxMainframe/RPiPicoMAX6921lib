# Simple MAXIM MAX6921AWI Driver Code using Raspberry Pi Pico W

## Overview

This project demonstrates how to drive a VFD (Vacuum Fluorescent Display) using a Raspberry Pi Pico W and the MAXIM MAX6921AWI chip. The display setup includes a 7-segment display with a decimal point and 8 grids, controlled using a 20-bit buffer, leaving room for additional control logic. The VFD of choice this time around is the Soviet (Sovtek) IV-18 a.k.a _ИВ-18 (Совтек)_. 

## Pin Mapping and Layout

### Mapping Layout

- **Display Details**:
  - The display consists of a 7-segment display + 1 dot (decimal point).
  - There are 8 grids (digits), requiring 16 pins for control (8 segment pins + 8 grid pins).
  - Only one grid pin is active high at a time, leaving 4 unused bits, which can be reserved for future use.

- **Pin Mapping**:
  - **Pins 0–7**: Drive the segments (7-segment + dot).
  - **Pins 8–15**: Control the grids (digits).

- **Buffer Details**:
  - This setup uses 2 bytes (16 bits), but the MAXIM MAX6921AWI chip requires 20 bits.
  - The 4 remaining bits can be padded or reserved for control logic.
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

The remaining 4 bits of the 20-bit buffer can be used for control logic. Here is the mapping of the control bits:

### Control Bits Command Table

| Decimal | Binary | Hexadecimal | Command Description |
|---------|--------|-------------|---------------------|
| 0       | 0000   | 0x0         | N/A                 |
| 1       | 0001   | 0x1         | N/A                 |
| 2       | 0010   | 0x2         | N/A                 |
| 3       | 0011   | 0x3         | N/A                 |
| 4       | 0100   | 0x4         | N/A                 |
| 5       | 0101   | 0x5         | N/A                 |
| 6       | 0110   | 0x6         | N/A                 |
| 7       | 0111   | 0x7         | N/A                 |
| 8       | 1000   | 0x8         | N/A                 |
| 9       | 1001   | 0x9         | N/A                 |
| 10      | 1010   | 0xA         | N/A                 |
| 11      | 1011   | 0xB         | N/A                 |
| 12      | 1100   | 0xC         | N/A                 |
| 13      | 1101   | 0xD         | N/A                 |
| 14      | 1110   | 0xE         | N/A                 |
| 15      | 1111   | 0xF         | N/A                 |

---

## Example Driver Code

### Sending Data to the VFD
```c
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

    // Implementation here...
}
