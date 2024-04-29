cmake_minimum_required(VERSION 3.0.0)
project(VoxelEngine VERSION 0.1.0)

find_package(OpenGL)
set(CMAKE_CXX_STANDARD 14)
add_subdirectory(mingw_stdthreads)
include_directories(${OPENGL_INCLUDE_DIRS})
include_directories(glew-2.1.0/include)
link_directories(glew-2.1.0/lib/Release/x64)
include_directories(SDL2-2.28.5/include)
link_directories(SDL2-2.28.5/lib/x64)


# add_compile_definitions(DISABLE_BUFFER)
# add_compile_definitions(DISABLE_THREAD)


add_executable(VoxelEngine main.cpp
    class/utility/graphics/screen.cpp
    class/utility/graphics/openGL_related.cpp
    class/utility/graphics/buffer.cpp

    class/utility/math/vector3.cpp
    
    class/world/materials.cpp
    class/world/world_generator.cpp
    class/world/chunk.cpp
    class/world/world.cpp

    class/gameplay/player.cpp
)

target_link_libraries(${PROJECT_NAME} mingw_stdthreads SDL2 SDL2main glew32 ${OPENGL_LIBRARY})