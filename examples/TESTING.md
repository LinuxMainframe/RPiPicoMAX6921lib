# Testing and Verification Guide

How to test the library and verify hardware integration.

## Pre-Testing Checklist

### Hardware Verification
- [ ] IV-18 VFD is powered (filament heating)
- [ ] MAX6921 is powered (5V or appropriate voltage)
- [ ] Pico is powered and programmable
- [ ] SPI pins connected correctly:
  - [ ] GPIO 11 (MOSI) → MAX6921 SDI
  - [ ] GPIO 10 (SCK) → MAX6921 SCK
  - [ ] GPIO 13 (LE) → MAX6921 LE
- [ ] Ground connections verified
- [ ] No loose connections

### Software Verification
- [ ] Pico SDK installed and path set
- [ ] CMake version 3.13+
- [ ] ARM GCC toolchain available
- [ ] Library files in place

## Test 1: Compilation Test

Verify the library compiles without errors.

```bash
cd your_project
mkdir build
cd build
cmake -DPICO_SDK_PATH=/path/to/pico-sdk ..
make
```

**Expected Result:**
- No compilation errors
- `libpico_vfd6921.a` created
- Your executable created
- No warnings (or only system warnings)

**If fails:**
- Check Pico SDK path is correct
- Verify CMakeLists.txt includes library
- Check all include paths

## Test 2: Basic Initialization

Verify the library initializes correctly.

**Code:**
```c
#include "max6921.h"
#include "pico/stdlib.h"

int main(void) {
    stdio_init_all();
    
    printf("Attempting initialization...\n");
    vfd_error_t err = vfd_init(NULL);
    
    if (err != VFD_OK) {
        printf("Init failed: %s\n", vfd_strerror(err));
        return 1;
    }
    
    printf("Init successful\n");
    printf("Initialized: %s\n", 
           vfd_is_initialized() ? "yes" : "no");
    
    while(1) {
        sleep_ms(1000);
    }
    return 0;
}
```

**Expected Result:**
- Pico outputs "Attempting initialization..."
- Pico outputs "Init successful"
- Pico outputs "Initialized: yes"
- No crashes

**If fails:**
- Check SPI pin assignments
- Verify Pico can output to USB/UART
- Check power connections

## Test 3: Hardware Communication Test

Verify SPI communication to MAX6921.

**Code:**
```c
#include "max6921.h"
#include "pico/stdlib.h"

int main(void) {
    stdio_init_all();
    vfd_init(NULL);
    
    printf("Testing display update...\n");
    
    // All segments on (full brightness test)
    vfd_fill_buffer(0xFF);
    vfd_refresh();
    
    printf("Display updated\n");
    printf("All segments should be on\n");
    
    while(1);
    return 0;
}
```

**Expected Result:**
- All 9 display digits light up
- All 7 segments + decimal points visible
- Roughly equal brightness across digits

**If fails - No display activity:**
- Check Latch pin connection
- Verify SPI baudrate (try 1 MHz instead of 2 MHz)
- Check ground connections
- Verify Pico GPIO 10, 11, 13 are not in use by other code

**If fails - Partial segments:**
- One or two segments off: possible SPI timing issue
- Try increasing refresh_interval_us to 2000
- Check for noise on signal lines with oscilloscope

**If fails - Flickering:**
- Increase refresh_interval_us
- Reduce SPI baudrate
- Check for power supply noise

## Test 4: Digit Display Test

Verify each digit position works independently.

**Code:**
```c
#include "max6921.h"
#include "pico/stdlib.h"

int main(void) {
    stdio_init_all();
    vfd_init(NULL);
    
    printf("Testing each grid position...\n");
    
    for (int grid = 0; grid < 9; grid++) {
        vfd_clear();
        vfd_write_digit(grid, 8);  // Display "8"
        vfd_refresh();
        
        printf("Grid %d: displaying 8\n", grid);
        sleep_ms(1000);
    }
    
    printf("All grids tested\n");
    while(1);
    return 0;
}
```

**Expected Result:**
- "8" appears in each position left to right
- One position at a time
- Each digit fully visible (all 7 segments)

**If fails:**
- One position blank: possible grid control issue
- One position very dim: possible SPI data issue
- Wrong digit displayed: segment pattern incorrect

## Test 5: Digit Range Test

Verify all digits display correctly.

**Code:**
```c
#include "max6921.h"
#include "pico/stdlib.h"

int main(void) {
    stdio_init_all();
    vfd_init(NULL);
    
    printf("Testing digit range 0-9...\n");
    
    for (int digit = 0; digit < 10; digit++) {
        vfd_clear();
        vfd_write_digit(4, digit);  // Center position
        vfd_refresh();
        
        printf("Digit: %d\n", digit);
        sleep_ms(500);
    }
    
    printf("Digit test complete\n");
    while(1);
    return 0;
}
```

**Expected Result:**
- Digits 0-9 appear in sequence
- Each clearly distinguishable
- Clean, standard 7-segment display

**If fails:**
- Wrong segments lit: segment mapping issue
- Segments not lighting: SPI timing issue

## Test 6: String Display Test

Verify text display capability.

**Code:**
```c
#include "max6921.h"
#include "pico/stdlib.h"

int main(void) {
    stdio_init_all();
    vfd_init(NULL);
    
    printf("Testing string display...\n");
    
    const char *test_strings[] = {
        "123456789",
        "1-2-3",
        "12 34 56",
        "999",
        "00000",
        NULL
    };
    
    for (int i = 0; test_strings[i] != NULL; i++) {
        vfd_write_string(test_strings[i]);
        vfd_refresh();
        printf("String: %s\n", test_strings[i]);
        sleep_ms(2000);
    }
    
    printf("String test complete\n");
    while(1);
    return 0;
}
```

**Expected Result:**
- Each string appears correctly
- Dashes visible in right positions
- Spaces appear as blank digits
- Characters align properly

**If fails:**
- Characters in wrong positions: grid numbering issue
- Dashes not visible: special character handling issue

## Test 7: Configuration Test

Verify custom configuration works.

**Code:**
```c
#include "library.h"
#include "pico/stdlib.h"

int main(void) {
    stdio_init_all();
    
    printf("Testing custom configuration...\n");
    
    vfd_config_t cfg = vfd_default_config();
    printf("Default SPI baudrate: %u\n", cfg.spi_baudrate);
    printf("Default refresh interval: %u us\n", 
           cfg.refresh_interval_us);
    
    // Try slower SPI if needed for reliability
    cfg.spi_baudrate = 1000000;  // 1 MHz
    cfg.refresh_interval_us = 2000;  // 2 ms
    
    vfd_error_t err = vfd_init(&cfg);
    if (err != VFD_OK) {
        printf("Custom config failed: %s\n", vfd_strerror(err));
        return 1;
    }
    
    printf("Custom config successful\n");
    
    vfd_write_string("CONFIG");
    vfd_refresh();
    
    while(1);
    return 0;
}
```

**Expected Result:**
- Custom configuration accepted
- Library initializes successfully
- Display operates with new settings

**If fails:**
- Check configuration values are valid
- Verify SPI baudrate in reasonable range

## Test 8: Error Handling Test

Verify error detection works.

**Code:**
```c
#include "library.h"
#include "pico/stdlib.h"

int main(void) {
    stdio_init_all();
    
    printf("Testing error handling...\n");
    
    // Test 1: Operation before init
    vfd_error_t err = vfd_write_digit(0, 5);
    printf("Write before init: %s\n", vfd_strerror(err));
    
    // Initialize
    vfd_init(NULL);
    
    // Test 2: Invalid grid
    err = vfd_write_digit(9, 5);  // Grid 9 doesn't exist
    printf("Invalid grid: %s\n", vfd_strerror(err));
    
    // Test 3: Valid operation
    err = vfd_write_digit(0, 5);
    printf("Valid operation: %s\n", vfd_strerror(err));
    
    // Test 4: Read from initialized display
    uint8_t segments;
    err = vfd_read_segments(0, &segments);
    printf("Read segments: %s\n", vfd_strerror(err));
    
    printf("Error handling test complete\n");
    while(1);
    return 0;
}
```

**Expected Result:**
```
Write before init: VFD not initialized
Invalid grid: Grid index out of range
Valid operation: Operation successful
Read segments: Operation successful
```

**If fails:**
- Error codes not matching: API implementation issue

## Test 9: Rapid Update Test

Verify display can handle rapid updates without corruption.

**Code:**
```c
#include "max6921.h"
#include "pico/stdlib.h"

int main(void) {
    stdio_init_all();
    vfd_init(NULL);
    
    printf("Testing rapid updates...\n");
    
    for (int cycle = 0; cycle < 100; cycle++) {
        for (int digit = 0; digit < 10; digit++) {
            vfd_clear();
            vfd_write_digit(0, digit);
            vfd_refresh();
        }
        
        if (cycle % 10 == 0) {
            printf("Cycle %d complete\n", cycle);
        }
    }
    
    printf("Rapid update test complete\n");
    vfd_write_string("DONE");
    vfd_refresh();
    
    while(1);
    return 0;
}
```

**Expected Result:**
- Digits cycle smoothly 0-9 repeatedly
- No flickering or corruption
- Display remains stable throughout

**If fails:**
- Garbled display: SPI timing too fast
- Intermittent corruption: possible EMI
- Display locks up: infinite loop or hang

## Test 10: Stress Test

Verify stability under extended operation.

**Code:**
```c
#include "max6921.h"
#include "pico/stdlib.h"

int main(void) {
    stdio_init_all();
    vfd_init(NULL);
    
    printf("Running stress test (60+ seconds)...\n");
    
    uint32_t start_time = time_us_32();
    uint32_t updates = 0;
    
    while (time_us_32() - start_time < 60000000) {  // 60 seconds
        static uint8_t counter = 0;
        vfd_clear();
        vfd_write_digit(0, counter % 10);
        vfd_write_digit(1, (counter / 10) % 10);
        vfd_write_digit(2, (counter / 100) % 10);
        vfd_refresh();
        
        counter++;
        updates++;
        sleep_ms(100);
    }
    
    printf("Stress test complete\n");
    printf("Total updates: %u\n", updates);
    printf("Expected: ~600\n");
    
    vfd_write_string("PASS");
    vfd_refresh();
    
    while(1);
    return 0;
}
```

**Expected Result:**
- Runs for full 60 seconds without hanging
- Display remains operational
- No gradual degradation
- Finishes with "PASS" displayed

**If fails:**
- Hangs after N iterations: possible resource leak
- Display degrades: possible buffer overflow
- Random crashes: memory corruption

## Performance Validation

### Timing Test
```c
// Measure refresh time
uint32_t start = time_us_32();
vfd_refresh();
uint32_t elapsed = time_us_32() - start;
printf("Refresh time: %u microseconds\n", elapsed);
// Expected: ~13500 microseconds (9 grids × 1500 µs)
```

### SPI Verification (with oscilloscope)
1. Monitor GPIO 10 (SCK) - should see square wave pulses
2. Monitor GPIO 11 (MOSI) - should see data pattern
3. Monitor GPIO 13 (LE) - should see pulse after each 3-byte SPI sequence

**Expected pattern:**
- SCK: 2 MHz clock (default)
- MOSI: Data changes with clock
- LE: Low-to-high pulse after 24 bits (3 bytes)

## Test Result Log

Use this template to document your testing:

```
Device: [Pico model]
Display: [IV-18 or variant]
Date: [date]

Test 1 - Compilation:        [PASS/FAIL]
Test 2 - Initialization:     [PASS/FAIL]
Test 3 - Hardware Comm:      [PASS/FAIL]
Test 4 - Digit Display:      [PASS/FAIL]
Test 5 - Digit Range:        [PASS/FAIL]
Test 6 - String Display:     [PASS/FAIL]
Test 7 - Configuration:      [PASS/FAIL]
Test 8 - Error Handling:     [PASS/FAIL]
Test 9 - Rapid Updates:      [PASS/FAIL]
Test 10 - Stress Test:       [PASS/FAIL]

Overall Status: [WORKING/NEEDS_DEBUG]

Notes:
[Any observations or issues]
```

## Troubleshooting Tests

If hardware test fails, try these diagnostic tests:

### Diagnostic 1: GPIO Test
```c
// Verify GPIO pins respond to commands
gpio_init(13);
gpio_set_dir(13, GPIO_OUT);
gpio_put(13, 1);
sleep_ms(1000);
gpio_put(13, 0);
// Measure voltage on pin 13 with multimeter
```

### Diagnostic 2: SPI Clock Test
```c
// Verify SPI clock is running
// Use oscilloscope on GPIO 10 while calling:
vfd_refresh();
// Should see 2 MHz square wave
```

### Diagnostic 3: Data Pattern Test
```c
// Verify SPI data is being sent
// Use logic analyzer to capture 3-byte SPI sequence
vfd_write_digit(0, 5);
vfd_refresh();
// Check output matches expected bit pattern
```

## What to Check if Tests Fail

| Issue | Check |
|-------|-------|
| Compilation error | Pico SDK path, CMake version |
| Init fails | Pin assignments, power |
| No display activity | Latch pin, ground connections |
| Partial segments | SPI baudrate, timing |
| Flickering | Refresh interval, power supply |
| Garbled text | Segment mapping, SPI speed |
| Wrong digits | Grid numbering, SPI data |
| Random crashes | Memory corruption, buffer size |

## Success Criteria

All tests pass when:
- ✅ Compilation succeeds without errors
- ✅ All 9 digits display correctly
- ✅ All digits 0-9 display properly
- ✅ Strings display with correct spacing
- ✅ Error handling catches invalid operations
- ✅ Updates are smooth without corruption
- ✅ System runs stably for 60+ seconds
- ✅ Performance meets specifications

## Next Steps

Once all tests pass:
1. Integrate library into your application
2. Add time keeping (RTC) if needed
3. Add button/sensor input handling
4. Implement your display logic
5. Test complete system

Library is production-ready for your VFD project!