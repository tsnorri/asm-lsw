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


#ifndef ASM_LSW_Y_FAST_TRIE_COMPACT_HH
#define ASM_LSW_Y_FAST_TRIE_COMPACT_HH

#include <asm_lsw/fast_trie_common.hh>
#include <asm_lsw/y_fast_trie_base.hh>
#include <vector>


namespace asm_lsw {
	
	template <typename t_spec>
	class map_adaptor_phf;
	
	template <typename t_key, typename t_value>
	class x_fast_trie_compact;
	
	template <typename t_key, typename t_value>
	class y_fast_trie;
	
		
	template <typename t_key, typename t_value>
	using y_fast_trie_compact_spec = y_fast_trie_base_spec <
		t_key,
		t_value,
		fast_trie_compact_map_adaptor,
		x_fast_trie_compact <t_key, void>
	>;
	
	
	// Use perfect hashing instead of the one provided by STL.
	template <typename t_key, typename t_value = void>
	class y_fast_trie_compact : public y_fast_trie_base <y_fast_trie_compact_spec <t_key, t_value>>
	{
	public:
		typedef y_fast_trie_base <y_fast_trie_compact_spec <t_key, t_value>> base_class;
		typedef typename base_class::key_type key_type;
		typedef typename base_class::value_type value_type;
		typedef typename base_class::size_type size_type;
		typedef typename base_class::subtree_iterator subtree_iterator;
		typedef typename base_class::const_subtree_iterator const_subtree_iterator;
		typedef typename base_class::iterator iterator;
		typedef typename base_class::const_iterator const_iterator;
		
	protected:
		typedef typename base_class::subtree_map subtree_map;
		
	public:
		y_fast_trie_compact() = default;
		y_fast_trie_compact(y_fast_trie_compact const &) = default;
		y_fast_trie_compact(y_fast_trie_compact &&) = default;
		y_fast_trie_compact &operator=(y_fast_trie_compact const &) & = default;
		y_fast_trie_compact &operator=(y_fast_trie_compact &&) & = default;
		
		explicit y_fast_trie_compact(y_fast_trie <key_type, value_type> &other):
			base_class(other.m_reps, other.m_subtrees.map(), other.m_size)
		{
		}
	};
}

#endif
