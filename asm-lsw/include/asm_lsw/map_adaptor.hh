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


#ifndef ASM_LSW_MAP_ADAPTOR_HH
#define ASM_LSW_MAP_ADAPTOR_HH

#include <asm_lsw/map_adaptor_helper.hh>
#include <asm_lsw/util.hh>
#include <utility>


namespace asm_lsw {
	template <
		template <typename ...> class t_map,
		typename t_key,
		typename t_val = void,
		typename t_access_key = map_adaptor_access_key <t_key>,
		typename t_hash = detail::map_adaptor_hash <t_access_key>,
		typename t_key_equal = detail::map_adaptor_key_equal <t_access_key>
	>
	class map_adaptor
	{
	public:
		typedef t_key key_type;
		typedef t_val value_type;
		typedef t_access_key access_key_fn_type;
		typedef t_hash hash;
		typedef t_key_equal key_equal;
		typedef detail::map_adaptor_trait <t_map, key_type, value_type, access_key_fn_type, hash, key_equal> trait_type;
		typedef typename trait_type::map_type map_type;
		typedef typename map_type::size_type size_type;
		typedef typename trait_type::mapped_type mapped_type;
		typedef typename map_type::value_type kv_type; // FIXME: change to value_type and value_type to mapped_type.
		typedef typename map_type::iterator iterator;
		typedef typename map_type::const_iterator const_iterator;
		typedef typename access_key_fn_type::accessed_type accessed_type;
		
		// For compatibility with map_adaptor_phf.
		template <template <typename> class t_new_allocator>
		using rebind_allocator = map_adaptor;

		enum { can_serialize = false };
		
	protected:
		access_key_fn_type m_access_key_fn;
		map_type m_map;
		
	protected:
		typename map_type::key_type adapted_key(typename map_type::key_type key) const { return key; } // No-op.
		
	public:
		map_adaptor() = default;
		map_adaptor(map_adaptor const &) = default;
		map_adaptor(map_adaptor &&) = default;
		map_adaptor &operator=(map_adaptor const &) & = default;
		map_adaptor &operator=(map_adaptor &&) & = default;

		map_adaptor(map_type &map): m_map(std::move(map)) {}
		
		map_adaptor(access_key_fn_type const &access_key):
			m_access_key_fn(access_key),
			m_map()
		{
		}

		map_adaptor(map_type &map, access_key_fn_type const &access_key):
			m_access_key_fn(access_key),
			m_map(std::move(map))
		{
		}
		
		template <
			template <typename ...> class t_other_map,
			typename t_other_key,
			typename t_other_val,
			typename t_other_access_key,
			typename t_other_hash,
			typename t_other_key_equal
		>
		auto operator==(
			map_adaptor <t_other_map, t_other_key, t_other_val, t_other_access_key, t_other_hash, t_other_key_equal> const &other
		) const -> typename std::enable_if <
			std::is_same <
				mapped_type,
				typename util::remove_ref_t <decltype(other)>::mapped_type
			>::value, bool
		>::type
		{ /* FIXME: what about access_key? */ return m_map == other.m_map; }

		map_type const &map() const						{ return m_map; }
		map_type &map()									{ return m_map; }
		size_type size() const							{ return m_map.size(); }
		access_key_fn_type const &access_key_fn() const	{ return m_access_key_fn; }
		
		iterator find(key_type const &key)				{ return m_map.find(key); }
		const_iterator find(key_type const &key) const	{ return m_map.find(key); }
		iterator begin()								{ return m_map.begin(); }
		const_iterator begin() const					{ return m_map.begin(); }
		const_iterator cbegin() const					{ return m_map.cbegin(); }
		iterator end()									{ return m_map.end(); }
		const_iterator end() const						{ return m_map.end(); }
		const_iterator cend() const						{ return m_map.cend(); }
	};
}

#endif
