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


#ifndef ASM_LSW_X_FAST_TRIE_HH
#define ASM_LSW_X_FAST_TRIE_HH

#include <asm_lsw/fast_trie_common.hh>
#include <asm_lsw/x_fast_trie_base.hh>


namespace asm_lsw {

	template <typename t_key, typename t_value, typename t_access_key = map_adaptor_access_key <t_key>>
	struct x_fast_trie_map_adaptor_tpl
	{
		typedef map_adaptor <fast_trie_map_adaptor_map <t_value>::template type, t_key, t_value, t_access_key> type;
		static constexpr bool needs_custom_constructor() { return false; }
	};
	
	
	// Specialization for nodes / edges with a custom hash function.
	template <typename t_key, typename t_value>
	struct x_fast_trie_map_adaptor_tpl <x_fast_trie_node <t_key>, t_value, typename x_fast_trie_node <t_key>::access_key>
	{
		typedef x_fast_trie_node <t_key> key_type;
		typedef map_adaptor <
			fast_trie_map_adaptor_map <t_value>::template type,
			key_type,
			t_value,
			typename key_type::access_key
		> type;

		static constexpr bool needs_custom_constructor() { return true; }
		
		template <typename t_array>
		static void construct_map_adaptors(t_array &array)
		{
			for (typename t_array::size_type i(0), count(array.size()); i < count; ++i)
			{
				typedef typename t_array::value_type adaptor_type;
				typedef typename adaptor_type::map_type map_type;
				typedef typename adaptor_type::access_key_fn_type access_key_fn_type;

				access_key_fn_type acc(1 + i);
				map_type map(0, acc, acc);
				adaptor_type adaptor(map, acc); // Takes ownership.
				array[i] = std::move(adaptor);
			}
		}
	};
	
	
	struct x_fast_trie_lss_find_fn
	{
		template <typename t_lss_acc, typename t_iterator>
		bool operator()(
			t_lss_acc &lss,
			typename t_lss_acc::level_idx_type const level,
			typename t_lss_acc::key_type const key,
			t_iterator &out_it
		)
		{
			typename t_lss_acc::node node;
			node[0] = key;
			node[1] = 0;
			
			t_iterator it(lss.m_lss[level].find(node));
			if (lss.m_lss[level].cend() == it)
				return false;
	
			out_it = it;
			return true;
		}
	};
	
	
	template <typename t_key, typename t_value>
	using x_fast_trie_spec = x_fast_trie_base_spec <
		t_key,
		t_value,
		x_fast_trie_map_adaptor_tpl,
		x_fast_trie_lss_find_fn
	>;
	
	
	template <typename t_key, typename t_value = void>
	class x_fast_trie : public x_fast_trie_base <x_fast_trie_spec <t_key, t_value>>
	{
		template <typename, typename> friend class x_fast_trie_compact;
		
	protected:
		typedef x_fast_trie_base <x_fast_trie_spec <t_key, t_value>> base_class;
		typedef typename base_class::level_idx_type level_idx_type;
		typedef typename base_class::level_map level_map;
		typedef typename base_class::leaf_link_map leaf_link_map;
		typedef typename base_class::node node;
		typedef typename base_class::edge edge;

	public:
		typedef typename base_class::key_type key_type;
		typedef typename base_class::value_type value_type;
		typedef typename base_class::leaf_iterator leaf_iterator;
		typedef typename base_class::const_leaf_iterator const_leaf_iterator;
		typedef typename base_class::iterator iterator;
		typedef typename base_class::const_iterator const_iterator;
		
	public:
		x_fast_trie() = default;
		x_fast_trie(x_fast_trie const &) = default;
		x_fast_trie(x_fast_trie &&) = default;
		x_fast_trie &operator=(x_fast_trie const &) & = default;
		x_fast_trie &operator=(x_fast_trie &&) & = default;
		
		// Conditionally enable either.
		// (Return type of the first one is void == std::enable_if<...>::type.)
		template <typename T = value_type>
		typename std::enable_if <std::is_void <T>::value, void>::type
		insert(key_type const key);
		
		template <typename T = value_type>
		void insert(key_type const key, typename std::enable_if <!std::is_void <T>::value, T>::type const val);
		
		void erase(key_type const key);

		using base_class::find;
		using base_class::find_predecessor;
		using base_class::find_successor;
		using base_class::find_nearest;
		using base_class::find_lowest_ancestor;
		using base_class::find_node;
		
		bool find(key_type const key, leaf_iterator &it);
		bool find_predecessor(key_type const key, leaf_iterator &pred, bool allow_equal = false);
		bool find_successor(key_type const key, leaf_iterator &succ, bool allow_equal = false);
		void find_nearest(key_type const key, leaf_iterator &it);
		bool find_lowest_ancestor(key_type const key, typename level_map::iterator &it, level_idx_type &level);
		bool find_node(key_type const key, level_idx_type const level, typename level_map::iterator &node);
	};
	
	
	template <typename t_key, typename t_value>
	template <typename T>
	typename std::enable_if <std::is_void <T>::value, void>::type
	x_fast_trie <t_key, t_value>::insert(key_type const key)
	{
		// Update leaf links.
		auto &map(this->m_leaf_links.map());
		if (0 == this->m_lss.level_size(base_class::s_levels - 1))
		{
			// Special case for empty trie to make things simpler.
			map.emplace(
				std::piecewise_construct,
				std::forward_as_tuple(key),
				std::forward_as_tuple(key, key)
			);
			this->m_min = key;
		}
		else
		{
			leaf_iterator it;
			if (find_predecessor(key, it))
			{
				auto next(it->second.next);
				it->second.next = key;
				map.emplace(
					std::piecewise_construct,
					std::forward_as_tuple(key),
					std::forward_as_tuple(it->first, next)
				);
				map[next].prev = key;
			}
			else
			{
				it = map.find(this->m_min);
				auto prev(it->second.prev);
				it->second.prev = key;
				map[prev].next = key;
				map.emplace(
					std::piecewise_construct,
					std::forward_as_tuple(key),
					std::forward_as_tuple(prev, it->first)
				);
				this->m_min = key;
			}
		}
		this->m_lss.update_levels(key);
	}
	

	template <typename t_key, typename t_value>
	template <typename T>
	void x_fast_trie <t_key, t_value>::insert(key_type const key, typename std::enable_if <!std::is_void <T>::value, T>::type const val)
	{
		// Update leaf links.
		auto &map(this->m_leaf_links.map());
		if (0 == this->m_lss.level_size(base_class::s_levels - 1))
		{
			// Special case for empty trie to make things simpler.
			map.emplace(
				std::piecewise_construct,
				std::forward_as_tuple(key),
				std::forward_as_tuple(key, key, val)
			);
			this->m_min = key;
		}
		else
		{
			leaf_iterator it;
			if (find_predecessor(key, it))
			{
				auto next(it->second.next);
				it->second.next = key;
				map.emplace(
					std::piecewise_construct,
					std::forward_as_tuple(key),
					std::forward_as_tuple(it->first, next, val)
				);
				map[next].prev = key;
			}
			else
			{
				it = map.find(this->m_min);
				auto prev(it->second.prev);
				it->second.prev = key;
				map[prev].next = key;
				map.emplace(
					std::piecewise_construct,
					std::forward_as_tuple(key),
					std::forward_as_tuple(prev, it->first, val)
				);
				this->m_min = key;
			}
		}
		this->m_lss.update_levels(key);
	}

	
	template <typename t_key, typename t_value>
	void x_fast_trie <t_key, t_value>::erase(key_type const key)
	{
		leaf_iterator leaf_it;
		if (!find(key, leaf_it))
			return;
		
		key_type const prev(leaf_it->second.prev);
		key_type const next(leaf_it->second.next);
		
		auto &map(this->m_leaf_links.map());
		map[prev].next = next;
		map[next].prev = prev;
		map.erase(leaf_it);

		this->m_lss.erase_key(key, prev, next);
		
		if (key < prev)
			this->m_min = next;
	}


	template <typename t_key, typename t_value>
	bool x_fast_trie <t_key, t_value>::find(key_type const key, leaf_iterator &it)
	{
		return find(*this, key, it);
	}


	template <typename t_key, typename t_value>
	bool x_fast_trie <t_key, t_value>::find_predecessor(key_type const key, leaf_iterator &pred, bool allow_equal)
	{
		return find_predecessor(*this, key, pred, allow_equal);
	}


	template <typename t_key, typename t_value>
	bool x_fast_trie <t_key, t_value>::find_successor(key_type const key, leaf_iterator &succ, bool allow_equal)
	{
		return find_successor(*this, key, succ, allow_equal);
	}


	template <typename t_key, typename t_value>
	void x_fast_trie <t_key, t_value>::find_nearest(key_type const key, leaf_iterator &it)
	{
		find_nearest(*this, key, it);
	}


	template <typename t_key, typename t_value>
	bool x_fast_trie <t_key, t_value>::find_lowest_ancestor(
		key_type const key,
		typename level_map::iterator &it,
		level_idx_type &level
	)
	{
		return find_lowest_ancestor(*this, key, it, level);
	}


	template <typename t_key, typename t_value>
	bool x_fast_trie <t_key, t_value>::find_node(
		key_type const key,
		level_idx_type const level,
		typename level_map::iterator &node
	)
	{
		return this->m_lss.find_node(key, level, node);
	}
}

#endif
