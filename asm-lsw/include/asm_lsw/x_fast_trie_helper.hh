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
#include <asm_lsw/fast_trie_common.hh>
#include <asm_lsw/x_fast_trie_base.hh>


namespace asm_lsw { namespace detail {

	// Only for non-compact, hence t_enable_serialize may be ignored.
	template <
		typename t_key,
		typename t_value,
		bool t_enable_serialize = false,
		typename t_access_key = map_adaptor_access_key <t_key>
	>
	struct x_fast_trie_map_adaptor_tpl
	{
		typedef map_adaptor <fast_trie_map_adaptor_map <t_value>::template type, t_key, t_value, t_access_key> type;
		static constexpr bool needs_custom_constructor() { return false; }
	};
	
	
	// Specialization for nodes / edges with a custom hash function.
	template <typename t_key, typename t_value, bool t_enable_serialize>
	struct x_fast_trie_map_adaptor_tpl <x_fast_trie_node <t_key>, t_value, t_enable_serialize, typename x_fast_trie_node <t_key>::access_key>
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
			typename t_lss_acc::node node(key, 0);
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
		false,
		x_fast_trie_lss_find_fn
	>;
}}

#endif
