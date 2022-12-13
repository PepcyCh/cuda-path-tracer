#include <iostream>

#include "scene/loader.hpp"
#include "scene/camera.hpp"
#include "window/window.hpp"
#include "pathtracer/pathtracer.hpp"

int main(int argc, char **argv) {
    struct {
        bool ui = true;
        int max_depth = -1;
        int max_spp = -1;
        const char *capture_name = "capture";
        const char *bsdf_type = "blinn-phong";
        int bsdf_type_i = 2;
    } cmd_args;

    if (argc < 3) {
        std::cout << "usage: " << argv[0] << " <path-to-obj-file> <path-to-extra-file> [OPTIONS]\n";
        std::cout << "\nOPTIONS\n";
        std::cout << "  --ui            0 or 1, whether show UI (default 1)\n";
        std::cout << "  --max-depth     max ray tracing depth (default -1, no limit)\n";
        std::cout << "  --max-spp       max ray tracing spp when ui is 0 (default -1, no limit)\n";
        std::cout << "  --output | -o   output .exr name (default 'capture')\n";
        std::cout << "  --bsdf-type     which BSDF to use (default 'blinn-phong')\n";
        return -1;
    }
    for (int i = 3; i < argc; i++) {
        if (strcmp(argv[i], "--ui") == 0) {
            cmd_args.ui = std::atoi(argv[++i]);
        } else if (strcmp(argv[i], "--max-depth") == 0) {
            cmd_args.max_depth = std::atoi(argv[++i]);
        } else if (strcmp(argv[i], "--max-spp") == 0) {
            cmd_args.max_spp = std::atoi(argv[++i]);
        } else if (strcmp(argv[i], "--output") == 0 || strcmp(argv[i], "-o") == 0) {
            cmd_args.capture_name = argv[++i];
        } else if (strcmp(argv[i], "--bsdf-type") == 0) {
            cmd_args.bsdf_type = argv[++i];
        }
    }
    const char *bsdf_type_names[] = {
        "lambert", "phong", "blinn-phong", "microfacet"
    };
    for (int i = 0; i < 4; i++) {
        if (strcmp(cmd_args.bsdf_type, bsdf_type_names[i]) == 0) {
            cmd_args.bsdf_type_i = i;
        }
    }

    std::filesystem::path obj_path(argv[1]);
    if (!std::filesystem::exists(obj_path)) {
        std::cout << "obj file '" << obj_path << "' doesn't exist" << std::endl;
        return -1;
    }

    std::filesystem::path extra_path(argv[2]);
    if (!std::filesystem::exists(extra_path)) {
        std::cout << "extra file '" << extra_path << "' doesn't exist" << std::endl;
        return -1;
    }

    Scene scene {};
    if (!LoadObjScene(scene, obj_path, cmd_args.bsdf_type_i)) {
        std::cout << "failed to load obj file '" << obj_path << "'" << std::endl;
        return -1;
    }
    if (extra_path.extension() == ".json") {
        if (!LoadJsonScene(scene, extra_path)) {
            std::cout << "failed to load json file '" << extra_path << "'" << std::endl;
            return -1;
        }
    } else if (extra_path.extension() == ".xml") {
        if (!LoadXmlScene(scene, extra_path)) {
            std::cout << "failed to load xml file '" << extra_path << "'" << std::endl;
            return -1;
        }
    } else {
        std::cout << "unknown extra file extension '" << extra_path.extension().string() << "'" << std::endl;
        return -1;
    }

    uint32_t window_width = 1280;
    uint32_t window_height = 720;
    Window window(window_width, window_height, "CUDA pathtracer", !cmd_args.ui);

    auto camera_object = scene.FirstObjectWith<CameraComponent>();
    uint32_t film_width = window_width;
    uint32_t film_height = window_height;
    if (!cmd_args.ui) {
        auto camera_comp = camera_object->GetComponent<CameraComponent>();
        film_width = camera_comp->film_width;
        film_height = camera_comp->film_height;
    }
    Film film(film_width, film_height);

    auto path_tracer = camera_object->AddComponent<PathTracer>(scene, film);
    path_tracer->SetMaxDepth(cmd_args.max_depth);
    path_tracer->SetCaptureName(cmd_args.capture_name);
    path_tracer->BuildBuffers();

    if (cmd_args.ui) {
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
    } else {
        uint32_t max_spp = std::min(static_cast<uint32_t>(cmd_args.max_spp), 65536u);
        for (uint32_t i = 0; i < max_spp; i++) {
            scene.Update();
            if ((i + 1) % 16 == 0) {
                std::cout << "spp " << i + 1 << std::endl;
            }
        }
        auto exr_name = std::string(cmd_args.capture_name) + ".exr";
        film.SaveTo(exr_name.c_str());
    }

    return 0;
}
