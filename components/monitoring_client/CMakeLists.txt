idf_component_register(SRCS "monitoring_client.c" "monitoring_utils.c"
                    INCLUDE_DIRS "include"
                    REQUIRES driver esp_timer nvs_flash thread_child
)
