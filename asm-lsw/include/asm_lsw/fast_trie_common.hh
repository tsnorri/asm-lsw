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


#ifndef ASM_LSW_FAST_COMMON_HH
#define ASM_LSW_FAST_COMMON_HH

namespace asm_lsw {
	template <typename>
	class pool_allocator;
	
	template <template <typename ...> class, template <typename> class, typename, typename, typename>
	struct map_adaptor_phf_spec;
	
	template <typename>
	class map_adaptor_phf;
	
	
	// FIXME: move to namespace detail.
	// Fix the vector and allocator parameters.
	template <typename t_access_key, typename t_value>
	using fast_trie_compact_map_adaptor_spec = map_adaptor_phf_spec <
		std::vector,
		pool_allocator,
		typename t_access_key::key_type,
		t_value,
		t_access_key
	>;
	
	// Pass the adaptor parameters received from the trie to the specification.
	template <typename t_access_key>
	struct fast_trie_compact_map_adaptor
	{
		template <typename t_value>
		using type = map_adaptor_phf <
			fast_trie_compact_map_adaptor_spec <
				t_access_key,
				t_value
			>
		>;
	};
}

#endif
