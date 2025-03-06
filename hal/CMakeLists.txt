# CMakeList.txt for HAL
#   Build a library (`hal`) which exposes the header files as "hal/*.h"
#   Use header as: #include "hal/button.h"

# set(CURL_INCLUDE_DIR "/usr/include/x86_64-linux-gnu")
# set(CURL_LIBRARY "/usr/lib/x86_64-linux-gnu/libcurl.so")
find_package(CURL REQUIRED)
find_package(cJSON REQUIRED)
include_directories(${CURL_INCLUDE_DIRS})

include_directories(hal/include)
file(GLOB MY_SOURCES "src/*.c")

add_library(hal STATIC ${MY_SOURCES})

target_include_directories(hal PUBLIC include)

target_include_directories(hal PUBLIC include ${CURL_INCLUDE_DIRS})
target_link_libraries(hal PRIVATE ${CURL_LIBRARIES})

target_include_directories(hal PUBLIC include ${CJSON_INCLUDE_DIR})
target_link_libraries(hal PRIVATE ${CJSON_LIBRARY})
