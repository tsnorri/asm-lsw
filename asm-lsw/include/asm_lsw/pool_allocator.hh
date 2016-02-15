/*
 Copyright (c) 2016 Tuukka Norri
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program.  If not, see http://www.gnu.org/licenses/ .
 */

#ifndef ASM_LSW_STATIC_ALLOCATOR_HH
#define ASM_LSW_STATIC_ALLOCATOR_HH

#include <cassert>
#include <iostream>
#include <memory>


namespace asm_lsw { namespace detail {
	class pool_allocator_impl
	{
	protected:
		std::unique_ptr <unsigned char[]> m_ptr{};
		std::size_t m_n{0};			// Total space in bytes.
		std::size_t m_idx{0};		// Next available byte.
		
	public:
		pool_allocator_impl() = default;
		pool_allocator_impl(pool_allocator_impl const &) = delete;
		pool_allocator_impl(pool_allocator_impl &&) = default;
		pool_allocator_impl &operator=(pool_allocator_impl const &) & = delete;
		pool_allocator_impl &operator=(pool_allocator_impl &&) & = default;
		
		std::size_t bytes_left() const
		{
			return m_n - m_idx;
		}
		
		bool operator==(pool_allocator_impl const &other) const
		{
			return m_ptr.get() == other.m_ptr.get();
		}
		
		template <typename t_element>
		void allocate_pool(std::size_t n)
		{
			assert(!m_ptr.get());
			
			auto const size(n * sizeof(t_element));
			m_ptr.reset(
				reinterpret_cast <unsigned char *> (
					new std::aligned_storage <1, alignof(t_element)>[size]
				)
			);
			
			m_n = size;
			m_idx = 0;
		}
		
		void destroy_pool()
		{
			assert(m_ptr.get());
			m_ptr.reset(nullptr);
			m_n = 0;
			m_idx = 0;
		}
		
		template <typename t_element>
		unsigned char *allocate(std::size_t n)
		{
			// Make sure that the pointer is aligned properly.
			uintptr_t const al(alignof(t_element));
			uintptr_t const start(reinterpret_cast <uintptr_t>(m_ptr.get()));
			uintptr_t const addr(start + m_idx);
			uintptr_t const diff(addr % al);
			std::size_t add(diff ? (al - diff) : 0);
			std::size_t const size(n * sizeof(t_element));
			
			// Compare the byte counts (not addresses).
			if (m_idx + add + size <= m_n)
			{
				m_idx += add;
				unsigned char *retval(m_ptr.get() + m_idx);
				m_idx += size;
				return retval;
			}
			
			std::bad_alloc exc;
			throw exc;
		}
	};
}}


namespace asm_lsw {
	template <typename t_element>
	class pool_allocator
	{
		template <typename> friend class pool_allocator;
		
	public:
		typedef t_element value_type;
		
		template <typename U>
		struct rebind
		{
			typedef pool_allocator <U> other;
		};
		
	protected:
		std::shared_ptr <detail::pool_allocator_impl> m_a;
		
	public:
		pool_allocator(std::size_t n):
			m_a(new detail::pool_allocator_impl())
		{
			m_a->allocate_pool <value_type>(n);
		}
		
		template <typename U>
		pool_allocator(pool_allocator <U> const &other)
		{
			m_a = other.m_a;
		}
		
		value_type *allocate(std::size_t n)
		{
			return reinterpret_cast <value_type *>(m_a->allocate <value_type>(n));
		}
		
		void deallocate(value_type *, std::size_t n) {} // No-op.
		
		std::size_t bytes_left() const
		{
			return m_a->bytes_left();
		}
	};
	
	template <typename T, typename U>
	bool operator==(pool_allocator <T> const &a, pool_allocator <U> const &b)
	{
		return a.m_a == b.m_a;
	}
	
	template <typename T, typename U>
	bool operator!=(pool_allocator <T> const &a, pool_allocator <U> const &b)
	{
		return !(a == b);
	}
}

#endif
