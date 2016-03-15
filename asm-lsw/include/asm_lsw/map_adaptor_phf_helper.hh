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


#ifndef ASM_LSW_MAP_ADAPTOR_PHF_HELPER_HH
#define ASM_LSW_MAP_ADAPTOR_PHF_HELPER_HH

#include <utility>


namespace asm_lsw { namespace detail {

	// Specialize when t_value = void.
	template <typename t_spec, bool t_value_is_void = std::is_void <typename t_spec::value_type>::value>
	struct map_adaptor_phf_trait
	{
	};

	template <typename t_spec>
	struct map_adaptor_phf_trait <t_spec, true>
	{
		typedef typename t_spec::access_key_fn_type::key_type	key_type;
		typedef typename t_spec::value_type						value_type;
		typedef key_type										kv_type;

		template <typename t_kv>
		static key_type key(t_kv &kv)
		{
			return kv;
		}

		template <typename t_kv>
		static kv_type kv(t_kv &kv)
		{
			return kv;
		}
	};

	template <typename t_spec>
	struct map_adaptor_phf_trait <t_spec, false>
	{
		typedef typename t_spec::access_key_fn_type::key_type	key_type;
		typedef typename t_spec::value_type						value_type;
		typedef std::pair <key_type, value_type>				kv_type;

		template <typename t_kv>
		static key_type key(t_kv &kv)
		{
			return kv.first;
		}

		template <typename t_kv>
		static kv_type kv(t_kv &kv)
		{
			return std::make_pair(kv.first, std::move(kv.second));
		}
	};
	
	
	template <typename t_spec>
	struct map_adaptor_phf_access_value
	{
		typedef typename map_adaptor_phf_trait <t_spec>::kv_type kv_type;
		
		template <typename t_kv>
		kv_type operator()(t_kv &kv)
		{
			return map_adaptor_phf_trait <t_spec>::kv(kv);
		}
	};
}}

#endif
