# Минимальная требуемая версия CMake
cmake_minimum_required(VERSION 3.10)

# Название проекта
project(libNoise C CXX)

# Устанавливаем стандарт C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# =========================================================
# 1. СТАТИЧЕСКАЯ БИБЛИОТЕКА ЛОГИКИ ШУМОВ (libNoise.a)
# =========================================================

# Список файлов реализации алгоритмов шума
set(NOISE_SOURCES
    src/noises/src/value.cpp
    src/noises/src/perlin.cpp
    src/noises/src/simplex.cpp
    src/noises/src/fbm.cpp
    src/noises/src/worley.cpp
)

# Создаем статическую библиотеку
add_library(libNoise STATIC ${NOISE_SOURCES})

# Указываем, где искать заголовочные файлы библиотеки (для внешних проектов и для demo)
target_include_directories(libNoise PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/noises/include
)


# =========================================================
# 2. ЗАВИСИМОСТИ (VENDOR) - Статическая линковка ImGui и Glad
# =========================================================

# --- A. GLAD ---
add_library(glad STATIC
    vendor/glad/src/glad.c
)
set_property(TARGET glad PROPERTY LINKER_LANGUAGE C)
set_target_properties(glad PROPERTIES C_STANDARD 99)
target_include_directories(glad PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/glad/include
)


# --- B. IMGUI ---
set(IMGUI_SOURCES
    vendor/imgui/imgui.cpp
    vendor/imgui/imgui_draw.cpp
    vendor/imgui/imgui_widgets.cpp
    vendor/imgui/imgui_tables.cpp
    vendor/imgui/backends/imgui_impl_glfw.cpp
    vendor/imgui/backends/imgui_impl_opengl3.cpp
)
add_library(imgui STATIC ${IMGUI_SOURCES})

target_include_directories(imgui PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/glad/include
)
target_compile_definitions(imgui PRIVATE
    IM_GL_LOADER_GLAD
)

# --- C. GLFW (Системная) ---
find_path(GLFW_INCLUDE_DIR GLFW/glfw3.h)
find_library(GLFW_LIBRARY glfw)

if(NOT GLFW_LIBRARY OR NOT GLFW_INCLUDE_DIR)
    message(FATAL_ERROR "Could not find GLFW library or headers. Check if GLFW development packages are installed.")
endif()


# =========================================================
# 3. ОСНОВНОЙ ИСПОЛНЯЕМЫЙ ФАЙЛ libNoise_demo
# =========================================================

add_executable(libNoise_demo
    src/main.cpp
    src/render/ui.cpp # Файл UI, рендеринга и цикла
)

# Устанавливаем директории включения
target_include_directories(libNoise_demo PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${GLFW_INCLUDE_DIR}
)

# Линкуем библиотеки
target_link_libraries(libNoise_demo PRIVATE
    # Линкуем НАШУ библиотеку шумов!
    libNoise

    # Статические библиотеки Vendor
    imgui
    glad

    # Системные и динамические библиотеки
    ${GLFW_LIBRARY}
    OpenGL

    m
    dl
)

# Опционально: полезные флаги компилятора
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(libNoise_demo PRIVATE
        -Wall
        -Wextra
        -pedantic
    )
endif()
