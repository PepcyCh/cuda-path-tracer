#include "loader.hpp"

#include <fstream>

#include <tiny_obj_loader.h>
#include <nlohmann/json.hpp>
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

bool LoadObjScene(Scene &scene, const std::filesystem::path &path) {
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
        mat->diffuse = glm::vec3(in_mat.diffuse[0], in_mat.diffuse[1] ,in_mat.diffuse[2]);
        mat->specular = glm::vec3(in_mat.specular[0], in_mat.specular[1] ,in_mat.specular[2]);
        mat->emission = glm::vec3(in_mat.emission[0], in_mat.emission[1], in_mat.emission[2]);
        mat->ior = in_mat.ior;
        mat->shininess = in_mat.shininess;
        if (!in_mat.diffuse_texname.empty()) {
            LoadTexture(mat->diffuse_map, base_dir / in_mat.diffuse_texname);
        }
        if (!in_mat.specular_texname.empty()) {
            LoadTexture(mat->specular_map, base_dir / in_mat.specular_texname);
        }
        if (!in_mat.emissive_texname.empty()) {
            LoadTexture(mat->emission_map, base_dir / in_mat.emissive_texname);
        }
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

    return true;
}
