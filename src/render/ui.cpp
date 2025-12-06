// src/render/ui.cpp

#include <iostream>
#include <memory>
#include <vector>
#include <algorithm> // For std::clamp, std::min, std::max
#include <cmath>
#include <limits>    // For std::numeric_limits
#include <string>

// --- STB_IMAGE_WRITE integration for PNG export (Vendor) ---
#define STB_IMAGE_WRITE_IMPLEMENTATION
// Ensure stb_image_write.h is in vendor/
#include "../vendor/stb_image_write.h"
// -----------------------------------------------------------

// Vendor headers
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

// Project headers (Noise library)
#include "../noises/include/noise_base.h"
#include "../noises/include/perlin.h"
#include "../noises/include/simplex.h"
#include "../noises/include/value.h"
#include "../noises/include/fbm.h"
#include "../noises/include/worley.h"

// =================================================================
// Global State Variables
// =================================================================

// Noise parameters
static std::unique_ptr<NoiseBase> g_noiseGenerator;
static int g_noiseType = 1; // Simplex default
static float g_scale = 5.0f;
static int g_octaves = 6;
static float g_persistence = 0.5f;
static float g_lacunarity = 2.0f;

// --- CRITICAL PARAMETER: SEED ---
static int g_seed = 1337;

// Worley Noise specific parameters
static int g_worleyFunction = 2; // F2-F1 (for boundaries)
static int g_worleyMetric = 0;   // Euclidean

// Rendering and Texture parameters
static GLuint g_noiseTexture = 0;
static const int TEXTURE_SIZE = 512;
static bool g_textureNeedsUpdate = true;

// Colorization settings (float[3] for ImGui::ColorEdit3)
static float g_color1[3] = {0.0f, 0.0f, 0.0f}; // Color for low values (0.0)
static float g_color2[3] = {1.0f, 1.0f, 1.0f}; // Color for high values (1.0)
static bool g_useColorization = false;

// Vector to store the last generated CPU data (used for export)
static std::vector<unsigned char> g_last_texture_data;
static const int TEXTURE_CHANNELS = 3; // We use RGB for colorization


// =================================================================
// Helper Functions
// =================================================================

// GLFW error logging callback
void glfw_error_callback(int error, const char* description) {
    std::cerr << "GLFW Error " << error << ": " << description << std::endl;
}

// Initializes or switches the current noise generator based on g_noiseType
void initNoiseGenerator() {
    g_textureNeedsUpdate = true;

    // Cast the user-input integer seed to unsigned int
    unsigned int current_seed = (unsigned int)g_seed;

    switch (g_noiseType) {
        // Pass the current seed to the base noise constructors
        case 0: g_noiseGenerator = std::make_unique<PerlinNoise>(current_seed); break;
        case 1: g_noiseGenerator = std::make_unique<SimplexNoise>(current_seed); break;
        case 2: g_noiseGenerator = std::make_unique<ValueNoise>(current_seed); break;

        case 3: // FBM (Fractal Brownian Motion)
            g_noiseGenerator = std::make_unique<FBMNoise>(
                std::make_unique<SimplexNoise>(current_seed), // Base noise also uses the seed
                g_octaves,
                g_lacunarity,
                g_persistence
            );
            break;

        case 4: // Worley (Cellular Noise)
            g_noiseGenerator = std::make_unique<WorleyNoise>(
                current_seed,
                (DistanceMetric)g_worleyMetric,
                (WorleyFunction)g_worleyFunction,
                1 // points_per_cell (fixed for simplicity in UI)
            );
            break;

        default: g_noiseGenerator = std::make_unique<SimplexNoise>(current_seed); break;
    }
}


// Generates noise data on the CPU, normalizes/colorizes it, and uploads to an OpenGL texture
void generateAndUploadNoiseTexture(int width, int height) {
    if (!g_noiseGenerator || !g_textureNeedsUpdate) return;

    std::vector<real_t> raw_noise(width * height);
    g_last_texture_data.resize(width * height * TEXTURE_CHANNELS);

    real_t min_val = std::numeric_limits<real_t>::max();
    real_t max_val = std::numeric_limits<real_t>::min();

    // 1. Generate raw noise data and find the actual min/max values
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            // Map pixel coordinates to the noise space using g_scale (frequency)
            real_t nx = (real_t)x / (real_t)width * g_scale;
            real_t ny = (real_t)y / (real_t)height * g_scale;

            real_t noise_val = g_noiseGenerator->GetNoise(nx, ny);
            raw_noise[y * width + x] = noise_val;

            min_val = std::min(min_val, noise_val);
            max_val = std::max(max_val, noise_val);
        }
    }

    // 2. Normalize, Colorize, and convert to 8-bit RGB
    real_t range = max_val - min_val;
    if (range < 0.0001) range = 1.0; // Avoid division by zero

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            real_t noise_val = raw_noise[y * width + x];
            real_t normalized;

            // Normalization strategy differs for Worley (Voronoi) vs Gradient/Value Noise
            if (g_noiseType == 4) { // Worley Noise (F1, F2, F2-F1 are usually [0, X])
                // Normalize using calculated min/max range
                normalized = (noise_val - min_val) / range;
            } else { // Perlin/Simplex/Value/FBM (usually normalized to roughly [-1, 1])
                // Map the theoretical [-1, 1] range to [0.0, 1.0]
                normalized = noise_val * 0.5 + 0.5;
            }

            // Clamp final normalized value to ensure it's strictly [0.0, 1.0]
            normalized = std::clamp(normalized, 0.0, 1.0);

            size_t idx = (y * width + x) * TEXTURE_CHANNELS;

            if (g_useColorization) {
                // Interpolate between Color Low (g_color1) and Color High (g_color2)
                for (int c = 0; c < 3; ++c) {
                    float color_component = g_color1[c] + (g_color2[c] - g_color1[c]) * normalized;
                    g_last_texture_data[idx + c] = (unsigned char)(std::clamp(color_component, 0.0f, 1.0f) * 255.0f);
                }
            } else {
                // Standard Grayscale
                unsigned char gray = (unsigned char)(normalized * 255.0);
                g_last_texture_data[idx + 0] = gray;
                g_last_texture_data[idx + 1] = gray;
                g_last_texture_data[idx + 2] = gray;
            }
        }
    }

    // 3. Upload to OpenGL Texture
    if (g_noiseTexture == 0) {
        glGenTextures(1, &g_noiseTexture);
    }
    glBindTexture(GL_TEXTURE_2D, g_noiseTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, g_last_texture_data.data());

    g_textureNeedsUpdate = false;
}

// Exports the current texture data to a PNG file using stb_image_write
void exportImage(int width, int height) {
    if (g_last_texture_data.empty()) {
        std::cerr << "Error: No noise data generated yet to export." << std::endl;
        return;
    }

    std::string filename = "noise_export_" + std::to_string(width) + "x" + std::to_string(height) + ".png";
    int stride_in_bytes = width * TEXTURE_CHANNELS;

    int result = stbi_write_png(
        filename.c_str(),
        width,
        height,
        TEXTURE_CHANNELS,
        g_last_texture_data.data(),
        stride_in_bytes
    );

    if (result) {
        std::cout << "Image exported successfully: " << filename << std::endl;
    } else {
        std::cerr << "Failed to export image: " << filename << std::endl;
    }
}


// Renders the ImGui user interface controls
void renderUI() {
    ImGui::Begin("Noise Settings");

    // --- SEED INPUT ---
    ImGui::Text("Generation:");
    int prev_seed = g_seed;
    if (ImGui::InputInt("Seed", &g_seed)) {
        // Only regenerate the noise object if the seed value actually changed
        if (prev_seed != g_seed) initNoiseGenerator();
    }
    ImGui::Separator();

    // 1. Noise Type Selection
    const char* noise_types[] = { "Perlin", "Simplex", "Value", "FBM", "Worley" };
    if (ImGui::Combo("Noise Type", &g_noiseType, noise_types, IM_ARRAYSIZE(noise_types))) {
        initNoiseGenerator();
    }

    // 2. Common Parameters
    if (ImGui::SliderFloat("Scale (Frequency)", &g_scale, 0.1f, 20.0f)) g_textureNeedsUpdate = true;

    // 3. FBM (Fractal) Parameters - Only show if FBM is selected
    if (g_noiseType == 3) {
        ImGui::Separator();
        ImGui::Text("FBM Parameters:");
        // These parameters modify the FBM object structure, so we must call initNoiseGenerator
        if (ImGui::SliderInt("Octaves", &g_octaves, 1, 12)) initNoiseGenerator();
        if (ImGui::SliderFloat("Persistence (Amplitude)", &g_persistence, 0.1f, 0.9f)) initNoiseGenerator();
        if (ImGui::SliderFloat("Lacunarity (Frequency)", &g_lacunarity, 1.5f, 4.0f)) initNoiseGenerator();
        ImGui::Separator();
    }

    // 4. Worley (Cellular) Parameters - Only show if Worley is selected
    if (g_noiseType == 4) {
        ImGui::Separator();
        ImGui::Text("Worley Parameters:");
        const char* worley_funcs[] = { "F1 (Closest)", "F2 (Second Closest)", "F2 - F1 (Boundary)" };
        if (ImGui::Combo("Function", &g_worleyFunction, worley_funcs, IM_ARRAYSIZE(worley_funcs))) initNoiseGenerator();

        const char* worley_metrics[] = { "Euclidean", "Manhattan", "Chebyshev" };
        if (ImGui::Combo("Distance Metric", &g_worleyMetric, worley_metrics, IM_ARRAYSIZE(worley_metrics))) initNoiseGenerator();
        ImGui::Separator();
    }

    // 5. Colorization Settings
    ImGui::Text("Colorization:");
    if (ImGui::Checkbox("Use Color Map", &g_useColorization)) g_textureNeedsUpdate = true;

    if (g_useColorization) {
        // ImGui::ColorEdit3 for low and high color selection
        if (ImGui::ColorEdit3("Color Low (0.0)", g_color1)) g_textureNeedsUpdate = true;
        if (ImGui::ColorEdit3("Color High (1.0)", g_color2)) g_textureNeedsUpdate = true;
    } else {
        ImGui::TextDisabled("Color map disabled. Showing Grayscale.");
    }

    ImGui::Separator();

    // 6. Action Buttons
    if (ImGui::Button("Regenerate Noise")) g_textureNeedsUpdate = true;

    ImGui::SameLine();
    if (ImGui::Button("Export PNG")) {
        exportImage(TEXTURE_SIZE, TEXTURE_SIZE);
    }

    // Display the generated noise texture
    ImGui::Text("Generated Texture (%dx%d):", TEXTURE_SIZE, TEXTURE_SIZE);
    // Cast the OpenGL texture ID to ImGui's expected texture handle type
    ImGui::Image((void*)(intptr_t)g_noiseTexture, ImVec2((float)TEXTURE_SIZE, (float)TEXTURE_SIZE));

    ImGui::End();
}

// -------------------------------------------------------------
// Main Rendering Loop and Initialization
// -------------------------------------------------------------
int main_render_loop() {
    // GLFW initialization
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) return 1;

    // Set OpenGL version (3.3 Core Profile is required for modern GL)
    const char* glsl_version = "#version 330";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create the window
    GLFWwindow* window = glfwCreateWindow(800, 600, "libNoise Demo", NULL, NULL);
    if (window == NULL) {
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable V-Sync

    // Initialize GLAD (loads OpenGL function pointers)
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return 1;
    }

    // ImGui Initialization
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Initial setup of the noise generator
    initNoiseGenerator();

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // 1. Start the ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // 2. Generate/Update the noise texture and render the UI controls
        generateAndUploadNoiseTexture(TEXTURE_SIZE, TEXTURE_SIZE);
        renderUI();

        // 3. Render ImGui drawing data
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);

        // Clear the background
        glClearColor(0.2f, 0.2f, 0.2f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
