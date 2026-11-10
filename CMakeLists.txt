cmake_minimum_required(VERSION 3.20)
project(spi_mini CXX)
set(CMAKE_CXX_STANDARD 17)
find_package(OpenCV REQUIRED)
find_package(GTest REQUIRED)

add_library(spi src/synth.cpp src/inspect.cpp)
target_include_directories(spi PUBLIC include)
target_link_libraries(spi PUBLIC ${OpenCV_LIBS})

add_executable(spi_run src/main.cpp)
target_link_libraries(spi_run spi)

enable_testing()
add_executable(spi_test tests/test_inspect.cpp)
target_link_libraries(spi_test spi GTest::gtest_main)
add_test(NAME spi_test COMMAND spi_test)