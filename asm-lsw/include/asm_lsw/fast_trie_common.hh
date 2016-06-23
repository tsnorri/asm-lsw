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


namespace asm_lsw { namespace detail {
	struct x_fast_trie_tag {};
	struct y_fast_trie_tag {};
}}
	
	
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
	
	
namespace asm_lsw { namespace detail {
	
	template <typename t_mapped_type, bool t_dummy = true>
	struct x_fast_trie_cmp
	{
		template <typename T, typename U>
		bool contains(T const &trie_1, U const &trie_2)
		{
			typename T::const_iterator it;
			for (auto const &kv : trie_2)
			{
				if (!trie_1.find(kv.first, it))
					return false;
				
				if (! (kv.second.value == it->second.value))
					return false;
			}
			
			return true;
		}
	};
	
	
	template <bool t_dummy>
	struct x_fast_trie_cmp <void, t_dummy>
	{
		template <typename T, typename U>
		bool contains(T const &trie_1, U const &trie_2)
		{
			typename T::const_iterator it;
			for (auto const &kv : trie_2)
			{
				if (!trie_1.find(kv.first, it)) // kv is a leaf link.
					return false;
			}
			
			return true;
		}
	};
	
	
	template <typename t_mapped_type, bool t_dummy = true>
	struct y_fast_trie_cmp
	{
		template <typename T, typename U>
		bool contains(T const &trie_1, U const &trie_2)
		{
			typename T::const_iterator it;
			for (auto const &kv : trie_2)
			{
				if (!trie_1.find(kv.first, it))
					return false;
				
				if (! (kv.second.value == it->second.value))
					return false;
			}
			
			return true;
		}
	};
	
	
	template <bool t_dummy>
	struct y_fast_trie_cmp <void, t_dummy>
	{
		template <typename T, typename U>
		bool contains(T const &trie_1, U const &trie_2)
		{
			typename T::const_iterator it;
			for (auto const &k : trie_2)
			{
				if (!trie_1.find(k, it))
					return false;
			}
			
			return true;
		}
	};
	
	
	template <typename T, typename U>
	bool contains_subtrees(T const &trie_1, U const &trie_2)
	{
		typename detail::y_fast_trie_cmp <typename T::mapped_type> cmp;
		for (auto const &subtree_kv : trie_2.subtree_map_proxy())
		{
			if (!cmp.contains(trie_1, subtree_kv.second))
				return false;
		}
		return true;
	}
}}
	

namespace asm_lsw {
	
	// FIXME: slow; should be replaced by iterating two ranges and comparing the objects.
	template <typename T, typename U>
	auto operator==(
		T const &trie_1, U const &trie_2
	) -> typename std::enable_if <
		std::is_same <detail::x_fast_trie_tag, typename T::x_fast_trie_tag>::value &&
		std::is_same <detail::x_fast_trie_tag, typename U::x_fast_trie_tag>::value &&
		std::is_same <typename T::mapped_type, typename U::mapped_type>::value,
		bool
	>::type
	{
		typename detail::x_fast_trie_cmp <typename T::mapped_type> cmp;
		return cmp.contains(trie_1, trie_2) && cmp.contains(trie_2, trie_1);
	}
	
	
	// FIXME: extremely slow, only for tests.
	template <typename T, typename U>
	auto operator==(
		T const &trie_1, U const &trie_2
	) -> typename std::enable_if <
		std::is_same <detail::y_fast_trie_tag, typename T::y_fast_trie_tag>::value &&
		std::is_same <detail::y_fast_trie_tag, typename U::y_fast_trie_tag>::value &&
		std::is_same <typename T::mapped_type, typename U::mapped_type>::value,
		bool
	>::type
	{
		return detail::contains_subtrees(trie_1, trie_2) && detail::contains_subtrees(trie_2, trie_1);
	}
}

#endif
