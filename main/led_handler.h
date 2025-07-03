#ifndef LED_HANDLER_H
#define LED_HANDLER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Configuration structure for LED strip
typedef struct {
    uint8_t red;           // Red component (0-255)
    uint8_t green;         // Green component (0-255)
    uint8_t blue;          // Blue component (0-255)
    uint8_t intensity;     // Overall intensity (0-255)
    uint32_t speed_ms;     // Speed/timing parameter in milliseconds
    uint32_t duration_ms;  // Duration of pattern in milliseconds (0 = infinite)
    uint8_t pattern;       // Pattern type (0-6)
} led_config_t;

// Pattern definitions - using enum instead of #define to avoid conflicts
typedef enum {
    LED_PATTERN_STATIC = 0,   // Solid color
    LED_PATTERN_BLINK = 1,    // Blinking pattern
    LED_PATTERN_RAINBOW = 2,  // Rainbow color cycle
    LED_PATTERN_BREATH = 3,   // Breathing/pulse effect
    LED_PATTERN_FADE = 4,     // Fade in/out
    LED_PATTERN_GRADIENT = 5, // Color gradient
    LED_PATTERN_TWINKLE = 6   // Twinkling effect
} led_pattern_type_t;

// Global LED configuration (can be modified by BLE)
extern led_config_t led_strip_config;

// Core LED handler functions
void led_handler_init(void);
void led_handler_deinit(void);

// Pattern control functions
void animate_pattern(const led_config_t *cfg);
void led_handler_update_config(const led_config_t *new_config);
void led_handler_clear(void);

// Demo and utility functions
void led_handler_demo(void);
void led_handler_set_custom_gradient(uint8_t r1, uint8_t g1, uint8_t b1,
                                   uint8_t r2, uint8_t g2, uint8_t b2,
                                   uint8_t intensity);

#ifdef __cplusplus
}
#endif

#endif // LED_HANDLER_H