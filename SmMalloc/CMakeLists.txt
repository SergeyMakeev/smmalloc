set(SOURCES
    smmalloc.cpp
    smmalloc_generic.cpp
    smmalloc_tls.cpp
    )

set(HEADERS
    smmalloc.h
    )

add_library(smmalloc STATIC ${SOURCES} ${HEADERS})
target_include_directories(smmalloc PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

