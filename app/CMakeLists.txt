# Build the app, using the HAL

include_directories(include)
file(GLOB MY_SOURCES "src/*.c")
add_executable(project ${MY_SOURCES})

# Make use of the HAL library
target_link_libraries(project LINK_PRIVATE hal)
target_link_libraries(project LINK_PRIVATE lcd)
target_link_libraries(project LINK_PRIVATE lgpio)
target_link_libraries(project PRIVATE hal ${CURL_LIBRARIES})
target_link_libraries(project PRIVATE hal ${CJSON_LIBRARY})


# Copy executable to final location (change `project` to project name as needed)
add_custom_command(TARGET project POST_BUILD 
  COMMAND "${CMAKE_COMMAND}" -E copy 
     "$<TARGET_FILE:project>"
     "~/cmpt433/public/project" 
  COMMENT "Copying ARM executable to public NFS directory")

# Copy the GPS txt data for demo
add_custom_command(TARGET project POST_BUILD 
COMMAND "${CMAKE_COMMAND}" -E copy 
    "${CMAKE_SOURCE_DIR}/demo_gps.txt"
    "~/cmpt433/public/demo_gps.txt" 
COMMENT "Copying GPS txt data for demo")


add_custom_command(TARGET project POST_BUILD 
COMMAND "${CMAKE_COMMAND}" -E copy 
    "${CMAKE_SOURCE_DIR}/app/src/ai_api.py"
    "~/cmpt433/public/ai_api.py" 
COMMENT "Copying ai_api.py")


add_custom_command(TARGET project POST_BUILD 
COMMAND "${CMAKE_COMMAND}" -E copy 
    "${CMAKE_SOURCE_DIR}/app/src/ai_api.py"
    "~/cmpt433/public/my_speech.py" 
COMMENT "Copying my_speech.py")

add_custom_command(TARGET project POST_BUILD 
COMMAND "${CMAKE_COMMAND}" -E copy 
    "${CMAKE_SOURCE_DIR}/setup_env.sh"
    "~/cmpt433/public/setup_env.sh" 
COMMENT "Copying setup_env.sh")

find_library(GPIOD_LIBRARY gpiod)       
target_link_libraries(project LINK_PRIVATE gpiod)
