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

#include "camera.h"
#include "color.h"
#include "defer.h"
#include "shader.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

void framebufferSizeCallback(GLFWwindow* window, int width, int height);
void cursorCallback(GLFWwindow* window, double xpos, double ypos);
void scrollCallback(GLFWwindow* window, double xpos, double ypos);
void processInput(GLFWwindow* window);

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
    Shader lightingShader;
    Shader lightingCubeShader;
    glm::vec3 lightSourcePos;

    uint32_t cubeVertexArrayObj;
    uint32_t lightSourceCubeVertexArrayObj;

    float deltaSec, prevSec;
    glm::vec4 clearColor;

    bool showDemoWindow;
    bool cursorReleased;
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
    glfwSetScrollCallback(window, scrollCallback);

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

    std::optional<Shader> lightingShader = Shader::init("shaders/1-colors.vs", "shaders/1-colors.fs");
    if (!lightingShader) {
        return 1;
    }
    std::optional<Shader> lightingCubeShader = Shader::init("shaders/1-light-cube.vs", "shaders/1-light-cube.fs");
    if (!lightingCubeShader) {
        return 1;
    }

    DEFER({
        glDeleteProgram(lightingShader->ID);
        glDeleteProgram(lightingCubeShader->ID);
    });

    uint32_t vertexBufferObj = 0;
    uint32_t cubeVertexArrayObj = 0;
    uint32_t lightSourceCubeVertexArrayObj = 0;
    {
        // clang-format off
        float vertices[] = {
            // positions          // normals           // tex
            -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
             0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
             0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
             0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
            -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
            -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,

            -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,
             0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 0.0f,
             0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
             0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
            -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f,
            -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,

            -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
            -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
            -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
            -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
            -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
            -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

             0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
             0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
             0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
             0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
             0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
             0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

            -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
             0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
             0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
             0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
            -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
            -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,

            -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
             0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
             0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
             0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
            -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
            -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f
        };
        // clang-format on

        glGenBuffers(1, &vertexBufferObj);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBufferObj);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glGenVertexArrays(1, &cubeVertexArrayObj);
        glBindVertexArray(cubeVertexArrayObj);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBufferObj);
        // position
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        // normal
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glGenVertexArrays(1, &lightSourceCubeVertexArrayObj);
        glBindVertexArray(lightSourceCubeVertexArrayObj);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBufferObj);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
    }
    SPDLOG_DEBUG("Created Vertice Buffer and Layout Objects");
    DEFER({
        glDeleteBuffers(1, &vertexBufferObj);
        glDeleteVertexArrays(1, &cubeVertexArrayObj);
    });

    AppCtx ctx = {
        .camera = {
            .pos = glm::vec3(1.3f, 2.3f, 4.0f),
            .front = glm::vec3(-0.14f, -0.46f, -0.9f),
            .up = glm::vec3(0.0f, 1.0f, 0.0f),
            .speed = 3.0f,
            .fov = 45.0f,
            .lastX = SCR_WIDTH / 2,
            .lastY = SCR_HEIGHT / 2,
            .mouseSensitivity = 0.01f,
            .yaw = -90.0f, // 0 means +ve X axis, and as yaw increases, the turn is anticlockwise
            .pitch = 0.0f },
        .lightingShader = *lightingShader,
        .lightingCubeShader = *lightingCubeShader,
        .lightSourcePos = glm::vec3(1.2f, 1.0f, 2.0f),

        .cubeVertexArrayObj = cubeVertexArrayObj,
        .lightSourceCubeVertexArrayObj = lightSourceCubeVertexArrayObj,

        .deltaSec = 0,
        .prevSec = 0,
        .clearColor = Color::SLATE_950,
        .showDemoWindow = true,
        .cursorReleased = false,
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

    processInput(window);

    // Start the Dear ImGui frame
    ImGuiIO& io = ImGui::GetIO();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGuiDockNodeFlags flags = ImGuiDockNodeFlags_PassthruCentralNode;
    ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), flags);

    if (ctx.showDemoWindow) {
        ImGui::ShowDemoWindow(&ctx.showDemoWindow);
    }

    {
        ImGui::Begin("App Ctx");

        ImGui::Checkbox("Demo Window", &ctx.showDemoWindow);

        ImGui::SliderFloat("Mouse Sensitivity", &ctx.camera.mouseSensitivity, 0.01f, 0.1f);
        ImGui::Text("Mouse Cursor. x: %.2f, y: %.2f", ctx.camera.lastX, ctx.camera.lastY);
        ImGui::Text("Avg %.3f ms/frame | %.1f FPS", 1000.0f / io.Framerate, io.Framerate);

        ImGui::End();
    }

    {
        ImGui::Begin("Camera");

        ImGui::SliderFloat("fov", &ctx.camera.fov, 0.1f, 150.0f);
        ImGui::SliderFloat("speed", &ctx.camera.speed, 0.1f, 150.0f);
        ImGui::Text("pos   (%0.2f, %0.2f, %0.2f)", ctx.camera.pos.x, ctx.camera.pos.y, ctx.camera.pos.z);
        ImGui::Text("front (%0.2f, %0.2f, %0.2f)", ctx.camera.front.x, ctx.camera.front.y, ctx.camera.front.z);

        ImGui::End();
    }

    ImGui::Render();

    glClearColor(ctx.clearColor.x * ctx.clearColor.w, ctx.clearColor.y * ctx.clearColor.w, ctx.clearColor.z * ctx.clearColor.w, ctx.clearColor.w);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    ctx.lightingShader.useProgram();
    ctx.lightingShader.setUniformVec3f("objectColor", Color::ORANGE_400);
    ctx.lightingShader.setUniformVec3f("lightColor", Color::WHITE);
    ctx.lightingShader.setUniformVec3f("lightPos", ctx.lightSourcePos);
    ctx.lightingShader.setUniformVec3f("viewPos", ctx.camera.pos);

    int fbWidth, fbHeight;
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);

    float aspect = (float)fbWidth / (float)fbHeight;

    glm::mat4 projection = glm::perspective(
        glm::radians(ctx.camera.fov), aspect,
        0.1f, 100.0f);
    ctx.lightingShader.setUniformMatrix4f("projection", projection);

    glm::mat4 view = glm::lookAt(ctx.camera.pos, ctx.camera.pos + ctx.camera.front, ctx.camera.up);
    ctx.lightingShader.setUniformMatrix4f("view", view);

    glm::mat4 model = glm::mat4(1.0f);
    ctx.lightingShader.setUniformMatrix4f("model", model);

    glBindVertexArray(ctx.cubeVertexArrayObj);
    glDrawArrays(GL_TRIANGLES, 0, 36);

    ctx.lightingCubeShader.useProgram();
    ctx.lightingCubeShader.setUniformMatrix4f("projection", projection);
    ctx.lightingCubeShader.setUniformMatrix4f("view", view);

    glm::mat4 lightModel = glm::mat4(1.0f);
    lightModel = glm::translate(lightModel, ctx.lightSourcePos);
    lightModel = glm::scale(lightModel, glm::vec3(0.2f));

    ctx.lightingCubeShader.setUniformMatrix4f("model", lightModel);

    glBindVertexArray(ctx.lightSourceCubeVertexArrayObj);
    glDrawArrays(GL_TRIANGLES, 0, 36);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwPollEvents();
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

    bool wantCursorReleased = glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS;
    if (wantCursorReleased) {
        return;
    }

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

void scrollCallback(GLFWwindow* window, double xoffsetd, double yoffsetd)
{
    AppCtx& ctx = *(AppCtx*)glfwGetWindowUserPointer(window);

    (void)xoffsetd;
    float yoffset = (float)yoffsetd;
    ctx.camera.fov = std::clamp(ctx.camera.fov - yoffset, 1.0f, 80.0f);
}

void processInput(GLFWwindow* window)
{
    AppCtx& ctx = *(AppCtx*)glfwGetWindowUserPointer(window);

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        SPDLOG_DEBUG("Pressed Esc Key");
        glfwSetWindowShouldClose(window, true);
    }

    bool wantCursorReleased = glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS;

    if (wantCursorReleased != ctx.cursorReleased) {
        glfwSetInputMode(
            window,
            GLFW_CURSOR,
            wantCursorReleased ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);

        if (wantCursorReleased) {
            ImGui::SetWindowFocus(nullptr);
        }

        ctx.cursorReleased = wantCursorReleased;
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
