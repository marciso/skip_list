add_executable(tester
	skip_list_test.cpp)

target_link_libraries(tester
	skip_list
	libgtest
	libgtest_main)

add_test(NAME skip_list_test
	COMMAND tester)

