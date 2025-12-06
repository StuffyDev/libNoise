# 📚 libNoise: Procedural Noise Algorithms Demonstration

**libNoise** is a demonstration project created for visualizing and studying various procedural noise algorithms (Perlin, Simplex, Worley, FBM, and Value Noise).

A key feature of the project is its modular architecture: all core noise algorithms are compiled into a separate **static library (`libNoise.a`)** which can be easily integrated into any third-party project requiring noise generation. The main application (`libNoise_demo`) serves as an interactive UI for configuring and visualizing these algorithms.

-----

## 🚀 Features

  * **Noise Core (libNoise.a):** A standalone static library providing a clean API for 2D noise generation.
  * **Supported Algorithms:** Perlin, Simplex, Value, Worley (Cellular), and FBM (Fractal Brownian Motion).
  * **Interactive UI:** Allows real-time adjustment of Seed, Scale, Octaves, Persistence, and other parameters.
  * **Export:** Capability to save the generated pattern to a PNG file.

-----

## 🛠️ Building the Project (Compilation)

The project uses **CMake** for cross-platform compilation.

### 1\. Prerequisites

  * C/C++ Compiler (GCC/G++ or Clang) with **C++17** support.
  * **CMake** (version 3.10+).

### 2\. Build Steps

Execute the following commands in the root directory of the project:

```bash
mkdir build
cd build

cmake ..

make -j4 # Use -jN where N is the number of cores on your CPU
```

### 3\. Running the Demo

The executable file will be located in the `build/` folder:

```bash
./libNoise_demo
```

-----

## 💡 Using the `libNoise.a` Library

The noise core is designed for direct integration into your C++ projects.

### A. Library Structure

After compilation, the static file **`libNoise.a`** will be located in the `build/` folder. The necessary header files for the library are located in the `src/noises/include/` directory.

### B. CMake Integration

The recommended way to use `libNoise.a` in your third-party project (`YourProject`) is via CMake:

1.  **Copy** the `src/noises` folder (containing the implementations and headers) into your project structure.
2.  **Add** the following lines to the `CMakeLists.txt` of your third-party project:

```cmake
# Include the noise algorithms as a static library target
add_library(libNoise STATIC
    src/noises/src/value.cpp
    src/noises/src/perlin.cpp
    src/noises/src/simplex.cpp
    src/noises/src/fbm.cpp
    src/noises/src/worley.cpp
)

# Specify the location of the header files
target_include_directories(libNoise PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/noises/include
)

# Link the noise library to your executable
target_link_libraries(YourProject PRIVATE libNoise)
```

### C. Code Example

Once linked, you can use the noise classes directly:

```cpp
#include <iostream>
#include "perlin.h"
#include "fbm.h"

int main() {
    auto base_perlin = std::make_unique<PerlinNoise>(123);
    
    FBMNoise fbm_generator(std::move(base_perlin), 6, 2.0, 0.5);
    
    real_t noise_value = fbm_generator.GetNoise(5.0, 10.0);
    
    std::cout << "FBM Noise at (5, 10): " << noise_value << std::endl;
    
    return 0;
}
```

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
