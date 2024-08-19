idf_component_register(SRCS  "main.c"
                       INCLUDE_DIRS "."
                       REQUIRES  esptool_py json  monitoring_client measurement_soil_moisture )

list(APPEND EXTRA_COMPONENT_DIRS components)
