#include "map_storage.h"

#include <cstdio>
#include <fstream>
#include <iterator>

namespace sba::costume_map {
namespace {

constexpr std::size_t kMaxMapBytes = 8 * 1024 * 1024;

void set_error(std::string* error, const char* message) {
    if (error) *error = message;
}

}  // namespace

bool save_file(const std::string& path, const Document& document, std::string* error) {
    if (path.empty()) {
        set_error(error, "Map file path is empty.");
        return false;
    }
    const std::string encoded = encode(document);
    if (encoded.empty() || encoded.size() > kMaxMapBytes) {
        set_error(error, "Map is invalid or too large to save.");
        return false;
    }

    const std::string temporary_path = path + ".tmp";
    {
        std::ofstream output(temporary_path, std::ios::binary | std::ios::trunc);
        if (!output) {
            set_error(error, "Could not open the temporary map file.");
            return false;
        }
        output.write(encoded.data(), static_cast<std::streamsize>(encoded.size()));
        output.flush();
        if (!output) {
            output.close();
            std::remove(temporary_path.c_str());
            set_error(error, "Could not write the map file.");
            return false;
        }
    }

    if (std::rename(temporary_path.c_str(), path.c_str()) != 0) {
        std::remove(temporary_path.c_str());
        set_error(error, "Could not replace the saved map file.");
        return false;
    }
    if (error) error->clear();
    return true;
}

bool load_file(const std::string& path, Document* document, std::string* error) {
    if (path.empty() || !document) {
        set_error(error, "Map file path or output is missing.");
        return false;
    }

    std::ifstream input(path, std::ios::binary);
    if (!input) {
        set_error(error, "Could not open the map file.");
        return false;
    }
    input.seekg(0, std::ios::end);
    const std::streamoff size = input.tellg();
    if (size < 0 || static_cast<std::uint64_t>(size) > kMaxMapBytes) {
        set_error(error, "Map file is too large.");
        return false;
    }
    input.seekg(0, std::ios::beg);
    const std::string data((std::istreambuf_iterator<char>(input)),
                           std::istreambuf_iterator<char>());
    if (data.size() != static_cast<std::size_t>(size)) {
        set_error(error, "Could not read the complete map file.");
        return false;
    }
    return decode(data, document, error);
}

}  // namespace sba::costume_map
