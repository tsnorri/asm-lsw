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


#ifndef ASM_LSW_UTIL_HH
#define ASM_LSW_UTIL_HH

#include <limits>
#include <type_traits>
 

namespace asm_lsw { namespace util {
	// Preserve C++11 compatibility.
	template <typename T>
	using remove_c_t = typename std::remove_const <T>::type;

	template <typename T>
	using remove_ref_t = typename std::remove_reference <T>::type;

	template <typename T>
	using remove_c_ref_t = remove_c_t <remove_ref_t <T>>;
	
	template <typename t_dst, typename t_src, typename t_key, typename T = typename t_dst::value_type>
	struct fill_trie
	{
		static_assert(std::is_same <
			typename remove_ref_t <t_src>::value_type,
			typename remove_ref_t <t_dst>::value_type
		>::value, "");

		void operator()(t_dst &dst, t_src const &src, t_key const offset) const
		{
			for (auto const &kv : src)
			{
				assert(kv.first - offset < std::numeric_limits <typename t_dst::key_type>::max());
				typename t_dst::key_type key(kv.first - offset);
				typename t_dst::value_type value(kv.second.value);
				dst.insert(key, value);
			}
		}
	};

	template <typename t_dst, typename t_src, typename t_key>
	struct fill_trie <t_dst, t_src, t_key, void>
	{
		static_assert(std::is_same <
			typename remove_ref_t <t_src>::value_type,
			typename remove_ref_t <t_dst>::value_type
		>::value, "");

		void operator()(t_dst &dst, t_src const &src, t_key const offset) const
		{
			for (auto const &kv : src)
			{
				assert(kv.first - offset < std::numeric_limits <typename t_dst::key_type>::max());
				typename t_dst::key_type key(kv.first - offset);
				dst.insert(key);
			}
		}
	};
}}

#endif
