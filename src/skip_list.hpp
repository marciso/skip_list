#pragma once

#include <cstdint>
#include <cstring>
#include <cassert>
#include <climits>
#include <cmath>

#include <vector>
#include <utility>
#include <random>
#include <string>
#include <array>
#include <algorithm>
#include <iterator>
#include <unordered_set>

#include "utils.hpp"
#include "allocator.hpp"

template<typename key_type, typename value_type,
	size_t N = (64 - sizeof(key_type) - sizeof(value_type)) / sizeof(void*) >
struct skip_list
{
	public:
		using side_t = uint8_t;
		constexpr static bool use_uniform_dist = false;
		constexpr static bool use_log_dist = false;
		constexpr static bool use_sqrt_dist = false;
		constexpr static bool use_rev_sqrt_dist = false;
		constexpr static bool use_rev_log_dist = true;

		constexpr static bool allow_duplicates = false;
	protected:
		static bool eq(key_type a, key_type b) { return a==b; }
		bool lt(key_type a, key_type b) const { return sd_?(b<a):(a<b); }
		bool gt(key_type a, key_type b) const { return sd_?(a<b):(b<a); }

		bool ge(key_type a, key_type b) const { return gt(a,b) || eq(a,b); }

		struct elem
		{
			constexpr static size_t align = 64;
			key_type key;
			value_type value;
			std::array<elem*, N> forwards { nullptr };

			elem(key_type k, value_type v) : key(k), value(v), forwards({}) {}

			elem(key_type k, value_type v, std::array<elem*, N> const & a)
				: key(k), value(v), forwards(a)
			{}

			static void set_forwards(std::array<elem*, N> & forwards, size_t a, size_t b, elem * p)
			{
				for(size_t i = a; i < b; ++i) { forwards[i] = p; }
			}
			void set_forwards(size_t a, size_t b, elem * p) { set_forwards(forwards, a, b, p); }

			static std::string distance_between(elem const * p, elem const * o)
			{
				if ( !p ) return ".";
				if ( !o ) return "-";
				int r = 0;
				for(; p != o; p = p->forwards[0], ++r);
				return std::to_string(r);
			}

			template<typename ostream>
				static ostream & dump_distances(ostream & o,
						elem const * p, std::array<elem *,N> const & f)
				{
					char const * sep = "[";
					for(auto & e : f) {
						o << sep << distance_between(p, e) << " (" << e << ")";
						sep = ", ";
					}
					o << "]";
					return o;
				}

			template<typename ostream>
				ostream & dump(ostream & o) const
				{
					return dump_distances( o << "[" << (void*)this << ": "
							<< key << " -> " << value << " ", this, forwards ) << "]";
				}
		};

		static_assert( sizeof(elem) == 64, "elem is expected to fit into a cache line" );

		side_t sd_;
		size_t size_ = 0;
		elem * head_ = nullptr;

		using allocator_type = aligned_allocator< elem, elem::align >;

	public:

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

		std::vector< std::pair<key_type, value_type> > to_vector() const
		{
			std::vector< std::pair<key_type, value_type> > r;
			r.reserve( size_ );
			for(elem const * p = head_; p; p = p->forwards[0]) {
				r.push_back( std::make_pair(p->key, p->value) );
			}
			return r;
		}

		value_type * find(key_type k) const
		{
			if ( elem * p = head_ )
			{
				if( eq( p->key, k ) ) return &(p->value);

				for(size_t lvl = N; lvl > 0;) {
					--lvl;
					while( p->forwards[lvl] && lt( p->forwards[lvl]->key, k) ) {
						p = p->forwards[lvl];
					}
					assert( p->forwards[lvl] == nullptr || ge( p->forwards[lvl]->key, k) );
				}

				if ( p->forwards[0] ) {
					return ( eq( p->forwards[0]->key, k ) ?  &(p->forwards[0]->value) : nullptr );
				}
			}

			return nullptr;
		}
		bool contains(key_type k) const { return find(k) != nullptr; }

		template<typename ostream>
			ostream & dump(ostream & o, char const * sep = ", ", int limit = INT_MAX,
					std::set<elem const*> marked = {}) const
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
					p->dump(o << sep << (marked.count(p) ? "*" : " " ));
				}
				o << "]";

				return o;
			}

		bool insert(key_type k, value_type v)
		{
			//dump(std::cout << "-- insert (" << k << "->" << v << "), into:\n", "\n") << std::endl;
			std::array<elem*, N> forwards {};
			if ( elem * p = head_ ) {
				if ( lt( p->key, k ) ) {
					for(size_t lvl = N; lvl > 0;) {
						--lvl;
						while( p->forwards[lvl] && lt( p->forwards[lvl]->key, k) ) {
							p = p->forwards[lvl];
						}
						forwards[lvl] = p;
						assert( lt(p->key, k) ); // p->key < k, k is strictly greater than p->key
					}

					if ( !allow_duplicates && p->forwards[0] && eq( p->forwards[0]->key, k ) )
						return false;

					insert_after(p, k, v, random_level(), forwards);
				}
				else {
					if ( !allow_duplicates && eq(p->key, k) ) return false;

					assert( p == head_ );
					assert( lt(k, p->key) || eq(p->key, k) ); // k < p->key

					// insert before head: take current head's forward as new refs, p->forwards
					if ( p->forwards[0] ) {
						assert( p->forwards[N-1] != nullptr && "head needs to have all levels");
						insert_head( k, v, random_level(), p->forwards );
					}
					else {
						assert( size_ == 1 );
						elem::set_forwards( forwards, 0, N, p );
						insert_head( k, v, random_level(), forwards );
						assert( size_ == 2 );
					}
					assert( head_->forwards[N-1] != nullptr && "head needs to have all levels");
					assert( head_ != p && "new elem inserted as a head" );
				}
			}
			else {
				assert( head_ == nullptr );
				assert( std::all_of(forwards.begin(), forwards.end(),
							[](auto * p){ return p == nullptr; }));

				insert_head( k, v, 0, forwards );
				assert( size_ == 1 );
			}
			return true;
		}

		void erase_head()
		{
			assert( head_ && size_ > 0 );
			elem * old = head_;
			head_ = head_->forwards[0];
			if ( head_ ) {
				size_t i = 1;
				for( ; i < N && head_->forwards[i]; ++i ) ; // skip filled cells
				for( ; i < N; ++i ) {
					assert( head_->forwards[i] == nullptr );
					// copy from old head unless it would cause a loop
					head_->forwards[i] = ( old->forwards[i] != head_ ) ?
						old->forwards[i] :
						head_->forwards[i-1];
				}
			}

			assert( size_ > 0 );
			size_--;
			allocator_type::destroy(old);
			allocator_type::deallocate(old, 1);
		}

		void erase_after( elem * prev, elem * del, std::array<elem*, N> & fwrds)
		{
			assert( prev && size_ > 0 );
			assert( del && del == prev->forwards[0]);
			assert( fwrds[0] == prev );

			fwrds[0]->forwards[0] = del->forwards[0];
			for(size_t i = 1; i < N; ++i) {
				// fixup level pointers
				if ( del == fwrds[i]->forwards[i] ) {
					// copy from deleted, unless it would cause a loop, then take previous forwards
					fwrds[i]->forwards[i] =
						( del->forwards[i] && del->forwards[i] != fwrds[i] ) ?
						del->forwards[i] :
						fwrds[i]->forwards[i-1];
				}
			}

			assert( size_ > 0 );
			size_--;
			allocator_type::destroy(del);
			allocator_type::deallocate(del, 1);
		};

		std::pair<value_type, size_t> erase(key_type k)
		{
			if ( elem * p = head_ ) {
				std::array<elem*, N> forwards {};
				if ( lt( p->key, k ) ) {
					for(size_t lvl = N; lvl > 0;) {
						--lvl;
						while( p->forwards[lvl] && lt( p->forwards[lvl]->key, k) ) {
							p = p->forwards[lvl];
						}
						forwards[lvl] = p;
						assert( lt(p->key, k) ); // p->key < k, k is strictly greater than p->key
					}

					elem * q = p->forwards[0];
					if ( q && eq( q->key, k ) ) {
						auto r = q->value;
						erase_after(p, q, forwards);
						return std::make_pair(r, 1);
					}
				}
				else {
					if ( eq(p->key, k) ) {
						auto r = p->value;
						assert( p == head_ );
						erase_head();
						return std::make_pair(r, 1);
					}
				}
			}
			return std::make_pair(value_type{}, 0);
		}

		struct iter_impl : public std::iterator< std::forward_iterator_tag, elem >
		{
			elem * p;

			iter_impl(elem * x) : p(x) {}
			void next() { p = p->forwards[0]; }
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

	protected:

		size_t random_level()
		{
			static std::random_device rd;
			size_t r = N;
			if ( use_uniform_dist )
			{
				std::uniform_int_distribution<int> dist_{1,N};
				r = dist_(rd);
			}
			else if ( use_log_dist )
			{
				std::uniform_int_distribution<int> dist_{2,1<<N};
				r = std::log2(dist_(rd));
			}
			else if ( use_rev_log_dist )
			{
				std::uniform_int_distribution<int> dist_{2,1<<N};
				r = N-std::log2(dist_(rd))+1;
			}
			else if ( use_sqrt_dist )
			{
				std::uniform_int_distribution<int> dist_{1,N*N};
				r = std::sqrt(dist_(rd));
			}
			else if ( use_rev_sqrt_dist )
			{
				std::uniform_int_distribution<int> dist_{1,N*N};
				r = N-std::sqrt(dist_(rd))+1;
			}
			assert( r != 0 );
			assert( 1 <= r && r <= N );
			return r;
		}

		void insert_after(elem* p, key_type k, value_type v,
				size_t lvl, std::array<elem*, N> const fwrds)
		{
			elem * e = allocator_type::allocate(1);
			allocator_type::construct(e, k, v);

			//elem::dump_distances(std::cout << "-- insert_after (p=" << p << ", lvl=" << lvl << ", "
			//	<< k << "->" << v << ", fwrds=", nullptr, fwrds) << ")" << std::endl;

			size_t n = fwrds[N-1] == head_ ? N : lvl;
			for(size_t i = 0; i < n; ++i)
			{
				//TODO: embed ranvom level
				elem * f = fwrds[i];
				elem * aux = f->forwards[i];
				f->forwards[i] = e;
				e->forwards[i] = aux;
			}
			size_++;
		}

		void insert_head( key_type k, value_type v, size_t lvl, std::array<elem*, N> const & fwrds)
		{
			elem * e = allocator_type::allocate(1);
			allocator_type::construct(e, k, v, fwrds);

			//elem::dump_distances(std::cout << "-- insert_head (lvl=" << lvl << ", "
			//		<< k << "->" << v << ", fwrds=", nullptr, fwrds) << ")" << std::endl;

			elem * p = head_;
			head_ = e;
			head_->set_forwards(0, lvl, p);
			if ( p ) p->set_forwards(lvl, N, nullptr);
			size_++;
		}

};



