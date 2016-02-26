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
		template <typename, typename, typename> class t_map_adaptor_trait,
		typename t_lss_find_fn
	>
	struct x_fast_trie_base_spec
	{
		typedef t_key key_type;		// Trie key type.
		typedef t_value value_type;	// Trie value type.

		// A class that has a member type “type” which is the actual adaptor.
		// Other parameteres are supposed to have been fixed earlier.
		template <typename t_map_key, typename t_map_value, typename t_map_access_key = map_adaptor_access_key <t_map_key>>
		using map_adaptor_trait = t_map_adaptor_trait <t_map_key, t_map_value, t_map_access_key>;
		
		typedef t_lss_find_fn lss_find_fn;
	};


	// FIXME: used by the base class.
	// FIXME: refactor so that m_is_descendant is not needed.
	template <typename t_key>
	class x_fast_trie_edge
	{
	protected:
		typedef t_key key_type;

	protected:
		key_type m_key;			// Key of the child node in LSS.
		bool m_is_descendant;	// If true, points to m_leaf_links.
		
	public:
		x_fast_trie_edge(): x_fast_trie_edge(0, false) {};
		
		x_fast_trie_edge(key_type key):
			m_key(key),
			m_is_descendant(false)
		{
		}

		x_fast_trie_edge(key_type key, bool is_descendant):
			m_key(key),
			m_is_descendant(is_descendant)
		{
		}

		key_type key() const { return m_key; }
		key_type level_key(std::size_t level) const { assert(0 <= level); return m_key >> level; }
		bool is_descendant() const { return m_is_descendant; }
		
#if 0
		// FIXME: Takes into account all the bits, not just the level-specific ones.
		bool operator==(x_fast_trie_edge const &other) const
		{
			return m_key == other.m_key;
		}
#endif
	};
	
	
	template <typename t_key>
	class x_fast_trie_node : protected std::array <x_fast_trie_edge <t_key>, 2>
	{
	protected:
		typedef std::array <x_fast_trie_edge <t_key>, 2> base_class;
		
	public:
		using base_class::base_class;
		using base_class::operator[];
		
#if 0
		bool operator==(x_fast_trie_node const &other) const
		{
			std::cout << "node equals: (" << +((*this)[0].key()) << ", " << +((*this)[1].key()) << ") other: (" << +(other[0].key()) << ", " << +(other[1].key()) << ")" << std::endl;
			return (*this)[0] == other[0] && (*this)[1] == other[1];
		}
#endif

		class access_key
		{
		protected:
			uint8_t m_level;

		public:
			typedef x_fast_trie_node <t_key> key_type;
			typedef t_key accessed_type;
			
			access_key(): m_level(0) {}
			access_key(uint8_t const level): m_level(level) {}
			
			uint8_t level() const { return m_level; }
			
			accessed_type operator()(key_type const &node) const
			{
				return node[0].level_key(m_level);
			}
		};
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
