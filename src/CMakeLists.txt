
add_library(skip_list
	allocator.cpp
	allocator.hpp
	skip_list.hpp
	utils.hpp
)

target_include_directories(skip_list
	INTERFACE .)
