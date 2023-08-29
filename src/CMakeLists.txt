include_directories(${PROJECT_SOURCE_DIR}/include)

aux_source_directory(. LIB_SRCS)
add_library(fio_mine ${LIB_SRCS})
link_libraries(fio_mine)

