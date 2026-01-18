#include <cstdint>

#include <glad/glad.h> // glad has to be above glfw3 header

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <spdlog/spdlog.h>

#include "defer.h"
#include "shader.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

struct Camera {
    glm::vec3 pos, front, up;
    float speed, fov;
    float lastX, lastY;
    float mouseSensitivity;
    float yaw, pitch;
};

void framebufferSizeCallback(GLFWwindow* window, int width, int height);
void cursorCallback(GLFWwindow* window, double xpos, double ypos);
void processInput(GLFWwindow* window);
constexpr ImVec4 color(uint32_t hex);

template <glm::length_t L, typename T, glm::qualifier Q>
struct fmt::formatter<glm::vec<L, T, Q>> {
    constexpr auto parse(fmt::format_parse_context& ctx)
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const glm::vec<L, T, Q>& v, FormatContext& ctx) const
    {
        auto out = ctx.out();
        *out++ = '(';
        for (glm::length_t i = 0; i < L; ++i) {
            out = fmt::format_to(out, "{:.3f}", v[i]);
            if (i + 1 < L) {
                *out++ = ',';
                *out++ = ' ';
            }
        }
        *out++ = ')';
        return out;
    }
};

struct AppCtx {
    Camera camera;
    Shader shader;

    uint32_t texture[2];
    uint32_t vertexArrayObj;

    float deltaSec, prevSec;
    float mixFactor;
    ImVec4 clearColor;
    glm::vec3* cubePos;
    int cubePosLen;

    bool showDemoWindow;
    bool cursorCaptured;
    bool firstMouse;
};

void render(GLFWwindow* window, AppCtx& ctx);

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

    constexpr int SCR_WIDTH = 1200;
    constexpr float ASPECT_RATIO = 4.0f / 3.0f;
    constexpr int SCR_HEIGHT = static_cast<int>(SCR_WIDTH / ASPECT_RATIO);
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
    glfwSetCursorPosCallback(window, cursorCallback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

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
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable; // Enable Docking

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();
        // ImGui::StyleColorsLight();

        // Setup scaling
        ImGuiStyle& style = ImGui::GetStyle();
        style.ScaleAllSizes(mainScale);
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

    glEnable(GL_DEPTH_TEST);

    uint32_t vertexBufferObj = 0;
    uint32_t vertexArrayObj = 0;
    // uint32_t elementBufferObj = 0;
    {
        // clang-format off
        float vertices[] = {
            // positions          // tex
            -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
             0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
             0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
             0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
            -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
            -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

            -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
             0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
             0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
             0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
            -0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
            -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

            -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
            -0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
            -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
            -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
            -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
            -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

             0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
             0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
             0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
             0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
             0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
             0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

            -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
             0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
             0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
             0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
            -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
            -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,

            -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
             0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
             0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
             0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
            -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
            -0.5f,  0.5f, -0.5f,  0.0f, 1.0f
        };
        // clang-format on

        glGenBuffers(1, &vertexBufferObj);
        glGenVertexArrays(1, &vertexArrayObj);
        // glGenBuffers(1, &elementBufferObj);

        glBindVertexArray(vertexArrayObj);

        glBindBuffer(GL_ARRAY_BUFFER, vertexBufferObj);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBufferObj);
        // glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
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

    glm::vec3 cubePos[] = {
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(2.0f, 5.0f, -15.0f),
        glm::vec3(-1.5f, -2.2f, -2.5f),
        glm::vec3(-3.8f, -2.0f, -12.3f),
        glm::vec3(2.4f, -0.4f, -3.5f),
        glm::vec3(-1.7f, 3.0f, -7.5f),
        glm::vec3(1.3f, -2.0f, -2.5f),
        glm::vec3(1.5f, 2.0f, -2.5f),
        glm::vec3(1.5f, 0.2f, -1.5f),
        glm::vec3(-1.3f, 1.0f, -1.5f)
    };

    AppCtx ctx = {
        .camera = {
            .pos = glm::vec3(0.0f, 0.0f, 3.0f),
            .front = glm::vec3(0.0f, 0.0f, -1.0f),
            .up = glm::vec3(0.0f, 1.0f, 0.0f),
            .speed = 0.1f,
            .fov = 45.0f,
            .lastX = SCR_WIDTH / 2,
            .lastY = SCR_HEIGHT / 2,
            .mouseSensitivity = 0.01f,
            .yaw = -90.0f, // 0 means +ve X axis, and as yaw increases, the turn is anticlockwise
            .pitch = 0.0f },
        .shader = shader,

        .texture = { texture, texture1 },
        .vertexArrayObj = vertexArrayObj,

        .deltaSec = 0,
        .prevSec = 0,
        .mixFactor = 0.5f,
        .clearColor = color(0x01090d),
        .cubePos = cubePos,
        .cubePosLen = sizeof(cubePos) / sizeof(cubePos[0]),
        .showDemoWindow = true,
        .cursorCaptured = true,
        .firstMouse = true
    };
    glfwSetWindowUserPointer(window, &ctx);

    SPDLOG_INFO("Enetring Render Loop");
    while (!glfwWindowShouldClose(window)) {
        render(window, ctx);
    }

    return 0;
}

void render(GLFWwindow* window, AppCtx& ctx)
{
    float currentSec = (float)glfwGetTime();
    ctx.deltaSec = currentSec - ctx.prevSec;
    ctx.prevSec = currentSec;

    // Start the Dear ImGui frame
    ImGuiIO& io = ImGui::GetIO();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGuiDockNodeFlags flags = ImGuiDockNodeFlags_PassthruCentralNode;
    ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), flags);

    glfwPollEvents();
    processInput(window);

    int fbWidth, fbHeight;
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);

    float aspect = (float)fbWidth / (float)fbHeight;

    glm::mat4 view = glm::lookAt(ctx.camera.pos, ctx.camera.pos + ctx.camera.front, ctx.camera.up);

    glm::mat4 projection(1.0f);
    projection = glm::perspective(glm::radians(ctx.camera.fov), aspect, 0.1f, 100.0f);

    if (ctx.showDemoWindow) {
        ImGui::ShowDemoWindow(&ctx.showDemoWindow);
    }

    {
        ImGui::Begin("Uniforms");

        ImGui::Checkbox("Demo Window", &ctx.showDemoWindow);

        ImGui::SliderFloat("Mix Factor", &ctx.mixFactor, 0.0f, 1.0f);
        ImGui::SliderFloat("FOV", &ctx.camera.fov, 0.1f, 150.0f);
        ImGui::SliderFloat("Camera Speed", &ctx.camera.speed, 0.1f, 150.0f);
        ImGui::SliderFloat("Mouse Sensitivity", &ctx.camera.mouseSensitivity, 0.01f, 100.0f);

        ImGui::Text("Mouse Cursor. x: %.2f, y: %.2f", ctx.camera.lastX, ctx.camera.lastY);
        ImGui::Text("Camera Front. (%0.2f, %0.2f, %0.2f)", ctx.camera.front.x, ctx.camera.front.y, ctx.camera.front.z);
        ImGui::Text("Avg %.3f ms/frame | %.1f FPS", 1000.0f / io.Framerate, io.Framerate);
        ImGui::End();
    }

    ImGui::Render();

    glClearColor(ctx.clearColor.x * ctx.clearColor.w, ctx.clearColor.y * ctx.clearColor.w, ctx.clearColor.z * ctx.clearColor.w, ctx.clearColor.w);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    ctx.shader.useProgram();
    ctx.shader.setUniformInt("texture0", 0);
    ctx.shader.setUniformInt("texture1", 1);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, ctx.texture[0]);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, ctx.texture[1]);
    glBindVertexArray(ctx.vertexArrayObj);

    ctx.shader.setUniformFloat("mixFactor", ctx.mixFactor);
    ctx.shader.setUniformMatrix4f("view", view);
    ctx.shader.setUniformMatrix4f("projection", projection);

    for (int i = 0; i < ctx.cubePosLen; i++) {
        glm::mat4 model(1.0f);
        model = glm::scale(model, glm::vec3(0.3f));
        model = glm::translate(model, ctx.cubePos[i]);
        float angle = (i % 3 == 0 ? currentSec : 1.0f) * glm::radians(20.0f * i);
        model = glm::rotate(model, angle, glm::vec3(0.5f, 1.0f, 0.0f));

        ctx.shader.setUniformMatrix4f("model", model);
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window);
}

void framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    (void)window;
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
    SPDLOG_DEBUG("Window resized to {}x{}", width, height);

    // AppCtx* ctx = (AppCtx*)glfwGetWindowUserPointer(window);
    // render(window, *ctx);
}

void cursorCallback(GLFWwindow* window, double xposd, double yposd)
{
    AppCtx& ctx = *(AppCtx*)glfwGetWindowUserPointer(window);

    float xpos = (float)xposd;
    float ypos = (float)yposd;

    if (ctx.firstMouse) {
        SPDLOG_DEBUG("First Mouse event {}x{}", xpos, ypos);
        ctx.camera.lastX = xpos;
        ctx.camera.lastY = ypos;
        ctx.firstMouse = false;
    }

    float xoffset = (xpos - ctx.camera.lastX) * ctx.camera.mouseSensitivity;
    float yoffset = (ctx.camera.lastY - ypos) * ctx.camera.mouseSensitivity;

    ctx.camera.lastX = xpos;
    ctx.camera.lastY = ypos;

    ctx.camera.yaw += xoffset;
    ctx.camera.pitch = std::clamp(ctx.camera.pitch + yoffset, -89.0f, 89.0f);

    float cosYaw = std::cos(glm::radians(ctx.camera.yaw));
    float sinYaw = std::sin(glm::radians(ctx.camera.yaw));
    float cosPitch = std::cos(glm::radians(ctx.camera.pitch));
    float sinPitch = std::sin(glm::radians(ctx.camera.pitch));

    glm::vec3 dir(cosYaw * cosPitch, sinPitch, sinYaw * cosPitch);
    ctx.camera.front = glm::normalize(dir);
}

void processInput(GLFWwindow* window)
{
    AppCtx& ctx = *(AppCtx*)glfwGetWindowUserPointer(window);

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        SPDLOG_DEBUG("Pressed Esc Key");
        glfwSetWindowShouldClose(window, true);
    }

    float cameraSpeed = ctx.camera.speed * ctx.deltaSec;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        ctx.camera.pos += cameraSpeed * ctx.camera.front;
    }

    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        ctx.camera.pos -= cameraSpeed * ctx.camera.front;
    }

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        ctx.camera.pos -= glm::normalize(glm::cross(ctx.camera.front, ctx.camera.up)) * cameraSpeed;
    }

    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        ctx.camera.pos += glm::normalize(glm::cross(ctx.camera.front, ctx.camera.up)) * cameraSpeed;
    }
}

constexpr ImVec4 color(uint32_t hex)
{
    const float inv255 = 1.0f / 255.0f;

    return ImVec4(
        ((hex >> 16) & 0xFF) * inv255,
        ((hex >> 8) & 0xFF) * inv255,
        (hex & 0xFF) * inv255,
        1.0f);
}
