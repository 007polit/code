# CMake generated Testfile for 
# Source directory: F:/code/c++/editer/test
# Build directory: F:/code/c++/editer/test/build
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
if(CTEST_CONFIGURATION_TYPE MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test(EditerTests "F:/code/c++/editer/test/build/Debug/editer_tests.exe")
  set_tests_properties(EditerTests PROPERTIES  _BACKTRACE_TRIPLES "F:/code/c++/editer/test/CMakeLists.txt;17;add_test;F:/code/c++/editer/test/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
  add_test(EditerTests "F:/code/c++/editer/test/build/Release/editer_tests.exe")
  set_tests_properties(EditerTests PROPERTIES  _BACKTRACE_TRIPLES "F:/code/c++/editer/test/CMakeLists.txt;17;add_test;F:/code/c++/editer/test/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
  add_test(EditerTests "F:/code/c++/editer/test/build/MinSizeRel/editer_tests.exe")
  set_tests_properties(EditerTests PROPERTIES  _BACKTRACE_TRIPLES "F:/code/c++/editer/test/CMakeLists.txt;17;add_test;F:/code/c++/editer/test/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
  add_test(EditerTests "F:/code/c++/editer/test/build/RelWithDebInfo/editer_tests.exe")
  set_tests_properties(EditerTests PROPERTIES  _BACKTRACE_TRIPLES "F:/code/c++/editer/test/CMakeLists.txt;17;add_test;F:/code/c++/editer/test/CMakeLists.txt;0;")
else()
  add_test(EditerTests NOT_AVAILABLE)
endif()
