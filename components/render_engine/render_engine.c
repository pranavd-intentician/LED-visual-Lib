#include "render_engine.h"
#include <stdio.h>
#include <time.h>
#include  <math.h>
#include "main.h"
// Forward declarations for static functions
static void apply_static_pattern(LedEdgeConfigState_t* configState, Pattern* pattern, uint32_t time);
static void apply_blink_pattern(LedEdgeConfigState_t* configState, Pattern* pattern, uint32_t time);
static void apply_fade_pattern(LedEdgeConfigState_t* configState, Pattern* pattern, uint32_t time);
static void apply_pulse_pattern(LedEdgeConfigState_t* configState, Pattern* pattern, uint32_t time);
static void apply_shift_pattern(LedEdgeConfigState_t* configState, Pattern* pattern, uint32_t time);
static void apply_gradient_pattern(LedEdgeConfigState_t* configState, Pattern* pattern, uint32_t time);
static void apply_twinkle_pattern(LedEdgeConfigState_t* configState, Pattern* pattern, uint32_t time);
static void apply_palette_cycle_pattern(LedEdgeConfigState_t* configState, Pattern* pattern, uint32_t time);

// Global random seed
static bool random_seeded = false;

// Initialize random seed if not done
static void ensure_random_seed() {
    if (!random_seeded) {
        srand((unsigned int)time(NULL));
        random_seeded = true;
    }
}

//--------------------------------------Controller operations------------------------------------//

LEDController* led_controller_create(int num_edges, int* leds_per_edge) {
    LEDController* controller = malloc(sizeof(LEDController));
    if (!controller) return NULL;
    controller->pattern_count = 0;
    controller->current_time = 0;
    
    // Initialize patterns array
    memset(controller->patterns, 0, sizeof(controller->patterns));
    if(framebuffer_init(num_edges, (uint32_t*)leds_per_edge) != pdPASS) {
        printf("Error: Failed to initialize frame buffer\n");
        free(controller);
        return pdFAIL;
    }
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
    framebuffer_cleanup();
    free(controller);
}

void led_controller_update(LEDController* controller, uint32_t time) {
    if (!controller || !nextLedConfigState->data) return;

    controller->current_time = time;

    // Process each active pattern
    for (int i = 0; i < controller->pattern_count; i++) {
        Pattern* pattern = &controller->patterns[i];
        if (!pattern->active) continue;
        
        uint32_t pattern_time = time - pattern->start_time;
        if (pattern->duration > 0 && pattern_time > pattern->duration) {
            pattern->active = false;
            continue;
        }
        
        led_matrix_clear(nextLedConfigState);
        
        // Apply pattern based on type
        switch (pattern->type) {
            case PATTERN_STATIC:
                apply_static_pattern(nextLedConfigState, pattern, pattern_time);
                break;
            case PATTERN_BLINK:
                apply_blink_pattern(nextLedConfigState, pattern, pattern_time);
                break;
            case PATTERN_FADE:
                apply_fade_pattern(nextLedConfigState, pattern, pattern_time);
                break;
            case PATTERN_PULSE:
                apply_pulse_pattern(nextLedConfigState, pattern, pattern_time);
                break;
            case PATTERN_SHIFT:
                apply_shift_pattern(nextLedConfigState, pattern, pattern_time);
                break;
            case PATTERN_GRADIENT:
                apply_gradient_pattern(nextLedConfigState, pattern, pattern_time);
                break;
            case PATTERN_TWINKLE:
                apply_twinkle_pattern(nextLedConfigState, pattern, pattern_time);
                break;
            case PATTERN_PALETTE_CYCLE:
                apply_palette_cycle_pattern(nextLedConfigState, pattern, pattern_time);
                break;
        }
        
        //swap frames in framebuffer
        framebuffer_swap();
    }
}

//render engine task fucntion
void led_controller_task(void* params){
    TickType_t last_wake_time = xTaskGetTickCount();
    const TickType_t render_period = pdMS_TO_TICKS(50); // 20 FPS
    
    while (1) {
        uint32_t current_time = get_current_time_ms();
        
        // Update visual LED controller
        if (led_controller) {
            led_controller_update(led_controller, current_time);
            
            // Notify display task that new frame is ready
            if (physical_led_task_handle) {
                xTaskNotify(physical_led_task_handle, 1, eSetValueWithOverwrite);
            }
        }
        
        vTaskDelayUntil(&last_wake_time, render_period);
    }
    
    vTaskDelete(NULL);
}

void led_controller_clear(LEDController* controller) {
    if (!controller) return;
    // led_matrix_clear(&controller->matrix);
}
//--------------------------------------Controller operations------------------------------------//

//-----------------------------------------Matrix operations--------------------------------------//

void led_matrix_clear(LedEdgeConfigState_t* configState) {
    if (!configState) return;
    for(int i=0; i< configState->num_edges; i++) {
        memset(configState->data[i], 0, configState->num_led_per_edge[i] * sizeof(LedState_t));
    }
}

void led_matrix_set_led(LedEdgeConfigState_t* configState, int edge, int index, LedState_t color) {
    if (!configState->data || edge >= configState->num_edges || index >= configState->num_led_per_edge[edge])return;
    configState->data[edge][index] = color;
}

LedState_t led_matrix_get_led(LedEdgeConfigState_t* configState, int edge, int index) {
    LedState_t black = {0, 0, 0, 0};
    if (!configState->data || edge >= configState->num_edges || index >= configState->num_led_per_edge[edge]) return black;
    return configState->data[edge][index];
}

void led_matrix_blend(LedEdgeConfigState_t* dest,LedEdgeConfigState_t* src, BlendMode mode) {
    if (!dest || !src) return;
    
    for (int e = 0; e < dest->num_edges; e++) {
        for (int i = 0; i < dest->num_led_per_edge[e]; i++) {
            LedState_t current = dest->data[e][i];
            LedState_t new_color = src->data[e][i];
            dest->data[e][i] = led_color_blend(current, new_color, mode);
        }
    }
}
//-----------------------------------------Matrix operations--------------------------------------//

//-----------------------------------------Color operations---------------------------------------//
LedState_t led_color_create(uint8_t r, uint8_t g, uint8_t b, uint8_t intensity) {
    LedState_t color = {r, g, b, intensity};
    return color;
}

LedState_t led_color_interpolate(LedState_t start, LedState_t end, float t) {
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    
    LedState_t result;
    result.r = (uint8_t)(start.r + t * (end.r - start.r));
    result.g = (uint8_t)(start.g + t * (end.g - start.g));
    result.b = (uint8_t)(start.b + t * (end.b - start.b));
    result.intensity = (uint8_t)(start.intensity + t * (end.intensity - start.intensity));
    return result;
}

LedState_t led_color_blend(LedState_t c1, LedState_t c2, BlendMode mode) {
    LedState_t result;
    
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

LedState_t led_color_scale(LedState_t color, float scale) {
    if (scale < 0.0f) scale = 0.0f;
    if (scale > 1.0f) scale = 1.0f;
    
    LedState_t result;
    result.r = (uint8_t)(color.r * scale);
    result.g = (uint8_t)(color.g * scale);
    result.b = (uint8_t)(color.b * scale);
    result.intensity = (uint8_t)(color.intensity * scale);
    return result;
}
//-----------------------------------------Color operations---------------------------------------//

//-------------------------- Pattern application functions (internal)-----------------------------//
static void apply_static_pattern(LedEdgeConfigState_t* configState, Pattern* pattern, uint32_t time) {
    StaticParams* params = (StaticParams*)pattern->params;
    
    for (int i = pattern->start_index; i <= pattern->end_index; i++) {
        led_matrix_set_led(configState, pattern->edge, i, params->color);
    }
}

static void apply_blink_pattern(LedEdgeConfigState_t* configState, Pattern* pattern, uint32_t time) {
    BlinkParams* params = (BlinkParams*)pattern->params;
    
    uint32_t cycle_time = params->on_time + params->off_time;
    uint32_t phase = time % cycle_time;
    
    if (phase < params->on_time) {
        for (int i = pattern->start_index; i <= pattern->end_index; i++) {
            led_matrix_set_led(configState, pattern->edge, i, params->on_color);
        }
    }
}

static void apply_fade_pattern(LedEdgeConfigState_t* configState, Pattern* pattern, uint32_t time) {
    FadeParams* params = (FadeParams*)pattern->params;
    
    float t = (float)time / (float)pattern->duration;
    if (t > 1.0f) t = 1.0f;
    
    LedState_t current = led_color_interpolate(params->start_color, params->end_color, t);
    
    for (int i = pattern->start_index; i <= pattern->end_index; i++) {
        led_matrix_set_led(configState, pattern->edge, i, current);
    }
}

static void apply_pulse_pattern(LedEdgeConfigState_t* configState, Pattern* pattern, uint32_t time) {
    PulseParams* params = (PulseParams*)pattern->params;
    
    float phase = (float)(time % params->period) / (float)params->period;
    float intensity_factor = (sinf(2.0f * M_PI * phase) + 1.0f) / 2.0f;
    
    LedState_t pulsed = params->base_color;
    pulsed.intensity = (uint8_t)(params->peak_intensity * intensity_factor);
    
    for (int i = pattern->start_index; i <= pattern->end_index; i++) {
        led_matrix_set_led(configState, pattern->edge, i, pulsed);
    }
}

static void apply_shift_pattern(LedEdgeConfigState_t* configState, Pattern* pattern, uint32_t time) {
    ShiftParams* params = (ShiftParams*)pattern->params;
    
    // Calculate the current shift offset based on time
    int shift_amount = (time / params->period) % params->pattern_length;
    
    for (int i = pattern->start_index; i <= pattern->end_index; i++) {
        int led_position = i - pattern->start_index;
        int total_leds = pattern->end_index - pattern->start_index + 1;
        
        // Calculate which pattern element to use for this LED
        int pattern_idx = (led_position + shift_amount + params->offset) % params->pattern_length;
        
        // If we have fewer LEDs than pattern length, wrap around
        if (pattern_idx >= total_leds) {
            pattern_idx = pattern_idx % total_leds;
        }
        
        led_matrix_set_led(configState, pattern->edge, i, params->pattern[pattern_idx]);
    }
}

static void apply_gradient_pattern(LedEdgeConfigState_t* configState, Pattern* pattern, uint32_t time) {
    GradientParams* params = (GradientParams*)pattern->params;
    
    int led_count = pattern->end_index - pattern->start_index + 1;
    
    for (int i = pattern->start_index; i <= pattern->end_index; i++) {
        float t = (float)(i - pattern->start_index) / (float)(led_count - 1);
        LedState_t gradient_color = led_color_interpolate(params->start_color, params->end_color, t);
        led_matrix_set_led(configState, pattern->edge, i, gradient_color);
    }
}

static void apply_twinkle_pattern(LedEdgeConfigState_t* configState, Pattern* pattern, uint32_t time) {
    TwinkleParams* params = (TwinkleParams*)pattern->params;
    
    // Use time-based seeding for more predictable but still random behavior
    srand(time / 100);  // Change seed every 100ms
    
    for (int i = pattern->start_index; i <= pattern->end_index; i++) {
        float random_val = (float)rand() / RAND_MAX;
        if (random_val < params->probability) {
            // Add some intensity variation for more natural twinkling
            float intensity_variation = 0.7f + (random_val * 0.3f);
            LedState_t twinkle_color = led_color_scale(params->color, intensity_variation);
            led_matrix_set_led(configState, pattern->edge, i, twinkle_color);
        }
    }
}

static void apply_palette_cycle_pattern(LedEdgeConfigState_t* configState, Pattern* pattern, uint32_t time) {
    PaletteCycleParams* params = (PaletteCycleParams*)pattern->params;
    
    float cycle_position = (float)(time % params->cycle_period) / (float)params->cycle_period;
    
    for (int i = pattern->start_index; i <= pattern->end_index; i++) {
        float led_position = cycle_position + (float)(i + params->offset) / 10.0f;
        led_position = fmodf(led_position, 1.0f);
        
        // Interpolate between colors for smoother transitions
        float color_pos = led_position * (params->palette.count - 1);
        int color_idx = (int)color_pos;
        float t = color_pos - color_idx;
        
        LedState_t color1 = params->palette.colors[color_idx % params->palette.count];
        LedState_t color2 = params->palette.colors[(color_idx + 1) % params->palette.count];
        
        LedState_t final_color = led_color_interpolate(color1, color2, t);
        led_matrix_set_led(configState, pattern->edge, i, final_color);
    }
}
//-------------------------- Pattern application functions (internal)-----------------------------//

//-----------------------------------Pattern creation functions-----------------------------------//
int led_pattern_static(LEDController* controller, int edge, int start_idx, int end_idx, LedState_t color) {
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
                     LedState_t color, uint32_t on_time, uint32_t off_time, int repeats) {
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
                    LedState_t start_color, LedState_t end_color, uint32_t duration) {
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
                     LedState_t base_color, uint8_t peak_intensity, uint32_t period) {
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

// Improved shift pattern creation function
int led_pattern_shift(LEDController* controller, int edge, int start_idx, int end_idx,
                     LedState_t* pattern_colors, int pattern_length, uint32_t period, int offset) {
    if (!controller || controller->pattern_count >= MAX_PATTERNS) return -1;
    if (!pattern_colors || pattern_length <= 0 || pattern_length > MAX_LEDS_PER_EDGE) return -1;
    
    int pattern_id = controller->pattern_count++;
    Pattern* pattern = &controller->patterns[pattern_id];
    
    pattern->type = PATTERN_SHIFT;
    pattern->edge = edge;
    pattern->start_index = start_idx;
    pattern->end_index = end_idx;
    pattern->start_time = controller->current_time;
    pattern->duration = 0; // Continuous shift pattern
    pattern->active = true;
    
    ShiftParams* params = malloc(sizeof(ShiftParams));
    params->pattern_length = pattern_length;
    params->period = period;
    params->offset = offset;
    
    // Copy pattern colors
    for (int i = 0; i < pattern_length; i++) {
        params->pattern[i] = pattern_colors[i];
    }
    
    pattern->params = params;
    
    return pattern_id;
}

// Convenience function to create a simple comet-like shift pattern
int led_pattern_shift_comet(LEDController* controller, int edge, int start_idx, int end_idx,
                           LedState_t color, int comet_length, uint32_t period) {
    if (comet_length <= 0 || comet_length > MAX_LEDS_PER_EDGE) return -1;
    
    LedState_t pattern_colors[MAX_LEDS_PER_EDGE];
    LedState_t black = led_color_create(0, 0, 0, 0);
    
    // Create comet pattern: bright head followed by fading tail
    for (int i = 0; i < comet_length; i++) {
        float intensity = (float)(comet_length - i) / (float)comet_length;
        pattern_colors[i] = led_color_scale(color, intensity);
    }
    
    // Fill the rest with black
    for (int i = comet_length; i < MAX_LEDS_PER_EDGE; i++) {
        pattern_colors[i] = black;
    }
    
    int total_leds = end_idx - start_idx + 1;
    int pattern_length = (total_leds > comet_length * 2) ? comet_length * 2 : total_leds;
    
    return led_pattern_shift(controller, edge, start_idx, end_idx, pattern_colors, pattern_length, period, 0);
}

// Convenience function to create a simple moving dot pattern
int led_pattern_shift_dot(LEDController* controller, int edge, int start_idx, int end_idx,
                         LedState_t color, int spacing, uint32_t period) {
    if (spacing <= 0 || spacing > MAX_LEDS_PER_EDGE) return -1;
    
    LedState_t pattern_colors[MAX_LEDS_PER_EDGE];
    LedState_t black = led_color_create(0, 0, 0, 0);
    
    // Create dot pattern: one bright dot followed by spacing black LEDs
    pattern_colors[0] = color;
    for (int i = 1; i < spacing; i++) {
        pattern_colors[i] = black;
    }
    
    return led_pattern_shift(controller, edge, start_idx, end_idx, pattern_colors, spacing, period, 0);
}

int led_pattern_gradient(LEDController* controller, int edge, int start_idx, int end_idx,
                        LedState_t start_color, LedState_t end_color) {
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
                       LedState_t color, float probability) {
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
//-----------------------------------Pattern creation functions-----------------------------------//

//------------------------------------------Utility functions-------------------------------------//
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
//------------------------------------------Utility functions-------------------------------------//

ColorPalette led_palette_create(LedState_t* colors, int count) {
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

//--------------------------------------- Pattern control functions------------------------------//
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
//--------------------------------------- Pattern control functions------------------------------//