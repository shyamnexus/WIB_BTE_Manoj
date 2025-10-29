# Encoder GPIO Test

This directory contains code to test encoder inputs using simple GPIO edge detection to verify that encoder signals are reaching the MCU.

## Files

- `encoder_gpio_test.h` - Header file with function prototypes and data structures
- `encoder_gpio_test.c` - Implementation of GPIO-based encoder pulse counting
- `encoder_simple_test.c` - Standalone test program for encoder verification
- `encoder_gpio_test_main.c` - Alternative main program for encoder testing

## Pin Configuration

- **PA0**: Encoder A input (GPIO input with interrupt)
- **PA1**: Encoder B input (GPIO input with interrupt)  
- **PD17**: Enable pin (GPIO output, active low)

## How It Works

The GPIO test system:

1. **Configures pins as GPIO inputs** with pull-up resistors
2. **Sets up interrupt handlers** for both rising and falling edges
3. **Counts pulses** on both encoder lines
4. **Stores results** in debug variables for analysis

## Usage

### Option 1: Use with existing main.c

The GPIO test is already integrated into `main.c`. It will run after the existing encoder tests and count pulses for 10 seconds.

### Option 2: Use standalone test

Compile and run `encoder_simple_test.c` for a focused encoder test:

```c
// The test will:
// 1. Initialize GPIO pins
// 2. Enable encoder monitoring  
// 3. Count pulses for a specified duration
// 4. Store results in debug variables
```

### Option 3: Use continuous test

For oscilloscope verification, use the continuous test:

```c
encoder_gpio_test_continuous(); // Runs forever
```

## Debug Variables

The following variables are available for debugging:

```c
typedef struct {
    uint32_t encoder_a_pulses;      // Total pulses on Encoder A
    uint32_t encoder_b_pulses;      // Total pulses on Encoder B
    uint32_t encoder_a_rising;      // Rising edges on Encoder A
    uint32_t encoder_a_falling;     // Falling edges on Encoder A
    uint32_t encoder_b_rising;      // Rising edges on Encoder B
    uint32_t encoder_b_falling;     // Falling edges on Encoder B
    bool enabled;                   // Encoder enable status
    bool initialized;               // Initialization status
    bool current_a_state;           // Current state of Encoder A pin
    bool current_b_state;           // Current state of Encoder B pin
    bool enable_pin_state;          // Current state of enable pin
} encoder_gpio_data_t;
```

## Testing Steps

1. **Connect encoder** to PA0 (A) and PA1 (B) pins
2. **Connect enable signal** to PD17 (active low)
3. **Compile and run** the test program
4. **Check debug variables** to see if pulses are being counted
5. **Use oscilloscope** to verify signal integrity if needed

## Expected Results

- **If encoder is working**: Pulse counts should increase as encoder rotates
- **If encoder is not connected**: Pulse counts should remain at 0
- **If there's noise**: You might see spurious pulses

## Troubleshooting

- **No pulses counted**: Check connections, enable signal, and pull-up resistors
- **Intermittent counting**: Check for loose connections or signal integrity
- **Wrong pulse count**: Verify encoder resolution and signal quality

## Functions Available

```c
// Initialize the GPIO encoder test
bool encoder_gpio_test_init(void);

// Enable/disable encoder monitoring
bool encoder_gpio_test_enable(bool enable);

// Reset all counters to zero
void encoder_gpio_test_reset_counters(void);

// Get current encoder data
encoder_gpio_data_t encoder_gpio_test_get_data(void);

// Run test for specified duration (in milliseconds)
void encoder_gpio_test_run_duration(uint32_t duration_ms);

// Run continuous test (for oscilloscope)
void encoder_gpio_test_continuous(void);

// Verify pin configuration
void encoder_gpio_test_pin_verification(void);
```