#include "physical_led_updater.h"
#include "render_engine.h"
#include "led_strip.h"
#include "led_strip_types.h"
#include <string.h>
#include <math.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <inttypes.h>
#include "main.h"

#define LED_STRIP_GPIO 17
#define LED_STRIP_LENGTH 60
#define NUM_EDGES 4
#define LEDS_PER_EDGE (LED_STRIP_LENGTH / NUM_EDGES)  // 15 LEDs per edge

static const char *TAG = "LED_HANDLER";

// LED strip handle
static led_strip_handle_t strip = NULL;
static bool led_task_running = false;




// Pattern tracking for each edge
typedef struct {
    uint8_t pattern;
    uint8_t r, g, b;
    uint8_t intensity;
    uint32_t speed_ms;
    bool active;
    int visual_pattern_id;  // ID from visual_LED library
} edge_state_t;

static edge_state_t edge_states[NUM_EDGES] = {0};

// Pattern names for easy reference
static const char* pattern_names[] = {
    "OFF",
    "STATIC", 
    "BLINK",
    "BREATH",
    "RAINBOW",
    "FADE_IN",
    "FADE_OUT",
    "TWINKLE"
};



// Get LED index range for an edge
static void get_edge_range(uint8_t edge_id, int* start_idx, int* end_idx) {
    *start_idx = edge_id * LEDS_PER_EDGE;
    *end_idx = *start_idx + LEDS_PER_EDGE - 1;
    if (*end_idx >= LED_STRIP_LENGTH) {
        *end_idx = LED_STRIP_LENGTH - 1;
    }
}

// Convert visual LED matrix to physical LED strip
static void update_physical_strip(void) {
    if (!led_controller || !strip) return;
    
    for (int edge = 0; edge < NUM_EDGES; edge++) {
        int start_idx, end_idx;
        get_edge_range(edge, &start_idx, &end_idx);
        
        for (int i = start_idx; i <= end_idx; i++) {
            int led_idx = i - start_idx;
            LedState_t color = led_matrix_get_led(currentLedConfigState, edge, led_idx);
            
            // Apply intensity scaling
            uint8_t r = (color.r * color.intensity) / 255;
            uint8_t g = (color.g * color.intensity) / 255;
            uint8_t b = (color.b * color.intensity) / 255;
            
            led_strip_set_pixel(strip, i, r, g, b);
        }
    }
}
// LED update task
 void led_update_task(void *param) {
    uint32_t notification_value;
    
    while (led_task_running) {
        // Wait for notification from render task
        if (xTaskNotifyWait(0, ULONG_MAX, &notification_value, pdMS_TO_TICKS(100)) == pdTRUE) {
            // New frame is ready, update physical strip
            update_physical_strip();
            
            // Refresh strip
            if (strip) {
                led_strip_refresh(strip);
            }
        }
        // If timeout occurs, continue loop
    }
    
    vTaskDelete(NULL);
}
// Initialize LED handler
void led_handler_init(void) {
    ESP_LOGI(TAG, "Initializing LED handler with Visual LED library...");
    
    // Initialize LED strip
    led_strip_config_t strip_config = {
        .strip_gpio_num = LED_STRIP_GPIO,
        .max_leds = LED_STRIP_LENGTH,
        .led_model = LED_MODEL_WS2812,
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB,
        .flags = {
            .invert_out = false
        }
    };

    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10 * 1000 * 1000,
        .mem_block_symbols = 64,
        .flags = {
            .with_dma = false
        }
    };

    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &strip));
    ESP_ERROR_CHECK(led_strip_clear(strip));
    
    // Initialize visual LED controller
    int leds_per_edge[NUM_EDGES];
    for (int i = 0; i < NUM_EDGES; i++) {
        leds_per_edge[i] = LEDS_PER_EDGE;
    }
    
    led_controller = led_controller_create(NUM_EDGES, leds_per_edge);
    if (!led_controller) {
        ESP_LOGE(TAG, "Failed to create LED controller");
        return;
    }
    
    // Initialize edge states
    for (int i = 0; i < NUM_EDGES; i++) {
        edge_states[i].pattern = LED_PATTERN_OFF;
        edge_states[i].active = false;
        edge_states[i].visual_pattern_id = -1;
    }
    
    // Start LED update task
    led_task_running = true;
    
    // ESP_LOGI(TAG, "LED handler initialized with Visual LED library - %" PRIu32 "edges, %"PRIu32" LEDs per edge", 
            //  NUM_EDGES, LEDS_PER_EDGE);
}

// Deinitialize LED handler
void led_tasks_cleanup(void) {
    led_task_running = false;
    
    // Wait for tasks to finish
    if (physical_led_task_handle) {
        vTaskDelete(physical_led_task_handle);
        physical_led_task_handle = NULL;
    }
    
    if (render_engine_task_handle) {
        vTaskDelete(render_engine_task_handle);
        render_engine_task_handle = NULL;
    }
    
}



// Create visual LED pattern based on handler pattern type
static int create_visual_pattern(uint8_t edge_id, uint8_t pattern, 
                                uint8_t r, uint8_t g, uint8_t b, 
                                uint8_t intensity, uint32_t speed_ms) {
    if (!led_controller || edge_id >= NUM_EDGES) return -1;
    
    int start_idx = 0;
    int end_idx = LEDS_PER_EDGE - 1;
    LedState_t color = led_color_create(r, g, b, intensity);
    
    switch (pattern) {
        case LED_PATTERN_OFF:
            return led_pattern_static(led_controller, edge_id, start_idx, end_idx, 
                                    led_color_create(0, 0, 0, 0));
            
        case LED_PATTERN_STATIC:
            return led_pattern_static(led_controller, edge_id, start_idx, end_idx, color);
            
        case LED_PATTERN_BLINK:
            return led_pattern_blink(led_controller, edge_id, start_idx, end_idx, 
                                   color, speed_ms/2, speed_ms/2, 0);
            
        case LED_PATTERN_BREATH:
            return led_pattern_pulse(led_controller, edge_id, start_idx, end_idx, 
                                   color, intensity, speed_ms);
            
        case LED_PATTERN_RAINBOW:
        {
            ColorPalette rainbow = led_palette_rainbow(12);
            return led_pattern_palette_cycle(led_controller, edge_id, start_idx, end_idx, 
                                           rainbow, speed_ms, 0);
        }
        
        case LED_PATTERN_FADE_IN:
        {
            LedState_t black = led_color_create(0, 0, 0, 0);
            return led_pattern_fade(led_controller, edge_id, start_idx, end_idx, 
                                  black, color, speed_ms);
        }
        
        case LED_PATTERN_FADE_OUT:
        {
            LedState_t black = led_color_create(0, 0, 0, 0);
            return led_pattern_fade(led_controller, edge_id, start_idx, end_idx, 
                                  color, black, speed_ms);
        }
        
        case LED_PATTERN_TWINKLE:
            return led_pattern_twinkle(led_controller, edge_id, start_idx, end_idx, 
                                     color, 0.2f);
            
        default:
            return -1;
    }
}



// Set pattern for specific edge - MAIN FUNCTION
void led_set_edge_pattern(uint8_t edge_id, uint8_t pattern, 
                         uint8_t r, uint8_t g, uint8_t b, 
                         uint8_t intensity, uint32_t speed_ms) {
    if (edge_id >= NUM_EDGES) {
        ESP_LOGE(TAG, "Invalid edge_id: %d", edge_id);
        return;
    }
    
    if (pattern > LED_PATTERN_TWINKLE) {
        ESP_LOGE(TAG, "Invalid pattern: %d", pattern);
        return;
    }
    
    edge_state_t* state = &edge_states[edge_id];
    
    // Remove existing pattern if active
    if (state->visual_pattern_id >= 0) {
        led_pattern_remove(led_controller, state->visual_pattern_id);
        state->visual_pattern_id = -1;
    }
    
    // Set new pattern parameters
    state->pattern = pattern;
    state->r = r;
    state->g = g;
    state->b = b;
    state->intensity = intensity;
    state->speed_ms = speed_ms < 1000 ? 1000 : speed_ms;
    state->active = (pattern != LED_PATTERN_OFF);
    
    // Create visual pattern
    if (state->active) {
        state->visual_pattern_id = create_visual_pattern(edge_id, pattern, r, g, b, 
                                                       intensity, speed_ms);
        if (state->visual_pattern_id < 0) {
            // ESP_LOGE(TAG, "Failed to create visual pattern for edge %d", edge_id);
            state->active = false;
        }
    }
    
    // ESP_LOGI(TAG, "Edge %"PRIu32": Pattern %s, RGB(%"PRIu32",%"PRIu32",%"PRIu32"), Intensity=%"PRIu32", Speed=%" PRIu32 "ms", 
            //  edge_id, pattern_names[pattern], r, g, b, intensity, speed_ms);
}

// Turn off specific edge
void led_turn_off_edge(uint8_t edge_id) {
    led_set_edge_pattern(edge_id, LED_PATTERN_OFF, 0, 0, 0, 0, 1000);
}

// Turn off all edges
void led_turn_off_all(void) {
    for (int i = 0; i < NUM_EDGES; i++) {
        led_turn_off_edge(i);
    }
}

// Show all available patterns
void led_show_all_patterns(void) {
    ESP_LOGI(TAG, "Available patterns:");
    for (int i = 0; i <= LED_PATTERN_TWINKLE; i++) {
        // ESP_LOGI(TAG, "  %"PRIu32": %s", i, pattern_names[i]);
    }
}

// Demo function - cycles through all patterns on a specific edge
void led_demo_edge_patterns(uint8_t edge_id) {
    if (edge_id >= NUM_EDGES) return;
    
    // ESP_LOGI(TAG, "Starting pattern demo on edge %"PRIu32"", edge_id);
    
    // Turn off all other edges
    for (int i = 0; i < NUM_EDGES; i++) {
        if (i != edge_id) {
            led_turn_off_edge(i);
        }
    }
    
    // Demo colors
    uint8_t demo_colors[][3] = {
        {255, 0, 0},    // Red
        {0, 255, 0},    // Green
        {0, 0, 255},    // Blue
        {255, 255, 0},  // Yellow
        {255, 0, 255},  // Magenta
        {0, 255, 255},  // Cyan
        {255, 255, 255} // White
    };
    
    for (int pattern = 1; pattern <= LED_PATTERN_TWINKLE; pattern++) {
        uint8_t* color = demo_colors[pattern % 7];
        // ESP_LOGI(TAG, "Edge %" PRIu32" Testing pattern %s", edge_id, pattern_names[pattern]);
        
        led_set_edge_pattern(edge_id, pattern, color[0], color[1], color[2], 200, 2000);
        vTaskDelay(pdMS_TO_TICKS(5000));  // Show each pattern for 5 seconds
    }
    
    // ESP_LOGI(TAG, "Pattern demo completed on edge %" PRIu32, edge_id);
}

// Test function - shows different patterns on all edges simultaneously
void led_test_all_edges(void) {
    ESP_LOGI(TAG, "Testing different patterns on all edges");
    
    // Set different patterns on each edge
    led_set_edge_pattern(0, LED_PATTERN_STATIC, 255, 0, 0, 200, 1000);     // Red static
    led_set_edge_pattern(1, LED_PATTERN_BLINK, 0, 255, 0, 200, 1000);      // Green blink
    led_set_edge_pattern(2, LED_PATTERN_BREATH, 0, 0, 255, 200, 3000);     // Blue breath
    led_set_edge_pattern(3, LED_PATTERN_RAINBOW, 255, 255, 255, 200, 5000); // Rainbow
    
    ESP_LOGI(TAG, "All edges running different patterns");
}

// Get edge status
void led_get_edge_status(uint8_t edge_id) {
    if (edge_id >= NUM_EDGES) return;
    
    edge_state_t* state = &edge_states[edge_id];
    // ESP_LOGI(TAG, "Edge %"PRIu32": Pattern=%s, RGB(%"PRIu32",%"PRIu32",%"PRIu32"), Intensity=%"PRIu32", Speed=%"PRIu32"ms, Active=%s",
            //  edge_id, pattern_names[state->pattern], 
            //  state->r, state->g, state->b, state->intensity, state->speed_ms,
            //  state->active ? "Yes" : "No");
}

// Get all edges status
void led_get_all_status(void) {
    ESP_LOGI(TAG, "=== LED Status ===");
    for (int i = 0; i < NUM_EDGES; i++) {
        led_get_edge_status((uint8_t)i);
    }
}

// Clear all patterns and turn off LEDs
void led_clear_all(void) {
    for (int i = 0; i < NUM_EDGES; i++) {
        if (edge_states[i].visual_pattern_id >= 0) {
            led_pattern_remove(led_controller, edge_states[i].visual_pattern_id);
            edge_states[i].visual_pattern_id = -1;
        }
        edge_states[i].pattern = LED_PATTERN_OFF;
        edge_states[i].active = false;
    }
    
    if (led_controller) {
        led_controller_clear(led_controller);
    }
    
    if (strip) {
        led_strip_clear(strip);
        led_strip_refresh(strip);
    }
    
    ESP_LOGI(TAG, "All LEDs cleared");
}