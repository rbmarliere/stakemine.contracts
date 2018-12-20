add_contract(darma.token darma.token ${CMAKE_CURRENT_SOURCE_DIR}/src/darma.token.cpp)
target_include_directories(darma.token.wasm
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(darma.token.wasm
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

