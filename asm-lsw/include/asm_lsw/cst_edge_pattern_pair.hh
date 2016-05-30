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


#ifndef ASM_LSW_CST_EDGE_PATTERN_PAIR_HH
#define ASM_LSW_CST_EDGE_PATTERN_PAIR_HH

#include <asm_lsw/cst_edge_adaptor.hh>


namespace asm_lsw { namespace detail {
	
	template <typename t_pair>
	class cst_edge_pattern_pair_iterator_tpl : public boost::iterator_facade <
		cst_edge_pattern_pair_iterator_tpl <t_pair>,
		typename t_pair::value_type,
		boost::random_access_traversal_tag,
		typename t_pair::value_type
	>
	{
		friend class boost::iterator_core_access;

	protected:
		typedef boost::iterator_facade <
			cst_edge_pattern_pair_iterator_tpl <t_pair>,
			typename t_pair::value_type,
			boost::random_access_traversal_tag,
			typename t_pair::value_type
		> base_class;
		
	public:
		typedef typename t_pair::size_type size_type;
		typedef typename base_class::difference_type difference_type;
		
	protected:
		t_pair const *m_pair{nullptr};
		size_type m_idx{0};
		
	private:
		// For the converting constructor below.
		struct enabler {};
		
	public:
		cst_edge_pattern_pair_iterator_tpl() = default;
		
		cst_edge_pattern_pair_iterator_tpl(t_pair const &pair, size_type idx):
			m_pair(&pair),
			m_idx(idx)
		{
		}
		
	private:
		void increment() { advance(1); }
		void decrement() { advance(-1); }
		
		void advance(difference_type n);
		
		difference_type distance_to(cst_edge_pattern_pair_iterator_tpl <t_pair> const &other) const;
		
		bool equal(cst_edge_pattern_pair_iterator_tpl <t_pair> const &other) const;
		
		typename t_pair::value_type dereference() const;
	};
}}


namespace asm_lsw {
	
	template <typename t_cst, typename t_pattern>
	class cst_edge_pattern_pair
	{
	public:
		typedef t_cst							cst_type;
		typedef typename cst_type::node_type	node_type;
		typedef t_pattern						pattern_type;
		typedef typename cst_type::size_type	size_type;
		typedef typename std::common_type <
			typename cst_type::char_type,
			typename pattern_type::value_type
		>::type									value_type;
		typedef detail::cst_edge_pattern_pair_iterator_tpl <cst_edge_pattern_pair> const_iterator;
		
	protected:
		cst_edge_adaptor <t_cst>	m_edge_adaptor;
		pattern_type const			*m_pattern{nullptr};
		size_type					m_pattern_start{0};
		
	public:
		cst_edge_pattern_pair() {}
		
		cst_edge_pattern_pair(
			cst_type const &cst,
			typename cst_type::node_type node,
			pattern_type const &pattern,
			size_type pattern_start
		):
			m_edge_adaptor(cst, node, cst.depth(node)),
			m_pattern(&pattern),
			m_pattern_start(pattern_start)
		{
		}
		
		cst_edge_pattern_pair(
			cst_type const &cst,
			typename cst_type::node_type node,
			size_type length,
			pattern_type const &pattern,
			size_type pattern_start
		):
			m_edge_adaptor(cst, node, length),
			m_pattern(&pattern),
			m_pattern_start(pattern_start)
		{
		}
		
		size_type size() const { return m_edge_adaptor.size() + (m_pattern->size() - m_pattern_start); }
		value_type operator[](size_type i) const;
		const_iterator cbegin() const	{ return const_iterator(*this, 0); }
		const_iterator cend() const		{ return const_iterator(*this, size()); }
		explicit operator std::vector <value_type>() const { return std::vector <value_type> (cbegin(), cend()); }
	};
	
	
	template <typename t_cst, typename t_pattern>
	auto cst_edge_pattern_pair <t_cst, t_pattern>::operator[](size_type i) const -> value_type
	{
		assert(i < size());
		
		auto const edge_size(m_edge_adaptor.size());
		if (i < edge_size)
			return m_edge_adaptor[i];
		else
			return (*m_pattern)[i - edge_size + m_pattern_start];
	}
}


namespace asm_lsw { namespace detail {

	template <typename t_pair>
	void cst_edge_pattern_pair_iterator_tpl <t_pair>::advance(difference_type n)
	{
		m_idx += n;
	}
	
	
	template <typename t_pair>
	auto cst_edge_pattern_pair_iterator_tpl <t_pair>::distance_to(
		cst_edge_pattern_pair_iterator_tpl <t_pair> const &other
	) const -> difference_type
	{
		assert(m_pair == other.m_pair);
		return other.m_idx - m_idx;
	}
	
	
	template <typename t_pair>
	bool cst_edge_pattern_pair_iterator_tpl <t_pair>::equal(
		cst_edge_pattern_pair_iterator_tpl <t_pair> const &other
	) const
	{
		return (this->m_pair == other.m_pair &&
				this->m_idx == other.m_idx);
	}
	
	
	template <typename t_pair>
	auto cst_edge_pattern_pair_iterator_tpl <t_pair>::dereference() const -> typename t_pair::value_type
	{
		return (*m_pair)[m_idx];
	}
}}

#endif
