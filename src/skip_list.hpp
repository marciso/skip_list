#pragma once

#include <cstdint>
#include <cstring>
#include <cassert>
#include <climits>

#include <array>
#include <algorithm>
#include <iterator>
#include <unordered_set>

#include "utils.hpp"
#include "allocator.hpp"

template<typename key_type, typename value_type, size_t N = (64 - sizeof(key_type) - sizeof(value_type)) / sizeof(void*) >
struct skip_list
{
	using side_t = uint8_t;

	static bool eq(key_type a, key_type b) { return a==b; }
	bool lt(key_type a, key_type b) const { return sd_?(b<a):(a<b); }
	bool gt(key_type a, key_type b) const { return sd_?(a<b):(b<a); }

	struct elem
	{
		constexpr static size_t align = 64;
		key_type key;
		value_type value;
		std::array<elem*, N> forwards { nullptr };

		elem(key_type k, value_type v, std::array<elem*, N> const & a)
			: key(k), value(v), forwards(a)
		{}

		int distance_to(elem* o) const
		{
			int r = 0;
			for(elem const * p = this; p && p != o; p = p->forwards[0], ++r);
			return r;
		}

		template<typename ostream>
			ostream & dump(ostream & o) const
			{

				o << "[" << (void*)this << ": "
					<< key << " -> " << value;
				char const * sep = " [";
				for(auto & e : forwards) {
					o << sep << distance_to(e);
					sep = ", ";
				}
				o << "]";
				return o;
			}
	};

	static_assert( sizeof(elem) == 64, "elem is expected to fit into a cache line" );

	side_t sd_;
	size_t size_ = 0;
	elem * head_ = nullptr;

	using allocator_type = aligned_allocator< elem, elem::align >;

	skip_list(side_t sd) : sd_(sd) {}
	~skip_list() { clear(); }

	bool empty() const { return head_ == nullptr; }
	size_t size() const { return size_; }
	void clear()
	{
		elem * p = head_;
		while( p ) {
			elem * n = p->forwards[0];
			allocator_type::destroy(p);
			allocator_type::deallocate(p,1);
			p = n;
		}
		size_ = 0;
	}

	size_t count() const
	{
		size_t r = 0;
		for(elem const * p = head_; p; p = p->forwards[0], ++r);
		return r;
	}

	value_type * find(key_type k) const
	{
		elem * p = head_;
		if ( p )
		{
			if( eq( p->key, k ) ) return &(p->value);

			for(size_t lvl = N; lvl > 0;) {
				--lvl;
				while( p->forwards[lvl] && lt( p->forwards[lvl]->key, k) ) {
					p = p->forwards[lvl];
				}
			}

			if ( p->forwards[0] ) {
				return ( eq( p->forwards[0]->key, k ) ?  &(p->forwards[0]->value) : nullptr );
			}
		}

		return nullptr;
	}
	bool contains(key_type k) const { return find(k) != nullptr; }

	template<typename ostream>
	ostream & dump(ostream & o, char const * sep = ", ", int limit = INT_MAX) const
	{
		o << "[sd=" << (int)sd_ << ", size=" << size_ << ", head=" << (void*)head_ << ": ";
		int cnt = 0;
		char const * ssep = sep;
		for(elem const * p = head_; p; p = p->forwards[0], ssep=sep)
		{
			if ( cnt++ >= limit ) {
				o << sep << "...";
				break;
			}
			p->dump(o << sep);
		}
		o << "]";

		return o;
	}

	size_t randon_level()
	{
		return 2;
	}
	void insert_head(key_type k, value_type v, std::array<elem*, N> const & forwards)
	{
		elem * e = allocator_type::allocate(1);
		allocator_type::construct(e, k, v, forwards);

		//assert( head_ == nullptr || forwards[0] == head_->forwards[0] );
		elem * aux = head_;
		head_ = e;
		e->forwards[0] = aux;
		size_++;
	}
	void insert_after(elem * p, size_t lvl,
			key_type k, value_type v, std::array<elem*, N> const & forwards)
	{
		elem * e = allocator_type::allocate(1);
		allocator_type::construct(e, k, v, forwards);

		assert( forwards[0] == p->forwards[0] );
		for(size_t i = lvl; i > 1;) {
			p->forwards[--i] = e;
		}
		p->forwards[0] = e;
		size_++;
	}
	bool insert(key_type k, value_type v)
	{
		//dump(std::cout << "insert " << k << "->" << v << ", into:\n", "\n") << std::endl;
		std::array<elem*, N> forwards {};
		if (elem * p = head_ )
		{
			if ( eq( p->key, k ) ) return false;

			for(size_t lvl = N; lvl > 0;) {
				--lvl;
				while( p->forwards[lvl] && lt( p->forwards[lvl]->key, k) ) {
					p = p->forwards[lvl];
				}
				forwards[lvl] = p;
			}

			if ( p->forwards[0] )
			{
				if ( eq( p->forwards[0]->key, k ) ) return false;
				assert( gt( p->forwards[0]->key, k ) );

				// insert between p and p->next
				if ( p == head_ && gt(p->key, k) )
					insert_head(k, v, forwards);
				else
					insert_after(p, randon_level(), k, v, forwards);
			}
			else
			{
				// p is the last element, we need to insert after it
				assert( std::all_of(p->forwards.begin(), p->forwards.end(),
							[this](auto * p){ return p == nullptr; }));

				if ( p == head_ && gt(p->key, k) )
					insert_head(k, v, forwards);
				else
					insert_after(p, N, k, v, p->forwards);
				// insert between head_ and p
			}
		}
		else {
			assert( head_ == nullptr );
			assert( std::all_of(forwards.begin(), forwards.end(),
						[](auto * p){ return p == nullptr; }));
			// insert as head
			insert_head(k, v, forwards);
		}

		return true;
	}

	value_type erase(key_type k)
	{
		return nullptr;
	}

	struct iter_impl : public std::iterator< std::forward_iterator_tag, elem >
	{
		elem * p;

		iter_impl(elem * x) : p(x) {}
		void next();
		bool operator==(iter_impl o) const { return p == o.p; }
		bool operator!=(iter_impl o) const { return p != o.p; }
		elem & operator*() { return *p; }
		elem const & operator*() const { return *p; }
		elem * operator->() { return p; }
		elem const * operator->() const { return p; }
		iter_impl & operator++() { next(); return *this; }
		iter_impl operator++(int) { iter_impl tmp{*this}; next(); return tmp; }
	};

	using iterator = iter_impl;
	using const_iterator = const iter_impl;

	iterator begin() { return iterator{head_}; }
	iterator end() { return iterator{nullptr}; }
	const_iterator cbegin() { return const_iterator{head_}; }
	const_iterator cend() { return const_iterator{nullptr}; }
	const_iterator begin() const { return const_iterator{head_}; }
	const_iterator end() const { return const_iterator{nullptr}; }
};



