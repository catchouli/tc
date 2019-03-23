project(think-cell VERSION 0.1 LANGUAGES CXX)

add_executable(think-cell
  exercise.cpp
)

enableCXX17(think-cell)

target_link_libraries(think-cell catch)