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


#ifndef ASM_LSW_Y_FAST_TRIE_HELPER_HH
#define ASM_LSW_Y_FAST_TRIE_HELPER_HH

#include <map>
#include <set>


namespace asm_lsw {
	
	template <typename t_key, typename t_value>
	struct y_fast_trie_subtree_trait
	{
		typedef std::map <t_key, t_value> subtree_type;
	};

	
	template <typename t_key>
	struct y_fast_trie_subtree_trait <t_key, void>
	{
		typedef std::set <t_key> subtree_type;
	};

	
	template <
		typename t_key,
		typename t_value,
		template <typename ...> class t_map,
		template <template <typename ...> class, typename, typename> class t_map_adaptor,
		template <typename, typename> class t_x_fast_trie
	>
	struct y_fast_trie_base_spec
	{
		typedef t_key key_type;														// Trie key type.
		typedef t_value value_type;													// Trie value type.
		
		template <typename ... Args>
		using map_type = t_map <Args ...>;											// Hash map type.
		
		template <typename t_map_key, typename t_map_value>
		using map_adaptor_type = t_map_adaptor <map_type, t_map_key, t_map_value>;	// Hash map adaptor. Parameters are map type, key type and value type.
		
		typedef t_x_fast_trie <t_key, void> trie_type;								// Representative trie type.
		
		// Subtree type.
		typedef typename y_fast_trie_subtree_trait<key_type, value_type>::subtree_type subtree_type;
	};

	
	template <typename t_key, typename t_value>
	struct y_fast_trie_trait
	{
		typedef t_key key_type;
		typedef t_value value_type;

		constexpr bool has_value() const
		{
			return true;
		}
		
		template <typename t_iterator>
		key_type key(t_iterator it) const
		{
			return it->first;
		}

		template <typename t_iterator>
		value_type const &value(t_iterator it) const
		{
			return it->second;
		}
	};
	
	
	template <typename t_key>
	struct y_fast_trie_trait <t_key, void>
	{
		typedef t_key key_type;
		typedef t_key value_type;
		
		constexpr bool has_value() const
		{
			return false;
		}

		template <typename t_iterator>
		key_type key(t_iterator it)
		{
			return *it;
		}

		template <typename t_iterator>
		value_type const &value(t_iterator it)
		{
			return *it;
		}
	};
}

#endif
