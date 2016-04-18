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

#ifndef ASM_LSW_K1_MATRIX_HH
#define ASM_LSW_K1_MATRIX_HH

#include <sdsl/int_vector.hpp>


namespace asm_lsw {
	
	template <uint8_t t_bits>
	class matrix
	{
	protected:
		typedef sdsl::int_vector <t_bits> content_type;
		typedef typename content_type::reference reference;
		typedef typename content_type::const_reference const_reference;
		
	public:
		typedef typename content_type::size_type size_type;
		typedef typename content_type::value_type value_type;
		
	protected:
		content_type m_columns;
		size_type m_row_count;
		
	public:
		matrix():
			m_row_count(0)
		{
		}
		
		matrix(size_type const rows, size_type const columns, uint8_t bits, value_type const default_value):
			m_columns(rows * columns, default_value, bits),
			m_row_count(rows)
		{
			assert(0 == t_bits || t_bits == bits);
		}
		
		matrix(size_type const rows, size_type const columns, value_type const default_value):
			m_columns(rows * columns, default_value, t_bits),
			m_row_count(rows)
		{
		}
		
		static uint64_t max_value(uint8_t width) { return content_type::max_value(width); }
		uint64_t max_value() const { return m_columns.max_value(); }
		
		size_type rows() const { return m_row_count; }
		size_type columns() const { return m_row_count ? m_columns.size() / m_row_count : 0; }
		
		inline reference operator()(size_type const i, size_type const j)
		{
			assert(i < rows());
			assert(j < columns());
			return m_columns[i + j * m_row_count];
		}
		
		inline const_reference operator()(size_type const i, size_type const j) const
		{
			assert(i < rows());
			assert(j < columns());
			return m_columns[i + j * m_row_count];
		}
		
		void fill(value_type const value) { std::fill(m_columns.begin(), m_columns.end(), value); }
	};
}

#endif
