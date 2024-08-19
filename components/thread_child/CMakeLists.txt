idf_component_register(SRCS "thread_child.c"
                    INCLUDE_DIRS "include"
                    REQUIRES driver log esp_common esp_timer openthread nvs_flash vfs)
