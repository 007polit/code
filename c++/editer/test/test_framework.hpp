#pragma once
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

// Lightweight test framework to extend with simple blanks
namespace editer::test {

struct TestCase {
    std::string name;
    std::function<void()> fn;
};

inline std::vector<TestCase>& registry() {
    static std::vector<TestCase> tests;
    return tests;
}

inline void register_test(const std::string& name, std::function<void()> fn) {
    registry().push_back({name, std::move(fn)});
}

inline int run_all() {
    int failed = 0;
    for (auto& t : registry()) {
        try {
            t.fn();
            std::cout << "[PASS] " << t.name << "\n";
        } catch (const std::exception& ex) {
            ++failed;
            std::cout << "[FAIL] " << t.name << " -> " << ex.what() << "\n";
        } catch (...) {
            ++failed;
            std::cout << "[FAIL] " << t.name << " -> unknown error\n";
        }
    }
    std::cout << "\nSummary: " << (registry().size() - failed) << " / " << registry().size() << " passed\n";
    return failed == 0 ? 0 : 1;
}

// Assertion helpers
class AssertError : public std::runtime_error {
public:
    explicit AssertError(const std::string& msg) : std::runtime_error(msg) {}
};

inline void assert_true(bool cond, const std::string& msg) {
    if (!cond) throw AssertError(msg);
}

template <typename L, typename R>
inline void assert_eq(const L& lhs, const R& rhs, const std::string& msg) {
    if (!(lhs == rhs)) {
        std::ostringstream oss;
        oss << msg << " | expect=" << lhs << " actual=" << rhs;
        throw AssertError(oss.str());
    }
}

// Macros for concise test definitions
#define EDITER_TEST(name) \
    static void name##_impl(); \
    namespace { \
        struct name##_Registrar { \
            name##_Registrar() { \
                ::editer::test::register_test(#name, name##_impl); \
            } \
        }; \
        static name##_Registrar name##_instance; \
    } \
    static void name##_impl()

#define ASSERT_TRUE(cond) ::editer::test::assert_true((cond), "ASSERT_TRUE failed: " #cond)
#define ASSERT_EQ(lhs, rhs) ::editer::test::assert_eq((lhs), (rhs), "ASSERT_EQ failed: " #lhs " == " #rhs)

} // namespace editer::test

