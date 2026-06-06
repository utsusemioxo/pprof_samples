cmake_minimum_required(VERSION 3.20)
project(pprof_samples CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_compile_options(-fno-omit-frame-pointer -g)
find_package(Threads REQUIRED)

foreach(sample
    02_inline_effect
    03_complexity
    04_false_sharing
    05_cache_miss
    06_alloc_pressure
    07_lock_contention)

    add_executable(sample_${sample}
        samples/${sample}/main.cpp)

    target_include_directories(sample_${sample}
        PRIVATE common)
    target_link_libraries(sample_${sample}
        PRIVATE Threads::Threads)
endforeach()

foreach(variant bad good)
    add_executable(sample_01_${variant}
        samples/01_flat_vs_cum/${variant}.cpp)
endforeach()
