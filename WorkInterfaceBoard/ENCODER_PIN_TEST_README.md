# Encoder Pin Toggle Test for Oscilloscope Verification

This test helps verify that the encoder pins are properly soldered by toggling them in a specific pattern that can be observed on an oscilloscope.

## Pin Configuration

Based on the pin configuration file, the encoder pins are:
- **PA0 (Pin 52)**: Encoder A input (TIOA0)
- **PA1 (Pin 70)**: Encoder B input (TIOB0)  
- **PD17 (Pin 53)**: Encoder Enable (active low)

## Test Functions Available

### 1. `encoder1_pin_toggle_test()`
- Simple test that toggles each pin individually for 1 second each
- Each pin toggles at ~1kHz frequency
- Good for basic verification

### 2. `encoder1_test_all_pins_sequence()`
- Comprehensive test with multiple phases:
  - Phase 1: All pins low (2s) - baseline
  - Phase 2: PA0 square wave 1Hz (3s) - Encoder A
  - Phase 3: PA1 square wave 2Hz (3s) - Encoder B
  - Phase 4: PD17 square wave 0.5Hz (4s) - Enable
  - Phase 5: All pins high (2s)
  - Phase 6: Alternating PA0/PA1 pattern (2s)
  - Phase 7: All pins low (2s) - end

### 3. `encoder1_standalone_pin_test()`
- Runs the comprehensive test in a continuous loop
- 5-second pause between test cycles
- Perfect for oscilloscope setup and verification

## How to Use

### Option 1: Use the test in main.c (current setup)
The test is already integrated into the main application and will run once at startup.

### Option 2: Use standalone test
1. Replace `main.c` with `pin_test_main.c` temporarily
2. Build and flash the project
3. The test will run continuously

### Option 3: Call test functions manually
You can call any of the test functions from your code:
```c
encoder1_pin_toggle_test();           // Simple test
encoder1_test_all_pins_sequence();    // Comprehensive test (runs once)
encoder1_standalone_pin_test();       // Continuous test
```

## Oscilloscope Setup

1. **Connect probes to the encoder pins:**
   - Channel 1: PA0 (Pin 52) - Encoder A
   - Channel 2: PA1 (Pin 70) - Encoder B
   - Channel 3: PD17 (Pin 53) - Encoder Enable

2. **Oscilloscope settings:**
   - Timebase: 1-2 seconds per division
   - Voltage: 3.3V scale
   - Trigger: Edge trigger on any channel

3. **What to look for:**
   - All pins should show clear square wave patterns
   - No floating or undefined states
   - Proper 3.3V high levels and 0V low levels
   - Clean transitions without excessive noise

## Troubleshooting

### If a pin shows no activity:
- Check soldering on that specific pin
- Verify the pin is not shorted to ground or VCC
- Check for cold solder joints

### If a pin shows erratic behavior:
- Check for loose connections
- Look for solder bridges to adjacent pins
- Verify the pin is not damaged

### If all pins show the same pattern:
- Check if pins are shorted together
- Verify the MCU is running the test code
- Check power supply stability

## Test Pattern Identification

The comprehensive test uses different frequencies to make pin identification easy:
- **PA0 (Encoder A)**: 1Hz square wave (slow)
- **PA1 (Encoder B)**: 2Hz square wave (medium)  
- **PD17 (Enable)**: 0.5Hz square wave (very slow)

This makes it easy to identify which pin is which on the oscilloscope display.

## Restoring Normal Operation

After testing, the pins can be restored to peripheral mode for normal encoder operation:
```c
encoder1_restore_pins_as_peripheral();
```

Or simply restart the application with the normal main.c file.