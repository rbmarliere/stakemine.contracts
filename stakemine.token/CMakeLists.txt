add_contract(stakemine.token stakemine.token ${CMAKE_CURRENT_SOURCE_DIR}/src/stakemine.token.cpp)
target_include_directories(stakemine.token.wasm
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(stakemine.token.wasm
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

