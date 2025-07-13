#ifndef VISUAL_LED_H
#define VISUAL_LED_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "framebuffer.h"

#ifdef __cplusplus
extern "C" {
#endif

// Constants
#define MAX_EDGES 8
#define MAX_LEDS_PER_EDGE 256
#define MAX_PATTERNS 16
#define MAX_PALETTE_COLORS 32
#define M_PI 3.14159265358979323846

// Forward declarations
typedef struct LEDController LEDController;
typedef struct Pattern Pattern;
typedef struct ColorPalette ColorPalette;

// Enumerations
typedef enum {
    PATTERN_STATIC,
    PATTERN_BLINK,
    PATTERN_FADE,
    PATTERN_PULSE,
    PATTERN_SHIFT,
    PATTERN_GRADIENT,
    PATTERN_TWINKLE,
    PATTERN_PALETTE_CYCLE
} PatternType;

typedef enum {
    BLEND_ADD,
    BLEND_MAX,
    BLEND_AVERAGE,
    BLEND_MULTIPLY
} BlendMode;

// Structure definitions


struct ColorPalette {
    LedState_t colors[MAX_PALETTE_COLORS];
    int count;
};

// Pattern parameter structures
typedef struct {
    LedState_t color;
} StaticParams;

typedef struct {
    LedState_t on_color;
    uint32_t on_time;
    uint32_t off_time;
    int repeat_count;
} BlinkParams;

typedef struct {
    LedState_t start_color;
    LedState_t end_color;
} FadeParams;

typedef struct {
    LedState_t base_color;
    uint8_t peak_intensity;
    uint32_t period;
} PulseParams;

typedef struct {
    LedState_t pattern[MAX_LEDS_PER_EDGE];
    int pattern_length;
    int offset;
    uint32_t period;
} ShiftParams;

typedef struct {
    LedState_t start_color;
    LedState_t end_color;
} GradientParams;

typedef struct {
    LedState_t color;
    float probability;
} TwinkleParams;

typedef struct {
    ColorPalette palette;
    uint32_t cycle_period;
    int offset;
} PaletteCycleParams;

struct Pattern {
    PatternType type;
    int edge;
    int start_index;
    int end_index;
    uint32_t start_time;
    uint32_t duration;
    bool active;
    void* params;
};

struct LEDController {
    Pattern patterns[MAX_PATTERNS];
    int pattern_count;
    uint32_t current_time;
};

// Core controller functions
LEDController* led_controller_create(int num_edges, int* leds_per_edge);
void led_controller_destroy(LEDController* controller);
void led_controller_update(LEDController* controller, uint32_t time);
void led_controller_clear(LEDController* controller);

// Matrix operations
void led_matrix_init(LedEdgeConfigState_t* matrix, int num_edges, int* leds_per_edge);
void led_matrix_clear(LedEdgeConfigState_t* matrix);
void led_matrix_set_led(LedEdgeConfigState_t* matrix, int edge, int index, LedState_t color);
LedState_t led_matrix_get_led(LedEdgeConfigState_t* matrix, int edge, int index);
void led_matrix_blend(LedEdgeConfigState_t* dest, LedEdgeConfigState_t* src, BlendMode mode);

// Color utilities
LedState_t led_color_create(uint8_t r, uint8_t g, uint8_t b, uint8_t intensity);
LedState_t led_color_interpolate(LedState_t start, LedState_t end, float t);
LedState_t led_color_blend(LedState_t c1, LedState_t c2, BlendMode mode);
LedState_t led_color_scale(LedState_t color, float scale);

// Pattern creation functions
int led_pattern_static(LEDController* controller, int edge, int start_idx, int end_idx, LedState_t color);
int led_pattern_blink(LEDController* controller, int edge, int start_idx, int end_idx, 
                     LedState_t color, uint32_t on_time, uint32_t off_time, int repeats);
int led_pattern_fade(LEDController* controller, int edge, int start_idx, int end_idx,
                    LedState_t start_color, LedState_t end_color, uint32_t duration);
int led_pattern_pulse(LEDController* controller, int edge, int start_idx, int end_idx,
                     LedState_t base_color, uint8_t peak_intensity, uint32_t period);
int led_pattern_shift(LEDController* controller, int edge, int start_idx, int end_idx,
                     LedState_t* pattern_colors, int pattern_length, uint32_t period, int offset);
int led_pattern_gradient(LEDController* controller, int edge, int start_idx, int end_idx,
                        LedState_t start_color, LedState_t end_color);
int led_pattern_twinkle(LEDController* controller, int edge, int start_idx, int end_idx,
                       LedState_t color, float probability);
int led_pattern_palette_cycle(LEDController* controller, int edge, int start_idx, int end_idx,
                             ColorPalette palette, uint32_t cycle_period, int offset);

// Convenience shift pattern functions
int led_pattern_shift_comet(LEDController* controller, int edge, int start_idx, int end_idx,
                           LedState_t color, int comet_length, uint32_t period);
int led_pattern_shift_dot(LEDController* controller, int edge, int start_idx, int end_idx,
                         LedState_t color, int spacing, uint32_t period);

// Utility functions
ColorPalette led_palette_rainbow(int steps);
ColorPalette led_palette_create(LedState_t* colors, int count);
float led_ease_in_out(float t);
uint32_t led_random_range(uint32_t min, uint32_t max);

// Pattern control functions
void led_pattern_remove(LEDController* controller, int pattern_id);
void led_pattern_stop(LEDController* controller, int pattern_id);
void led_pattern_start(LEDController* controller, int pattern_id, uint32_t start_time);

#ifdef __cplusplus
}
#endif

#endif // VISUAL_LED_H