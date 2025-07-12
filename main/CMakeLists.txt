idf_component_register(
    SRCS
        "main.c"
    INCLUDE_DIRS "."
    REQUIRES
        physical_led_updater
        render_engine
        framebuffer
)
