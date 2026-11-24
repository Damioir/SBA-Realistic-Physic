#pragma once

#include "costume_map.h"

#include <string>

namespace sba::costume_map {

// The caller supplies a file path in the mod's own app data directory.
bool save_file(const std::string& path, const Document& document, std::string* error);
bool load_file(const std::string& path, Document* document, std::string* error);

}  // namespace sba::costume_map
