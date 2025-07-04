#ifndef VISUAL_LED_H
#define VISUAL_LED_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

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
typedef struct LEDColor LEDColor;
typedef struct LEDMatrix LEDMatrix;
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
struct LEDColor {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t intensity;
};

struct LEDMatrix {
    LEDColor leds[MAX_EDGES][MAX_LEDS_PER_EDGE];
    int num_edges;
    int leds_per_edge[MAX_EDGES];
};

struct ColorPalette {
    LEDColor colors[MAX_PALETTE_COLORS];
    int count;
};

// Pattern parameter structures
typedef struct {
    LEDColor color;
} StaticParams;

typedef struct {
    LEDColor on_color;
    uint32_t on_time;
    uint32_t off_time;
    int repeat_count;
} BlinkParams;

typedef struct {
    LEDColor start_color;
    LEDColor end_color;
} FadeParams;

typedef struct {
    LEDColor base_color;
    uint8_t peak_intensity;
    uint32_t period;
} PulseParams;

typedef struct {
    LEDColor pattern[MAX_LEDS_PER_EDGE];
    int pattern_length;
    int offset;
    uint32_t period;
} ShiftParams;

typedef struct {
    LEDColor start_color;
    LEDColor end_color;
} GradientParams;

typedef struct {
    LEDColor color;
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
    LEDMatrix matrix;
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
void led_matrix_init(LEDMatrix* matrix, int num_edges, int* leds_per_edge);
void led_matrix_clear(LEDMatrix* matrix);
void led_matrix_set_led(LEDMatrix* matrix, int edge, int index, LEDColor color);
LEDColor led_matrix_get_led(LEDMatrix* matrix, int edge, int index);
void led_matrix_blend(LEDMatrix* dest, LEDMatrix* src, BlendMode mode);

// Color utilities
LEDColor led_color_create(uint8_t r, uint8_t g, uint8_t b, uint8_t intensity);
LEDColor led_color_interpolate(LEDColor start, LEDColor end, float t);
LEDColor led_color_blend(LEDColor c1, LEDColor c2, BlendMode mode);
LEDColor led_color_scale(LEDColor color, float scale);

// Pattern creation functions
int led_pattern_static(LEDController* controller, int edge, int start_idx, int end_idx, LEDColor color);
int led_pattern_blink(LEDController* controller, int edge, int start_idx, int end_idx, 
                     LEDColor color, uint32_t on_time, uint32_t off_time, int repeats);
int led_pattern_fade(LEDController* controller, int edge, int start_idx, int end_idx,
                    LEDColor start_color, LEDColor end_color, uint32_t duration);
int led_pattern_pulse(LEDController* controller, int edge, int start_idx, int end_idx,
                     LEDColor base_color, uint8_t peak_intensity, uint32_t period);
int led_pattern_shift(LEDController* controller, int edge, int start_idx, int end_idx,
                     LEDColor* pattern_colors, int pattern_length, uint32_t period, int offset);
int led_pattern_gradient(LEDController* controller, int edge, int start_idx, int end_idx,
                        LEDColor start_color, LEDColor end_color);
int led_pattern_twinkle(LEDController* controller, int edge, int start_idx, int end_idx,
                       LEDColor color, float probability);
int led_pattern_palette_cycle(LEDController* controller, int edge, int start_idx, int end_idx,
                             ColorPalette palette, uint32_t cycle_period, int offset);

// Convenience shift pattern functions
int led_pattern_shift_comet(LEDController* controller, int edge, int start_idx, int end_idx,
                           LEDColor color, int comet_length, uint32_t period);
int led_pattern_shift_dot(LEDController* controller, int edge, int start_idx, int end_idx,
                         LEDColor color, int spacing, uint32_t period);

// Utility functions
ColorPalette led_palette_rainbow(int steps);
ColorPalette led_palette_create(LEDColor* colors, int count);
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