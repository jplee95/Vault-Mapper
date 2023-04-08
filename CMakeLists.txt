cmake_minimum_required(VERSION 3.23)
project(mapper)

set(CMAKE_CXX_STANDARD 17)

find_package(glfw3 REQUIRED CONFIG)
find_package(glm REQUIRED CONFIG)

add_executable(mapper main.cpp bitmap.hpp types.hpp)
set_target_properties(mapper PROPERTIES OUTPUT_NAME "VaultMapper")

target_include_directories(mapper
        PRIVATE
        extern/lodepng/include
        extern/glad/include
        )
target_sources(mapper PRIVATE
        extern/lodepng/include/lodepng.h extern/lodepng/src/lodepng.cpp
        extern/lodepng/include/lodepng_util.h extern/lodepng/src/lodepng_util.cpp

        extern/glad/include/glad/glad.h extern/glad/glad.c

        info.rc

        resource/background.h
        resource/compass.h
        resource/help.h
        resource/icon_16.h
        resource/icon_32.h
        resource/icon_48.h
        resource/icon_64.h
        resource/shader_frag.h
        resource/shader_vert.h
        resource/texture.h
        )
target_link_options(mapper PRIVATE -mwindows)

target_link_libraries(mapper PUBLIC opengl32 glfw glm::glm)

add_custom_command(TARGET mapper POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:mapper> $<TARGET_FILE_DIR:mapper>
        COMMAND_EXPAND_LISTS
        )