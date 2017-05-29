#pragma once

template<typename R, uintptr_t align, typename T> constexpr inline R mask_ptr(T * p)
{
	static_assert( align && !(align & (align-1)), "power of 2" );
	uintptr_t const aux = reinterpret_cast<uintptr_t>(p) & ~(align-1);
	return reinterpret_cast<R>(aux);
}

template<typename T> constexpr inline bool is_power_of_two(T n)
{
	return (n > 0) & ( 0 == ( n & (n-1) ));
}

