cmake_minimum_required(VERSION 3.0.2 FATAL_ERROR)
project(think-cell LANGUAGES CXX)

# Macro for enabling C++17 support on a project in VS
macro(enableCXX17 proj)
  if (MSVC_VERSION GREATER_EQUAL "1900")
    target_compile_options(${proj} PUBLIC $<$<COMPILE_LANGUAGE:CXX>:-std:c++latest>)
  else()
    target_compile_options(${proj} PUBLIC $<$<COMPILE_LANGUAGE:CXX>:-std=c++17>)
  endif()
endmacro(enableCXX17)

add_subdirectory(catch)
add_subdirectory(src)
