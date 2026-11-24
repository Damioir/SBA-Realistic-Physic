#include "editor_session.h"

#include <cmath>
#include <utility>

namespace sba::costume_map {
namespace {

Element* find_element(Document* document, const std::string& id) {
    if (!document) return nullptr;
    for (Element& element : document->elements) {
        if (element.id == id) return &element;
    }
    return nullptr;
}

bool contains_id(const Document& document, const std::string& id) {
    for (const Element& element : document.elements) {
        if (element.id == id) return true;
    }
    return false;
}

float snapped(float value, float step) {
    return step > 0.0f ? std::round(value / step) * step : value;
}

}  // namespace

EditorSession::EditorSession(Document document) : document_(std::move(document)) {
    std::string error;
    if (!validate(document_, &error)) {
        document_ = Document{};
        document_.id = "new-map";
        document_.title = "New Map";
    }
}

bool EditorSession::select(const std::string& id) {
    if (id.empty()) {
        selected_id_.clear();
        return true;
    }
    if (!contains_id(document_, id)) return false;
    selected_id_ = id;
    return true;
}

bool EditorSession::add(Element element, std::string* error) {
    Document next = document_;
    next.elements.push_back(std::move(element));
    if (!validate(next, error)) return false;
    const std::string new_id = next.elements.back().id;
    if (!commit(std::move(next))) return false;
    selected_id_ = new_id;
    if (error) error->clear();
    return true;
}

bool EditorSession::erase_selected() {
    if (selected_id_.empty()) return false;
    Document next = document_;
    for (auto it = next.elements.begin(); it != next.elements.end(); ++it) {
        if (it->id == selected_id_) {
            next.elements.erase(it);
            if (!commit(std::move(next))) return false;
            selected_id_.clear();
            return true;
        }
    }
    selected_id_.clear();
    return false;
}

bool EditorSession::move_selected(float x, float y, float z) {
    if (!selected_element()) return false;
    Document next = document_;
    Element* element = find_element(&next, selected_id_);
    element->x = snapped(x, grid_step_);
    element->y = snapped(y, grid_step_);
    element->z = snapped(z, grid_step_);
    return commit(std::move(next));
}

bool EditorSession::rotate_selected(float yaw) {
    if (!selected_element()) return false;
    Document next = document_;
    find_element(&next, selected_id_)->yaw = yaw;
    return commit(std::move(next));
}

bool EditorSession::scale_selected(float x, float y, float z) {
    if (!selected_element()) return false;
    Document next = document_;
    Element* element = find_element(&next, selected_id_);
    element->scale_x = x;
    element->scale_y = y;
    element->scale_z = z;
    return commit(std::move(next));
}

void EditorSession::set_grid_snap(float step) {
    if (std::isfinite(step) && step >= 0.0f && step <= 100.0f) grid_step_ = step;
}

bool EditorSession::undo() {
    if (undo_.empty()) return false;
    redo_.push_back(std::move(document_));
    document_ = std::move(undo_.back());
    undo_.pop_back();
    keep_selection_if_present();
    return true;
}

bool EditorSession::redo() {
    if (redo_.empty()) return false;
    undo_.push_back(std::move(document_));
    document_ = std::move(redo_.back());
    redo_.pop_back();
    keep_selection_if_present();
    return true;
}

void EditorSession::clear_history() {
    undo_.clear();
    redo_.clear();
}

bool EditorSession::commit(Document next) {
    if (!validate(next, nullptr)) return false;
    undo_.push_back(document_);
    if (undo_.size() > kHistoryLimit) undo_.erase(undo_.begin());
    redo_.clear();
    document_ = std::move(next);
    keep_selection_if_present();
    return true;
}

Element* EditorSession::selected_element() {
    return selected_id_.empty() ? nullptr : find_element(&document_, selected_id_);
}

void EditorSession::keep_selection_if_present() {
    if (!selected_id_.empty() && !contains_id(document_, selected_id_)) selected_id_.clear();
}

}  // namespace sba::costume_map
