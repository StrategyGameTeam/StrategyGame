cmake_minimum_required(VERSION 3.21)
project(stratgametest)
include(FetchContent)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

# setting lua
if(NOT DEFINED ENV{LUA_DIR})
    message(FATAL_ERROR " Lua could not be found. Please, set LUA_DIR to the location of the /src directory in lua source files. It should contain the header and library files.")
endif()

# platform test
if (WIN32)
    # names on windows are different, because symbols are bound to file names, and
    # luajit is compatible with lua
    set(LUA_DYN_FILES "$ENV{LUA_DIR}/lua51.dll")
    set(LUA_LIB_FILES "$ENV{LUA_DIR}/lua51.lib")
elseif(APPLE)
    set(LUA_DYN_FILES "$ENV{LUA_DIR}/libluajit.so")
    set(LUA_LIB_FILES "$ENV{LUA_DIR}/libluajit.a")
elseif(UNIX)
    set(LUA_DYN_FILES "$ENV{LUA_DIR}/libluajit.so")
    set(LUA_LIB_FILES "$ENV{LUA_DIR}/libluajit.a")
else()
    message(FATAL_ERROR "Unknown platform, we have no clue what file to copy")
endif()


# Copy required Lua files
file(
    COPY "$ENV{LUA_DIR}/jit"
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/lua"
)
file(
    COPY "${LUA_DYN_FILES}"
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}"
)

# rename .so to .dylib
if (APPLE)
    file(RENAME "${CMAKE_CURRENT_BINARY_DIR}/libluajit.so" "${CMAKE_CURRENT_BINARY_DIR}/libluajit.dylib")
endif()

FetchContent_Declare(
    sol2
    GIT_REPOSITORY https://github.com/ThePhD/sol2.git
    GIT_TAG v3.3.0
)
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 4.2.0
)

set(BUILD_EXAMPLES OFF)
FetchContent_MakeAvailable(raylib)

FetchContent_MakeAvailable(sol2)

add_executable(
    stratgametest
        src/main.cpp
        src/hex.cpp
        src/input.cpp
        src/module.cpp
)

target_include_directories(
    stratgametest
        PRIVATE
            $ENV{LUA_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(
    stratgametest
        PRIVATE
        raylib
        sol2
        ${LUA_LIB_FILES}
)

add_custom_command(
    TARGET stratgametest POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/ $<TARGET_FILE_DIR:stratgametest>/resources
)