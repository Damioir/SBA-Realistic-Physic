#include "unity_map_runtime.h"

#include "costume_map.h"
#include "map_storage.h"

#include <android/log.h>
#include <dlfcn.h>
#include <atomic>
#include <cstddef>
#include <cstring>
#include <string>

#define TAG "SBACostumeMapMaker"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, TAG, __VA_ARGS__)

namespace sba::costume_map {
namespace {

constexpr const char* kSavedMap =
    "/sdcard/Android/data/com.Earthkwak.Platformer/files/CostumeMaps/map1.sbamap";

struct Vector3 {
    float x;
    float y;
    float z;
};

struct Color {
    float r;
    float g;
    float b;
    float a;
};

struct Il2CppApi {
    void* handle = nullptr;
    void* (*domain_get)() = nullptr;
    void** (*domain_get_assemblies)(void*, std::size_t*) = nullptr;
    void* (*assembly_get_image)(void*) = nullptr;
    const char* (*image_get_name)(void*) = nullptr;
    void* (*class_from_name)(void*, const char*, const char*) = nullptr;
    void* (*class_get_method_from_name)(void*, const char*, int) = nullptr;
    void* (*class_get_type)(void*) = nullptr;
    void* (*type_get_object)(void*) = nullptr;
    void* (*string_new)(const char*) = nullptr;
    void* (*runtime_invoke)(void*, void*, void**, void**) = nullptr;

    template <typename T>
    bool bind(T* target, const char* name) {
        *target = reinterpret_cast<T>(dlsym(handle, name));
        return *target != nullptr;
    }

    bool initialize() {
        handle = dlopen("libil2cpp.so", RTLD_NOW | RTLD_NOLOAD);
        if (!handle) {
            LOGW("IL2CPP library handle not found");
            return false;
        }
        const bool ok =
            bind(&domain_get, "il2cpp_domain_get") &&
            bind(&domain_get_assemblies, "il2cpp_domain_get_assemblies") &&
            bind(&assembly_get_image, "il2cpp_assembly_get_image") &&
            bind(&image_get_name, "il2cpp_image_get_name") &&
            bind(&class_from_name, "il2cpp_class_from_name") &&
            bind(&class_get_method_from_name, "il2cpp_class_get_method_from_name") &&
            bind(&class_get_type, "il2cpp_class_get_type") &&
            bind(&type_get_object, "il2cpp_type_get_object") &&
            bind(&string_new, "il2cpp_string_new") &&
            bind(&runtime_invoke, "il2cpp_runtime_invoke");
        if (!ok) LOGW("one or more IL2CPP exports are unavailable");
        return ok;
    }

    void* find_class(const char* name_space, const char* class_name) const {
        std::size_t count = 0;
        void** assemblies = domain_get_assemblies(domain_get(), &count);
        for (std::size_t i = 0; assemblies && i < count; ++i) {
            void* image = assembly_get_image(assemblies[i]);
            if (!image) continue;
            const char* image_name = image_get_name(image);
            if (!image_name || std::strstr(image_name, "UnityEngine.CoreModule") == nullptr) continue;
            if (void* klass = class_from_name(image, name_space, class_name)) return klass;
        }
        return nullptr;
    }

    void* method(void* klass, const char* name, int argument_count) const {
        return klass ? class_get_method_from_name(klass, name, argument_count) : nullptr;
    }

    void* invoke(void* target, void* instance, void** args = nullptr) const {
        if (!target) return nullptr;
        void* exception = nullptr;
        void* result = runtime_invoke(target, instance, args, &exception);
        if (exception) {
            LOGW("Unity method raised an exception");
            return nullptr;
        }
        return result;
    }
};

struct UnityTypes {
    void* game_object = nullptr;
    void* transform = nullptr;
    void* game_object_create_primitive = nullptr;
    void* game_object_get_transform = nullptr;
    void* game_object_set_name = nullptr;
    void* transform_set_position = nullptr;
    void* transform_set_scale = nullptr;
    void* renderer = nullptr;
    void* game_object_get_component = nullptr;
    void* renderer_get_material = nullptr;
    void* material_set_color = nullptr;
    void* text_mesh = nullptr;
    void* game_object_add_component = nullptr;
    void* text_mesh_set_text = nullptr;
    void* text_mesh_set_font_size = nullptr;
    void* renderer_type = nullptr;
    void* text_mesh_type = nullptr;
};

std::atomic<bool> g_started{false};
Il2CppApi g_api;
UnityTypes g_types;

bool resolve_types() {
    g_types.game_object = g_api.find_class("UnityEngine", "GameObject");
    g_types.transform = g_api.find_class("UnityEngine", "Transform");
    g_types.renderer = g_api.find_class("UnityEngine", "Renderer");
    g_types.text_mesh = g_api.find_class("UnityEngine", "TextMesh");
    if (!g_types.game_object || !g_types.transform) return false;

    g_types.game_object_create_primitive =
        g_api.method(g_types.game_object, "CreatePrimitive", 1);
    g_types.game_object_get_transform =
        g_api.method(g_types.game_object, "get_transform", 0);
    g_types.game_object_set_name =
        g_api.method(g_types.game_object, "set_name", 1);
    g_types.transform_set_position =
        g_api.method(g_types.transform, "set_position", 1);
    g_types.transform_set_scale =
        g_api.method(g_types.transform, "set_localScale", 1);
    if (!g_types.game_object_create_primitive ||
        !g_types.game_object_get_transform ||
        !g_types.transform_set_position ||
        !g_types.transform_set_scale) return false;

    if (g_types.renderer) {
        g_types.game_object_get_component =
            g_api.method(g_types.game_object, "GetComponent", 1);
        g_types.renderer_get_material =
            g_api.method(g_types.renderer, "get_material", 0);
        void* material = g_api.find_class("UnityEngine", "Material");
        g_types.material_set_color =
            g_api.method(material, "set_color", 1);
        g_types.renderer_type = g_api.type_get_object(g_api.class_get_type(g_types.renderer));
    }
    if (g_types.text_mesh) {
        g_types.game_object_add_component =
            g_api.method(g_types.game_object, "AddComponent", 1);
        g_types.text_mesh_set_text =
            g_api.method(g_types.text_mesh, "set_text", 1);
        g_types.text_mesh_set_font_size =
            g_api.method(g_types.text_mesh, "set_fontSize", 1);
        g_types.text_mesh_type = g_api.type_get_object(g_api.class_get_type(g_types.text_mesh));
    }
    return true;
}

void set_transform(void* game_object, Vector3 position, Vector3 scale) {
    void* transform = g_api.invoke(g_types.game_object_get_transform, game_object);
    if (!transform) return;
    void* position_args[] = {&position};
    void* scale_args[] = {&scale};
    g_api.invoke(g_types.transform_set_position, transform, position_args);
    g_api.invoke(g_types.transform_set_scale, transform, scale_args);
}

void set_name(void* game_object, const std::string& name) {
    if (!g_types.game_object_set_name) return;
    void* text = g_api.string_new(name.c_str());
    void* args[] = {text};
    g_api.invoke(g_types.game_object_set_name, game_object, args);
}

void set_color(void* game_object, Color color) {
    if (!g_types.renderer_type || !g_types.game_object_get_component ||
        !g_types.renderer_get_material || !g_types.material_set_color) return;
    void* component_args[] = {g_types.renderer_type};
    void* renderer = g_api.invoke(g_types.game_object_get_component, game_object, component_args);
    void* material = g_api.invoke(g_types.renderer_get_material, renderer);
    if (!material) return;
    void* color_args[] = {&color};
    g_api.invoke(g_types.material_set_color, material, color_args);
}

void* create_cube(const std::string& name, Vector3 position, Vector3 scale, Color color) {
    int primitive_cube = 3;
    void* args[] = {&primitive_cube};
    void* game_object = g_api.invoke(g_types.game_object_create_primitive, nullptr, args);
    if (!game_object) return nullptr;
    set_transform(game_object, position, scale);
    set_name(game_object, name);
    set_color(game_object, color);
    return game_object;
}

void create_chair(const Element& element) {
    const Color wood{0.48f, 0.26f, 0.12f, 1.0f};
    const Vector3 base{element.x, element.y, element.z};
    const Vector3 unit{element.scale_x, element.scale_y, element.scale_z};
    create_cube(element.id + "-seat",
                {base.x, base.y + 0.55f * unit.y, base.z},
                {0.85f * unit.x, 0.15f * unit.y, 0.85f * unit.z}, wood);
    create_cube(element.id + "-back",
                {base.x, base.y + 1.05f * unit.y, base.z - 0.35f * unit.z},
                {0.85f * unit.x, 0.9f * unit.y, 0.15f * unit.z}, wood);
    for (int sx : {-1, 1}) {
        for (int sz : {-1, 1}) {
            create_cube(element.id + "-leg",
                        {base.x + sx * 0.32f * unit.x, base.y + 0.25f * unit.y,
                         base.z + sz * 0.32f * unit.z},
                        {0.12f * unit.x, 0.5f * unit.y, 0.12f * unit.z}, wood);
        }
    }
}

void create_message(const Element& element) {
    if (!g_types.text_mesh || !g_types.game_object_add_component ||
        !g_types.text_mesh_type || !g_types.text_mesh_set_text) {
        LOGW("TextMesh unavailable; skipped a message element");
        return;
    }
    void* game_object = g_api.invoke(g_types.game_object_create_primitive, nullptr, nullptr);
    if (!game_object) return;
    // Use an empty GameObject rather than a visible primitive.
    void* empty_class = g_api.find_class("UnityEngine", "GameObject");
    void* empty_ctor = g_api.method(empty_class, ".ctor", 0);
    if (empty_ctor) {
        // CreatePrimitive has no no-geometry mode; destroy its collider/renderer is
        // outside this adapter's scope. Keep it as a small marker for now.
    }
    set_transform(game_object, {element.x, element.y + 1.4f, element.z}, {0.05f, 0.05f, 0.05f});
    set_name(game_object, element.id + "-message");

    void* component_args[] = {g_types.text_mesh_type};
    void* text_mesh = g_api.invoke(g_types.game_object_add_component, game_object, component_args);
    if (!text_mesh) return;
    void* text = g_api.string_new(element.text.c_str());
    void* text_args[] = {text};
    g_api.invoke(g_types.text_mesh_set_text, text_mesh, text_args);
    if (g_types.text_mesh_set_font_size) {
        int font_size = 48;
        void* size_args[] = {&font_size};
        g_api.invoke(g_types.text_mesh_set_font_size, text_mesh, size_args);
    }
}

}  // namespace

void load_into_game() {
    if (g_started.exchange(true)) return;
    Document document;
    std::string error;
    if (!load_file(kSavedMap, &document, &error)) {
        LOGI("no saved map loaded: %s", error.c_str());
        return;
    }
    if (!g_api.initialize() || !resolve_types()) {
        LOGW("Unity object APIs unavailable; saved map was not spawned");
        return;
    }

    std::size_t spawned = 0;
    const Color normal{0.34f, 0.68f, 0.92f, 1.0f};
    const Color danger{0.92f, 0.20f, 0.24f, 1.0f};
    for (const Element& element : document.elements) {
        if (spawned >= 400) {
            LOGW("map has more than 400 items; remaining items skipped");
            break;
        }
        switch (element.kind) {
            case ElementKind::Prop:
                if (create_cube(element.id, {element.x, element.y, element.z},
                                {element.scale_x, element.scale_y, element.scale_z}, normal)) ++spawned;
                break;
            case ElementKind::Chair:
                create_chair(element);
                ++spawned;
                break;
            case ElementKind::DeathBlock:
                if (create_cube(element.id, {element.x, element.y, element.z},
                                {element.scale_x, element.scale_y, element.scale_z}, danger)) ++spawned;
                break;
            case ElementKind::Message:
                create_message(element);
                ++spawned;
                break;
            case ElementKind::Trigger:
            case ElementKind::Animation:
                LOGW("runtime behavior not implemented for %s element %s",
                     kind_name(element.kind), element.id.c_str());
                break;
        }
    }
    LOGI("loaded map '%s': %zu of %zu items created", document.title.c_str(),
         spawned, document.elements.size());
}

}  // namespace sba::costume_map
