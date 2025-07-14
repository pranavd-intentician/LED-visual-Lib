idf_component_register(
    SRCS "physical_led_updater.c"
    INCLUDE_DIRS "."
    REQUIRES driver render_engine esp_timer main
    PRIV_REQUIRES espressif__led_strip
)