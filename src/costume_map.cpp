#include "costume_map.h"

#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <set>
#include <sstream>
#include <utility>

namespace sba::costume_map {
namespace {

constexpr std::size_t kMaxElements = 2000;
constexpr std::size_t kMaxLineBytes = 8192;
constexpr std::size_t kMaxDocumentBytes = 8 * 1024 * 1024;

bool valid_id(const std::string& value) {
    if (value.empty() || value.size() > 64) return false;
    for (unsigned char c : value) {
        const bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                        (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.';
        if (!ok) return false;
    }
    return true;
}

std::string escape(const std::string& value) {
    static const char hex[] = "0123456789ABCDEF";
    std::string out;
    out.reserve(value.size());
    for (unsigned char c : value) {
        if (c == '%' || c == '\t' || c == '\n' || c == '\r') {
            out.push_back('%');
            out.push_back(hex[c >> 4]);
            out.push_back(hex[c & 0x0F]);
        } else {
            out.push_back(static_cast<char>(c));
        }
    }
    return out;
}

int hex_value(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}

bool unescape(const std::string& value, std::string* out) {
    out->clear();
    out->reserve(value.size());
    for (std::size_t i = 0; i < value.size(); ++i) {
        if (value[i] != '%') {
            out->push_back(value[i]);
            continue;
        }
        if (i + 2 >= value.size()) return false;
        const int hi = hex_value(value[i + 1]);
        const int lo = hex_value(value[i + 2]);
        if (hi < 0 || lo < 0) return false;
        out->push_back(static_cast<char>((hi << 4) | lo));
        i += 2;
    }
    return true;
}

std::vector<std::string> split_tabs(const std::string& line) {
    std::vector<std::string> fields;
    std::size_t start = 0;
    for (;;) {
        const std::size_t end = line.find('\t', start);
        fields.push_back(line.substr(start, end == std::string::npos ? end : end - start));
        if (end == std::string::npos) break;
        start = end + 1;
    }
    return fields;
}

bool parse_float(const std::string& text, float* value) {
    if (text.empty()) return false;
    errno = 0;
    char* end = nullptr;
    const float parsed = std::strtof(text.c_str(), &end);
    if (errno == ERANGE || end == text.c_str() || *end != '\0' || !std::isfinite(parsed)) return false;
    *value = parsed;
    return true;
}

std::string number(float value) {
    std::ostringstream out;
    out << std::setprecision(9) << value;
    return out.str();
}

void set_error(std::string* error, const char* message) {
    if (error) *error = message;
}

}  // namespace

const char* kind_name(ElementKind kind) {
    switch (kind) {
        case ElementKind::Prop: return "prop";
        case ElementKind::Chair: return "chair";
        case ElementKind::DeathBlock: return "death_block";
        case ElementKind::Message: return "message";
        case ElementKind::Trigger: return "trigger";
        case ElementKind::Animation: return "animation";
    }
    return "unknown";
}

bool parse_kind(const std::string& name, ElementKind* kind) {
    if (!kind) return false;
    if (name == "prop") *kind = ElementKind::Prop;
    else if (name == "chair") *kind = ElementKind::Chair;
    else if (name == "death_block") *kind = ElementKind::DeathBlock;
    else if (name == "message") *kind = ElementKind::Message;
    else if (name == "trigger") *kind = ElementKind::Trigger;
    else if (name == "animation") *kind = ElementKind::Animation;
    else return false;
    return true;
}

bool validate(const Document& document, std::string* error) {
    if (!valid_id(document.id)) {
        set_error(error, "Map ID must use 1-64 letters, numbers, dots, dashes, or underscores.");
        return false;
    }
    if (document.title.empty() || document.title.size() > 80) {
        set_error(error, "Map title must contain 1-80 characters.");
        return false;
    }
    if (document.description.size() > 1000) {
        set_error(error, "Map description is longer than 1000 characters.");
        return false;
    }
    if (document.elements.size() > kMaxElements) {
        set_error(error, "A map can contain at most 2000 elements.");
        return false;
    }

    std::set<std::string> ids;
    for (const Element& element : document.elements) {
        if (!valid_id(element.id) || !ids.insert(element.id).second) {
            set_error(error, "Every element needs a unique, valid ID.");
            return false;
        }
        if (!std::isfinite(element.x) || !std::isfinite(element.y) ||
            !std::isfinite(element.z) || std::fabs(element.x) > 5000.0f ||
            std::fabs(element.y) > 5000.0f || std::fabs(element.z) > 5000.0f ||
            !std::isfinite(element.yaw) || std::fabs(element.yaw) > 360000.0f) {
            set_error(error, "An element has an invalid position or rotation.");
            return false;
        }
        if (!std::isfinite(element.scale_x) || !std::isfinite(element.scale_y) ||
            !std::isfinite(element.scale_z) || element.scale_x < 0.05f ||
            element.scale_y < 0.05f || element.scale_z < 0.05f ||
            element.scale_x > 100.0f || element.scale_y > 100.0f || element.scale_z > 100.0f) {
            set_error(error, "Element scale must be between 0.05 and 100.");
            return false;
        }
        if (element.prefab.size() > 128 || element.text.size() > 1000 ||
            element.event_key.size() > 64 || element.animation_key.size() > 64) {
            set_error(error, "An element contains a field that is too long.");
            return false;
        }
        if ((element.kind == ElementKind::Message && element.text.empty()) ||
            (element.kind == ElementKind::Trigger && element.event_key.empty()) ||
            (element.kind == ElementKind::Animation && element.animation_key.empty())) {
            set_error(error, "Messages, triggers, and animations need their matching text or key.");
            return false;
        }
    }
    if (error) error->clear();
    return true;
}

std::string encode(const Document& document) {
    if (!validate(document, nullptr)) return {};
    std::ostringstream out;
    out << "SBA_MAP\t1\n";
    out << "META\t" << escape(document.id) << '\t' << escape(document.title) << '\t'
        << escape(document.description) << '\n';
    for (const Element& e : document.elements) {
        out << "ELEMENT\t" << escape(e.id) << '\t' << kind_name(e.kind) << '\t'
            << escape(e.prefab) << '\t' << number(e.x) << '\t' << number(e.y) << '\t'
            << number(e.z) << '\t' << number(e.yaw) << '\t' << number(e.scale_x) << '\t'
            << number(e.scale_y) << '\t' << number(e.scale_z) << '\t' << escape(e.text) << '\t'
            << escape(e.event_key) << '\t' << escape(e.animation_key) << '\n';
    }
    return out.str();
}

bool decode(const std::string& data, Document* document, std::string* error) {
    if (!document) {
        set_error(error, "No map output was provided.");
        return false;
    }
    if (data.size() > kMaxDocumentBytes) {
        set_error(error, "Map file is too large.");
        return false;
    }

    Document parsed;
    bool has_header = false;
    bool has_meta = false;
    std::istringstream input(data);
    std::string line;
    std::size_t line_number = 0;
    while (std::getline(input, line)) {
        ++line_number;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.size() > kMaxLineBytes) {
            set_error(error, "Map file contains an oversized line.");
            return false;
        }
        if (line.empty()) continue;
        const std::vector<std::string> fields = split_tabs(line);
        if (!has_header) {
            if (fields.size() != 2 || fields[0] != "SBA_MAP" || fields[1] != "1") {
                set_error(error, "Unsupported map file format.");
                return false;
            }
            has_header = true;
            continue;
        }
        if (fields[0] == "META") {
            if (has_meta || fields.size() != 4 ||
                !unescape(fields[1], &parsed.id) || !unescape(fields[2], &parsed.title) ||
                !unescape(fields[3], &parsed.description)) {
                set_error(error, "Invalid map metadata.");
                return false;
            }
            has_meta = true;
            continue;
        }
        if (fields[0] != "ELEMENT" || !has_meta || fields.size() != 14 ||
            parsed.elements.size() >= kMaxElements) {
            set_error(error, "Invalid map element or element limit exceeded.");
            return false;
        }
        Element e;
        if (!unescape(fields[1], &e.id) || !parse_kind(fields[2], &e.kind) ||
            !unescape(fields[3], &e.prefab) || !parse_float(fields[4], &e.x) ||
            !parse_float(fields[5], &e.y) || !parse_float(fields[6], &e.z) ||
            !parse_float(fields[7], &e.yaw) || !parse_float(fields[8], &e.scale_x) ||
            !parse_float(fields[9], &e.scale_y) || !parse_float(fields[10], &e.scale_z) ||
            !unescape(fields[11], &e.text) || !unescape(fields[12], &e.event_key) ||
            !unescape(fields[13], &e.animation_key)) {
            set_error(error, "Invalid element data in map file.");
            return false;
        }
        parsed.elements.push_back(std::move(e));
    }

    if (!has_header || !has_meta) {
        set_error(error, "Map file is missing its header or metadata.");
        return false;
    }
    if (!validate(parsed, error)) return false;
    *document = std::move(parsed);
    return true;
}

}  // namespace sba::costume_map
