add_library(boots SHARED)
file(GLOB SOURCE_FILES *.cc)
target_sources(boots PRIVATE ${SOURCE_FILES})

