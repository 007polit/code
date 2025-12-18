#include "test_framework.hpp"
#include "../include/core/buffer.hpp"
#include "../include/core/editor.hpp"

using namespace editer;

// ============================================
// Buffer Unit Tests
// ============================================

EDITER_TEST(Buffer_Insert_and_Size) {
    Buffer buf;
    buf.insert({0, 0}, 'A');
    buf.insert({0, 1}, '\n');
    buf.insert({1, 0}, 'B');
    
    ASSERT_EQ(buf.size(), 2);
    ASSERT_EQ(buf.line(0), std::string("A"));
    ASSERT_EQ(buf.line(1), std::string("B"));
}

EDITER_TEST(Buffer_Find) {
    Buffer buf;
    buf.insert({0, 0}, 'H');
    buf.insert({0, 1}, 'i');
    buf.insert({0, 2}, '!');
    
    auto pos = buf.find("i");
    ASSERT_EQ(pos.row, 0);
    ASSERT_EQ(pos.col, 1);
}

EDITER_TEST(Buffer_Delete) {
    Buffer buf;
    buf.insert({0, 0}, 'H');
    buf.insert({0, 1}, 'e');
    buf.insert({0, 2}, 'l');
    buf.insert({0, 3}, 'l');
    buf.insert({0, 4}, 'o');
    
    buf.erase({0, 2});
    ASSERT_EQ(buf.line(0), std::string("Helo"));
}

EDITER_TEST(Buffer_MultiLine) {
    Buffer buf;
    buf.insert({0, 0}, 'L');
    buf.insert({0, 1}, 'i');
    buf.insert({0, 2}, 'n');
    buf.insert({0, 3}, 'e');
    buf.insert({0, 4}, ' ');
    buf.insert({0, 5}, '1');
    buf.insert({0, 6}, '\n');

    buf.insert({1, 0}, 'L');
    buf.insert({1, 1}, 'i');
    buf.insert({1, 2}, 'n');
    buf.insert({1, 3}, 'e');
    buf.insert({1, 4}, ' ');
    buf.insert({1, 5}, '2');
    buf.insert({1, 6}, '\n');

    buf.insert({2, 0}, 'L');
    buf.insert({2, 1}, 'i');
    buf.insert({2, 2}, 'n');
    buf.insert({2, 3}, 'e');
    buf.insert({2, 4}, ' ');
    buf.insert({2, 5}, '3');

    ASSERT_EQ(buf.size(), 3);
    ASSERT_EQ(buf.line(0), std::string("Line 1"));
    ASSERT_EQ(buf.line(1), std::string("Line 2"));
    ASSERT_EQ(buf.line(2), std::string("Line 3"));
}

// ============================================
// Editor Integration Tests
// ============================================

EDITER_TEST(Editor_Initialize) {
    Editor editor;
    ASSERT_EQ(editor.buffer().size(), 1);
}

EDITER_TEST(Editor_InsertAndDisplay) {
    Editor editor;
    editor.insert_char('H');
    editor.insert_char('i');
    ASSERT_EQ(editor.buffer().line(0), std::string("Hi"));
}

EDITER_TEST(Editor_Undo) {
    Editor editor;
    editor.insert_char('X');
    editor.insert_char('Y');
    ASSERT_EQ(editor.buffer().line(0), std::string("XY"));
    
    editor.undo();
    ASSERT_EQ(editor.buffer().line(0), std::string("X"));
    
    editor.undo();
    ASSERT_EQ(editor.buffer().size(), 1);
}

EDITER_TEST(Editor_Redo) {
    Editor editor;
    editor.insert_char('A');
    editor.undo();
    ASSERT_EQ(editor.buffer().size(), 1);
    
    editor.redo();
    ASSERT_EQ(editor.buffer().line(0), std::string("A"));
}

EDITER_TEST(Editor_UndoRedoMultiple) {
    Editor editor;
    editor.insert_char('a');
    editor.insert_char('b');
    editor.insert_char('c');
    ASSERT_EQ(editor.buffer().line(0), std::string("abc"));
    
    editor.undo();
    editor.undo();
    ASSERT_EQ(editor.buffer().line(0), std::string("a"));
    
    editor.redo();
    editor.redo();
    ASSERT_EQ(editor.buffer().line(0), std::string("abc"));
}
