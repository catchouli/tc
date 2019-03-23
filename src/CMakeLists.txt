project(think-cell VERSION 0.1 LANGUAGES CXX)

add_executable(think-cell
  exercise.cpp
)

target_link_libraries(think-cell catch)