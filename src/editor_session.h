#pragma once

#include "costume_map.h"

#include <cstddef>
#include <string>
#include <vector>

namespace sba::costume_map {

// In-memory editing operations shared by the future editor UI and runtime
// adapter. This class does not create Unity objects or touch game saves.
class EditorSession {
public:
    explicit EditorSession(Document document);

    const Document& document() const { return document_; }
    const std::string& selected_id() const { return selected_id_; }

    bool select(const std::string& id);
    bool add(Element element, std::string* error);
    bool erase_selected();
    bool move_selected(float x, float y, float z);
    bool rotate_selected(float yaw);
    bool scale_selected(float x, float y, float z);
    void set_grid_snap(float step);

    bool undo();
    bool redo();
    void clear_history();

private:
    bool commit(Document next);
    Element* selected_element();
    void keep_selection_if_present();

    Document document_;
    std::string selected_id_;
    float grid_step_ = 0.5f;
    std::vector<Document> undo_;
    std::vector<Document> redo_;
    static constexpr std::size_t kHistoryLimit = 32;
};

}  // namespace sba::costume_map
