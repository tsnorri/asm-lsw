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


#ifndef ASM_LSW_X_FAST_TRIE_COMPACT_HH
#define ASM_LSW_X_FAST_TRIE_COMPACT_HH

#include <asm_lsw/x_fast_trie_base.hh>


namespace asm_lsw {
	
	template <typename t_key, typename t_value>
	class x_fast_trie;
	
	
	template <typename t_key, typename t_value>
	using x_fast_trie_compact_spec = x_fast_trie_base_spec <t_key, t_value, std::vector, map_adaptor_phf>;
	
	
	// Use perfect hashing instead of the one provided by STL.
	template <typename t_key, typename t_value = void>
	class x_fast_trie_compact : public x_fast_trie_base <x_fast_trie_compact_spec <t_key, t_value>>
	{
	protected:
		typedef x_fast_trie_base <x_fast_trie_compact_spec <t_key, t_value>> base_class;
		typedef typename base_class::lss lss;
		typedef typename base_class::level_map level_map;
		typedef typename base_class::leaf_link_map leaf_link_map;
		
	public:
		typedef typename base_class::key_type key_type;
		typedef typename base_class::value_type value_type;
		typedef typename base_class::iterator iterator;
		typedef typename base_class::const_iterator const_iterator;
		
	public:
		x_fast_trie_compact() = default;
		x_fast_trie_compact(x_fast_trie_compact const &) = default;
		x_fast_trie_compact(x_fast_trie_compact &&) = default;
		x_fast_trie_compact &operator=(x_fast_trie_compact const &) & = default;
		x_fast_trie_compact &operator=(x_fast_trie_compact &&) & = default;
		
		explicit x_fast_trie_compact(x_fast_trie <key_type, value_type> &other);
	};
	
	
	template <typename t_key, typename t_value>
	x_fast_trie_compact <t_key, t_value>::x_fast_trie_compact(x_fast_trie <key_type, value_type> &other):
		x_fast_trie_compact()
	{
		// LSS
		typename lss::size_type const count(this->m_lss.size());
		assert(other.m_lss.size() == count);
		for (typename lss::size_type i(0); i < count; ++i)
		{
			// Move the contents of the other map.
			auto &other_adaptor(other.m_lss[i]);
			level_map adaptor(other_adaptor.map());
			
			// Store the new adaptor.
			this->m_lss[i] = std::move(adaptor);
		}
		
		// Leaf links
		leaf_link_map adaptor(other.m_leaf_links.map());
		this->m_leaf_links = std::move(adaptor);
		
		// Min. value
		this->m_min = other.m_min;
	}
}

#endif
