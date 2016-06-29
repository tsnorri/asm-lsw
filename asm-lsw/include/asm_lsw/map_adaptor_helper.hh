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
#include <functional>
#include <sdsl/io.hpp>


namespace asm_lsw {
	
	template <typename t_key>
	struct map_adaptor_access_key
	{
		typedef std::size_t size_type; // Needed for has_serialize.
		
		typedef t_key key_type;
		typedef t_key accessed_type;
		accessed_type operator()(key_type const &key) const { return key; }
		
		// Nothing to serialize.
		size_type serialize(std::ostream& out, sdsl::structure_tree_node *v = nullptr, std::string name = "") const { return 0; }
		void load(std::istream& in) {}
	};
}


namespace asm_lsw { namespace detail {

	// Hash and key_equal assumed stateless here.

	template <typename t_access_key, typename t_hash = std::hash <typename t_access_key::accessed_type>>
	class map_adaptor_hash : protected t_hash
	{
	protected:
		t_access_key m_access_key;

	public:
		map_adaptor_hash() = default;
		map_adaptor_hash(t_access_key const &access_key): m_access_key(access_key) {}
		
		t_access_key const &access_key_fn() { return m_access_key; }

		std::size_t operator()(typename t_access_key::key_type const &key) const
		{
			return t_hash::operator()(m_access_key(key));
		}
	};


	template <typename t_access_key, typename t_key_equal = std::equal_to <typename t_access_key::accessed_type>>
	class map_adaptor_key_equal : protected t_key_equal
	{
	protected:
		t_access_key m_access_key;

	public:
		map_adaptor_key_equal() = default;
		map_adaptor_key_equal(t_access_key const &access_key): m_access_key(access_key) {}

		t_access_key const &access_key_fn() { return m_access_key; }

		typedef typename t_access_key::key_type key_type;
		bool operator()(key_type const &lhs, key_type const &rhs) const
		{
			return t_key_equal::operator()(m_access_key(lhs), m_access_key(rhs));
		}
	};


	template <
		template <typename ...> class t_map,
		typename t_key,
		typename t_value,
		typename t_access_key,
		typename t_hash,
		typename t_key_equal
	>
	struct map_adaptor_trait
	{
		typedef t_map <t_key, t_value, t_hash, t_key_equal> map_type;
		typedef typename map_type::mapped_type mapped_type;
	};


	// Specialization for t_value = void.
	template <
		template <typename ...> class t_set,
		typename t_key,
		typename t_access_key,
		typename t_hash,
		typename t_key_equal
	>
	struct map_adaptor_trait <t_set, t_key, void, t_access_key, t_hash, t_key_equal>
	{
		typedef t_set <t_key, t_hash, t_key_equal> map_type;
		typedef t_key mapped_type;
	};
}}

#endif
