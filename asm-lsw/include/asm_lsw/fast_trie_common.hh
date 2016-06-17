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


#ifndef ASM_LSW_FAST_TRIE_COMMON_HH
#define ASM_LSW_FAST_TRIE_COMMON_HH

#include <asm_lsw/map_adaptor_helper.hh>
#include <asm_lsw/map_adaptor_phf.hh>
#include <asm_lsw/pool_allocator.hh>
#include <unordered_map>
#include <unordered_set>
#include <vector>


namespace asm_lsw {

	// FIXME: move to namespace detail.
	template <typename t_value>
	struct fast_trie_map_adaptor_map
	{
		template <typename ... Args>
		using type = std::unordered_map<Args ...>;
	};
	
	
	template <>
	struct fast_trie_map_adaptor_map <void>
	{
		template <typename ... Args>
		using type = std::unordered_set<Args ...>;
	};
	
	

	template <typename t_key, typename t_value, bool t_enable_serialize = false, typename t_access_key = map_adaptor_access_key <t_key>>
	using fast_trie_compact_map_adaptor = map_adaptor_phf <
		map_adaptor_phf_spec <std::vector, pool_allocator, t_key, t_value, t_enable_serialize, t_access_key>
	>;
}

#endif
