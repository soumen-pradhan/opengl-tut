#include <cstdint>

#include <glad/glad.h> // glad has to be above glfw3 header

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <spdlog/spdlog.h>

#include "defer.h"
#include "shader.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

void framebufferSizeCallback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);

int main()
{
    spdlog::set_level(spdlog::level::trace);
    spdlog::set_pattern("%Y-%m-%d %H:%M:%S.%e %^%-8l%$ | %s:%5# | %v");

    if (!glfwInit()) {
        SPDLOG_ERROR("Failed to init GLFW");
        return 1;
    }
    DEFER(glfwTerminate());

    const char* glslVersion = "#version 300 es";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    const int32_t SCR_WIDTH = 800;
    const int32_t SCR_HEIGHT = 600;
    float mainScale = ImGui_ImplGlfw_GetContentScaleForMonitor(glfwGetPrimaryMonitor());

    GLFWwindow* window = glfwCreateWindow(
        (int)(SCR_WIDTH * mainScale), (int)(SCR_HEIGHT * mainScale),
        "OpenGL Hello World", nullptr, nullptr);

    if (!window) {
        SPDLOG_ERROR("Failed to create window");
        return 1;
    }
    SPDLOG_DEBUG("glfwCreateWindow");
    DEFER(glfwDestroyWindow(window));

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // vsync
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        SPDLOG_ERROR("Failed to init GLAD");
        return 1;
    }

    SPDLOG_INFO("Loaded OpenGL {}", reinterpret_cast<const char*>(glGetString(GL_VERSION)));

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    {
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();
        // ImGui::StyleColorsLight();

        // Setup scaling
        ImGuiStyle& style = ImGui::GetStyle();
        style.ScaleAllSizes(mainScale); // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
        style.FontScaleDpi = mainScale; // Set initial font scale. (using io.ConfigDpiScaleFonts=true makes this unnecessary. We leave both here for documentation purpose)

        // Setup Platform/Renderer backends
        ImGui_ImplGlfw_InitForOpenGL(window, true);

        ImGui_ImplOpenGL3_Init(glslVersion);
    }
    SPDLOG_DEBUG("Created ImGui context");
    DEFER({
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    });

    uint32_t vertexBufferObj = 0;
    uint32_t vertexArrayObj = 0;
    uint32_t elementBufferObj = 0;
    {
        // clang-format off
        float vertices[] = {
            // positions          // colors            // tex
             0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
             0.5f, -0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
            -0.5f, -0.5f,  0.0f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
            -0.5f,  0.5f,  0.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f,
        };

        uint32_t indices[] = {
            0, 1, 3,
            1, 2, 3
        };
        // clang-format on

        glGenBuffers(1, &vertexBufferObj);
        glGenVertexArrays(1, &vertexArrayObj);
        glGenBuffers(1, &elementBufferObj);

        glBindVertexArray(vertexArrayObj);

        glBindBuffer(GL_ARRAY_BUFFER, vertexBufferObj);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBufferObj);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);
    }
    SPDLOG_DEBUG("Created Vertice Buffer and Layout Objects");
    DEFER({
        glDeleteBuffers(1, &vertexBufferObj);
        glDeleteVertexArrays(1, &vertexArrayObj);
    });

    Shader shader("shaders/shader.vert", "shaders/shader.frag");
    SPDLOG_DEBUG("Created Shader Program");
    DEFER(glDeleteProgram(shader.ID));

    stbi_set_flip_vertically_on_load(true);

    uint32_t texture = 0;
    {
        int width, height, nrChannels;
        const uint8_t* data = stbi_load("textures/Texturelabs_Brick_159M.jpg",
            &width, &height, &nrChannels, 0);
        DEFER(stbi_image_free((void*)data));

        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0,
            GL_RGB, GL_UNSIGNED_BYTE, (void*)data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    SPDLOG_DEBUG("Created Texture");
    DEFER(glDeleteTextures(1, &texture));

    uint32_t texture1 = 0;
    {
        int width, height, nrChannels;
        const uint8_t* data = stbi_load("textures/awesomeface.png",
            &width, &height, &nrChannels, 0);
        DEFER(stbi_image_free((void*)data));

        glGenTextures(1, &texture1);
        glBindTexture(GL_TEXTURE_2D, texture1);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0,
            GL_RGBA, GL_UNSIGNED_BYTE, (void*)data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    SPDLOG_DEBUG("Created Texture1");
    DEFER(glDeleteTextures(1, &texture1));

    bool showDemoWindow = true;
    bool showAnotherWindow = false;
    ImVec4 clearColor = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if (showDemoWindow) {
            ImGui::ShowDemoWindow(&showDemoWindow);
        }

        {

            static float f = 0.0f;
            static int counter = 0;

            ImGui::Begin("Hello, world!"); // Create a window called "Hello, world!" and append into it.

            ImGui::Text("This is some useful text."); // Display some text (you can use a format strings too)
            ImGui::Checkbox("Demo Window", &showDemoWindow); // Edit bools storing our window open/close state
            ImGui::Checkbox("Another Window", &showDemoWindow);

            ImGui::SliderFloat("float", &f, 0.0f, 1.0f); // Edit 1 float using a slider from 0.0f to 1.0f
            ImGui::ColorEdit3("clear color", (float*)&clearColor); // Edit 3 floats representing a color

            // Buttons return true when clicked (most widgets return true when edited/activated)
            if (ImGui::Button("Button")) {
                counter++;
            }
            ImGui::SameLine();
            ImGui::Text("counter = %d", counter);

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
            ImGui::End();
        }

        if (showAnotherWindow) {

            ImGui::Begin("Another Window", &showAnotherWindow); // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
            ImGui::Text("Hello from another window!");
            if (ImGui::Button("Close Me")) {
                showAnotherWindow = false;
            }
            ImGui::End();
        }

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clearColor.x * clearColor.w, clearColor.y * clearColor.w, clearColor.z * clearColor.w, clearColor.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);

        // processInput(window);

        // glClearColor(0.1f, 0.1f, 0.2f, 1.0f);
        // glClear(GL_COLOR_BUFFER_BIT);

        // // render
        // {
        //     shader.useProgram();

        //     glActiveTexture(GL_TEXTURE0);
        //     glBindTexture(GL_TEXTURE_2D, texture);
        //     shader.setUniformInt("texture0", 0);

        //     glActiveTexture(GL_TEXTURE1);
        //     glBindTexture(GL_TEXTURE_2D, texture1);
        //     shader.setUniformInt("texture1", 1);

        //     glBindVertexArray(vertexArrayObj);
        //     glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)0);
        // }

        // glfwSwapBuffers(window);
    }

    return 0;
}

void framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        SPDLOG_DEBUG("Pressed Esc Key");
        glfwSetWindowShouldClose(window, true);
    }
}
