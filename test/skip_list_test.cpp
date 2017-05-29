#include <gtest/gtest.h>

#include <iostream>

#include "skip_list.hpp"

using test_type = skip_list<int32_t, void*>;

struct skip_list_test : public ::testing::TestWithParam<int8_t> {};

INSTANTIATE_TEST_CASE_P(bid_or_ask, skip_list_test, ::testing::Values(0,1));

TEST_P(skip_list_test, instance)
{
	EXPECT_NO_THROW({
			test_type x(GetParam());
			test_type y = x;
			//const test_type x = y;
	});
}

TEST_P(skip_list_test, empty)
{
	test_type x(GetParam());

	EXPECT_TRUE( x.empty() );
	EXPECT_EQ( 0, x.size() );
	EXPECT_EQ( 0, x.count() );
	EXPECT_EQ( nullptr, x.find(0) );
	EXPECT_EQ( nullptr, x.find(1) );
	EXPECT_FALSE( x.contains(0) );
	EXPECT_FALSE( x.contains(1) );
}

TEST_P(skip_list_test, insert_1)
{
	test_type x(GetParam());

	EXPECT_TRUE( x.empty() );

	EXPECT_TRUE( x.insert(1, (void*)1UL ) );
	EXPECT_FALSE( x.empty() );
	EXPECT_EQ( 1, x.size() );
	EXPECT_EQ( x.size(), x.count() );
	EXPECT_TRUE( x.contains(1) );
	ASSERT_NE( nullptr, x.find(1) );
	EXPECT_EQ( (void*)1UL, *x.find(1) );

	EXPECT_FALSE( x.insert(1, (void*)1UL ) );
	EXPECT_EQ( 1, x.size() );
	EXPECT_TRUE( x.contains(1) );
}

TEST_P(skip_list_test, insert_315)
{
	test_type x(GetParam());
	EXPECT_TRUE ( x.insert(3, (void*)1UL ) );
	EXPECT_TRUE ( x.insert(1, (void*)1UL ) );
	EXPECT_TRUE ( x.insert(5, (void*)1UL ) );

	x.dump(std::cout, "\n") << std::endl;

	if ( GetParam() == 0 )
	{
		//EXPECT_THAT( ElementsAre({1,3 5}), x);
	}
	else
	{
		//EXPECT_THAT( ElementsAre({5,3,1}), x);
	}
}

TEST_P(skip_list_test, insert_duplicates)
{
	test_type x(GetParam());

	EXPECT_TRUE ( x.insert(1, (void*)1UL ) );
	EXPECT_FALSE( x.insert(1, (void*)1UL ) );
	EXPECT_TRUE ( x.insert(3, (void*)1UL ) );
	EXPECT_FALSE( x.insert(1, (void*)1UL ) );
	EXPECT_FALSE( x.insert(3, (void*)1UL ) );
	EXPECT_TRUE ( x.insert(5, (void*)1UL ) );
	EXPECT_FALSE( x.insert(1, (void*)1UL ) );
	EXPECT_FALSE( x.insert(3, (void*)1UL ) );
	EXPECT_FALSE( x.insert(5, (void*)1UL ) );
}


TEST_P(skip_list_test, contains)
{
	test_type x(GetParam());

	ASSERT_TRUE( x.insert(1, (void*)1UL ) );
	ASSERT_TRUE( x.insert(3, (void*)1UL ) );
	ASSERT_TRUE( x.insert(5, (void*)1UL ) );

	x.dump(std::cout, "\n") << std::endl;

	EXPECT_TRUE( x.contains(1) );
	EXPECT_TRUE( x.contains(3) );
	EXPECT_TRUE( x.contains(5) );

	EXPECT_FALSE( x.contains(0) );
	EXPECT_FALSE( x.contains(2) );
	EXPECT_FALSE( x.contains(4) );
	EXPECT_FALSE( x.contains(6) );
}

TEST_P(skip_list_test, insert_one_block)
{
	test_type x(GetParam());
	int N = 1000; //test_type::max_elements_in_block();

	EXPECT_TRUE( x.empty() );

	for(int i = 1; i <= N; ++i)
	{
		ASSERT_TRUE( x.insert(i, (void*)(size_t)i ) ) << "i=" << i;
	}
	EXPECT_FALSE( x.empty() );
	EXPECT_EQ( N, x.size() );
	EXPECT_EQ( x.size(), x.count() );

	x.dump(std::cout, "\n", 10) << std::endl;

	for(int i = 1; i <= N; ++i)
	{
		EXPECT_TRUE( x.contains(i) ) << "i=" << i;
	}
}

/*
TEST_P(skip_list_test, insert_one_block_rev)
{
	test_type x(GetParam());
	int N = test_type::max_elements_in_block();

	EXPECT_TRUE( x.empty() );

	for(int i = N; i >= 1; --i)
	{
		ASSERT_TRUE( x.insert(i, (void*)(size_t)i ) );
	}
	EXPECT_FALSE( x.empty() );
	EXPECT_EQ( N, x.size() );
	EXPECT_EQ( x.size(), x.count() );

	for(int i = 1; i <= N; ++i)
	{
		EXPECT_TRUE( x.contains(i) );
	}
}

TEST_P(skip_list_test, insert_two_blocks)
{
	test_type x(GetParam());
	int N = 2*test_type::max_elements_in_block();

	EXPECT_TRUE( x.empty() );

	for(int i = 1; i <= N; ++i)
	{
		ASSERT_TRUE( x.insert(i, (void*)(size_t)i ) );
	}
	EXPECT_FALSE( x.empty() );
	EXPECT_EQ( N, x.size() );
	EXPECT_EQ( x.size(), x.count() );

	for(int i = 1; i <= N; ++i)
	{
		EXPECT_TRUE( x.contains(i) );
	}
}

TEST_P(skip_list_test, insert_two_blocks_rev)
{
	test_type x(GetParam());
	int N = 2*test_type::max_elements_in_block();

	EXPECT_TRUE( x.empty() );

	for(int i = N; i >= 1; --i)
	{
		ASSERT_TRUE( x.insert(i, (void*)(size_t)i ) );
	}
	EXPECT_FALSE( x.empty() );
	EXPECT_EQ( N, x.size() );
	EXPECT_EQ( x.size(), x.count() );

	for(int i = 1; i <= N; ++i)
	{
		EXPECT_TRUE( x.contains(i) );
	}
	}
*/

TEST_P(skip_list_test, erase_when_empty)
{
	test_type x(GetParam());

	EXPECT_TRUE( x.empty() );
	EXPECT_EQ( nullptr, x.erase(1) );
}

TEST_P(skip_list_test, erase_1_after_insert_1)
{
	test_type x(GetParam());

	EXPECT_TRUE( x.empty() );

	EXPECT_TRUE( x.insert(1, (void*)1UL ) );
	EXPECT_FALSE( x.empty() );
	EXPECT_TRUE( x.contains(1) );

	EXPECT_EQ( (void*)1UL, x.erase(1) );
	EXPECT_TRUE( x.empty() );
	EXPECT_FALSE( x.contains(1) );

	EXPECT_EQ( nullptr, x.erase(1) );
}

