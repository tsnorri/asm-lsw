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

#include <utility>


namespace asm_lsw {
	
	template <template <typename ...> class t_map, typename t_key, typename t_val>
	class map_adaptor
	{
	public:
		typedef t_key key_type;
		typedef t_val value_type;
		typedef t_map <key_type, t_val> map_type;
		typedef typename map_type::size_type size_type;
		typedef typename map_type::iterator iterator;
		typedef typename map_type::const_iterator const_iterator;
		
	protected:
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

		map_type const &map() const	{ return m_map; }
		map_type &map()				{ return m_map; }
		size_type size() const		{ return m_map.size(); }
		
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
