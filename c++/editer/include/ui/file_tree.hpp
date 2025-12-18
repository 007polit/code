#pragma once
#include <string>
#include <vector>
#include <memory>
#include <filesystem>
#include <curses.h>

namespace editer {

// File tree node structure
struct TreeNode {
    std::string name;                                    // File/directory name
    std::string full_path;                              // Full path
    bool is_directory;                                  // Whether it's a directory
    bool is_expanded = false;                           // Whether expanded (directories only)
    bool children_loaded = false;                       // Whether children are loaded
    TreeNode* parent = nullptr;                         // Parent node pointer
    std::vector<std::unique_ptr<TreeNode>> children;    // Child nodes
    
    TreeNode(const std::string& name, const std::string& path, bool is_dir)
        : name(name), full_path(path), is_directory(is_dir) {}
};

// File tree manager class
class FileTree {
private:
    std::unique_ptr<TreeNode> root_;                    // Root node
    TreeNode* selected_node_ = nullptr;                 // Currently selected node
    std::vector<TreeNode*> visible_nodes_;              // Currently visible nodes list (flattened for rendering)
    std::vector<int> node_depths_;                      // Depth for each visible node
    int selected_index_ = 0;                            // Selected node index in visible list
    int scroll_offset_ = 0;                             // Scroll offset
    
    // UI related
    WINDOW* window_ = nullptr;                          // File tree window
    int width_ = 30;                                    // Fixed width
    int height_ = 0;                                    // Window height

public:
    enum class FileOperation {                          // Type of file operation
        None,
        CreateFile,
        CreateFolder,
        Rename,
        Delete,
        Cut,
        Copy,
        Paste
    };

private:
    // File opening state - parts you need to implement
    // TODO: Add variables to track file opening requests
    bool file_to_open_ = false;                         // Flag indicating file should be opened
    std::string file_path_to_open_;                     // Path of file to open
    
    // File management state - parts you need to implement
    // TODO: Add variables to track file management operations
    bool show_input_dialog_ = false;                    // Flag indicating input dialog should be shown
    std::string input_dialog_prompt_;                   // Prompt text for input dialog
    FileOperation pending_operation_ = FileOperation::None;
    
    // Clipboard state - parts you need to implement
    // TODO: Add variables to track clipboard operations
    std::string clipboard_path_;                        // Path of item in clipboard
    bool clipboard_is_cut_ = false;                     // True if cut, false if copy
    bool has_clipboard_item_ = false;                   // True if clipboard has item
    
    // Refresh request flag
    bool needs_full_refresh_ = false;                   // Request full refresh from main app
    bool needs_layout_update_ = false;                // Request layout update for width change

public:
    explicit FileTree(const std::string& root_path);
    ~FileTree();
    
    // Initialization and cleanup
    bool init(int height, int width, int start_y, int start_x);
    void cleanup();
    
    // Core functionality - parts you need to implement
    void load_directory_children(TreeNode* node);      // TODO: Load directory children
    void refresh_visible_nodes();                      // TODO: Refresh visible nodes list
    void expand_node(TreeNode* node);                  // TODO: Expand node
    void collapse_node(TreeNode* node);                // TODO: Collapse node
    
    // Navigation functionality - parts you need to implement
    void move_up();                                     // TODO: Move selection up
    void move_down();                                   // TODO: Move selection down
    void toggle_selected();                             // TODO: Toggle selected item expand/collapse
    std::string get_selected_file_path() const;        // TODO: Get selected file path
    
    // Rendering functionality - parts you need to implement
    void render();                                      // TODO: Render file tree
    void render_node(TreeNode* node, int y, int depth); // TODO: Render single node
    
    // Input handling - parts you need to implement
    bool handle_input(int key);                         // TODO: Handle keyboard input (returns true if should close)
    
    // File operations - parts you need to implement
    bool should_open_file() const;                      // TODO: Check if a file should be opened
    std::string get_file_to_open() const;               // TODO: Get the path of file to open
    void clear_file_open_request();                     // TODO: Clear file open request
    
    // File management operations - parts you need to implement
    bool create_new_file(const std::string& filename);  // TODO: Create new file in selected directory
    bool create_new_folder(const std::string& foldername); // TODO: Create new folder in selected directory
    bool delete_selected_item();                        // TODO: Delete selected file or folder
    bool rename_selected_item(const std::string& new_name); // TODO: Rename selected file or folder
    
    // File move operations - parts you need to implement
    bool cut_selected_item();                           // TODO: Cut selected file/folder to clipboard
    bool copy_selected_item();                          // TODO: Copy selected file/folder to clipboard
    bool paste_clipboard_item();                        // TODO: Paste clipboard item to current directory
    bool has_clipboard_item() const;                    // TODO: Check if clipboard has item
    
    // File management state - parts you need to implement
    bool should_show_input_dialog() const;              // TODO: Check if input dialog should be shown
    std::string get_input_dialog_prompt() const;        // TODO: Get input dialog prompt text
    void clear_input_dialog_request();                  // TODO: Clear input dialog request
    FileOperation get_pending_operation() const;        // TODO: Get current pending operation type
    
    // Refresh request - parts you need to implement
    bool needs_full_refresh() const { return needs_full_refresh_; }
    void clear_refresh_request() { needs_full_refresh_ = false; }
    
    // Layout update request for when content changes affect optimal width
    bool needs_layout_update() const { return needs_layout_update_; }
    void clear_layout_update_request() { needs_layout_update_ = false; }
    
    // Helper functions
    bool is_visible() const { return window_ != nullptr; }
    int get_width() const { return width_; }
    int calculate_optimal_width() const;                 // Calculate optimal width based on content
    
private:
    // Color pairs enumeration
    enum ColorPairs {
        COLOR_DIRECTORY = 1,
        COLOR_FILE = 2,
        COLOR_SELECTED = 3,
        COLOR_CPP_FILE = 4,
        COLOR_HEADER_FILE = 5,
        COLOR_TEXT_FILE = 6,
        COLOR_EXECUTABLE = 7,
        COLOR_HIDDEN = 8,
        COLOR_PYTHON_FILE = 9,
        COLOR_JSON_FILE = 10,

    };

    // Helper methods - parts you need to implement
    void build_visible_list(TreeNode* node, int depth = 0);  // TODO: Build visible nodes list
    std::string get_node_display_text(TreeNode* node, int depth) const; // TODO: Get node display text
    void ensure_selected_visible();                     // TODO: Ensure selected item is visible
    TreeNode* find_node_by_path(TreeNode* node, const std::string& path); // Find node by path
    
    // Color related methods
    void init_colors();
    int get_node_color(TreeNode* node) const;
};

} // namespace editer
