#include "window.hpp"

#include <iostream>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

namespace {

void GlfwErrorLogFunc(int error, const char *desc) {
    std::cerr << "glfw error: " << desc << std::endl;
}

const char *kDisplayVertex = R"(
#version 330

out vec2 texcoord;

void main() {
    texcoord = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
    gl_Position = vec4(texcoord.x * 2.0 - 1.0, 1.0 - texcoord.y * 2.0, 0.0, 1.0);
}
)";

const char *kDisplayFragment = R"(
#version 330

in vec2 texcoord;

out vec4 frag_color;

uniform sampler2D color_image;

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

constexpr float kImguiFontSize = 14.0f;

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
    glViewport(0, 0, width, height);

    float width_scale, height_scale;
    glfwGetWindowContentScale(glfw_window, &width_scale, &height_scale);

    auto dpi_scale = std::max(width_scale, height_scale);
    auto rel_scale = dpi_scale / window->imgui_last_scale_;
    ImGui::GetStyle().ScaleAllSizes(rel_scale);
    window->imgui_last_scale_ = dpi_scale;
    if (auto it = window->imgui_fonts_.find(dpi_scale); it != window->imgui_fonts_.end()) {
        window->imgui_curr_font_ = it->second;
    } else {
        ImGuiIO &io = ImGui::GetIO();
        ImFontConfig font_cfg {};
        font_cfg.SizePixels = kImguiFontSize * dpi_scale;
        auto font = io.Fonts->AddFontDefault(&font_cfg);
        window->imgui_fonts_[dpi_scale] = font;
        window->imgui_curr_font_ = font;
        io.Fonts->Build();
        ImGui_ImplOpenGL3_DestroyDeviceObjects();
    }

    width /= width_scale;
    height /= height_scale;
    if (window->width_ != width || window->height_ != height) {
        window->resize_callback_(width, height);
    }
}

Window::Window(uint32_t width, uint32_t height, const char *title) : width_(width), height_(width) {
    glfwSetErrorCallback(GlfwErrorLogFunc);
    glfwInit();

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);
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

    int frame_width, frame_height;
    glfwGetFramebufferSize(window_, &frame_width, &frame_height);
    glViewport(0, 0, frame_width, frame_height);

    float width_scale, height_scale;
    glfwGetWindowContentScale(window_, &width_scale, &height_scale);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();

    auto dpi_scale = std::max(width_scale, height_scale);
    ImFontConfig font_cfg {};
    font_cfg.SizePixels = kImguiFontSize * dpi_scale;
    imgui_curr_font_ = io.Fonts->AddFontDefault(&font_cfg);
    imgui_fonts_[dpi_scale] = imgui_curr_font_;

    ImGui::StyleColorsClassic();
    imgui_last_scale_ = dpi_scale;
    ImGui::GetStyle().ScaleAllSizes(dpi_scale);

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

        ImGui::PushFont(imgui_curr_font_);
        func();
        ImGui::PopFont();

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
    glBindTexture(GL_TEXTURE_2D, gl_texture);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindVertexArray(0);
    glUseProgram(0);
}

void Window::InitDisplayProgram() {
    glGenVertexArrays(1, &empty_vao_);

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

    glUseProgram(display_program_);
    glUniform1i(glGetUniformLocation(display_program_, "color_image"), 0);
    glUseProgram(0);
}
