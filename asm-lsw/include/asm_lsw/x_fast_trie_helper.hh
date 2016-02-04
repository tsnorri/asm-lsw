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


#ifndef ASM_LSW_X_FAST_TRIE_HELPER_HH
#define ASM_LSW_X_FAST_TRIE_HELPER_HH

#include <array>


namespace asm_lsw {

	template <
		typename t_key,
		typename t_value,
		template <typename ...> class t_map,
		template <template <typename ...> class, typename, typename> class t_map_adaptor
	>
	struct x_fast_trie_base_spec
	{
		typedef t_key key_type;														// Trie key type.
		typedef t_value value_type;													// Trie value type.

		template <typename ... Args>
		using map_type = t_map <Args ...>;											// Hash set / map type.
		
		template <typename t_map_key, typename t_map_value>
		using map_adaptor_type = t_map_adaptor <map_type, t_map_key, t_map_value>;	// Hash map adaptor. Parameters are map type, key type and value type.
	};


	// FIXME: used by the base class.
	template <typename t_key>
	class x_fast_trie_node_value
	{
	protected:
		typedef t_key key_type;

	protected:
		key_type m_key;			// Key of the child node in LSS.
		bool m_is_descendant;	// If true, points to m_leaf_links.
		
	public:
		x_fast_trie_node_value(): x_fast_trie_node_value(0, false) {};

		x_fast_trie_node_value(key_type key, bool is_descendant):
			m_key(key),
			m_is_descendant(is_descendant)
		{
		}

		key_type key() const { return m_key; }
		bool is_descendant() const { return m_is_descendant; }	
	};


	// FIXME: used by the base class.
	template <typename t_key, typename t_value>
	struct x_fast_trie_leaf_link
	{
		// FIXME: use iterators instead for prev and next?
		t_key prev;
		t_key next;
		t_value value;
		
		// FIXME: movability for val?
		x_fast_trie_leaf_link(): prev(0), next(0) {}
		x_fast_trie_leaf_link(t_key p, t_key n, t_value val): prev(p), next(n), value(val) {}
	};
	
	
	// Specialization for a set type collection.
	template <typename t_key>
	struct x_fast_trie_leaf_link <t_key, void>
	{
		// FIXME: use iterators instead for prev and next?
		t_key prev;
		t_key next;
		
		x_fast_trie_leaf_link(): x_fast_trie_leaf_link(0, 0) {}
		x_fast_trie_leaf_link(t_key p, t_key n): prev(p), next(n) {}
	};
	
	
	template <typename t_key, typename t_value>
	struct x_fast_trie_trait
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
			return it->second.value;
		}
	};
	
	
	template <typename t_key>
	struct x_fast_trie_trait <t_key, void>
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
			return it->first;
		}

		template <typename t_iterator>
		value_type const &value(t_iterator it)
		{
			return it->first;
		}
	};
}

#endif
