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

#include <memory>


#ifndef ASM_LSW_STATIC_ALLOCATOR_HH
#define ASM_LSW_STATIC_ALLOCATOR_HH

namespace asm_lsw { namespace detail {
	template <typename t_element>
	class pool_allocator_impl
	{
	public:
		typedef t_element value_type;
		
	protected:
		std::unique_ptr <value_type[]> m_ptr{};
		std::size_t m_n{0};
		std::size_t m_idx{0};
		
	public:
		pool_allocator_impl() = default;
		pool_allocator_impl(pool_allocator_impl const &) = delete;
		pool_allocator_impl(pool_allocator_impl &&) = default;
		pool_allocator_impl &operator=(pool_allocator_impl const &) & = delete;
		pool_allocator_impl &operator=(pool_allocator_impl &&) & = default;
	
		pool_allocator_impl(std::size_t n):
			m_ptr(new value_type[n]),
			m_n(n)
		{
		}
		
		value_type *allocate(std::size_t n)
		{
			if (n + m_idx <= m_n)
			{
				value_type *retval(m_ptr.get() + m_idx);
				m_idx += n;
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
	public:
		typedef t_element value_type;
		
		template <typename U>
		struct rebind
		{
			typedef pool_allocator <U> other;
		};
		
	protected:
		std::shared_ptr<detail::pool_allocator_impl <value_type>> m_a;
		
	public:
		pool_allocator(std::size_t n):
			m_a(new detail::pool_allocator_impl <value_type>(n))
		{
		}
		
		value_type *allocate(std::size_t n) { return m_a->allocate(n); }
		void deallocate(value_type *, std::size_t n) {} // No-op.
	};
}

#endif