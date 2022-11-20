#pragma once

#include <array>
#include <functional>
#include <typeinfo>
#include <unordered_map>

#include <glm/glm.hpp>

using TypeInfoRef = std::reference_wrapper<const std::type_info>;

template <>
struct std::hash<TypeInfoRef> {
    size_t operator()(TypeInfoRef v) const noexcept {
        return v.get().hash_code();
    }
};
inline bool operator==(TypeInfoRef a, TypeInfoRef b) {
    return a.get() == b.get();
}

class SceneObject;

template <typename T>
concept HasUpdate = requires (T comp) {
    { comp.Update() } -> std::same_as<void>;
};
template <typename T>
concept HasShowUi = requires (T comp) {
    { comp.ShowUi() } -> std::same_as<void>;
};

class ComponentStorage {
private:
    static constexpr size_t kBlockSize = 4096;

public:
    ~ComponentStorage();

    template <typename T>
    void Init() {
        size_ = sizeof(T) + sizeof(void *);
        block_offset_ = kBlockSize;
        funcs_.destructor = [](uint8_t *data) {
            reinterpret_cast<T *>(data)->~T();
        };
        if constexpr (HasUpdate<T>) {
            funcs_.update = [](uint8_t *data) {
                reinterpret_cast<T *>(data)->Update();
            };
        }
        if constexpr (HasShowUi<T>) {
            funcs_.show_ui = [](uint8_t *data) {
                reinterpret_cast<T *>(data)->ShowUi();
            };
        }
    }

    void Update();

    void ShowUi(void *component);

    template <typename T, std::invocable<T &> F>
    void ForEach(F &&func) {
        ForEachInner([&](uint8_t *addr) {
            func(*reinterpret_cast<T *>(addr));
        });
    }
    template <typename T, std::invocable<SceneObject &, T &> F>
    void ForEach(F &&func) {
        ForEachInner([&](SceneObject &object, uint8_t *addr) {
            func(object, *reinterpret_cast<T *>(addr));
        });
    }

    uint8_t *Allocate(SceneObject *object);

    size_t Count() const;

private:
    template <std::invocable<uint8_t *> F>
    void ForEachInner(F &&func) {
        auto last = storage_.end();
        --last;
        for (auto it = storage_.begin(); it != storage_.end(); it++) {
            auto count = it == last ? block_offset_ : kBlockSize;
            for (size_t i = 0; i < count; i += size_) {
                auto addr = &(*it)[i + sizeof(void *)];
                func(addr);
            }
        }
    }
    template <std::invocable<SceneObject &, uint8_t *> F>
    void ForEachInner(F &&func) {
        auto last = storage_.end();
        --last;
        for (auto it = storage_.begin(); it != storage_.end(); it++) {
            auto count = it == last ? block_offset_ : kBlockSize;
            for (size_t i = 0; i < count; i += size_) {
                auto addr = &(*it)[i];
                func(**reinterpret_cast<SceneObject **>(addr), addr + sizeof(void *));
            }
        }
    }

    struct {
        std::function<void(uint8_t *)> destructor;
        std::function<void(uint8_t *)> update;
        std::function<void(uint8_t *)> show_ui;
    } funcs_;
    size_t size_;
    size_t block_offset_;
    std::list<std::array<uint8_t, kBlockSize>> storage_;
};

class Scene {
public:
    std::shared_ptr<SceneObject> AddObject(std::string_view name);

    void ShowUi();

    void Update();

    template <typename T>
    size_t Count() {
        if (auto it = components_.find(typeid(T)); it != components_.end()) {
            return it->second.Count();
        } else {
            return 0;
        }
    }

    template <typename T1, typename... TOther, std::invocable<T1 &, TOther &...> F>
    void ForEach(F &&func);
    template <typename T1, typename... TOther, std::invocable<SceneObject &, T1 &, TOther &...> F>
    void ForEach(F &&func);

private:
    friend SceneObject;

    template <typename T>
    std::pair<ComponentStorage *, T *> AddComponent(SceneObject *object) {
        const auto &tid = typeid(T);
        auto it = components_.find(tid);
        if (it == components_.end()) {
            it = components_.insert({ tid, {} }).first;
            it->second.Init<T>();
        }
        auto storage = it->second.Allocate(object);
        return { &it->second, new (storage) T() };
    }

    std::vector<std::shared_ptr<SceneObject>> objects_;
    std::unordered_map<TypeInfoRef, ComponentStorage> components_;

    size_t curr_ui_object_ = 0;
};

class SceneObject {
public:
    void ShowUi();

    template <typename T>
    T *AddComponent() {
        const auto &tid = typeid(T);
        if (!components_.contains(tid)) {
            auto component = scene_->AddComponent<T>(this);
            components_.insert({ tid, component });
            return component.second;
        } else {
            return nullptr;
        }
    }
    
    template <typename T>
    T *GetComponent() {
        if (auto it = components_.find(typeid(T)); it != components_.end()) {
            return reinterpret_cast<T *>(it->second.second);
        } else {
            return nullptr;
        }
    }

    template <typename T>
    bool HasComponent() {
        return components_.contains(typeid(T));
    }
    template <typename T1, typename... TOther>
    bool HasComponents() {
        if constexpr (sizeof...(TOther) == 0) {
            return HasComponent<T1>();
        } else {
            return HasComponent<T1>() && HasComponents<TOther...>();
        }
    }

    glm::vec3 translate = glm::vec3(0.0f);
    glm::vec3 rotate = glm::vec3(0.0f);
    glm::vec3 scale = glm::vec3(1.0f);

    glm::mat4 GetTransform() const;

private:
    friend Scene;

    SceneObject(Scene *scene, std::string_view name) : scene_(scene), name_(name) {}

    Scene *scene_;
    std::string name_;
    std::unordered_map<TypeInfoRef, std::pair<ComponentStorage *, void *>> components_;
};

template <typename T1, typename... TOther, std::invocable<T1 &, TOther &...> F>
void Scene::ForEach(F &&func) {
    if constexpr (sizeof...(TOther) == 0) {
        if (auto it = components_.find(typeid(T1)); it != components_.end()) {
            it->second.ForEach<T1>(func);
        }
    } else {
        if (auto it = components_.find(typeid(T1)); it != components_.end()) {
            it->second.ForEach<T1>([&func](SceneObject &object, T1 &comp1) {
                if (object.HasComponents<TOther...>()) {
                    func(comp1, *object.GetComponent<TOther>()...);
                }
            });
        }
    }
}
template <typename T1, typename... TOther, std::invocable<SceneObject &, T1 &, TOther &...> F>
void Scene::ForEach(F &&func) {
    if constexpr (sizeof...(TOther) == 0) {
        if (auto it = components_.find(typeid(T1)); it != components_.end()) {
            it->second.ForEach<T1>(func);
        }
    } else {
        if (auto it = components_.find(typeid(T1)); it != components_.end()) {
            it->second.ForEach<T1>([&func](SceneObject &object, T1 &comp1) {
                if (object.HasComponents<TOther...>()) {
                    func(object, comp1, *object.GetComponent<TOther>()...);
                }
            });
        }
    }
}
