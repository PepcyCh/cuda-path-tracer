#include "loader.hpp"

#include <fstream>
#include <iostream>

#include <tiny_obj_loader.h>
#include <nlohmann/json.hpp>
#include <tinyxml2.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "mesh.hpp"
#include "material.hpp"
#include "camera.hpp"

namespace {

size_t HashCombine(size_t seed, size_t v) {
    seed ^= v + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    return seed;
}

void FetchVertexData(const tinyobj::attrib_t &in_attribs, const tinyobj::index_t &index, bool has_normal, bool has_uv,
    glm::vec3 &pos, glm::vec3 &norm, glm::vec2 &uv) {
    pos[0] = in_attribs.vertices[3 * index.vertex_index];
    pos[1] = in_attribs.vertices[3 * index.vertex_index + 1];
    pos[2] = in_attribs.vertices[3 * index.vertex_index + 2];
    if (has_normal) {
        norm[0] = in_attribs.normals[3 * index.normal_index];
        norm[1] = in_attribs.normals[3 * index.normal_index + 1];
        norm[2] = in_attribs.normals[3 * index.normal_index + 2];
    } else {
        norm = glm::vec3(0.0f);
    }
    if (has_uv) {
        uv[0] = in_attribs.texcoords[2 * index.texcoord_index];
        uv[1] = in_attribs.texcoords[2 * index.texcoord_index + 1];
    } else {
        uv = glm::vec2(0.0f);
    }
}

glm::vec3 Vec3From(const float *data) {
    return glm::vec3(data[0], data[1], data[2]);
}

void LoadTexture(std::unique_ptr<CuTexture> &texture, const std::filesystem::path &path) {
    int num_channels;
    int width;
    int height;
    auto image_data = stbi_load(path.string().c_str(), &width, &height, &num_channels, 0);
    auto data = image_data;
    bool need_delete = false;
    if (num_channels != 4) {
        need_delete = true;
        data = new uint8_t[width * height * 4];
        for (size_t i = 0; i < width * height; i++) {
            data[4 * i] = image_data[3 * i];
            data[4 * i + 1] = image_data[3 * i + 1];
            data[4 * i + 2] = image_data[3 * i + 2];
            data[4 * i + 3] = 255;
        }
    }
    texture = std::make_unique<CuTexture>(true, width, height, width * 4, data);
    if (need_delete) {
        delete[] data;
    }
    stbi_image_free(image_data);
}

void LoadTexture(std::unique_ptr<CuTexture> &texture, const std::filesystem::path &base_dir, const std::string &name) {
    if (name.empty()) {
        return;
    }
    auto path = base_dir / name;
    int num_channels;
    int width;
    int height;
    auto image_data = stbi_load(path.string().c_str(), &width, &height, &num_channels, 0);
    auto data = image_data;
    bool need_delete = false;
    if (num_channels != 4) {
        need_delete = true;
        data = new uint8_t[width * height * 4];
        for (size_t i = 0; i < width * height; i++) {
            data[4 * i] = image_data[3 * i];
            data[4 * i + 1] = image_data[3 * i + 1];
            data[4 * i + 2] = image_data[3 * i + 2];
            data[4 * i + 3] = 255;
        }
    }
    texture = std::make_unique<CuTexture>(true, width, height, width * 4, data);
    if (need_delete) {
        delete[] data;
    }
    stbi_image_free(image_data);
}

glm::vec3 Vec3FromCommaSplittedStr(const char *str) {
    auto length = strlen(str);
    glm::vec3 vec;
    size_t vec_i = 0, l = 0;
    for (size_t r = 0; r < length; r++) {
        if (str[r] == ',') {
            vec[vec_i] = std::atof(str + l);
            ++vec_i;
            if (vec_i == 3) {
                break;
            }
            l = r + 1;
        }
    }
    if (vec_i < 3) {
        vec[vec_i] = std::atof(str + l);
    }
    return vec;
}

}

bool operator==(const tinyobj::index_t &a, const tinyobj::index_t &b) noexcept {
    return a.vertex_index == b.vertex_index && a.normal_index == b.normal_index && a.texcoord_index == b.texcoord_index;
}
struct ObjIndex {
    tinyobj::index_t i;

    ObjIndex(const tinyobj::index_t &i) : i(i) {}

    bool operator==(const ObjIndex &rhs) const noexcept {
        return i == rhs.i;
    }
};
template <>
struct std::hash<ObjIndex> {
    size_t operator()(const ObjIndex &v) const noexcept {
        std::hash<int> hasher;
        size_t hash = hasher(v.i.vertex_index);
        hash = HashCombine(hash, hasher(v.i.normal_index));
        hash = HashCombine(hash, hasher(v.i.texcoord_index));
        return hash;
    }
};

bool LoadObjScene(Scene &scene, const std::filesystem::path &path, int bsdf_type) {
    tinyobj::attrib_t in_attribs;
    std::vector<tinyobj::shape_t> in_shapes;
    std::vector<tinyobj::material_t> in_materials;
    std::string load_warn;
    std::string load_err;
    auto base_dir = path.parent_path();
    bool result = tinyobj::LoadObj(&in_attribs, &in_shapes, &in_materials, &load_warn, &load_err,
        path.string().c_str(), base_dir.string().c_str());
    if (!result) {
        return false;
    }
    
    bool has_normal = !in_attribs.normals.empty();
    bool has_uv = !in_attribs.texcoords.empty();

    struct MeshData {
        std::vector<glm::vec3> positions;
        std::vector<glm::vec3> normals;
        std::vector<glm::vec2> texcoords;
        std::vector<uint32_t> indices;
        std::unordered_map<ObjIndex, size_t> index_map;
        int material_id;
        std::shared_ptr<SceneObject> object;
    };
    std::vector<MeshData> mesh_data;

    for (auto &in_shape : in_shapes) {
        std::unordered_map<int, size_t> objects_map;

        for (size_t f = 0; f < in_shape.mesh.indices.size() / 3; f++) {
            auto material_id = in_shape.mesh.material_ids[f];

            size_t object_id;
            {
                auto it = objects_map.find(material_id);
                if (it == objects_map.end()) {
                    auto mat_name = material_id < 0 ?  "default" : in_materials[material_id].name;
                    auto name = in_shape.name + "-" + mat_name;
                    mesh_data.push_back(MeshData {
                        .material_id = material_id,
                        .object = scene.AddObject(name),
                    });
                    it = objects_map.insert({ material_id, mesh_data.size() - 1 }).first;
                }
                object_id = it->second;
            }
            auto &data = mesh_data[object_id];

            for (size_t i = 0; i < 3; i++) {
                const auto &in_idx = in_shape.mesh.indices[3 * f + i];
                if (data.index_map.count(in_idx) == 0) {
                    data.index_map[in_idx] = data.positions.size();
                    glm::vec3 pos, norm;
                    glm::vec2 uv;
                    FetchVertexData(in_attribs, in_idx, has_normal, has_uv, pos, norm, uv);
                    data.positions.push_back(pos);
                    if (has_normal) {
                        data.normals.push_back(norm);
                    }
                    if (has_uv) {
                        data.texcoords.push_back(uv);
                    }
                }
                data.indices.push_back(data.index_map.at(in_idx));
            }
        }
    }

    for (auto &data : mesh_data) {
        auto mesh = std::make_shared<Mesh>();
        mesh->SetPositions(std::move(data.positions));
        mesh->SetIndices(std::move(data.indices));
        if (has_normal) {
            mesh->SetNormals(std::move(data.normals));
        } else {
            mesh->CalcNormals();
        }
        if (has_uv) {
            mesh->SetTexcoords(std::move(data.texcoords));
        } else {
            mesh->CalcTexcoords();
        }
        auto mesh_comp = data.object->AddComponent<MeshComponent>();
        mesh_comp->SetMesh(mesh);
    }

    std::vector<std::shared_ptr<Material>> materials(in_materials.size() + 1);
    for (size_t i = 0; i < in_materials.size(); i++) {
        const auto &in_mat = in_materials[i];
        auto mat = std::make_shared<Material>();
        mat->bsdf_type = bsdf_type;
        mat->name = in_mat.name;
        mat->diffuse = Vec3From(in_mat.diffuse);
        mat->specular = Vec3From(in_mat.specular);
        mat->transmittance = Vec3From(in_mat.transmittance);
        mat->emission = Vec3From(in_mat.emission);
        mat->ior = in_mat.ior;
        mat->shininess = in_mat.shininess;
        // mat->opacity = in_mat.dissolve;
        mat->opacity = in_mat.ior == 1.0f || mat->transmittance == glm::vec3(0.0f) ? 1.0f : 0.0f;
        LoadTexture(mat->diffuse_map, base_dir, in_mat.diffuse_texname);
        LoadTexture(mat->specular_map, base_dir, in_mat.specular_texname);
        LoadTexture(mat->emission_map, base_dir, in_mat.emissive_texname);
        mat->SetChanged();
        materials[i] = mat;
    }
    materials[in_materials.size()] = std::make_shared<Material>();

    for (const auto &data : mesh_data) {
        auto mat_comp = data.object->AddComponent<MaterialComponent>();
        auto mat_id = data.material_id < 0 ? in_materials.size() : data.material_id;
        mat_comp->SetMaterial(materials[mat_id]);
    }

    return true;
}

bool LoadJsonScene(Scene &scene, const std::filesystem::path &path) {
    auto extra_json = nlohmann::json::parse(std::ifstream(path), nullptr, false);
    if (extra_json.is_discarded()) {
        return false;
    }

    auto camera_json = extra_json["camera"];

    auto pos_json = camera_json["pos"];
    glm::vec3 pos(pos_json[0].get<float>(), pos_json[1].get<float>(), pos_json[2].get<float>());

    auto look_at_json = camera_json["look_at"];
    glm::vec3 look_at(look_at_json[0].get<float>(), look_at_json[1].get<float>(), look_at_json[2].get<float>());

    auto up_json = camera_json["up"];
    glm::vec3 up(up_json[0].get<float>(), up_json[1].get<float>(), up_json[2].get<float>());

    auto fov = camera_json["fov"].get<float>();

    auto camera_object = scene.AddObject("camera");
    auto camera_comp = camera_object->AddComponent<CameraComponent>();
    camera_comp->pos = pos;
    camera_comp->look_at = look_at;
    camera_comp->up = up;
    camera_comp->fov = fov;

    if (camera_json.contains("resolution")) {
        camera_comp->film_width = camera_json["resolution"][0].get<uint32_t>();
        camera_comp->film_height = camera_json["resolution"][1].get<uint32_t>();
    }

    return true;
}

bool LoadXmlScene(Scene &scene, const std::filesystem::path &path) {
    tinyxml2::XMLDocument scene_doc;
    auto load_ret = scene_doc.LoadFile(path.string().c_str());
    if (load_ret != tinyxml2::XML_SUCCESS) {
        std::cout << "xml parsing error: " << scene_doc.ErrorStr() << std::endl;
        return false;
    }

    // camera
    auto camera_node = scene_doc.FirstChildElement("camera");
    auto film_width = std::atoi(camera_node->Attribute("width"));
    auto film_height = std::atoi(camera_node->Attribute("height"));
    auto fov = std::atof(camera_node->Attribute("fovy"));

    auto pos_node = camera_node->FirstChildElement("eye");
    glm::vec3 pos(
        std::atof(pos_node->Attribute("x")),
        std::atof(pos_node->Attribute("y")),
        std::atof(pos_node->Attribute("z"))
    );
    auto look_at_node = camera_node->FirstChildElement("lookat");
    glm::vec3 look_at(
        std::atof(look_at_node->Attribute("x")),
        std::atof(look_at_node->Attribute("y")),
        std::atof(look_at_node->Attribute("z"))
    );
    auto up_node = camera_node->FirstChildElement("up");
    glm::vec3 up(
        std::atof(up_node->Attribute("x")),
        std::atof(up_node->Attribute("y")),
        std::atof(up_node->Attribute("z"))
    );

    auto camera_object = scene.AddObject("camera");
    auto camera_comp = camera_object->AddComponent<CameraComponent>();
    camera_comp->pos = pos;
    camera_comp->look_at = look_at;
    camera_comp->up = up;
    camera_comp->fov = fov;
    camera_comp->film_width = film_width;
    camera_comp->film_height = film_height;

    // area lights
    auto light_node = scene_doc.FirstChildElement("light");
    std::unordered_map<std::string, glm::vec3> radiance_map;
    while (light_node) {
        auto mtlname = light_node->Attribute("mtlname");
        auto radiance_str = light_node->Attribute("radiance");
        auto radiance = Vec3FromCommaSplittedStr(radiance_str);
        radiance_map[mtlname] = radiance;

        light_node = light_node->NextSiblingElement("light");
    }
    scene.ForEach<MaterialComponent>([&](MaterialComponent &material) {
        auto mat = material.GetMaterial();
        if (auto it = radiance_map.find(mat->name); it != radiance_map.end()) {
            mat->emission = it->second;
            mat->emission_map.reset();
            mat->SetChanged();
        }
    });

    return true;
}
