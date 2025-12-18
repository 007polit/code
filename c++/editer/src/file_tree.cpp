#include "ui/file_tree.hpp"
#include <algorithm>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <set>
#include <thread>
#include <chrono>

namespace editer {

FileTree::FileTree(const std::string& root_path) {
    // TODO: Initialize root node
    // Hint: Create root node, set basic properties, load first level children
    root_ = std::make_unique<TreeNode>(root_path, root_path, true);
    load_directory_children(root_.get());
    refresh_visible_nodes();
    selected_node_ = root_.get();
    selected_index_ = 0;
    scroll_offset_ = 0;

}

FileTree::~FileTree() {
    cleanup();
}

bool FileTree::init(int height, int width, int start_y, int start_x) {
    height_ = height;
    width_ = width;
    
    // Create ncurses window
    window_ = newwin(height, width, start_y, start_x);
    if (!window_) {
        return false;
    }
    
    keypad(window_, TRUE);
    
    // Initialize colors
    init_colors();
    
    // Refresh visible nodes to ensure width calculation is accurate
    refresh_visible_nodes();
    
    return true;
}

void FileTree::cleanup() {
    if (window_) {
        delwin(window_);
        window_ = nullptr;
    }
}

void FileTree::load_directory_children(TreeNode* node) {
    if (!node || !node->is_directory) {
        return;
    }
    
    // Save expanded state of existing children
    std::set<std::string> expanded_children;
    for (const auto& child : node->children) {
        if (child->is_directory && child->is_expanded) {
            expanded_children.insert(child->name);
        }
    }
    
    // Clear existing children to reload fresh content
    node->children.clear();
    
    try {
        for (const auto& entry : std::filesystem::directory_iterator(node->full_path)) {
            auto child = std::make_unique<TreeNode>(
                entry.path().filename().string(), 
                entry.path().string(), 
                entry.is_directory()
            );
            // Set parent pointer for proper tree structure
            child->parent = node;
            
            // Restore expanded state if it was previously expanded
            if (child->is_directory && expanded_children.count(child->name)) {
                child->is_expanded = true;
                child->children_loaded = true; // Mark as loaded to avoid reloading
                load_directory_children(child.get()); // Recursively load expanded children
            }
            
            node->children.push_back(std::move(child));
        }
    } catch (const std::filesystem::filesystem_error&) {
        // Handle permission errors or invalid paths
        return;
    }
    
    // Sort children: directories first, then files, both alphabetically
    std::sort(node->children.begin(), node->children.end(), 
        [](const std::unique_ptr<TreeNode>& a, const std::unique_ptr<TreeNode>& b) {
            // Directories come first
            if (a->is_directory != b->is_directory) {
                return a->is_directory > b->is_directory;
            }
            // Within same type, sort alphabetically
            return a->name < b->name;
        });
    
    node->children_loaded = true;
}

void FileTree::refresh_visible_nodes() {
    // TODO: Refresh visible nodes list
    // Hints:
    // 1. Clear visible_nodes_
    // 2. Recursively build visible list starting from root
    // 3. Only include children of expanded nodes
    
    visible_nodes_.clear();
    node_depths_.clear();
    if (root_) {
        build_visible_list(root_.get());
    }

}

void FileTree::build_visible_list(TreeNode* node, int depth) {
    // TODO: Recursively build visible nodes list
    // Hints:
    // 1. Add current node to visible_nodes_
    // 2. If node is expanded, recursively process children
    
    visible_nodes_.push_back(node);
    node_depths_.push_back(depth);
    if (node->is_expanded){
        for(const auto& child : node->children){
            build_visible_list(child.get(), depth + 1);
        }
    }
    
}

void FileTree::expand_node(TreeNode* node) {
    // TODO: Expand node
    // Hints:
    // 1. Check if it's a directory
    // 2. Load children (if not loaded)
    // 3. Set is_expanded = true
    // 4. Refresh visible nodes list
    
    if (!node || !node->is_directory) return;
    load_directory_children(node);
    node->is_expanded = true;
    refresh_visible_nodes();
    
    // Request layout update since visible content changed
    needs_layout_update_ = true;
}

void FileTree::collapse_node(TreeNode* node) {
    // TODO: Collapse node
    // Hints:
    // 1. Set is_expanded = false
    // 2. Refresh visible nodes list
    
    if (!node || !node->is_directory) return;
    node->is_expanded = false;
    refresh_visible_nodes();
    
    // Request layout update since visible content changed
    needs_layout_update_ = true;
}

void FileTree::move_up() {
    // TODO: Move selection up
    // Hints:
    // 1. Decrease selected_index_
    // 2. Boundary check
    // 3. Update selected_node_
    // 4. Ensure selected item is visible
    
    if (!visible_nodes_.empty() && selected_index_ > 0) {
        selected_index_--;
        selected_node_ = visible_nodes_[selected_index_];
        ensure_selected_visible();
    }
    
}

void FileTree::move_down() {
    // TODO: Move selection down
    // Hints:
    // 1. Increase selected_index_
    // 2. Boundary check
    // 3. Update selected_node_
    // 4. Ensure selected item is visible
    
    if (!visible_nodes_.empty() && selected_index_ < visible_nodes_.size() - 1) {
        selected_index_++;
        selected_node_ = visible_nodes_[selected_index_];
        ensure_selected_visible();
    }
    
}

void FileTree::toggle_selected() {
    // TODO: Toggle selected item expand/collapse
    // Hints:
    // 1. Check if selected node is a directory
    // 2. Decide expand or collapse based on is_expanded state
    
    if (selected_node_ && selected_node_->is_directory) {
        if (selected_node_->is_expanded) {
            collapse_node(selected_node_);
        } else {
            expand_node(selected_node_);
        }
    }
    
}

std::string FileTree::get_selected_file_path() const {
    // TODO: Get selected file path
    // Hints:
    // 1. Check if selected_node_ is valid
    // 2. If it's a file, return full_path
    // 3. If it's a directory, return empty string
    
    if (selected_node_ && !selected_node_->is_directory) {
        return selected_node_->full_path;
    }
    return "";
}

void FileTree::render() {
    // TODO: Render file tree
    // Hints:
    // 1. Clear window
    // 2. Draw border and title
    // 3. Traverse visible nodes and render
    // 4. Handle scrolling and highlighting
    // 5. Refresh window
    
    if (!window_) return;
    
    werase(window_);
    box(window_, 0, 0);
    
    // Title
    mvwprintw(window_, 0, 2, " Files ");
    
    for (size_t i = scroll_offset_; i < visible_nodes_.size() && i < scroll_offset_ + height_ - 2; ++i) {
        int y = i - scroll_offset_ + 1;
        int depth = node_depths_[i];
        render_node(visible_nodes_[i], y, depth);
    }
    
    wrefresh(window_);
}

void FileTree::render_node(TreeNode* node, int y, int depth) {
    // TODO: Render single node
    // Hints:
    // 1. Calculate indentation
    // 2. Choose appropriate icon (folder/file)
    // 3. Handle selection highlighting
    // 4. Draw to specified position
    
    std::string line = get_node_display_text(node, depth);
    
    // Select color
    int color_pair = get_node_color(node);
    
    if (node == selected_node_) {
        // Selected item uses special color
        wattron(window_, COLOR_PAIR(COLOR_SELECTED));
    } else {
        // Use different colors based on file type
        wattron(window_, COLOR_PAIR(color_pair));
    }
    
    // Ensure text doesn't exceed window boundaries
    mvwprintw(window_, y, 1, "%-*s", width_ - 2, line.c_str());
    
    // Turn off color attributes
    if (node == selected_node_) {
        wattroff(window_, COLOR_PAIR(COLOR_SELECTED));
    } else {
        wattroff(window_, COLOR_PAIR(color_pair));
    }
    
}

std::string FileTree::get_node_display_text(TreeNode* node, int depth) const {
    // TODO: Get node display text
    // Hints:
    // 1. Add indentation based on depth
    // 2. Add expand/collapse icon
    // 3. Add file/directory icon
    // 4. Add node name
    
    std::string result;
    
    // Add indentation
    for (int i = 0; i < depth; ++i) {
        result += "  ";  // 2 spaces per level
    }
    
    // Add expand/collapse indicator for directories
    if (node->is_directory) {
        if (node->is_expanded) {
            result += "[-] ";  // Expanded directory
        } else {
            result += "[+] ";  // Collapsed directory
        }
    } else {
        result += "    ";  // File (4 spaces to align with directories)
    }
    
    // Add name
    result += node->name;
    
    return result;
}

bool FileTree::handle_input(int key) {
    // TODO: Handle keyboard input
    // Hints:
    // 1. Arrow keys or hjkl for navigation
    // 2. Enter/Space for expand/collapse
    // 3. Other shortcuts
    
    switch (key) {
        case KEY_UP:
        case 'k':
            move_up();
            break;
            
        case KEY_DOWN:
        case 'j':
            move_down();
            break;
            
        case KEY_ENTER:
        case '\r':
        case '\n':
            // TODO: Handle Enter key
            // Hints:
            // 1. Check if selected_node_ is valid
            // 2. If it's a directory, call toggle_selected()
            // 3. If it's a file, set file opening request
            if (selected_node_){
                if (selected_node_ -> is_directory){
                    toggle_selected();
                }
                else{
                    file_to_open_ = true;
                    file_path_to_open_ = selected_node_ -> full_path;
                }
            }

            break;
            
        case ' ':
            toggle_selected();
            break;
            
        // File management shortcuts - parts you need to implement
        case 'n': // Create new file
            // TODO: Implement new file creation
            // Hints:
            // 1. Set pending_operation_ to CreateFile
            // 2. Set input_dialog_prompt_ to appropriate message
            // 3. Set show_input_dialog_ to true
            pending_operation_ = FileOperation::CreateFile;
            input_dialog_prompt_ = "Enter file name: ";
            show_input_dialog_ = true;
            break;
            
        case 'N': // Create new folder
            // TODO: Implement new folder creation
            // Hints:
            // 1. Set pending_operation_ to CreateFolder
            // 2. Set input_dialog_prompt_ to appropriate message
            // 3. Set show_input_dialog_ to true
            pending_operation_ = FileOperation::CreateFolder;
            input_dialog_prompt_ = "Enter folder name: ";
            show_input_dialog_ = true;
            break;
            
        case 'd': // Delete selected item
            // TODO: Implement delete functionality
            // Hints:
            // 1. Check if selected_node_ is valid
            // 2. Set pending_operation_ to Delete
            // 3. Set input_dialog_prompt_ to confirmation message
            // 4. Set show_input_dialog_ to true
            pending_operation_ = FileOperation::Delete;
            input_dialog_prompt_ = "Are you sure you want to delete this item? (y/n)";
            show_input_dialog_ = true;
            break;
            
        case 'r': // Rename selected item
            // TODO: Implement rename functionality
            // Hints:
            // 1. Check if selected_node_ is valid
            // 2. Set pending_operation_ to Rename
            // 3. Set input_dialog_prompt_ to appropriate message
            // 4. Set show_input_dialog_ to true
            pending_operation_ = FileOperation::Rename;
            input_dialog_prompt_ = "Enter new name: ";
            show_input_dialog_ = true;
            break;
            
        // File move shortcuts - parts you need to implement
        case 'x': // Cut selected item
            // TODO: Implement cut functionality
            // Hints:
            // 1. Check if selected_node_ is valid
            // 2. Call cut_selected_item()
            // 3. Show success message (optional)
            if (selected_node_){
                cut_selected_item();
                // renderer_->show_message("Item cut successfully");
            }
            break;
            
        case 'c': // Copy selected item
            // TODO: Implement copy functionality
            // Hints:
            // 1. Check if selected_node_ is valid
            // 2. Call copy_selected_item()
            // 3. Show success message (optional)
            if (selected_node_){
                copy_selected_item();
                // renderer_->show_message("Item copied successfully");
            }
            break;
            
        case 'v': // Paste clipboard item
            // TODO: Implement paste functionality
            // Hints:
            // 1. Check if clipboard has item with has_clipboard_item()
            // 2. Call paste_clipboard_item()
            // 3. Show success/error message
            if (has_clipboard_item()){
                paste_clipboard_item();
                // renderer_->show_message("Item pasted successfully");
            }
            break;
            
        case 27: // ESC key - exit file tree
        case 'q': // q key - exit file tree  
            return true; // Signal to close file tree
            
        default:
            break;
    }
    
    return false; // Continue showing file tree
}

void FileTree::ensure_selected_visible() {
    // TODO: Ensure selected item is in visible area
    // Hints:
    // 1. Calculate visible area range
    // 2. Adjust scroll_offset_
    
    if (selected_index_ < scroll_offset_) {
        scroll_offset_ = selected_index_;
    } else if (selected_index_ >= scroll_offset_ + height_) {
        scroll_offset_ = selected_index_ - height_ + 1;
    }
    
}

void FileTree::init_colors() {
    if (has_colors()) {
        init_pair(COLOR_DIRECTORY, COLOR_BLUE, -1);      // Blue directories
        init_pair(COLOR_FILE, COLOR_WHITE, -1);          // White regular files
        init_pair(COLOR_SELECTED, COLOR_BLACK, COLOR_CYAN); // Cyan background for selected items
        init_pair(COLOR_CPP_FILE, COLOR_GREEN, -1);      // Green C++ source files
        init_pair(COLOR_HEADER_FILE, COLOR_YELLOW, -1);  // Yellow header files
        init_pair(COLOR_TEXT_FILE, COLOR_MAGENTA, -1);   // Magenta text files
        init_pair(COLOR_EXECUTABLE, COLOR_RED, -1);      // Red executable files
        init_pair(COLOR_HIDDEN, COLOR_BLACK, -1);        // Black hidden files
        init_pair(COLOR_PYTHON_FILE, COLOR_CYAN, -1);    // Cyan Python files
        init_pair(COLOR_JSON_FILE, COLOR_WHITE, -1);     // White JSON files
       
    }
}

int FileTree::get_node_color(TreeNode* node) const {
    if (node->is_directory) {
        return COLOR_DIRECTORY;
    }
    
    std::string name = node->name;
    
    // Hidden files (starting with .)
    if (!name.empty() && name[0] == '.') {
        return COLOR_HIDDEN;
    }
    
    // Get file extension
    size_t dot_pos = name.find_last_of('.');
    if (dot_pos != std::string::npos) {
        std::string ext = name.substr(dot_pos);
        
        // C++ source files
        if (ext == ".cpp" || ext == ".cc" || ext == ".cxx" || ext == ".c") {
            return COLOR_CPP_FILE;
        }
        // Header files
        else if (ext == ".h" || ext == ".hpp" || ext == ".hxx") {
            return COLOR_HEADER_FILE;
        }
        // Text files
        else if (ext == ".txt" || ext == ".md" || ext == ".readme") {
            return COLOR_TEXT_FILE;
        }
        // Executable files
        else if (ext == ".exe" || ext == ".bat" || ext == ".cmd") {
            return COLOR_EXECUTABLE;
        }
        
        // Python files
        else if (ext == ".py" || ext == ".pyw") {
            return COLOR_PYTHON_FILE;
        }
        
        // JSON files
        else if (ext == ".json") {
            return COLOR_JSON_FILE;
        }
        
    }
    
    return COLOR_FILE;
}

bool FileTree::should_open_file() const {
    // TODO: Implement file opening check
    // Hints:
    // 1. Return true if a file should be opened
    // 2. Check your file opening state variables

    if (file_to_open_){
        return true;
    }
    return false;
}

std::string FileTree::get_file_to_open() const {
    // TODO: Implement get file path to open
    // Hints:
    // 1. Return the path of the file that should be opened
    // 2. Return empty string if no file to open
    if (file_to_open_){
        return file_path_to_open_;
    }
    return "";
}

void FileTree::clear_file_open_request() {
    // TODO: Implement clear file open request
    // Hints:
    // 1. Reset file opening state variables
    // 2. Clear any stored file paths
    file_to_open_ = false;
    file_path_to_open_ = "";
}

// File management operations - parts you need to implement
bool FileTree::create_new_file(const std::string& filename) {
    if (!selected_node_) return false;
    
    // Get the target directory
    TreeNode* target_dir = nullptr;
    if (selected_node_->is_directory) {
        target_dir = selected_node_;
    } else {
        // Find parent directory
        target_dir = selected_node_->parent;
    }
    
    if (!target_dir) return false;
    
    // Create full path
    std::string dir_path = target_dir->full_path;
    if (!dir_path.empty() && dir_path.back() != '/' && dir_path.back() != '\\') {
        dir_path += "/";
    }
    std::string full_path = dir_path + filename;
    
    // Create the file
    std::ofstream file(full_path);
    if (!file.is_open()) {
        return false;
    }
    file.close();
    
    // Reload the directory and refresh display
    load_directory_children(target_dir);
    refresh_visible_nodes();
    
    // Request full refresh to fix color issues
    needs_full_refresh_ = true;
    
    return true;
}

bool FileTree::create_new_folder(const std::string& foldername) {
    if (!selected_node_) return false;
    
    // Get the target directory
    TreeNode* target_dir = nullptr;
    if (selected_node_->is_directory) {
        target_dir = selected_node_;
    } else {
        // Find parent directory
        target_dir = selected_node_->parent;
    }
    
    if (!target_dir) return false;
    
    // Create full path
    std::string dir_path = target_dir->full_path;
    if (!dir_path.empty() && dir_path.back() != '/' && dir_path.back() != '\\') {
        dir_path += "/";
    }
    std::string full_path = dir_path + foldername;
    
    // Create the directory
    try {
        std::filesystem::create_directory(full_path);
        
        // Reload the directory and refresh display
        load_directory_children(target_dir);
        refresh_visible_nodes();
        
        // Request full refresh to fix color issues
        needs_full_refresh_ = true;
        
        return true;
    } catch (const std::exception& e) {
        return false;
    }
}

bool FileTree::delete_selected_item() {
    // TODO: Implement delete functionality
    // Hints:
    // 1. Check if selected_node_ is valid
    // 2. Use std::filesystem::remove or std::filesystem::remove_all
    // 3. For files: use std::filesystem::remove
    // 4. For directories: use std::filesystem::remove_all (be careful!)
    // 5. Refresh the file tree after deletion
    // 6. Return true if successful, false otherwise
    
    if (selected_node_){
        std::filesystem::remove_all(selected_node_ -> full_path);
        // Reload parent directory and refresh display
        if (selected_node_->parent) {
            load_directory_children(selected_node_->parent);
        }
        refresh_visible_nodes();
        
        // Request full refresh to fix color issues
        needs_full_refresh_ = true;
        
        return true;
    }
    return false;
}

bool FileTree::rename_selected_item(const std::string& new_name) {
    // TODO: Implement rename functionality
    // Hints:
    // 1. Check if selected_node_ is valid
    // 2. Get the parent directory of the selected item
    // 3. Create new full path with the new name
    // 4. Use std::filesystem::rename to rename the item
    // 5. Refresh the file tree to show the renamed item
    // 6. Return true if successful, false otherwise
    
    if (selected_node_){
        // Get parent directory path
        std::string parent_path = selected_node_->full_path;
        size_t last_slash = parent_path.find_last_of("/\\");
        if (last_slash != std::string::npos) {
            parent_path = parent_path.substr(0, last_slash + 1);
        }
        std::string new_path = parent_path + new_name;
        
        std::filesystem::rename(selected_node_->full_path, new_path);
        // Reload parent directory and refresh display
        if (selected_node_->parent) {
            load_directory_children(selected_node_->parent);
        }
        refresh_visible_nodes();
        return true;
    }
    return false;
}

// File management state - parts you need to implement
bool FileTree::should_show_input_dialog() const {
    // TODO: Implement input dialog check
    // Hints:
    // 1. Return true if show_input_dialog_ is true
    // 2. This will be used by main program to show input dialog
    
    return show_input_dialog_;
}

std::string FileTree::get_input_dialog_prompt() const {
    // TODO: Implement get input dialog prompt
    // Hints:
    // 1. Return the current input_dialog_prompt_
    // 2. This will be displayed to the user in the input dialog
    
    return input_dialog_prompt_;
}

void FileTree::clear_input_dialog_request() {
    // TODO: Implement clear input dialog request
    // Hints:
    // 1. Reset show_input_dialog_ to false
    // 2. Clear input_dialog_prompt_
    // 3. Reset pending_operation_ to None
    
    show_input_dialog_ = false;
    input_dialog_prompt_ = "";
    pending_operation_ = FileOperation::None;
}

FileTree::FileOperation FileTree::get_pending_operation() const {
    // TODO: Implement get pending operation
    // Hints:
    // 1. Return the current pending_operation_
    // 2. This will be used by main program to determine which operation to perform
    
    return pending_operation_;
}

// File move operations - parts you need to implement
bool FileTree::cut_selected_item() {
    // TODO: Implement cut functionality
    // Hints:
    // 1. Check if selected_node_ is valid
    // 2. Store the path in clipboard_path_
    // 3. Set clipboard_is_cut_ to true
    // 4. Set has_clipboard_item_ to true
    // 5. Return true if successful
    
    if (selected_node_){
        clipboard_path_ = selected_node_ -> full_path;
        clipboard_is_cut_ = true;
        has_clipboard_item_ = true;
        return true;
    }
    return false;
}

bool FileTree::copy_selected_item() {
    // TODO: Implement copy functionality
    // Hints:
    // 1. Check if selected_node_ is valid
    // 2. Store the path in clipboard_path_
    // 3. Set clipboard_is_cut_ to false
    // 4. Set has_clipboard_item_ to true
    // 5. Return true if successful
    
    if (selected_node_){
        clipboard_path_ = selected_node_ -> full_path;
        clipboard_is_cut_ = false;
        has_clipboard_item_ = true;
        return true;
    }
    return false;
}

bool FileTree::paste_clipboard_item() {
    // TODO: Implement paste functionality
    // Hints:
    // 1. Check if has_clipboard_item_ is true
    // 2. Get target directory (selected_node_ or its parent if it's a file)
    // 3. If clipboard_is_cut_: use std::filesystem::rename to move
    // 4. If not cut (copy): use std::filesystem::copy or std::filesystem::copy_file
    // 5. Clear clipboard after cut operation
    // 6. Refresh the file tree
    // 7. Return true if successful
    
    if (has_clipboard_item_){
        // Get target directory
        TreeNode* target_dir = nullptr;
        if (selected_node_->is_directory) {
            target_dir = selected_node_;
        } else {
            target_dir = selected_node_->parent;
        }
        
        if (!target_dir) return false;
        
        // Get filename from clipboard path
        std::string filename = clipboard_path_;
        size_t last_slash = filename.find_last_of("/\\");
        if (last_slash != std::string::npos) {
            filename = filename.substr(last_slash + 1);
        }
        
        // Create target path
        std::string target_path = target_dir->full_path;
        if (!target_path.empty() && target_path.back() != '/' && target_path.back() != '\\') {
            target_path += "/";
        }
        target_path += filename;
        
        try {
            if (clipboard_is_cut_){
                // Get source directory before deleting
                std::string source_dir_path = clipboard_path_;
                size_t last_slash = source_dir_path.find_last_of("/\\");
                if (last_slash != std::string::npos) {
                    source_dir_path = source_dir_path.substr(0, last_slash);
                }
                
                // For cut operation: copy then delete source
                std::filesystem::copy(clipboard_path_, target_path, 
                    std::filesystem::copy_options::recursive);
                
                // Delete the source file/folder after successful copy
                std::filesystem::remove_all(clipboard_path_);
                
                // Find and reload source directory to reflect deletion
                TreeNode* source_dir = find_node_by_path(root_.get(), source_dir_path);
                if (source_dir) {
                    load_directory_children(source_dir);
                }
                
                has_clipboard_item_ = false; // Clear clipboard after cut
            } else {
                // For copy operation: just copy
                std::filesystem::copy(clipboard_path_, target_path, 
                    std::filesystem::copy_options::recursive);
                // Keep clipboard for multiple paste operations
            }
            
            // Reload target directory and refresh display
            load_directory_children(target_dir);
            refresh_visible_nodes();
            
            // Request full refresh to fix color issues
            needs_full_refresh_ = true;
            
            return true;
        } catch (const std::exception& e) {
            return false;
        }

    }
    return false;
}

bool FileTree::has_clipboard_item() const {
    // TODO: Implement clipboard check
    // Hints:
    // 1. Return the value of has_clipboard_item_
    // 2. This will be used to check if paste operation is available
    
    return has_clipboard_item_;
}

TreeNode* FileTree::find_node_by_path(TreeNode* node, const std::string& path) {
    // Helper method to find a node by its full path
    if (!node) return nullptr;
    
    // Check if current node matches
    if (node->full_path == path) {
        return node;
    }
    
    // Recursively search in children
    for (const auto& child : node->children) {
        TreeNode* result = find_node_by_path(child.get(), path);
        if (result) {
            return result;
        }
    }
    
    return nullptr;
}

// Helper function to recursively calculate width including collapsed nodes
void calculate_node_width_recursive(const TreeNode* node, int depth, int& max_width) {
    if (!node) return;
    
    // Calculate display width for this node
    int display_width = depth * 2 + node->name.length() + 2; // 2 spaces for icon
    max_width = std::max(max_width, display_width);
    
    // Recursively check all children (even if node is collapsed)
    // This ensures width calculation includes all loaded content
    if (node->children_loaded) {
        for (const auto& child : node->children) {
            calculate_node_width_recursive(child.get(), depth + 1, max_width);
        }
    }
}

int FileTree::calculate_optimal_width() const {
    int max_width = 20; // Minimum width
    
    // Calculate width based on ALL loaded nodes (including collapsed ones)
    // This ensures consistent width even when nodes are collapsed
    if (root_) {
        calculate_node_width_recursive(root_.get(), 0, max_width);
    }
    
    // Add some padding and ensure minimum width
    max_width += 4; // Padding
    max_width = std::max(max_width, 25); // Minimum width
    
    return max_width;
}

} // namespace editer
