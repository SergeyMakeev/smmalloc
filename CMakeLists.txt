cmake_minimum_required(VERSION 2.8.12)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if(CMAKE_COMPILER_IS_GNUCC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif(CMAKE_COMPILER_IS_GNUCC)

set(PROJ_NAME smmalloc)
project(${PROJ_NAME})

set (CMAKE_CXX_STANDARD 11)

if(MSVC)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

# add test executable
set(TEST_SOURCES 
  smmalloc_test01.cpp
  smmalloc_test02.cpp
)
set (TEST_EXE_NAME ${PROJ_NAME}_test)
add_executable(${TEST_EXE_NAME} ${TEST_SOURCES})

if(MSVC)
  target_compile_options(${TEST_EXE_NAME} PRIVATE /W4 /WX)
else()
  target_compile_options(${TEST_EXE_NAME} PRIVATE -Wall -Wextra -pedantic -Werror)
endif()

if(MSVC)
  set(HOARD_SRC
    ${PROJECT_SOURCE_DIR}/extern/hoard/source/libhoard.cpp
    ${PROJECT_SOURCE_DIR}/extern/hoard/source/wintls.cpp
  )
else()
  set(HOARD_SRC )
endif()

set(LTALLOC_SRC
  ${PROJECT_SOURCE_DIR}/extern/ltalloc/ltalloc.cc
)

set(DLMALLOC_SRC
  ${PROJECT_SOURCE_DIR}/extern/dlmalloc/dlmalloc.cpp
)

set(RPMALLOC_SRC
  ${PROJECT_SOURCE_DIR}/extern/rpmalloc/rpmalloc.c
)

# add perf test executable
set(PERF_SOURCES 
  smmalloc_perf01.cpp
  smmalloc_test_impl.inl
)
set (PERF_EXE_NAME ${PROJ_NAME}_perf)
add_executable(${PERF_EXE_NAME} ${PERF_SOURCES} ${HOARD_SRC} ${LTALLOC_SRC} ${RPMALLOC_SRC} ${DLMALLOC_SRC})

if(MSVC)
  target_compile_options(${PERF_EXE_NAME} PRIVATE /W4)
else()
  target_compile_options(${PERF_EXE_NAME} PRIVATE -Wall -Wextra -pedantic)
endif()


# add ecs2
add_subdirectory("${PROJECT_SOURCE_DIR}/SmMalloc")
target_link_libraries(${TEST_EXE_NAME} smmalloc)
target_link_libraries(${PERF_EXE_NAME} smmalloc)

# add gtest
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
add_subdirectory("${PROJECT_SOURCE_DIR}/extern/googletest" "extern/googletest")
target_link_libraries(${TEST_EXE_NAME} gtest_main)


# add ubench
target_include_directories(${PERF_EXE_NAME} PRIVATE "${PROJECT_SOURCE_DIR}/extern/ubench")

if(UNIX OR MACOS OR IOS)
  target_link_libraries(${PERF_EXE_NAME} pthread)
endif()


# add test allocators include dirs
target_include_directories(${PERF_EXE_NAME} PRIVATE "${PROJECT_SOURCE_DIR}/extern/hoard/Heap-Layers")
target_include_directories(${PERF_EXE_NAME} PRIVATE "${PROJECT_SOURCE_DIR}/extern/hoard/include")
target_include_directories(${PERF_EXE_NAME} PRIVATE "${PROJECT_SOURCE_DIR}/extern/hoard/include/util")
target_include_directories(${PERF_EXE_NAME} PRIVATE "${PROJECT_SOURCE_DIR}/extern/hoard/include/hoard")
target_include_directories(${PERF_EXE_NAME} PRIVATE "${PROJECT_SOURCE_DIR}/extern/hoard/include/superblocks")
target_include_directories(${PERF_EXE_NAME} PRIVATE "${PROJECT_SOURCE_DIR}/extern/rpmalloc")




