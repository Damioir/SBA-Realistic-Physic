#pragma once

#include <string>
#include <vector>

namespace sba::costume_map {

// Portable map document model. The Unity runtime adapter will translate prefab
// names and event keys into game objects after they are validated.
enum class ElementKind {
    Prop,
    Chair,
    DeathBlock,
    Message,
    Trigger,
    Animation
};

struct Element {
    std::string id;
    ElementKind kind = ElementKind::Prop;
    std::string prefab;
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float yaw = 0.0f;
    float scale_x = 1.0f;
    float scale_y = 1.0f;
    float scale_z = 1.0f;
    std::string text;
    std::string event_key;
    std::string animation_key;
};

struct Document {
    std::string id;
    std::string title;
    std::string description;
    std::vector<Element> elements;
};

// Validate before saving or attempting to spawn anything. Returns a short,
// player-readable reason on failure.
bool validate(const Document& document, std::string* error);

// Versioned, escaped text format used for .sbamap files. Parsing never trusts
// file-provided counts or numeric values; call validate() before use.
std::string encode(const Document& document);
bool decode(const std::string& data, Document* document, std::string* error);

const char* kind_name(ElementKind kind);
bool parse_kind(const std::string& name, ElementKind* kind);

}  // namespace sba::costume_map
