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


#ifndef ASM_LSW_X_FAST_TRIE_COMPACT_HELPER_HH
#define ASM_LSW_X_FAST_TRIE_COMPACT_HELPER_HH

#include <array>
#include <asm_lsw/fast_trie_common.hh>
#include <asm_lsw/x_fast_trie_base.hh>


namespace asm_lsw { namespace detail {

	struct x_fast_trie_compact_lss_find_fn
	{
		template <typename t_lss, typename t_iterator>
		bool operator()(t_lss &lss, typename t_lss::level_idx_type const level, typename t_lss::key_type const key, t_iterator &out_it)
		{
			typename t_lss::key_type const current_key(lss.level_key(key, 1 + level));
			t_iterator it(lss.m_lss[level].find_acc(current_key));
			if (lss.m_lss[level].cend() == it)
				return false;
	
			out_it = it;
			return true;
		}
	};
	
	
	// Fix t_enable_serialize in order to pass it only from the compact trie.
	template <bool t_enable_serialize>
	struct x_fast_trie_compact_map_adaptor_trait
	{
		template <typename t_key, typename t_value, typename t_access_key = map_adaptor_access_key <t_key>>
		struct trait_type
		{
			using type = fast_trie_compact_map_adaptor <t_key, t_value, t_enable_serialize, t_access_key>;
			static constexpr bool needs_custom_constructor() { return false; }
		};
	};
	
	
	template <typename t_key, typename t_value, bool t_enable_serialize>
	using x_fast_trie_compact_spec = x_fast_trie_base_spec <
		t_key,
		t_value,
		x_fast_trie_compact_map_adaptor_trait <t_enable_serialize>::template trait_type,
		x_fast_trie_compact_lss_find_fn
	>;
}}

#endif
