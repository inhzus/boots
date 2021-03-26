add_executable(server)
file(GLOB SOURCE_FILES *.cc)
target_sources(server PRIVATE ${SOURCE_FILES})
target_link_libraries(server PRIVATE boots)

