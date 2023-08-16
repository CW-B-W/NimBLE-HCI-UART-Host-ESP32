FILE(GLOB freertos_includes LIST_DIRECTORIES true $ENV{IDF_PATH}/components/freertos/FreeRTOS-Kernel/include/freertos)
FILE(GLOB_RECURSE nimble_includes FOLLOW_SYMLINKS LIST_DIRECTORIES true 
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/include/nimble/*/include
)
# FILE(GLOB nimble_arduino_includes LIST_DIRECTORIES true ${CMAKE_SOURCE_DIR}/NimBLE-Arduino/src)

FILE(GLOB_RECURSE nimble_sources FOLLOW_SYMLINKS ${CMAKE_SOURCE_DIR}/src/*.*)
# FILE(GLOB nimble_arduino_sources ${CMAKE_SOURCE_DIR}/NimBLE-Arduino/src/*.*)

# These files are ignored by official Makefile
list(REMOVE_ITEM nimble_sources 
        ${CMAKE_SOURCE_DIR}/src/nimble/porting/nimble/src/hal_timer.c
        ${CMAKE_SOURCE_DIR}/src/nimble/porting/nimble/src/os_cputime.c
        ${CMAKE_SOURCE_DIR}/src/nimble/porting/nimble/src/os_cputime_pwr2.c
        ${CMAKE_SOURCE_DIR}/src/nimble/nimble/host/src/ble_gatts_lcl.c
)

idf_component_register(
    INCLUDE_DIRS
        ${freertos_includes}
        ${nimble_includes}
        # ${nimble_arduino_includes}
        
    SRCS
        ${nimble_sources}
        # ${nimble_arduino_sources}
)