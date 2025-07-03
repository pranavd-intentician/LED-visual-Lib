#include "visual_LED.h"
#include <stdio.h>
#include <time.h>

// Forward declarations for static functions
static void apply_static_pattern(LEDMatrix* matrix, Pattern* pattern, uint32_t time);
static void apply_blink_pattern(LEDMatrix* matrix, Pattern* pattern, uint32_t time);
static void apply_fade_pattern(LEDMatrix* matrix, Pattern* pattern, uint32_t time);
static void apply_pulse_pattern(LEDMatrix* matrix, Pattern* pattern, uint32_t time);
static void apply_shift_pattern(LEDMatrix* matrix, Pattern* pattern, uint32_t time);
static void apply_gradient_pattern(LEDMatrix* matrix, Pattern* pattern, uint32_t time);
static void apply_twinkle_pattern(LEDMatrix* matrix, Pattern* pattern, uint32_t time);
static void apply_palette_cycle_pattern(LEDMatrix* matrix, Pattern* pattern, uint32_t time);

// Global random seed
static bool random_seeded = false;

// Initialize random seed if not done
static void ensure_random_seed() {
    if (!random_seeded) {
        srand((unsigned int)time(NULL));
        random_seeded = true;
    }
}

// Core controller functions
LEDController* led_controller_create(int num_edges, int* leds_per_edge) {
    LEDController* controller = malloc(sizeof(LEDController));
    if (!controller) return NULL;
    
    led_matrix_init(&controller->matrix, num_edges, leds_per_edge);
    controller->pattern_count = 0;
    controller->current_time = 0;
    
    // Initialize patterns array
    memset(controller->patterns, 0, sizeof(controller->patterns));
    
    ensure_random_seed();
    return controller;
}

void led_controller_destroy(LEDController* controller) {
    if (!controller) return;
    
    // Free pattern parameters
    for (int i = 0; i < controller->pattern_count; i++) {
        if (controller->patterns[i].params) {
            free(controller->patterns[i].params);
        }
    }
    
    free(controller);
}

void led_controller_update(LEDController* controller, uint32_t time) {
    if (!controller) return;
    
    controller->current_time = time;
    led_matrix_clear(&controller->matrix);
    
    LEDMatrix temp_matrix;
    led_matrix_init(&temp_matrix, controller->matrix.num_edges, controller->matrix.leds_per_edge);
    
    // Process each active pattern
    for (int i = 0; i < controller->pattern_count; i++) {
        Pattern* pattern = &controller->patterns[i];
        if (!pattern->active) continue;
        
        uint32_t pattern_time = time - pattern->start_time;
        if (pattern->duration > 0 && pattern_time > pattern->duration) {
            pattern->active = false;
            continue;
        }
        
        led_matrix_clear(&temp_matrix);
        
        // Apply pattern based on type
        switch (pattern->type) {
            case PATTERN_STATIC:
                apply_static_pattern(&temp_matrix, pattern, pattern_time);
                break;
            case PATTERN_BLINK:
                apply_blink_pattern(&temp_matrix, pattern, pattern_time);
                break;
            case PATTERN_FADE:
                apply_fade_pattern(&temp_matrix, pattern, pattern_time);
                break;
            case PATTERN_PULSE:
                apply_pulse_pattern(&temp_matrix, pattern, pattern_time);
                break;
            case PATTERN_SHIFT:
                apply_shift_pattern(&temp_matrix, pattern, pattern_time);
                break;
            case PATTERN_GRADIENT:
                apply_gradient_pattern(&temp_matrix, pattern, pattern_time);
                break;
            case PATTERN_TWINKLE:
                apply_twinkle_pattern(&temp_matrix, pattern, pattern_time);
                break;
            case PATTERN_PALETTE_CYCLE:
                apply_palette_cycle_pattern(&temp_matrix, pattern, pattern_time);
                break;
        }
        
        // Blend with main matrix
        led_matrix_blend(&controller->matrix, &temp_matrix, BLEND_ADD);
    }
}

void led_controller_clear(LEDController* controller) {
    if (!controller) return;
    led_matrix_clear(&controller->matrix);
}

// Matrix operations
void led_matrix_init(LEDMatrix* matrix, int num_edges, int* leds_per_edge) {
    if (!matrix) return;
    
    matrix->num_edges = num_edges;
    for (int i = 0; i < num_edges && i < MAX_EDGES; i++) {
        matrix->leds_per_edge[i] = leds_per_edge[i];
    }
    led_matrix_clear(matrix);
}

void led_matrix_clear(LEDMatrix* matrix) {
    if (!matrix) return;
    memset(matrix->leds, 0, sizeof(matrix->leds));
}

void led_matrix_set_led(LEDMatrix* matrix, int edge, int index, LEDColor color) {
    if (!matrix || edge >= matrix->num_edges || index >= matrix->leds_per_edge[edge]) return;
    matrix->leds[edge][index] = color;
}

LEDColor led_matrix_get_led(LEDMatrix* matrix, int edge, int index) {
    LEDColor black = {0, 0, 0, 0};
    if (!matrix || edge >= matrix->num_edges || index >= matrix->leds_per_edge[edge]) return black;
    return matrix->leds[edge][index];
}

void led_matrix_blend(LEDMatrix* dest, LEDMatrix* src, BlendMode mode) {
    if (!dest || !src) return;
    
    for (int e = 0; e < dest->num_edges; e++) {
        for (int i = 0; i < dest->leds_per_edge[e]; i++) {
            LEDColor current = dest->leds[e][i];
            LEDColor new_color = src->leds[e][i];
            dest->leds[e][i] = led_color_blend(current, new_color, mode);
        }
    }
}

// Color utilities
LEDColor led_color_create(uint8_t r, uint8_t g, uint8_t b, uint8_t intensity) {
    LEDColor color = {r, g, b, intensity};
    return color;
}

LEDColor led_color_interpolate(LEDColor start, LEDColor end, float t) {
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    
    LEDColor result;
    result.r = (uint8_t)(start.r + t * (end.r - start.r));
    result.g = (uint8_t)(start.g + t * (end.g - start.g));
    result.b = (uint8_t)(start.b + t * (end.b - start.b));
    result.intensity = (uint8_t)(start.intensity + t * (end.intensity - start.intensity));
    return result;
}

LEDColor led_color_blend(LEDColor c1, LEDColor c2, BlendMode mode) {
    LEDColor result;
    
    switch (mode) {
        case BLEND_ADD:
            result.r = (c1.r + c2.r > 255) ? 255 : c1.r + c2.r;
            result.g = (c1.g + c2.g > 255) ? 255 : c1.g + c2.g;
            result.b = (c1.b + c2.b > 255) ? 255 : c1.b + c2.b;
            result.intensity = (c1.intensity + c2.intensity > 255) ? 255 : c1.intensity + c2.intensity;
            break;
            
        case BLEND_MAX:
            result.r = (c1.r > c2.r) ? c1.r : c2.r;
            result.g = (c1.g > c2.g) ? c1.g : c2.g;
            result.b = (c1.b > c2.b) ? c1.b : c2.b;
            result.intensity = (c1.intensity > c2.intensity) ? c1.intensity : c2.intensity;
            break;
            
        case BLEND_AVERAGE:
            result.r = (c1.r + c2.r) / 2;
            result.g = (c1.g + c2.g) / 2;
            result.b = (c1.b + c2.b) / 2;
            result.intensity = (c1.intensity + c2.intensity) / 2;
            break;
            
        case BLEND_MULTIPLY:
            result.r = (c1.r * c2.r) / 255;
            result.g = (c1.g * c2.g) / 255;
            result.b = (c1.b * c2.b) / 255;
            result.intensity = (c1.intensity * c2.intensity) / 255;
            break;
            
        default:
            result = c1;
    }
    
    return result;
}

LEDColor led_color_scale(LEDColor color, float scale) {
    if (scale < 0.0f) scale = 0.0f;
    if (scale > 1.0f) scale = 1.0f;
    
    LEDColor result;
    result.r = (uint8_t)(color.r * scale);
    result.g = (uint8_t)(color.g * scale);
    result.b = (uint8_t)(color.b * scale);
    result.intensity = (uint8_t)(color.intensity * scale);
    return result;
}

// Pattern application functions (internal)
static void apply_static_pattern(LEDMatrix* matrix, Pattern* pattern, uint32_t time) {
    StaticParams* params = (StaticParams*)pattern->params;
    
    for (int i = pattern->start_index; i <= pattern->end_index; i++) {
        led_matrix_set_led(matrix, pattern->edge, i, params->color);
    }
}

static void apply_blink_pattern(LEDMatrix* matrix, Pattern* pattern, uint32_t time) {
    BlinkParams* params = (BlinkParams*)pattern->params;
    
    uint32_t cycle_time = params->on_time + params->off_time;
    uint32_t phase = time % cycle_time;
    
    if (phase < params->on_time) {
        for (int i = pattern->start_index; i <= pattern->end_index; i++) {
            led_matrix_set_led(matrix, pattern->edge, i, params->on_color);
        }
    }
}

static void apply_fade_pattern(LEDMatrix* matrix, Pattern* pattern, uint32_t time) {
    FadeParams* params = (FadeParams*)pattern->params;
    
    float t = (float)time / (float)pattern->duration;
    if (t > 1.0f) t = 1.0f;
    
    LEDColor current = led_color_interpolate(params->start_color, params->end_color, t);
    
    for (int i = pattern->start_index; i <= pattern->end_index; i++) {
        led_matrix_set_led(matrix, pattern->edge, i, current);
    }
}

static void apply_pulse_pattern(LEDMatrix* matrix, Pattern* pattern, uint32_t time) {
    PulseParams* params = (PulseParams*)pattern->params;
    
    float phase = (float)(time % params->period) / (float)params->period;
    float intensity_factor = (sinf(2.0f * M_PI * phase) + 1.0f) / 2.0f;
    
    LEDColor pulsed = params->base_color;
    pulsed.intensity = (uint8_t)(params->peak_intensity * intensity_factor);
    
    for (int i = pattern->start_index; i <= pattern->end_index; i++) {
        led_matrix_set_led(matrix, pattern->edge, i, pulsed);
    }
}

static void apply_shift_pattern(LEDMatrix* matrix, Pattern* pattern, uint32_t time) {
    ShiftParams* params = (ShiftParams*)pattern->params;
    
    int current_offset = (params->offset + (time / params->period)) % params->pattern_length;
    
    for (int i = pattern->start_index; i <= pattern->end_index; i++) {
        int pattern_idx = (i + current_offset) % params->pattern_length;
        led_matrix_set_led(matrix, pattern->edge, i, params->pattern[pattern_idx]);
    }
}

static void apply_gradient_pattern(LEDMatrix* matrix, Pattern* pattern, uint32_t time) {
    GradientParams* params = (GradientParams*)pattern->params;
    
    int led_count = pattern->end_index - pattern->start_index + 1;
    
    for (int i = pattern->start_index; i <= pattern->end_index; i++) {
        float t = (float)(i - pattern->start_index) / (float)(led_count - 1);
        LEDColor gradient_color = led_color_interpolate(params->start_color, params->end_color, t);
        led_matrix_set_led(matrix, pattern->edge, i, gradient_color);
    }
}

static void apply_twinkle_pattern(LEDMatrix* matrix, Pattern* pattern, uint32_t time) {
    TwinkleParams* params = (TwinkleParams*)pattern->params;
    
    for (int i = pattern->start_index; i <= pattern->end_index; i++) {
        float random_val = (float)rand() / RAND_MAX;
        if (random_val < params->probability) {
            led_matrix_set_led(matrix, pattern->edge, i, params->color);
        }
    }
}

static void apply_palette_cycle_pattern(LEDMatrix* matrix, Pattern* pattern, uint32_t time) {
    PaletteCycleParams* params = (PaletteCycleParams*)pattern->params;
    
    float cycle_position = (float)(time % params->cycle_period) / (float)params->cycle_period;
    
    for (int i = pattern->start_index; i <= pattern->end_index; i++) {
        float led_position = cycle_position + (float)(i + params->offset) / 10.0f;
        led_position = fmodf(led_position, 1.0f);
        
        int color_idx = (int)(led_position * params->palette.count);
        LEDColor color = params->palette.colors[color_idx % params->palette.count];
        
        led_matrix_set_led(matrix, pattern->edge, i, color);
    }
}

// Pattern creation functions
int led_pattern_static(LEDController* controller, int edge, int start_idx, int end_idx, LEDColor color) {
    if (!controller || controller->pattern_count >= MAX_PATTERNS) return -1;
    
    int pattern_id = controller->pattern_count++;
    Pattern* pattern = &controller->patterns[pattern_id];
    
    pattern->type = PATTERN_STATIC;
    pattern->edge = edge;
    pattern->start_index = start_idx;
    pattern->end_index = end_idx;
    pattern->start_time = controller->current_time;
    pattern->duration = 0; // Static patterns run indefinitely
    pattern->active = true;
    
    StaticParams* params = malloc(sizeof(StaticParams));
    params->color = color;
    pattern->params = params;
    
    return pattern_id;
}

int led_pattern_blink(LEDController* controller, int edge, int start_idx, int end_idx, 
                     LEDColor color, uint32_t on_time, uint32_t off_time, int repeats) {
    if (!controller || controller->pattern_count >= MAX_PATTERNS) return -1;
    
    int pattern_id = controller->pattern_count++;
    Pattern* pattern = &controller->patterns[pattern_id];
    
    pattern->type = PATTERN_BLINK;
    pattern->edge = edge;
    pattern->start_index = start_idx;
    pattern->end_index = end_idx;
    pattern->start_time = controller->current_time;
    pattern->duration = repeats > 0 ? (on_time + off_time) * repeats : 0;
    pattern->active = true;
    
    BlinkParams* params = malloc(sizeof(BlinkParams));
    params->on_color = color;
    params->on_time = on_time;
    params->off_time = off_time;
    params->repeat_count = repeats;
    pattern->params = params;
    
    return pattern_id;
}

int led_pattern_fade(LEDController* controller, int edge, int start_idx, int end_idx,
                    LEDColor start_color, LEDColor end_color, uint32_t duration) {
    if (!controller || controller->pattern_count >= MAX_PATTERNS) return -1;
    
    int pattern_id = controller->pattern_count++;
    Pattern* pattern = &controller->patterns[pattern_id];
    
    pattern->type = PATTERN_FADE;
    pattern->edge = edge;
    pattern->start_index = start_idx;
    pattern->end_index = end_idx;
    pattern->start_time = controller->current_time;
    pattern->duration = duration;
    pattern->active = true;
    
    FadeParams* params = malloc(sizeof(FadeParams));
    params->start_color = start_color;
    params->end_color = end_color;
    pattern->params = params;
    
    return pattern_id;
}

int led_pattern_pulse(LEDController* controller, int edge, int start_idx, int end_idx,
                     LEDColor base_color, uint8_t peak_intensity, uint32_t period) {
    if (!controller || controller->pattern_count >= MAX_PATTERNS) return -1;
    
    int pattern_id = controller->pattern_count++;
    Pattern* pattern = &controller->patterns[pattern_id];
    
    pattern->type = PATTERN_PULSE;
    pattern->edge = edge;
    pattern->start_index = start_idx;
    pattern->end_index = end_idx;
    pattern->start_time = controller->current_time;
    pattern->duration = 0; // Continuous
    pattern->active = true;
    
    PulseParams* params = malloc(sizeof(PulseParams));
    params->base_color = base_color;
    params->peak_intensity = peak_intensity;
    params->period = period;
    pattern->params = params;
    
    return pattern_id;
}

// Add these functions to visual_LED.c after the existing pattern creation functions

int led_pattern_gradient(LEDController* controller, int edge, int start_idx, int end_idx,
                        LEDColor start_color, LEDColor end_color) {
    if (!controller || controller->pattern_count >= MAX_PATTERNS) return -1;
    
    int pattern_id = controller->pattern_count++;
    Pattern* pattern = &controller->patterns[pattern_id];
    
    pattern->type = PATTERN_GRADIENT;
    pattern->edge = edge;
    pattern->start_index = start_idx;
    pattern->end_index = end_idx;
    pattern->start_time = controller->current_time;
    pattern->duration = 0; // Static gradient pattern runs indefinitely
    pattern->active = true;
    
    GradientParams* params = malloc(sizeof(GradientParams));
    params->start_color = start_color;
    params->end_color = end_color;
    pattern->params = params;
    
    return pattern_id;
}

int led_pattern_twinkle(LEDController* controller, int edge, int start_idx, int end_idx,
                       LEDColor color, float probability) {
    if (!controller || controller->pattern_count >= MAX_PATTERNS) return -1;
    
    int pattern_id = controller->pattern_count++;
    Pattern* pattern = &controller->patterns[pattern_id];
    
    pattern->type = PATTERN_TWINKLE;
    pattern->edge = edge;
    pattern->start_index = start_idx;
    pattern->end_index = end_idx;
    pattern->start_time = controller->current_time;
    pattern->duration = 0; // Continuous twinkle pattern
    pattern->active = true;
    
    TwinkleParams* params = malloc(sizeof(TwinkleParams));
    params->color = color;
    params->probability = probability;
    pattern->params = params;
    
    return pattern_id;
}

int led_pattern_palette_cycle(LEDController* controller, int edge, int start_idx, int end_idx,
                             ColorPalette palette, uint32_t cycle_period, int offset) {
    if (!controller || controller->pattern_count >= MAX_PATTERNS) return -1;
    
    int pattern_id = controller->pattern_count++;
    Pattern* pattern = &controller->patterns[pattern_id];
    
    pattern->type = PATTERN_PALETTE_CYCLE;
    pattern->edge = edge;
    pattern->start_index = start_idx;
    pattern->end_index = end_idx;
    pattern->start_time = controller->current_time;
    pattern->duration = 0; // Continuous palette cycle
    pattern->active = true;
    
    PaletteCycleParams* params = malloc(sizeof(PaletteCycleParams));
    params->palette = palette;
    params->cycle_period = cycle_period;
    params->offset = offset;
    pattern->params = params;
    
    return pattern_id;
}
// Utility functions
ColorPalette led_palette_rainbow(int steps) {
    ColorPalette palette;
    palette.count = steps > MAX_PALETTE_COLORS ? MAX_PALETTE_COLORS : steps;
    
    for (int i = 0; i < palette.count; i++) {
        float hue = (float)i / (float)palette.count * 360.0f;
        
        // Convert HSV to RGB (simplified)
        float c = 1.0f; // Full saturation
        float x = c * (1.0f - fabsf(fmodf(hue / 60.0f, 2.0f) - 1.0f));
        float m = 0.0f; // Full value
        
        float r, g, b;
        if (hue < 60) { r = c; g = x; b = 0; }
        else if (hue < 120) { r = x; g = c; b = 0; }
        else if (hue < 180) { r = 0; g = c; b = x; }
        else if (hue < 240) { r = 0; g = x; b = c; }
        else if (hue < 300) { r = x; g = 0; b = c; }
        else { r = c; g = 0; b = x; }
        
        palette.colors[i] = led_color_create(
            (uint8_t)((r + m) * 255),
            (uint8_t)((g + m) * 255),
            (uint8_t)((b + m) * 255),
            255
        );
    }
    
    return palette;
}

ColorPalette led_palette_create(LEDColor* colors, int count) {
    ColorPalette palette;
    palette.count = count > MAX_PALETTE_COLORS ? MAX_PALETTE_COLORS : count;
    
    for (int i = 0; i < palette.count; i++) {
        palette.colors[i] = colors[i];
    }
    
    return palette;
}

float led_ease_in_out(float t) {
    return t * t * (3.0f - 2.0f * t);
}

uint32_t led_random_range(uint32_t min, uint32_t max) {
    ensure_random_seed();
    return min + (rand() % (max - min + 1));
}

// Pattern control functions
void led_pattern_remove(LEDController* controller, int pattern_id) {
    if (!controller || pattern_id >= controller->pattern_count) return;
    
    Pattern* pattern = &controller->patterns[pattern_id];
    if (pattern->params) {
        free(pattern->params);
        pattern->params = NULL;
    }
    pattern->active = false;
}

void led_pattern_stop(LEDController* controller, int pattern_id) {
    if (!controller || pattern_id >= controller->pattern_count) return;
    controller->patterns[pattern_id].active = false;
}

void led_pattern_start(LEDController* controller, int pattern_id, uint32_t start_time) {
    if (!controller || pattern_id >= controller->pattern_count) return;
    
    Pattern* pattern = &controller->patterns[pattern_id];
    pattern->start_time = start_time;
    pattern->active = true;
}