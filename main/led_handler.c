#include "led_handler.h"
#include "led_strip.h"
#include "led_strip_types.h"
#include "visual_LED.h"  // Include our LED library
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <esp_timer.h>

#define LED_STRIP_GPIO 17
#define LED_STRIP_LENGTH 60
#define NUM_EDGES 1  // Single LED strip as one edge

static const char *TAG = "LED_HANDLER";

static led_strip_handle_t strip;
static LEDController* led_controller = NULL;
static TaskHandle_t led_task_handle = NULL;
static bool led_task_running = false;

// Global config (modifiable by BLE)
led_config_t led_strip_config = {
    .red = 0,
    .green = 0,
    .blue = 0,
    .intensity = 255,
    .speed_ms = 100,
    .duration_ms = 1000,
    .pattern = 0
};

// Get current time in milliseconds
static uint32_t get_current_time_ms(void) {
    return esp_timer_get_time() / 1000;
}

// Apply LED matrix to physical strip
static void apply_led_matrix_to_strip(LEDController* controller) {
    if (!controller || !strip) return;
    
    // Clear strip first
    led_strip_clear(strip);
    
    // Apply colors from our LED matrix to the physical strip
    for (int i = 0; i < LED_STRIP_LENGTH && i < controller->matrix.leds_per_edge[0]; i++) {
        LEDColor color = led_matrix_get_led(&controller->matrix, 0, i);
        
        // Apply intensity scaling
        uint8_t final_r = (color.r * color.intensity) / 255;
        uint8_t final_g = (color.g * color.intensity) / 255;
        uint8_t final_b = (color.b * color.intensity) / 255;
        
        esp_err_t err = led_strip_set_pixel(strip, i, final_r, final_g, final_b);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to set pixel %d: %s", i, esp_err_to_name(err));
        }
    }
    
    // Refresh the strip to show changes
    led_strip_refresh(strip);
}

// LED update task
static void led_update_task(void *param) {
    TickType_t last_wake_time = xTaskGetTickCount();
    const TickType_t update_period = pdMS_TO_TICKS(16); // ~60 FPS
    
    while (led_task_running) {
        if (led_controller) {
            uint32_t current_time = get_current_time_ms();
            led_controller_update(led_controller, current_time);
            apply_led_matrix_to_strip(led_controller);
        }
        
        vTaskDelayUntil(&last_wake_time, update_period);
    }
    
    // Clean up - turn off LEDs
    if (strip) {
        led_strip_clear(strip);
    }
    
    vTaskDelete(NULL);
}

void led_handler_init(void) {
    // Initialize ESP32 LED strip
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
    
    // Initialize our LED controller
    int leds_per_edge[NUM_EDGES] = {LED_STRIP_LENGTH};
    led_controller = led_controller_create(NUM_EDGES, leds_per_edge);
    
    if (!led_controller) {
        ESP_LOGE(TAG, "Failed to create LED controller");
        return;
    }
    
    // Start the LED update task
    led_task_running = true;
    xTaskCreate(led_update_task, "led_update", 1024 * 12, NULL, 5, &led_task_handle);
    
    vTaskDelay(pdMS_TO_TICKS(100));
    ESP_LOGI(TAG, "LED Strip and Controller initialized");
}

void led_handler_deinit(void) {
    // Stop the update task
    led_task_running = false;
    if (led_task_handle) {
        vTaskDelay(pdMS_TO_TICKS(100)); // Give task time to exit
        led_task_handle = NULL;
    }
    
    // Clean up LED controller
    if (led_controller) {
        led_controller_destroy(led_controller);
        led_controller = NULL;
    }
    
    // Clear physical strip
    if (strip) {
        led_strip_clear(strip);
    }
}

void animate_pattern(const led_config_t *cfg) {
    if (!led_controller) {
        ESP_LOGE(TAG, "LED controller not initialized");
        return;
    }
    
    // Clear all existing patterns first
    for (int i = 0; i < MAX_PATTERNS; i++) {
        led_pattern_remove(led_controller, i);
    }
    
    // Create LED color from config
    LEDColor color = led_color_create(cfg->red, cfg->green, cfg->blue, cfg->intensity);
    
    uint32_t current_time = get_current_time_ms();
    int pattern_id = -1;
    
    ESP_LOGI(TAG, "Starting pattern %d with color (%d,%d,%d), intensity=%d", 
             cfg->pattern, cfg->red, cfg->green, cfg->blue, cfg->intensity);
    
    switch (cfg->pattern) {
        case LED_PATTERN_STATIC: // Use LED_PATTERN_STATIC instead of 0
            pattern_id = led_pattern_static(led_controller, 0, 0, LED_STRIP_LENGTH - 1, color);
            break;
            
        case LED_PATTERN_BLINK: // Use LED_PATTERN_BLINK instead of 1
            pattern_id = led_pattern_blink(led_controller, 0, 0, LED_STRIP_LENGTH - 1, 
                                         color, cfg->speed_ms, cfg->speed_ms, 
                                         cfg->duration_ms / (cfg->speed_ms * 2));
            break;
            
        case LED_PATTERN_RAINBOW: // Use LED_PATTERN_RAINBOW instead of 2 (using palette cycle)
        {
            ColorPalette rainbow = led_palette_rainbow(12);
            // Fixed: pass palette by value, not pointer, and include all 7 arguments
            pattern_id = led_pattern_palette_cycle(led_controller, 0, 0, LED_STRIP_LENGTH - 1,
                                                 rainbow, cfg->speed_ms * 10, 0);
            break;
        }
        
        case LED_PATTERN_BREATH: // Use LED_PATTERN_BREATH instead of 3 (using pulse)
            pattern_id = led_pattern_pulse(led_controller, 0, 0, LED_STRIP_LENGTH - 1,
                                         color, cfg->intensity, cfg->speed_ms * 20);
            break;
            
        case LED_PATTERN_FADE: // Use LED_PATTERN_FADE instead of 4
        {
            LEDColor black = led_color_create(0, 0, 0, 0);
            pattern_id = led_pattern_fade(led_controller, 0, 0, LED_STRIP_LENGTH - 1,
                                        black, color, cfg->duration_ms);
            break;
        }
        
        case LED_PATTERN_GRADIENT: // Use LED_PATTERN_GRADIENT instead of 5
        {
            LEDColor end_color = led_color_create(cfg->blue, cfg->red, cfg->green, cfg->intensity);
            pattern_id = led_pattern_gradient(led_controller, 0, 0, LED_STRIP_LENGTH - 1,
                                            color, end_color);
            break;
        }
        
        case LED_PATTERN_TWINKLE: // Use LED_PATTERN_TWINKLE instead of 6
            // Fixed: removed extra arguments - only 6 parameters expected
            pattern_id = led_pattern_twinkle(led_controller, 0, 0, LED_STRIP_LENGTH - 1,
                                           color, 0.1f);
            break;
            
        default:
            ESP_LOGW(TAG, "Unknown pattern %d, using static", cfg->pattern);
            pattern_id = led_pattern_static(led_controller, 0, 0, LED_STRIP_LENGTH - 1, color);
            break;
    }
    
    if (pattern_id >= 0) {
        led_pattern_start(led_controller, pattern_id, current_time);
        ESP_LOGI(TAG, "Pattern %d started with ID %d", cfg->pattern, pattern_id);
        
        // If duration is specified and not infinite, stop after duration
        if (cfg->duration_ms > 0 && cfg->pattern != LED_PATTERN_BLINK) { // Blink handles its own duration
            vTaskDelay(pdMS_TO_TICKS(cfg->duration_ms));
            led_pattern_stop(led_controller, pattern_id);
            led_controller_clear(led_controller);
        }
    } else {
        ESP_LOGE(TAG, "Failed to create pattern %d", cfg->pattern);
    }
}

// Demo function to test different patterns
void led_handler_demo(void) {
    if (!led_controller) {
        ESP_LOGE(TAG, "LED controller not initialized");
        return;
    }
    
    ESP_LOGI(TAG, "Starting LED demo sequence");
    
    // Demo 1: Red static
    led_config_t demo_config = {255, 0, 0, 200, 100, 2000, LED_PATTERN_STATIC};
    animate_pattern(&demo_config);
    vTaskDelay(pdMS_TO_TICKS(500));
    
    // Demo 2: Blue blink
    demo_config = (led_config_t){0, 0, 255, 200, 200, 3000, LED_PATTERN_BLINK};
    animate_pattern(&demo_config);
    vTaskDelay(pdMS_TO_TICKS(500));
    
    // Demo 3: Rainbow
    demo_config = (led_config_t){0, 0, 0, 150, 50, 5000, LED_PATTERN_RAINBOW};
    animate_pattern(&demo_config);
    vTaskDelay(pdMS_TO_TICKS(500));
    
    // Demo 4: Green breath
    demo_config = (led_config_t){0, 255, 0, 200, 100, 4000, LED_PATTERN_BREATH};
    animate_pattern(&demo_config);
    vTaskDelay(pdMS_TO_TICKS(500));
    
    // Demo 5: Twinkle white
    demo_config = (led_config_t){255, 255, 255, 180, 80, 3000, LED_PATTERN_TWINKLE};
    animate_pattern(&demo_config);
    
    ESP_LOGI(TAG, "Demo sequence completed");
}

void led_handler_update_config(const led_config_t *new_config) {
    if (!new_config) return;
    
    led_strip_config = *new_config;
    ESP_LOGI(TAG, "LED config updated: pattern=%d, color=(%d,%d,%d), intensity=%d, speed=%dms, duration=%dms",
             new_config->pattern, new_config->red, new_config->green, new_config->blue,
             new_config->intensity, new_config->speed_ms, new_config->duration_ms);
    
    // Apply the new configuration immediately
    animate_pattern(new_config);
}

// Additional utility functions for advanced patterns
void led_handler_set_custom_gradient(uint8_t r1, uint8_t g1, uint8_t b1,
                                   uint8_t r2, uint8_t g2, uint8_t b2,
                                   uint8_t intensity) {
    if (!led_controller) return;
    
    LEDColor start = led_color_create(r1, g1, b1, intensity);
    LEDColor end = led_color_create(r2, g2, b2, intensity);
    
    // Clear existing patterns
    for (int i = 0; i < MAX_PATTERNS; i++) {
        led_pattern_remove(led_controller, i);
    }
    
    int pattern_id = led_pattern_gradient(led_controller, 0, 0, LED_STRIP_LENGTH - 1, start, end);
    if (pattern_id >= 0) {
        led_pattern_start(led_controller, pattern_id, get_current_time_ms());
    }
}

void led_handler_clear(void) {
    if (led_controller) {
        for (int i = 0; i < MAX_PATTERNS; i++) {
            led_pattern_remove(led_controller, i);
        }
        led_controller_clear(led_controller);
    }
    
    if (strip) {
        led_strip_clear(strip);
    }
}