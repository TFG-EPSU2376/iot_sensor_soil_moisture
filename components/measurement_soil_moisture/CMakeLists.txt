idf_component_register(SRCS "measurement_soil_moisture.c" 
                    INCLUDE_DIRS "include"
                    REQUIRES driver esptool_py esp_adc esp_timer nvs_flash)
