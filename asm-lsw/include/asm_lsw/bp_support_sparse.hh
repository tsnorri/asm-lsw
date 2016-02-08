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


#ifndef ASM_LSW_BP_SUPPORT_SPARSE_HH
#define ASM_LSW_BP_SUPPORT_SPARSE_HH

#include <asm_lsw/exception_tpl.hh>
#include <asm_lsw/util.hh>
#include <boost/format.hpp>
#include <cstdint>
#include <cstdio>
#include <sdsl/bp_support.hpp>


namespace asm_lsw {
	
	template <typename t_vector>
	class bp_support_sparse_base
	{
	public:
		typedef sdsl::bit_vector	bp_type;

	protected:
		// Apparently all bps classes require the sequence as a plain (non-compressed) bit vector.
		// Fortunately in this case the vector isn't sparse.
		bp_type						m_bp;
		t_vector					m_mask;
		
	public:
		bp_support_sparse_base(sdsl::bit_vector &&bp, sdsl::bit_vector &&mask):
			m_bp(std::move(bp)),
			m_mask(std::move(mask))
		{
		}
		
		bp_support_sparse_base() = default;
		bp_support_sparse_base(bp_support_sparse_base const &) = default;
		bp_support_sparse_base(bp_support_sparse_base &&) = default;
		bp_support_sparse_base &operator=(bp_support_sparse_base const &) & = default;
		bp_support_sparse_base &operator=(bp_support_sparse_base &&) & = default;
		
		decltype(m_bp) const &bp() const { return m_bp; }
		decltype(m_mask) const &mask() const { return m_mask; }
	};
	
	
	// A bps class for sparse sequences i.e. ones that may contain spaces in addition to () characters.
	template <
		typename t_bps		= sdsl::bp_support_sada<>,
		typename t_vector	= sdsl::bit_vector			// Unfortunately select is needed, otherwise rrr_vector might be preferrable.
	>
	class bp_support_sparse : public bp_support_sparse_base<t_vector>
	{
	public:
		enum class input_value : uint8_t
		{
			Space,
			Opening,
			Closing
		};

		enum class error : uint32_t
		{
			no_error = 0,
			bad_parenthesis,
			out_of_range,
			sparse_index
		};
		
		typedef bp_support_sparse_base<t_vector>	base_class;
		typedef typename base_class::bp_type		bp_type;
		typedef typename t_bps::size_type			size_type;
		
	protected:
		// Container for rank and select support.
		struct rss
		{
			typename t_vector::rank_1_type			mask_rank1_support;
			typename t_vector::select_1_type		mask_select1_support;
			
			rss(t_vector const &mask):
				mask_rank1_support(&mask),
				mask_select1_support(&mask)
			{
			}
			
			rss() = default;
			rss(rss const &) = default;
			rss(rss &&) = default;
			rss &operator=(rss const &) & = default;
			rss &operator=(rss &&) & = default;

			void fix_support(t_vector const &mask)
			{
				mask_rank1_support.set_vector(&mask);
				mask_select1_support.set_vector(&mask);
			}
		};
		
	protected:
		t_bps	m_bps;
		rss		m_rss;
		
	protected:
		template <typename t_int_vector>
		static void fill_int_vector(t_int_vector &dst, char const *src);

	public:
		static void from_string(bp_support_sparse &bps, char const *input);
		
		template <typename t_input_vector>
		static void from_input_value_vector(bp_support_sparse &bps, t_input_vector const &bps_seq);
		
		bp_support_sparse(sdsl::bit_vector &&bp, sdsl::bit_vector &&mask):
			base_class(std::move(bp), std::move(mask)),
			m_bps(&this->m_bp),
			m_rss(this->m_mask)
		{
		}
		
		
		bp_support_sparse(): base_class() {}
		
		
		bp_support_sparse(bp_support_sparse const &other):
			base_class(other),
			m_bps(other.m_bp),
			m_rss(other.m_rss)
		{
			m_bps.set_vector(&this->m_bp);
			m_rss.fix_support(this->m_mask);
		}
		
		
		bp_support_sparse(bp_support_sparse &&other):
			base_class(std::move(other)),
			m_bps(std::move(other.m_bp)),
			m_rss(std::move(other.m_rss))
		{
			m_bps.set_vector(&this->m_bp);
			m_rss.fix_support(this->m_mask);
		}
		
		
		bp_support_sparse &operator=(bp_support_sparse const &other) &;
		bp_support_sparse &operator=(bp_support_sparse && other) &;
		
		template <typename t_input_vector>
		bp_support_sparse(t_input_vector const &bps_seq);
		
		decltype(m_bps) const &bps() const { return m_bps; }
		auto to_bp_idx(size_type const i) const -> typename bp_type::size_type;
		size_type to_sparse_idx(typename bp_type::size_type const idx) const;
		size_type find_open(size_type i) const;
		size_type find_close(size_type i) const;
		size_type rank(size_type i) const;
		size_type select(size_type i) const;
		
		// TODO: implement rmq_open
		// TODO: implement enclose
		// TODO: implement rr_enclose
		// TODO: implement double_enclose
	};
	
	
	template <typename t_bps, typename t_vector>
	template <typename t_int_vector>
	void bp_support_sparse <t_bps, t_vector>::fill_int_vector(t_int_vector &dst, char const *src)
	{
		typedef typename std::underlying_type<input_value>::type input_value_u;
		
		auto const len(dst.size());
		for (remove_c_t<decltype(len)> i(0); i < len; ++i)
		{
			input_value value(input_value::Space);
			switch (src[i])
			{
				case ' ':
					break;
					
				case '(':
					value = input_value::Opening;
					break;
					
				case ')':
					value = input_value::Closing;
					break;
					
				default:
					throw std::invalid_argument(boost::str(boost::format("Unexpected character '%c' at index %u.") % src[i] % i));
					break;
			}
			
			dst[i] = static_cast<input_value_u>(value);
		}
	}
	

	template <typename t_bps, typename t_vector>
	void bp_support_sparse <t_bps, t_vector>::from_string(bp_support_sparse &bps, char const *input)
	{
		typedef typename std::underlying_type<input_value>::type input_value_u;
		
		auto const len(strlen(input));
		sdsl::int_vector<std::numeric_limits<input_value_u>::digits> converted_input(len, 0);
		fill_int_vector(converted_input, input);
		bp_support_sparse tmp(converted_input);
		bps = std::move(tmp);
	}
	
	
	template <typename t_bps, typename t_vector>
	template <typename t_input_vector>
	void bp_support_sparse <t_bps, t_vector>::from_input_value_vector(bp_support_sparse &bps, t_input_vector const &bps_seq)
	{
		typename t_input_vector::size_type i(0);
		typename t_input_vector::size_type j(0);
		typename t_input_vector::size_type p_length(0);
		sdsl::bit_vector mask(bps_seq.size(), 0);
		
		// Count the () characters.
		for (typename t_input_vector::value_type c : bps_seq)
		{
			switch (static_cast<input_value>(c))
			{
				case input_value::Space:
					break;
					
				case input_value::Opening:
				case input_value::Closing:
					++p_length;
					break;
					
				default:
					throw std::invalid_argument(boost::str(boost::format("Got unexpected value '%u'.") % c));
					break;
			}
		}
		
		sdsl::bit_vector bp(p_length, 0);
		
		// Fill the indices and the supported vector.
		for (typename t_input_vector::value_type c : bps_seq)
		{
			switch (static_cast<input_value>(c))
			{
				case input_value::Space:
					++i;
					break;
					
				case input_value::Opening:
					bp[j] = 1;
					// Fall through.
					
				case input_value::Closing:
					mask[i] = 1;
					++i;
					++j;
					break;
					
				default:
					// Indicates that the string has changed.
					assert(0);
					break;
			}
		}
		
		bp_support_sparse tmp(std::move(bp), std::move(mask));
		bps = std::move(tmp);
	}
	
	
	template <typename t_bps, typename t_vector>
	auto bp_support_sparse <t_bps, t_vector>::operator=(bp_support_sparse const &other) & -> bp_support_sparse &
	{
		base_class::operator=(other);
		m_bps = other.m_bps;
		m_rss = other.m_rss;
		m_bps.set_vector(&this->m_bp);
		m_rss.fix_support(this->m_mask);
		return *this;
	}
	
	
	template <typename t_bps, typename t_vector>
	auto bp_support_sparse <t_bps, t_vector>::operator=(bp_support_sparse && other) & -> bp_support_sparse &
	{
		base_class::operator=(std::move(other));
		m_bps = std::move(other.m_bps);
		m_rss = std::move(other.m_rss);
		m_bps.set_vector(&this->m_bp);
		m_rss.fix_support(this->m_mask);
		return *this;
	}
	
	
	template <typename t_bps, typename t_vector>
	template <typename t_input_vector>
	bp_support_sparse <t_bps, t_vector>::bp_support_sparse(t_input_vector const &bps_seq):
		base_class()
	{
		{
			typename t_input_vector::size_type i(0);
			typename t_input_vector::size_type j(0);
			typename t_input_vector::size_type p_length(0);
			sdsl::bit_vector mask(bps_seq.size(), 0);
			
			// Count the () characters.
			for (typename t_input_vector::value_type c : bps_seq)
			{
				switch (static_cast<input_value>(c))
				{
					case input_value::Space:
						break;
						
					case input_value::Opening:
					case input_value::Closing:
						++p_length;
						break;
						
					default:
						throw std::invalid_argument(boost::str(boost::format("Got unexpected value '%u'") % c));
						break;
				}
			}
			
			sdsl::bit_vector bp(p_length, 0);
			
			// Fill the indices and the supported vector.
			for (typename t_input_vector::value_type c : bps_seq)
			{
				switch (static_cast<input_value>(c))
				{
					case input_value::Space:
						++i;
						break;
						
					case input_value::Opening:
						bp[j] = 1;
						// Fall through.
						
					case input_value::Closing:
						mask[i] = 1;
						++i;
						++j;
						break;
						
					default:
						// Indicates that the string has changed.
						assert(0);
						break;
				}
			}
			
			// Update instance variables.
			this->m_bp = std::move(bp);
			this->m_mask = std::move(mask);
		}
		
		// Update more instance variables.
		{
			decltype(m_bps) tmp{&this->m_bp};
			m_bps = std::move(tmp);
		}
		
		{
			rss tmp(this->m_mask);
			m_rss = std::move(tmp);
		}
	}

	
	template <typename t_bps, typename t_vector>
	auto bp_support_sparse <t_bps, t_vector>::to_bp_idx(size_type const i) const -> typename bp_type::size_type
	{
		auto retval(m_rss.mask_rank1_support(1 + i) - 1);
		return retval;
	}

	
	template <typename t_bps, typename t_vector>
	auto bp_support_sparse <t_bps, t_vector>::to_sparse_idx(typename bp_type::size_type const idx) const -> size_type
	{
		auto retval(m_rss.mask_select1_support(1 + idx));
		return retval;
	}
	
	
	template <typename t_bps, typename t_vector>
	auto bp_support_sparse <t_bps, t_vector>::find_open(size_type i) const -> size_type
	{
		asm_lsw_assert(i < this->m_mask.size(), std::invalid_argument, error::out_of_range);
		asm_lsw_assert(1 == this->m_mask[i], std::invalid_argument, error::sparse_index);

		auto bp_end(to_bp_idx(i));
		asm_lsw_assert(0 == this->m_bp[bp_end], std::invalid_argument, error::bad_parenthesis);

		auto bp_begin(m_bps.find_open(bp_end));
		auto begin(to_sparse_idx(bp_begin));
		return begin;
	}
	
	
	template <typename t_bps, typename t_vector>
	auto bp_support_sparse <t_bps, t_vector>::find_close(size_type i) const -> size_type
	{
		asm_lsw_assert(i < this->m_mask.size(), std::invalid_argument, error::out_of_range);
		asm_lsw_assert(1 == this->m_mask[i], std::invalid_argument, error::sparse_index);
		
		auto bp_begin(to_bp_idx(i));
		asm_lsw_assert(1 == this->m_bp[bp_begin], std::invalid_argument, error::bad_parenthesis);

		auto bp_end(m_bps.find_close(bp_begin));
		auto end(to_sparse_idx(bp_end));
		return end;
	}

	
	template <typename t_bps, typename t_vector>
	auto bp_support_sparse <t_bps, t_vector>::rank(size_type i) const -> size_type
	{
		asm_lsw_assert(i < this->m_mask.size(), std::invalid_argument, error::out_of_range);

		auto bp_i(m_rss.mask_rank1_support(1 + i) - 1);
		auto retval(m_bps.rank(bp_i));
		return retval;
	}
	
	
	template <typename t_bps, typename t_vector>
	auto bp_support_sparse <t_bps, t_vector>::select(size_type i) const -> size_type
	{
		auto bp_i(m_bps.select(i));
		auto retval(m_rss.mask_select1_support(1 + bp_i));
		return retval;
	}
}

#endif
