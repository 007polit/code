#pragma once
#include <stack>

namespace editer {
namespace ui {

// Focus targets in the application
enum class FocusTarget {
    Editor,
    FileTree,
    Terminal,
    AIChat
};

// Manages focus stack for different UI components
class FocusManager {
private:
    std::stack<FocusTarget> focus_stack_;

public:
    FocusManager() {
        // Initialize with editor focus
        focus_stack_.push(FocusTarget::Editor);
    }

    // Push new focus target
    void push(FocusTarget target) {
        focus_stack_.push(target);
    }

    // Pop current focus
    void pop() {
        if (!focus_stack_.empty()) {
            focus_stack_.pop();
        }
        // Ensure editor is always at bottom
        if (focus_stack_.empty()) {
            focus_stack_.push(FocusTarget::Editor);
        }
    }

    // Get current focus
    FocusTarget current() const {
        if (focus_stack_.empty()) {
            return FocusTarget::Editor;
        }
        return focus_stack_.top();
    }

    // Check if specific target has focus
    bool has_focus(FocusTarget target) const {
        return current() == target;
    }

    // Clear all and reset to editor
    void reset() {
        while (!focus_stack_.empty()) {
            focus_stack_.pop();
        }
        focus_stack_.push(FocusTarget::Editor);
    }
};

} // namespace ui
} // namespace editer
