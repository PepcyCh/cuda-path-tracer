#include <iostream>

#include "scene/loader.hpp"
#include "window/window.hpp"
#include "pathtracer/pathtracer.hpp"

int main(int argc, char **argv) {
    if (argc != 3) {
        std::cout << "usage: " << argv[0] << " <path-to-obj-file> <path-to-extra-json-file>" << std::endl;
        return -1;
    }

    std::filesystem::path obj_path(argv[1]);
    if (!std::filesystem::exists(obj_path)) {
        std::cout << "obj file '" << obj_path << "' doesn't exist" << std::endl;
        return -1;
    }

    std::filesystem::path json_path(argv[2]);
    if (!std::filesystem::exists(json_path)) {
        std::cout << "json file '" << json_path << "' doesn't exist" << std::endl;
        return -1;
    }

    uint32_t window_width = 1280;
    uint32_t window_height = 720;
    Window window(window_width, window_height, "CUDA pathtracer");

    Scene scene {};
    if (!LoadObjScene(scene, obj_path)) {
        std::cout << "failed to load obj file '" << obj_path << "'" << std::endl;
        return -1;
    }
    if (!LoadJsonScene(scene, json_path)) {
        std::cout << "failed to load json file '" << json_path << "'" << std::endl;
        return -1;
    }

    Film film(window_width, window_height);

    auto path_tracer_object = scene.AddObject("path tracer");
    auto path_tracer = path_tracer_object->AddComponent<PathTracer>(scene, film);
    path_tracer->BuildBuffers();

    window.SetResizeCallback([&](uint32_t width, uint32_t height) {
        window_width = width;
        window_height = height;
        film.Resize(width, height);
    });

    window.MainLoop([&]() {
        scene.Update();

        window.Display(film.GlTexture());

        if (ImGui::Begin("Status")) {
            float fps = ImGui::GetIO().Framerate;
            ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / fps, fps);
        }
        ImGui::End();

        scene.ShowUi();
    });

    return 0;
}
