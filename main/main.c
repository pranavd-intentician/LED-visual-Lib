#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <esp_system.h>
#include <esp_timer.h>
#include <nvs_flash.h>
#include "physical_led_updater.h"
#include  "render_engine.h"
#include  "main.h"

static const char *TAG = "MAIN";
TaskHandle_t physical_led_task_handle = NULL;
TaskHandle_t render_engine_task_handle = NULL;
// Visual LED controller
LEDController* led_controller = NULL;

void app_main(void) {
    ESP_LOGI(TAG, "Starting Simple LED Handler Demo");
    
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    // Initialize LED handler
    led_handler_init();
    xTaskCreate(led_update_task, "physical_led_update", 1024*4, NULL, 5, &physical_led_task_handle);
    vTaskDelay(pdMS_TO_TICKS(10));
    xTaskCreate(led_controller_task, "render_engine", 1024*4, NULL, 4, &render_engine_task_handle);

    
    // Show available patterns
    led_show_all_patterns();
    
    ESP_LOGI(TAG, "=== DEMO 1: Individual Edge Control ===");
    
    // Set different patterns on each edge
    led_set_edge_pattern(0, LED_PATTERN_STATIC, 255, 0, 0, 200, 1000);     // Edge 0: Red static
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    led_set_edge_pattern(1, LED_PATTERN_BLINK, 0, 255, 0, 200, 1000);      // Edge 1: Green blink
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    led_set_edge_pattern(2, LED_PATTERN_BREATH, 0, 0, 255, 200, 3000);     // Edge 2: Blue breath
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    led_set_edge_pattern(3, LED_PATTERN_RAINBOW, 255, 255, 255, 200, 5000); // Edge 3: Rainbow
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    ESP_LOGI(TAG, "All edges running different patterns");
    led_get_all_status();
    vTaskDelay(pdMS_TO_TICKS(10000));
    
    ESP_LOGI(TAG, "=== DEMO 2: Single Edge Pattern Demo ===");
    led_demo_edge_patterns(2);  // Demo all patterns on edge 2
    
    ESP_LOGI(TAG, "=== DEMO 3: Turn Off Individual Edges ===");
    led_turn_off_edge(0);
    ESP_LOGI(TAG, "Turned off edge 0");
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    led_turn_off_edge(1);
    ESP_LOGI(TAG, "Turned off edge 1");
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    ESP_LOGI(TAG, "=== DEMO 4: Test All Edges ===");
    led_test_all_edges();
    vTaskDelay(pdMS_TO_TICKS(10000));
    
    ESP_LOGI(TAG, "=== DEMO 5: Manual Pattern Assignment ===");
    
    // Example: Set specific pattern on edge 1
    ESP_LOGI(TAG, "Setting TWINKLE pattern on edge 1");
    led_set_edge_pattern(1, LED_PATTERN_TWINKLE, 255, 255, 0, 255, 500);
    vTaskDelay(pdMS_TO_TICKS(5000));
    
    // Example: Set FADE_IN pattern on edge 2
    ESP_LOGI(TAG, "Setting FADE_IN pattern on edge 2");
    led_set_edge_pattern(2, LED_PATTERN_FADE_IN, 0, 255, 255, 255, 3000);
    vTaskDelay(pdMS_TO_TICKS(5000));
    
    ESP_LOGI(TAG, "=== DEMO Complete ===");
    led_clear_all();
    
    
}
// Get current time in milliseconds
uint32_t get_current_time_ms(void) {
    return esp_timer_get_time() / 1000;
}