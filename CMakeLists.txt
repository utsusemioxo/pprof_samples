cmake_minimum_required(VERSION 3.20)
project(pprof_samples CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_compile_options(-fno-omit-frame-pointer -g)
find_package(Threads REQUIRED)

foreach(variant bad good)
    add_executable(sample_01_${variant}
        samples/01_flat_vs_cum/${variant}.cpp)
endforeach()

foreach(variant bad good)
    add_executable(sample_02_${variant}
        samples/02_inline_effect/${variant}.cpp)
    target_include_directories(sample_02_${variant}
        PRIVATE common)
endforeach()

foreach(variant bad good)
    add_executable(sample_03_${variant}
        samples/03_virtual_dispatch/${variant}.cpp)
endforeach()

foreach(variant bad good)
    add_executable(sample_04_${variant}
        samples/04_false_sharing/${variant}.cpp)
    target_link_libraries(sample_04_${variant}
        PRIVATE Threads::Threads)
endforeach()

foreach(variant bad good)
    add_executable(sample_05_${variant}
        samples/05_list_vs_vector/${variant}.cpp)
endforeach()

foreach(variant bad good)
    add_executable(sample_06_${variant}
        samples/06_vector_reserve/${variant}.cpp)
endforeach()

foreach(variant bad good)
    add_executable(sample_07_${variant}
        samples/07_lock_contention/${variant}.cpp)
    target_link_libraries(sample_07_${variant}
        PRIVATE Threads::Threads)
endforeach()

foreach(variant bad good)
    add_executable(sample_08_${variant}
        samples/08_branch_mispredict/${variant}.cpp)
endforeach()

foreach(variant bad good)
    add_executable(sample_09_${variant}
        samples/09_map_vs_unordered/${variant}.cpp)
endforeach()
