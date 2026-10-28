add_library(cfga_lib cfga_lib.cpp)

add_executable(cfga CFGA.cpp)
target_link_libraries(cfga PRIVATE
        cfga_lib
        ${SVF_LIB}
        ${LLVM_LIB}
        )
set_target_properties(cfga PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
