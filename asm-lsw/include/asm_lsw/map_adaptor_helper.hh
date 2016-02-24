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


#ifndef ASM_LSW_MAP_ADAPTOR_HELPER_HH
#define ASM_LSW_MAP_ADAPTOR_HELPER_HH

#include <array>


namespace asm_lsw {
	
	// FIXME: move to namespace detail.
	// FIXME: remove unused classes.
	// Return an unsigned integer type from the key object.
	template <typename t_key>
	struct map_adaptor_access_key
	{
		typedef t_key key_type;
		typedef t_key accessed_type;
		accessed_type operator()(key_type const &key) const { return key; }
	};
	
	
	template <
		typename t_key,
		typename t_access_key = map_adaptor_access_key <t_key>,
		template <typename> class t_hash = std::hash
	>
	struct map_adaptor_hash : protected t_access_key, t_hash <typename t_access_key::accessed_type>
	{
		typedef t_access_key access_key;
		typedef t_hash <typename access_key::accessed_type> hash;
		typedef typename hash::argument_type argument_type;
		typedef typename hash::result_type result_type;
		
		std::size_t operator()(t_key const &key) const { return hash::operator()(access_key::operator()(key)); }
	};
	
	
	template <
		template <typename ...> class t_map,
		typename t_key,
		typename t_value,
		typename t_hash = std::hash <t_key>
	>
	struct map_adaptor_trait
	{
		typedef t_map <t_key, t_value, t_hash> map_type;
	};

	template <
		template <typename ...> class t_set,
		typename t_key,
		typename t_hash
	>
	struct map_adaptor_trait <t_set, t_key, void, t_hash>
	{
		typedef t_set <t_key, t_hash> map_type;
	};
}

#endif
