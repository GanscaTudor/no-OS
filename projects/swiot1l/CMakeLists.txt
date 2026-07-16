add_executable(swiot1l)

# Common sources (shared by every variant)
target_sources(swiot1l PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/${PLATFORM}/main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/${PLATFORM}/parameters.c
)

# Stock init parameters (MAX14906/AD74413R/ADT75/etc); motor firmware brings
# its own minimal set of params and does not link these.
if(CONFIG_SWIOT1L_DEFAULT_FW OR CONFIG_SWIOT1L_MQTT_EXAMPLE)
    target_sources(swiot1l PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/common/common_data.c
    )
    target_include_directories(swiot1l PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/common
    )
endif()

# Default IIO firmware
if(CONFIG_SWIOT1L_DEFAULT_FW)
    target_sources(swiot1l PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/swiot_fw.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/common/swiot.c
    )
    target_include_directories(swiot1l PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )
endif()

# MQTT example
if(CONFIG_SWIOT1L_MQTT_EXAMPLE)
    target_sources(swiot1l PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/examples/swiot1l-mqtt/swiot1l_mqtt.c
    )
    target_include_directories(swiot1l PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/examples/swiot1l-mqtt
    )
endif()

# Motor control firmware
if(CONFIG_SWIOT1L_MOTOR_CONTROL)
    target_sources(swiot1l PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/examples/motor_control/motor_control.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/examples/motor_control/motor_control_data.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/examples/motor_control/pwm_isr.c
    )
    target_include_directories(swiot1l PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/examples/motor_control
    )
endif()

# OA TC6 SPI protocol flag
if(CONFIG_SWIOT1L_OA_TC6_SPI)
    target_compile_definitions(swiot1l PRIVATE SWIOT1L_OA_TC6_SPI=1)
else()
    target_compile_definitions(swiot1l PRIVATE SWIOT1L_OA_TC6_SPI=0)
endif()

target_compile_definitions(swiot1l PRIVATE
    NO_OS_LWIP_NETWORKING
    IIO_IGNORE_BUFF_OVERRUN_ERR
)

target_include_directories(swiot1l PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/common
    ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/${PLATFORM}
    ${CMAKE_CURRENT_SOURCE_DIR}/src/platform
)

target_link_libraries(swiot1l no-os)

config_platform_sdk(swiot1l)
add_flash_target(swiot1l)
post_build_config(swiot1l)
