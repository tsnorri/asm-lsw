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


#ifndef ASM_LSW_CST_EDGE_ADAPTOR_HH
#define ASM_LSW_CST_EDGE_ADAPTOR_HH

#include <asm_lsw/util.hh>
#include <boost/iterator/iterator_facade.hpp>
#include <cassert>


namespace asm_lsw { namespace detail {
	
	template <typename t_adaptor>
	class cst_edge_adaptor_iterator_tpl : public boost::iterator_facade <
		cst_edge_adaptor_iterator_tpl <t_adaptor>,
		typename t_adaptor::value_type,
		boost::random_access_traversal_tag,
		typename t_adaptor::value_type
	>
	{
		friend class boost::iterator_core_access;

	protected:
		typedef boost::iterator_facade <
			cst_edge_adaptor_iterator_tpl <t_adaptor>,
			typename t_adaptor::value_type,
			boost::random_access_traversal_tag,
			typename t_adaptor::value_type
		> base_class;
		
	public:
		typedef typename t_adaptor::size_type size_type;
		typedef typename base_class::difference_type difference_type;
		
	protected:
		t_adaptor const *m_adaptor{nullptr};
		size_type m_idx{0};
		
	private:
		// For the converting constructor below.
		struct enabler {};
		
	public:
		cst_edge_adaptor_iterator_tpl() = default;
		
		cst_edge_adaptor_iterator_tpl(t_adaptor const &adaptor, size_type idx):
			m_adaptor(&adaptor),
			m_idx(idx)
		{
		}
		
	private:
		void increment() { advance(1); }
		void decrement() { advance(-1); }
		
		void advance(difference_type n);
		
		difference_type distance_to(cst_edge_adaptor_iterator_tpl <t_adaptor> const &other) const;
		
		bool equal(cst_edge_adaptor_iterator_tpl <t_adaptor> const &other) const ASM_LSW_PURE;
		
		typename t_adaptor::value_type dereference() const;
	};
}}


namespace asm_lsw {
	
	template <typename t_cst>
	class cst_edge_adaptor
	{
	public:
		typedef t_cst							cst_type;
		typedef typename cst_type::node_type	node_type;
		typedef typename cst_type::size_type	size_type;
		typedef typename cst_type::char_type	value_type;
		
		typedef detail::cst_edge_adaptor_iterator_tpl <cst_edge_adaptor> const_iterator;
		
	protected:
		cst_type const		*m_cst{nullptr};
		node_type			m_node{};
		size_type			m_length{0};
		
	public:
		cst_edge_adaptor() {}
		
		cst_edge_adaptor(cst_type const &cst, node_type node):
			cst_edge_adaptor(cst, node, cst.depth(node))
		{
		}
		
		cst_edge_adaptor(cst_type const &cst, node_type node, size_type length):
			m_cst(&cst),
			m_node(node),
			m_length(length)
		{
		}
		
		size_type size() const { return m_length; }
		value_type operator[](size_type i) const { return m_cst->edge(m_node, 1 + i); }
		const_iterator cbegin() const	{ return const_iterator(*this, 0); }
		const_iterator cend() const		{ return const_iterator(*this, m_length); }
	};
}


namespace asm_lsw { namespace detail {

	template <typename t_adaptor>
	void cst_edge_adaptor_iterator_tpl <t_adaptor>::advance(difference_type n)
	{
		m_idx += n;
	}
	
	
	template <typename t_adaptor>
	auto cst_edge_adaptor_iterator_tpl <t_adaptor>::distance_to(
		cst_edge_adaptor_iterator_tpl <t_adaptor> const &other
	) const -> difference_type
	{
		assert(m_adaptor == other.m_adaptor);
		return other.m_idx - m_idx;
	}
	
	
	template <typename t_adaptor>
	bool cst_edge_adaptor_iterator_tpl <t_adaptor>::equal(
		cst_edge_adaptor_iterator_tpl <t_adaptor> const &other
	) const
	{
		return (this->m_adaptor == other.m_adaptor &&
				this->m_idx == other.m_idx);
	}
	
	
	template <typename t_adaptor>
	auto cst_edge_adaptor_iterator_tpl <t_adaptor>::dereference() const -> typename t_adaptor::value_type
	{
		return (*m_adaptor)[m_idx];
	}
}}

#endif
