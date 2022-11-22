#include "core.hpp"

#include <algorithm>
#include <format>

#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>

namespace {
    
Scene *g_scene = nullptr;

}

ComponentStorage::~ComponentStorage() {
    if (funcs_.destructor) {
        if (storage_.size() > 0) {
            auto &block = storage_.back();
            for (size_t i = 0; i < block_offset_; i += size_) {
                funcs_.destructor(&block[i + sizeof(void *)]);
            }
            storage_.pop_back();
        }
        for (auto &block : storage_) {
            for (size_t i = 0; i < block.size(); i += size_) {
                funcs_.destructor(&block[i + sizeof(void *)]);
            }
        }
    }
}

void ComponentStorage::Update() {
    if (funcs_.update) {
        ForEachInner(funcs_.update);
    }
}

void ComponentStorage::ShowUi(void *component) {
    if (funcs_.show_ui) {
        funcs_.show_ui(reinterpret_cast<uint8_t *>(component));
    }
}

uint8_t *ComponentStorage::Allocate(SceneObject *object) {
    if (block_offset_ + size_ > kBlockSize) {
        storage_.emplace_back();
        block_offset_ = 0;
    }
    auto addr = &storage_.back()[block_offset_];
    *reinterpret_cast<SceneObject **>(addr) = object;
    addr += sizeof(void *);
    block_offset_ += size_;
    return addr;
}

size_t ComponentStorage::Count() const {
    auto size = size_ + sizeof(void *);
    return block_offset_ / size + (storage_.size() - 1) * kBlockSize / size;
}


Scene::Scene() {
    g_scene = this;
}

std::shared_ptr<SceneObject> Scene::AddObject(std::string_view name) {
    auto index = objects_.size();
    auto obj = std::shared_ptr<SceneObject>(new SceneObject(this, name));
    objects_.push_back(obj);
    return obj;
}

void Scene::ShowUi() {
    if (ImGui::Begin("Scene")) {
        auto &curr_object = objects_[curr_ui_object_];
        ImGui::Text("%zu objects in total", objects_.size());
        int next_object = curr_ui_object_;
        ImGui::InputInt("select object", &next_object);
        curr_ui_object_ = std::clamp<int>(next_object, 0, objects_.size() - 1);

        ImGui::Separator();
        curr_object->ShowUi();
    }
    ImGui::End();
}

void Scene::Update() {
    for (auto &[_, comp] : components_) {
        comp.Update();
    }
}

Scene &GetGlobalScene() {
    return *g_scene;
}


void SceneObject::ShowUi() {
    ImGui::Text("object '%s'", name_.c_str());

    for (auto &[ty, comp] : components_) {
        auto component_str = std::format("component '{}'", ty.get().name());
        if (ImGui::CollapsingHeader(component_str.c_str())) {
            comp.first->ShowUi(comp.second);
        }
    }
}

glm::mat4 SceneObject::GetTransform() const {
    glm::mat4 trans(1.0f);
    trans = glm::translate(trans, translate);
    trans = glm::rotate(trans, glm::radians(rotate.z), glm::vec3(0.0f, 0.0f, 1.0f));
    trans = glm::rotate(trans, glm::radians(rotate.y), glm::vec3(0.0f, 1.0f, 0.0f));
    trans = glm::rotate(trans, glm::radians(rotate.x), glm::vec3(1.0f, 0.0f, 0.0f));
    trans = glm::scale(trans, scale);
    return trans;
}
