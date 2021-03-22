add_executable(local)
file(GLOB SOURCE_FILES *.cc)
target_sources(local PRIVATE ${SOURCE_FILES})

