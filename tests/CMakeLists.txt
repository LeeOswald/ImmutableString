# prevent GoogleTest from overriding our compiler/linker options
# when building with Visual Studio
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

include(GoogleTest)

enable_testing()

add_executable(string_tests EXCLUDE_FROM_ALL main.cpp shared_data.cpp string.cpp string_benchmark.cpp)
target_link_libraries(string_tests gtest_main)

gtest_discover_tests(string_tests)
