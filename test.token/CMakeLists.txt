add_contract(test.token test.token ${CMAKE_CURRENT_SOURCE_DIR}/src/test.token.cpp)
target_include_directories(test.token.wasm
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(test.token.wasm
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

