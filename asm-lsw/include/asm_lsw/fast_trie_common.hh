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
	
	template <template <typename ...> class, template <typename> class, typename, typename>
	struct map_adaptor_phf_spec;
	
	template <typename>
	class map_adaptor_phf;
	
	
	// FIXME: move to namespace detail.
	// Fix the allocator parameter of the vector used by the adaptor.
	// Replace the second parameter if a different allocator is desired.
	template <template <typename ...> class t_vector, typename t_key, typename t_value>
	using fast_trie_compact_map_adaptor_spec = map_adaptor_phf_spec <t_vector, pool_allocator, t_key, t_value>;
	
	// Pass the adaptor parameters received from the trie to the specification.
	template <template <typename ...> class t_vector, typename t_key, typename t_value>
	using fast_trie_compact_map_adaptor = map_adaptor_phf <fast_trie_compact_map_adaptor_spec <t_vector, t_key, t_value>>;
}

#endif
