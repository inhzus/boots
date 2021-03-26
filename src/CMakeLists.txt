add_library(boots STATIC)

file(GLOB SOURCE_FILES *.cc)
target_sources(boots PRIVATE ${SOURCE_FILES})

find_package(spdlog REQUIRED)
find_package(fmt REQUIRED)
target_compile_definitions(boots PUBLIC SPDLOG_FMT_EXTERNAL)
target_link_libraries(boots PRIVATE fmt spdlog)

target_include_directories(boots PUBLIC ../include)

