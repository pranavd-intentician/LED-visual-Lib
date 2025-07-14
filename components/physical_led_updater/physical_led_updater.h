// #ifndef LED_HANDLER_H
// #define LED_HANDLER_H

// #include <stdint.h>
// #include <stdbool.h>

// #ifdef __cplusplus
// extern "C" {
// #endif

// // Configuration structure for LED strip
// typedef struct {
//     uint8_t red;           // Red component (0-255)
//     uint8_t green;         // Green component (0-255)
//     uint8_t blue;          // Blue component (0-255)
//     uint8_t intensity;     // Overall intensity (0-255)
//     uint32_t speed_ms;     // Speed/timing parameter in milliseconds
//     uint32_t duration_ms;  // Duration of pattern in milliseconds (0 = infinite)
//     uint8_t pattern;       // Pattern type (0-7)
// } led_config_t;

// // Edge configuration structure for individual edge control
// typedef struct {
//     uint8_t edge_id;       // Edge identifier (0-3)
//     led_config_t config;   // LED configuration for this edge
// } edge_config_t;

// // Pattern definitions - using enum instead of #define to avoid conflicts
// typedef enum {
//     LED_PATTERN_STATIC = 0,     // Solid color
//     LED_PATTERN_BLINK = 1,      // Blinking pattern
//     LED_PATTERN_RAINBOW = 2,    // Rainbow color cycle
//     LED_PATTERN_BREATH = 3,     // Breathing/pulse effect
//     LED_PATTERN_FADE = 4,       // Fade in/out
//     LED_PATTERN_GRADIENT = 5,   // Color gradient
//     LED_PATTERN_TWINKLE = 6,    // Twinkling effect
//     LED_PATTERN_SHIFT = 7       // Shifting pattern
// } led_pattern_type_t;

// // Global LED configuration (can be modified by BLE)
// extern led_config_t led_strip_config;

// // Edge configurations for individual control
// extern edge_config_t edge_configs[4];

// // Core LED handler functions
// void led_handler_init(void);
// void led_handler_deinit(void);

// // Pattern control functions
// void animate_pattern(const led_config_t *cfg);
// void animate_edge_pattern(uint8_t edge_id, const led_config_t *cfg);
// void animate_all_edges_pattern(const led_config_t *cfg);
// void led_handler_update_config(const led_config_t *new_config);
// void led_handler_update_edge_config(uint8_t edge_id, const led_config_t *new_config);
// void led_handler_clear(void);
// void led_handler_clear_edge(uint8_t edge_id);

// // Demo and utility functions
// void led_handler_demo(void);
// void led_handler_edge_demo(void);
// void led_handler_set_custom_gradient(uint8_t r1, uint8_t g1, uint8_t b1,
//                                    uint8_t r2, uint8_t g2, uint8_t b2,
//                                    uint8_t intensity);
// void led_handler_set_edge_gradient(uint8_t edge_id, uint8_t r1, uint8_t g1, uint8_t b1,
//                                  uint8_t r2, uint8_t g2, uint8_t b2,
//                                  uint8_t intensity);

// // Edge control functions
// void led_handler_set_edge_pattern(uint8_t edge_id, uint8_t pattern, 
//                                 uint8_t r, uint8_t g, uint8_t b, uint8_t intensity,
//                                 uint32_t speed_ms, uint32_t duration_ms);
// void led_handler_stop_edge_pattern(uint8_t edge_id);
// void led_handler_stop_all_patterns(void);

// // Pattern testing functions
// void led_handler_test_pattern(uint8_t pattern_id);
// void led_handler_test_edge_pattern(uint8_t edge_id, uint8_t pattern_id);

// #ifdef __cplusplus
// }
// #endif

// #endif // LED_HANDLER_H

#ifndef LED_HANDLER_H
#define LED_HANDLER_H

#include <stdint.h>
#include <stdbool.h>

// Available LED patterns
typedef enum {
    LED_PATTERN_OFF = 0,
    LED_PATTERN_STATIC,
    LED_PATTERN_BLINK,
    LED_PATTERN_BREATH,
    LED_PATTERN_RAINBOW,
    LED_PATTERN_FADE_IN,
    LED_PATTERN_FADE_OUT,
    LED_PATTERN_TWINKLE
} led_pattern_t;

// Initialize LED handler
void led_handler_init(void);

// Deinitialize LED handler
void led_handler_deinit(void);

// prototype for update task function
 void led_update_task(void *param);


// MAIN FUNCTION: Set pattern for specific edge
// edge_id: 0-3 (for 4 edges)
// pattern: LED_PATTERN_* enum value
// r, g, b: RGB color values (0-255)
// intensity: brightness (0-255)
// speed_ms: animation speed in milliseconds (minimum 1000ms)
void led_set_edge_pattern(uint8_t edge_id, uint8_t pattern, 
                         uint8_t r, uint8_t g, uint8_t b, 
                         uint8_t intensity, uint32_t speed_ms);

// Turn off specific edge
void led_turn_off_edge(uint8_t edge_id);

// Turn off all edges
void led_turn_off_all(void);

// Show all available patterns in log
void led_show_all_patterns(void);

// Demo function - cycles through all patterns on specific edge
void led_demo_edge_patterns(uint8_t edge_id);

// Test function - shows different patterns on all edges
void led_test_all_edges(void);

// Get status of specific edge
void led_get_edge_status(uint8_t edge_id);

// Get status of all edges
void led_get_all_status(void);

// Clear all patterns and turn off LEDs
void led_clear_all(void);

#endif // LED_HANDLER_H