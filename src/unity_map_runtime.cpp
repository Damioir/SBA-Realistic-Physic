#include "unity_map_runtime.h"

#include "costume_map.h"
#include "map_storage.h"
#include "dobby.h"

#include <android/log.h>
#include <dlfcn.h>
#include <atomic>
#include <cstddef>
#include <cstring>
#include <cstdint>
#include <cmath>
#include <string>
#include <vector>

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
    void (*player_trigger_original)(void*, void*, void*) = nullptr;

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
    void* transform_set_euler_angles = nullptr;
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
    void* collider = nullptr;
    void* box_collider_type = nullptr;
    void* collider_set_is_trigger = nullptr;
    void* collider_get_game_object = nullptr;
    void* game_object_get_name = nullptr;
    void* game_object_find = nullptr;
    void* player_on_trigger_enter = nullptr;
    void* player_handle_death = nullptr;
    void* component_get_game_object = nullptr;
    void* player_anim_controller_type = nullptr;
    void* player_anim_play = nullptr;
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
    g_types.game_object_get_component =
        g_api.method(g_types.game_object, "GetComponent", 1);
    g_types.transform_set_scale =
        g_api.method(g_types.transform, "set_localScale", 1);
    g_types.transform_set_euler_angles =
        g_api.method(g_types.transform, "set_eulerAngles", 1);
    if (!g_types.game_object_create_primitive ||
        !g_types.game_object_get_transform ||
        !g_types.transform_set_position ||
        !g_types.transform_set_scale) return false;

    if (g_types.renderer) {
        g_types.renderer_get_material =
            g_api.method(g_types.renderer, "get_material", 0);
        void* material = g_api.find_class("UnityEngine", "Material");
        g_types.material_set_color =
            g_api.method(material, "set_color", 1);
        g_types.renderer_type = g_api.type_get_object(g_api.class_get_type(g_types.renderer));
    }
    g_types.collider = g_api.find_class("UnityEngine", "Collider");
    void* box_collider = g_api.find_class("UnityEngine", "BoxCollider");
    if (box_collider) g_types.box_collider_type =
        g_api.type_get_object(g_api.class_get_type(box_collider));
    if (g_types.collider) {
        g_types.collider_set_is_trigger = g_api.method(g_types.collider, "set_isTrigger", 1);
        g_types.collider_get_game_object = g_api.method(g_types.collider, "get_gameObject", 0);
    }
    g_types.game_object_get_name = g_api.method(g_types.game_object, "get_name", 0);
    g_types.game_object_find = g_api.method(g_types.game_object, "Find", 1);
    void* player = g_api.find_class("", "PlayerBearController");
    g_types.player_on_trigger_enter = g_api.method(player, "OnTriggerEnter", 1);
    g_types.player_handle_death = g_api.method(player, "HandlePlayerDeath", 0);
    void* component = g_api.find_class("UnityEngine", "Component");
    if (component) g_types.component_get_game_object =
        g_api.method(component, "get_gameObject", 0);
    void* player_anim = g_api.find_class("", "PlayerAnimController");
    if (player_anim) {
        g_types.player_anim_controller_type =
            g_api.type_get_object(g_api.class_get_type(player_anim));
        g_types.player_anim_play = g_api.method(player_anim, "PlayAnim", 2);
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

void set_transform(void* game_object, Vector3 position, Vector3 scale, float yaw) {
    void* transform = g_api.invoke(g_types.game_object_get_transform, game_object);
    if (!transform) return;
    void* position_args[] = {&position};
    void* scale_args[] = {&scale};
    g_api.invoke(g_types.transform_set_position, transform, position_args);
    g_api.invoke(g_types.transform_set_scale, transform, scale_args);
    if (g_types.transform_set_euler_angles) {
        Vector3 rotation{0.0f, yaw, 0.0f};
        void* rotation_args[] = {&rotation};
        g_api.invoke(g_types.transform_set_euler_angles, transform, rotation_args);
    }
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

void* create_cube(const std::string& name, Vector3 position, Vector3 scale, Color color, float yaw = 0.0f) {
    int primitive_cube = 3;
    void* args[] = {&primitive_cube};
    void* game_object = g_api.invoke(g_types.game_object_create_primitive, nullptr, args);
    if (!game_object) return nullptr;
    set_transform(game_object, position, scale, yaw);
    set_name(game_object, name);
    set_color(game_object, color);
    return game_object;
}

struct AnimationAction {
    std::string event_key;
    std::string animation_key;
};
std::vector<AnimationAction> g_animations;

void make_trigger(void* game_object) {
    if (!game_object || !g_types.box_collider_type || !g_types.game_object_get_component ||
        !g_types.collider_set_is_trigger) return;
    void* component_args[] = {g_types.box_collider_type};
    void* collider = g_api.invoke(g_types.game_object_get_component, game_object, component_args);
    if (!collider) return;
    bool enabled = true;
    void* args[] = {&enabled};
    g_api.invoke(g_types.collider_set_is_trigger, collider, args);
}

struct Il2CppStringView {
    void* klass;
    void* monitor;
    int32_t length;
    char16_t chars[1];
};

struct MessagePlacement {
    std::string event_key;
    std::string object_name;
    Vector3 position;
};
std::vector<MessagePlacement> g_messages;

bool managed_ascii(void* managed_string, std::string* output) {
    if (!managed_string || !output) return false;
    const auto* value = static_cast<const Il2CppStringView*>(managed_string);
    if (value->length < 0 || value->length > 256) return false;
    output->clear();
    output->reserve(static_cast<std::size_t>(value->length));
    for (int32_t i = 0; i < value->length; ++i) {
        if (value->chars[i] > 0x7f) return false;
        output->push_back(static_cast<char>(value->chars[i]));
    }
    return true;
}

bool has_prefix(const std::string& value, const char* prefix) {
    return value.compare(0, std::strlen(prefix), prefix) == 0;
}

void show_messages_for_event(const std::string& event_key) {
    if (!g_types.game_object_find || !g_types.game_object_get_transform ||
        !g_types.transform_set_position) return;
    for (const MessagePlacement& message : g_messages) {
        if (message.event_key != event_key) continue;
        void* object_name = g_api.string_new(message.object_name.c_str());
        void* find_args[] = {object_name};
        void* game_object = g_api.invoke(g_types.game_object_find, nullptr, find_args);
        if (!game_object) continue;
        void* transform = g_api.invoke(g_types.game_object_get_transform, game_object);
        void* position_args[] = {const_cast<Vector3*>(&message.position)};
        g_api.invoke(g_types.transform_set_position, transform, position_args);
    }
}

void play_animations_for_event(const std::string& event_key, void* player) {
    if (!g_types.player_anim_controller_type || !g_types.player_anim_play ||
        !g_types.component_get_game_object || !g_types.game_object_get_component) return;
    void* player_object = g_api.invoke(g_types.component_get_game_object, player);
    void* type_args[] = {g_types.player_anim_controller_type};
    void* controller = g_api.invoke(g_types.game_object_get_component, player_object, type_args);
    if (!controller) return;
    for (const AnimationAction& action : g_animations) {
        if (action.event_key != event_key) continue;
        void* animation_name = g_api.string_new(action.animation_key.c_str());
        bool force = true;
        void* args[] = {animation_name, &force};
        g_api.invoke(g_types.player_anim_play, controller, args);
    }
}

void on_player_trigger_enter(void* self, void* other, void* method_info) {
    if (g_api.player_trigger_original) {
        g_api.player_trigger_original(self, other, method_info);
    }
    if (!self || !other || !g_types.collider_get_game_object || !g_types.game_object_get_name) return;
    void* object = g_api.invoke(g_types.collider_get_game_object, other);
    if (!object) return;
    void* name = g_api.invoke(g_types.game_object_get_name, object);
    std::string object_name;
    if (!managed_ascii(name, &object_name)) return;
    if (has_prefix(object_name, "sba-costume-death:")) {
        if (g_types.player_handle_death) g_api.invoke(g_types.player_handle_death, self);
        return;
    }
    constexpr const char* kEventPrefix = "sba-costume-event:";
    if (has_prefix(object_name, kEventPrefix)) {
        const std::string event_key = object_name.substr(std::strlen(kEventPrefix));
        show_messages_for_event(event_key);
        play_animations_for_event(event_key, self);
    }
}

bool install_death_block_hook() {
    if (!g_types.player_on_trigger_enter || !g_types.player_handle_death) {
        LOGW("death block hook skipped: SBA 13.0.9 player methods unavailable");
        return false;
    }
    void* target = *reinterpret_cast<void**>(g_types.player_on_trigger_enter);
    if (!target) {
        LOGW("death block hook skipped: OnTriggerEnter has no native pointer");
        return false;
    }
    const int result = DobbyHook(target,
        reinterpret_cast<dobby_dummy_func_t>(on_player_trigger_enter),
        reinterpret_cast<dobby_dummy_func_t*>(&g_api.player_trigger_original));
    if (result != 0 || !g_api.player_trigger_original) {
        LOGW("death block hook installation failed (%d)", result);
        return false;
    }
    LOGI("death block hook installed for SBA 13.0.9");
    return true;
}

Vector3 chair_point(Vector3 base, float dx, float dy, float dz, float yaw) {
    const float radians = yaw * 0.017453292519943295f;
    const float cosine = std::cos(radians);
    const float sine = std::sin(radians);
    return {base.x + dx * cosine + dz * sine, base.y + dy,
            base.z - dx * sine + dz * cosine};
}

void create_chair(const Element& element) {
    const Color wood{0.48f, 0.26f, 0.12f, 1.0f};
    const Vector3 base{element.x, element.y, element.z};
    const Vector3 unit{element.scale_x, element.scale_y, element.scale_z};
    create_cube(element.id + "-seat",
                {base.x, base.y + 0.55f * unit.y, base.z},
                {0.85f * unit.x, 0.15f * unit.y, 0.85f * unit.z}, wood, element.yaw);
    create_cube(element.id + "-back",
                chair_point(base, 0.0f, 1.05f * unit.y, -0.35f * unit.z, element.yaw),
                {0.85f * unit.x, 0.9f * unit.y, 0.15f * unit.z}, wood, element.yaw);
    for (int sx : {-1, 1}) {
        for (int sz : {-1, 1}) {
            create_cube(element.id + "-leg",
                        chair_point(base, sx * 0.32f * unit.x, 0.25f * unit.y,
                                    sz * 0.32f * unit.z, element.yaw),
                        {0.12f * unit.x, 0.5f * unit.y, 0.12f * unit.z}, wood, element.yaw);
        }
    }
}

void create_message(const Element& element) {
    if (!g_types.text_mesh || !g_types.game_object_add_component ||
        !g_types.text_mesh_type || !g_types.text_mesh_set_text) {
        LOGW("TextMesh unavailable; skipped a message element");
        return;
    }
    const std::string object_name = "sba-costume-message:" + element.event_key + ":" + element.id;
    void* game_object = create_cube(object_name,
                                   {element.x, element.event_key.empty() ? element.y + 1.4f : -10000.0f, element.z},
                                   {0.05f, 0.05f, 0.05f},
                                   {0.58f, 0.32f, 0.88f, 1.0f}, element.yaw);
    if (!game_object) return;

    void* component_args[] = {g_types.text_mesh_type};
    void* text_mesh = g_api.invoke(g_types.game_object_add_component, game_object, component_args);
    if (!text_mesh) return;
    void* text = g_api.string_new(element.text.c_str());
    void* text_args[] = {text};
    g_api.invoke(g_types.text_mesh_set_text, text_mesh, text_args);
    if (!element.event_key.empty()) {
        g_messages.push_back({element.event_key, object_name,
                              {element.x, element.y + 1.4f, element.z}});
    }
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
                                {element.scale_x, element.scale_y, element.scale_z}, normal, element.yaw)) ++spawned;
                break;
            case ElementKind::Chair:
                create_chair(element);
                ++spawned;
                break;
            case ElementKind::DeathBlock: {
                void* block = create_cube(std::string("sba-costume-death:") + element.id,
                                          {element.x, element.y, element.z},
                                          {element.scale_x, element.scale_y, element.scale_z}, danger, element.yaw);
                if (block) {
                    make_trigger(block);
                    ++spawned;
                }
                break;
            }
            case ElementKind::Message:
                create_message(element);
                ++spawned;
                break;
            case ElementKind::Trigger: {
                const std::string name = "sba-costume-event:" + element.event_key;
                void* trigger = create_cube(name, {element.x, element.y, element.z},
                                            {element.scale_x, element.scale_y, element.scale_z},
                                            {0.20f, 0.82f, 0.47f, 1.0f}, element.yaw);
                if (trigger) {
                    make_trigger(trigger);
                    ++spawned;
                }
                break;
            }
            case ElementKind::Animation:
                if (!element.event_key.empty() && !element.animation_key.empty()) {
                    g_animations.push_back({element.event_key, element.animation_key});
                    ++spawned;
                }
                break;
        }
    }
    install_death_block_hook();
    LOGI("loaded map '%s': %zu of %zu items created", document.title.c_str(),
         spawned, document.elements.size());
}

}  // namespace sba::costume_map
