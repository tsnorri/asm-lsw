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


namespace asm_lsw {
	class space_requirement
	{
	protected:
		uintptr_t m_addr{0};
		std::size_t m_amount{0};
		
	public:
		space_requirement() = default;
		space_requirement(uintptr_t addr): m_addr(addr) {}

		// Calculate space requirement for (count * sizeof(t_element)) with proper alignment.
		template <typename t_element>
		static void bytes_from_address(uintptr_t const addr, std::size_t const count, std::size_t &size, std::size_t &add)
		{
			uintptr_t const al(alignof(t_element));
			uintptr_t const diff(addr % al);
			add = (diff ? (al - diff) : 0);
			size = (count * sizeof(t_element));
		}
		
		template <typename t_element>
		static std::size_t bytes_from_address(uintptr_t const addr, std::size_t const count)
		{
			std::size_t size{0}, add{0};
			bytes_from_address <t_element>(addr, count, size, add);
			return size + add;
		}
		
		std::size_t amount() const { return m_amount; }
		
		template <typename t_element>
		void add(std::size_t count)
		{
			auto const amt(bytes_from_address <t_element>(m_addr + m_amount, count));
			m_amount += amt;
		}
	};
}


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
		
		std::size_t bytes_total() const
		{
			return m_n;
		}
		
		std::size_t bytes_left() const
		{
			return m_n - m_idx;
		}
		
		bool operator==(pool_allocator_impl const &other) const
		{
			return m_ptr.get() == other.m_ptr.get();
		}
		
		template <typename t_element>
		void allocate_pool_bytes(std::size_t size)
		{
			assert(!m_ptr.get());

			m_ptr.reset(
				reinterpret_cast <unsigned char *> (
					new std::aligned_storage <1, alignof(t_element)>[size]
				)
			);
			
			m_n = size;
			m_idx = 0;
		}
		
		template <typename t_element>
		void allocate_pool(std::size_t n)
		{
			auto const size(n * sizeof(t_element));
			allocate_pool_bytes <t_element>(size);
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
			uintptr_t const start(reinterpret_cast <uintptr_t>(m_ptr.get()));
			uintptr_t const addr(start + m_idx);
			std::size_t size{0}, add{0};
			space_requirement::bytes_from_address <t_element>(addr, n, size, add);
			
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
		template <typename T, typename U> friend bool operator==(pool_allocator <T> const &, pool_allocator <U> const &);
		
	public:
		typedef t_element			value_type;
		typedef	value_type *		pointer;
		typedef value_type const *	const_pointer;
		typedef	value_type &		reference;
		typedef value_type const &	const_reference;
		typedef std::size_t			size_type;
		typedef std::ptrdiff_t		difference_type;
		
		typedef std::true_type		propagate_on_container_copy_assignment;
		typedef std::true_type		propagate_on_container_move_assignment;
		typedef std::true_type		propagate_on_container_swap;
		typedef std::false_type		is_always_equal;
		
		template <typename U>
		struct rebind
		{
			typedef pool_allocator <U> other;
		};
		
	protected:
		std::shared_ptr <detail::pool_allocator_impl> m_a;
		
	public:
		// Use std::allocator by default in order to get a working allocator with
		// the default constructor.
		pool_allocator(bool instantiate_empty_alloctor = false):
			m_a(
				instantiate_empty_alloctor ?
				new detail::pool_allocator_impl :
				nullptr
			)
		{
		}
		
		pool_allocator(std::size_t n):
			m_a(new detail::pool_allocator_impl())
		{
			m_a->allocate_pool <value_type>(n);
		}
		
		pool_allocator(space_requirement const &sr):
			m_a(new detail::pool_allocator_impl())
		{
			m_a->allocate_pool_bytes <value_type>(sr.amount());
		}
		
		void allocate_pool(space_requirement const &sr)
		{
			assert(m_a.get());
			m_a->allocate_pool_bytes <value_type>(sr.amount());
		}
		
		template <typename U>
		pool_allocator(pool_allocator <U> const &other)
		{
			m_a = other.m_a;
		}
		
		pool_allocator select_on_container_copy_construction() const
		{
			// Return a copy.
			return pool_allocator(*this);
		}
		
		value_type *allocate(std::size_t n, std::allocator <void>::const_pointer hint = nullptr)
		{
			detail::pool_allocator_impl *allocator(m_a.get());
			if (allocator)
				return reinterpret_cast <value_type *>(allocator->allocate <value_type>(n));
			else
			{
				std::allocator <value_type> allocator;
				return allocator.allocate(n, hint);
			}
		}
		
		void deallocate(value_type *ptr, std::size_t n)
		{
			auto *allocator(m_a.get());
			if (!allocator)
			{
				std::allocator <value_type> allocator;
				allocator.deallocate(ptr, n);
			}
		}
		
		std::size_t bytes_left() const
		{
			assert(m_a.get());
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
