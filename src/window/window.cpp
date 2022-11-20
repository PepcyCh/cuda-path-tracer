#include "window.hpp"

#include <iostream>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

namespace {

void GlfwErrorLogFunc(int error, const char *desc) {
    std::cerr << "glfw error: " << desc << std::endl;
}

void GLAPIENTRY GlMessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
    const GLchar *message, const void *user_param) {
    if (severity != GL_DEBUG_SEVERITY_HIGH) {
        return;
    }
    auto type_str = type == GL_DEBUG_TYPE_ERROR ? "[ERROR]"
        : type == GL_DEBUG_TYPE_PERFORMANCE ? "[PERFORMANCE]"
        : type == GL_DEBUG_TYPE_MARKER ? "[MARKER]"
        : type == GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR ? "[DEPRECATED]"
        : type == GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR ? "[UNDEFINED BEHAVIOR]" : "[OTHER]";
    auto severity_str = severity == GL_DEBUG_SEVERITY_HIGH ? "[HIGH]"
        : severity == GL_DEBUG_SEVERITY_MEDIUM ? "[MEDIUM]"
        : severity == GL_DEBUG_SEVERITY_LOW ? "[LOW]" : "[NOTIFICATION]";
    std::cerr << "OpenGL callback " << type_str << " " << severity_str << " " << message << std::endl;
    if (type == GL_DEBUG_TYPE_ERROR && severity == GL_DEBUG_SEVERITY_HIGH) {
        exit(-1);
    }
}

const char *kDisplayVertex = R"(
#version 460

layout(location = 0) out vec2 texcoord;

void main() {
    texcoord = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
    gl_Position = vec4(texcoord.x * 2.0 - 1.0, 1.0 - texcoord.y * 2.0, 0.0, 1.0);
}
)";

const char *kDisplayFragment = R"(
#version 460

layout(location = 0) in vec2 texcoord;

layout(location = 0) out vec4 frag_color;

layout(binding = 0) uniform sampler2D color_image;

float linear_to_srgb(float v) {
    return v < 0.0031308 ? v * 12.92 : pow(v, 1.0 / 2.4) * 1.055 - 0.055;
}
vec3 linear_to_srgb(vec3 v) {
    return vec3(linear_to_srgb(v.x), linear_to_srgb(v.y), linear_to_srgb(v.z));
}

void main() {
    vec3 color = texture(color_image, texcoord).xyz;
    frag_color = vec4(linear_to_srgb(color), 1.0);
}
)";

}

void GlfwCursorPosCallback(GLFWwindow *glfw_window, double x, double y) {
    auto window = reinterpret_cast<Window *>(glfwGetWindowUserPointer(glfw_window));
    uint32_t state = 0;
    if (glfwGetMouseButton(glfw_window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        state |= Window::eMouseLeft;
    }
    if (glfwGetMouseButton(glfw_window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
        state |= Window::eMouseRight;
    }
    if (glfwGetMouseButton(glfw_window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS) {
        state |= Window::eMouseMiddle;
    }
    window->mouse_callback_(state, x, y, window->last_mouse_x_, window->last_mouse_y_);
    window->last_mouse_x_ = x;
    window->last_mouse_y_ = y;
}

void GlfwKeyCallback(GLFWwindow *glfw_window, int key, int scancode, int action, int mods) {
    auto window = reinterpret_cast<Window *>(glfwGetWindowUserPointer(glfw_window));
    window->key_callback_(key, action);
}

void GlfwResizeCallback(GLFWwindow *glfw_window, int width, int height) {
    auto window = reinterpret_cast<Window *>(glfwGetWindowUserPointer(glfw_window));
    if (window->width_ != width || window->height_ != height) {
        window->resize_callback_(width, height);
    }
}

Window::Window(uint32_t width, uint32_t height, const char *title) : width_(width), height_(width) {
    glfwSetErrorCallback(GlfwErrorLogFunc);
    glfwInit();

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    window_ = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (window_ == nullptr) {
        std::cerr << "Failed to create glfw window\n";
        return;
    }
    glfwMakeContextCurrent(window_);

    glfwSetWindowUserPointer(window_, this);
    glfwSetCursorPosCallback(window_, GlfwCursorPosCallback);
    glfwSetKeyCallback(window_, GlfwKeyCallback);
    glfwSetFramebufferSizeCallback(window_, GlfwResizeCallback);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        std::cerr << "Failed to load OpenGL functions\n";
    }

    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(GlMessageCallback, nullptr);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;

    ImGui::StyleColorsClassic();

    ImGui_ImplGlfw_InitForOpenGL(window_, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    std::cout << "GL_VERSION: " << glGetString(GL_VERSION) <<  std::endl;
    std::cout << "GL_VENDOR: " << glGetString(GL_VENDOR) <<  std::endl;

    InitDisplayProgram();
}

Window::~Window() {
    glDeleteProgram(display_program_);
    glDeleteVertexArrays(1, &empty_vao_);

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window_);
    glfwTerminate();
}

void Window::MainLoop(const std::function<void()> &func) {
    while (!glfwWindowShouldClose(window_)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        func();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window_);
    }
}

void Window::SetMouseCallback(const MouseCallback &callback) {
    mouse_callback_ = callback;
}

void Window::SetKeyCallback(const KeyCallback &callback) {
    key_callback_ = callback;
}

void Window::SetResizeCallback(const ResizeCallback &callback) {
    resize_callback_ = callback;
}

void Window::SetTitle(const char *title) {
    glfwSetWindowTitle(window_, title);
}

void Window::Display(uint32_t gl_texture) {
    glUseProgram(display_program_);
    glBindVertexArray(empty_vao_);
    glBindTextureUnit(0, gl_texture);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
    glUseProgram(0);
}

void Window::InitDisplayProgram() {
    glCreateVertexArrays(1, &empty_vao_);

    auto display_vert = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(display_vert, 1, &kDisplayVertex, nullptr);
    glCompileShader(display_vert);

    auto display_frag = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(display_frag, 1, &kDisplayFragment, nullptr);
    glCompileShader(display_frag);

    display_program_ = glCreateProgram();
    glAttachShader(display_program_, display_vert);
    glAttachShader(display_program_, display_frag);
    glLinkProgram(display_program_);

    glDeleteShader(display_vert);
    glDeleteShader(display_frag);
}
