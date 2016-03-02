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

#include <asm_lsw/map_adaptor_helper.hh>
#include <map>
#include <set>


namespace asm_lsw {

	template <typename t_subtree_map>
	class y_fast_trie_subtree_map_proxy
	{
	protected:
		t_subtree_map const *m_map;

	public:
		typedef typename t_subtree_map::size_type size_type;
		typedef typename t_subtree_map::iterator iterator;
		typedef typename t_subtree_map::const_iterator const_iterator;

	public:
		y_fast_trie_subtree_map_proxy() = delete;
		explicit y_fast_trie_subtree_map_proxy(t_subtree_map const *map): m_map(map) {}
		size_type size() const			{ return m_map->size(); }
		const_iterator begin() const	{ return m_map->begin(); }
		const_iterator end() const		{ return m_map->end(); }
		const_iterator cbegin() const	{ return m_map->cbegin(); }
		const_iterator cend() const		{ return m_map->cend(); }
	};
}


namespace asm_lsw { namespace detail {
	
	template <typename, typename>
	struct y_fast_trie_trait;
	
	
	template <
		typename t_key,
		typename t_value,
		template <typename, typename, typename> class t_map_adaptor,
		typename t_x_fast_trie
	>
	struct y_fast_trie_base_spec
	{
		typedef t_key key_type;				// Trie key type.
		typedef t_value value_type;			// Trie value type.
		
		// Hash map adaptor. Parameters are map type, key type and value type.
		// Key type and allocator are supposed to have been fixed earlier.
		template <typename t_map_key, typename t_map_value, typename t_map_access_key = map_adaptor_access_key <t_key>>
		using map_adaptor = t_map_adaptor <t_map_key, t_map_value, t_map_access_key>;
		
		typedef t_x_fast_trie trie_type;	// Representative trie type.
		
		// Subtree type.
		typedef typename y_fast_trie_trait <key_type, value_type>::subtree_type subtree_type;
	};

	
	template <typename t_key, typename t_value>
	struct y_fast_trie_trait
	{
		typedef t_key key_type;
		typedef t_value value_type;
		typedef std::map <t_key, t_value> subtree_type;

		constexpr bool has_value() const
		{
			return true;
		}
		
		template <typename t_iterator>
		key_type key(t_iterator const &it) const
		{
			return it->first;
		}

		template <typename t_iterator>
		value_type const &value(t_iterator const &it) const
		{
			return it->second;
		}
	};
	
	
	template <typename t_key>
	struct y_fast_trie_trait <t_key, void>
	{
		typedef t_key key_type;
		typedef t_key value_type;
		typedef std::set <t_key> subtree_type;
		
		constexpr bool has_value() const
		{
			return false;
		}

		template <typename t_iterator>
		key_type key(t_iterator const &it)
		{
			return *it;
		}

		template <typename t_iterator>
		value_type const &value(t_iterator const &it)
		{
			return *it;
		}
	};
	
	
	template <typename, typename>
	class y_fast_trie_base_subtree_iterator_wrapper_impl;
		
		
	template <typename t_reference>
	class y_fast_trie_base_subtree_iterator_wrapper
	{
	protected:
		typedef y_fast_trie_base_subtree_iterator_wrapper <t_reference> this_class;
		
	public:
		virtual void advance(std::ptrdiff_t const) = 0;
		virtual std::ptrdiff_t distance_to(this_class const &other) const = 0;
		virtual bool equal(this_class const &other) const = 0;
		virtual t_reference dereference() const = 0;
		
		template <typename t_iterator>
		static this_class *construct(t_iterator &it)
		{
			return new y_fast_trie_base_subtree_iterator_wrapper_impl <t_iterator, t_reference>(it);
		}
	};
	
	
	template <typename t_iterator, typename t_reference>
	class y_fast_trie_base_subtree_iterator_wrapper_impl : public y_fast_trie_base_subtree_iterator_wrapper <t_reference>
	{
	protected:
		typedef y_fast_trie_base_subtree_iterator_wrapper <t_reference> base_class;
		typedef y_fast_trie_base_subtree_iterator_wrapper_impl <t_iterator, t_reference> this_class;
		
	protected:
		t_iterator m_it;
		
	public:
		y_fast_trie_base_subtree_iterator_wrapper_impl(t_iterator &it): m_it(std::move(it)) {}

		virtual void advance(std::ptrdiff_t const size) override { std::advance(m_it, size); }
		virtual t_reference dereference() const override { return *m_it; }

		virtual bool equal(base_class const &other) const override
		{
			return dynamic_cast <this_class const &>(other).m_it == m_it;
		}

		virtual std::ptrdiff_t distance_to(base_class const &other) const override
		{
			return std::distance(dynamic_cast <this_class const &>(other).m_it, m_it);
		}
	};
}}

#endif
