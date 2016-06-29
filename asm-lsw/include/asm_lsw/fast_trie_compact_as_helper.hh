/*
 Copyright (c) 2015-2016 Tuukka Norri
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

#ifndef ASM_LSW_FAST_TRIE_COMPACT_AS_HELPER_HH
#define ASM_LSW_FAST_TRIE_COMPACT_AS_HELPER_HH

#include <functional>
#include <sdsl/io.hpp>


namespace asm_lsw { namespace detail {

	template <typename t_value>
	struct fast_trie_compact_as_trait
	{
		typedef std::function <
			std::size_t(t_value const &, std::ostream &, sdsl::structure_tree_node *)
		> serialize_value_callback_type;
		typedef std::function <
			void(t_value &, std::istream &)
		> load_value_callback_type;
		enum { is_map_type = 1 };
	};
	
	
	template <>
	struct fast_trie_compact_as_trait <void>
	{
		typedef void * serialize_value_callback_type;
		typedef void * load_value_callback_type;
		enum { is_map_type = 0 };
	};
	
	
	template <typename t_value>
	struct fast_trie_compact_as_tpl_value_trait
	{
		template <typename t_trie, typename Fn>
		std::size_t serialize_keys(
			t_trie const &trie,
			std::ostream &out,
			Fn callback,
			sdsl::structure_tree_node *v,
			std::string name
		) const
		{
			return trie.m_trie.serialize_keys(out, callback, v, name);
		}

		template <typename t_trie, typename Fn>
		void load_keys(
			t_trie &trie,
			std::istream &in,
			Fn callback
		)
		{
			trie.m_trie.load_keys(in, callback);
		}
	};
	
	
	template <>
	struct fast_trie_compact_as_tpl_value_trait <void>
	{
		template <typename t_trie, typename Fn>
		std::size_t serialize_keys(
			t_trie const &trie,
			std::ostream &out,
			Fn callback,
			sdsl::structure_tree_node *v,
			std::string name
		) const
		{
			return 0;
		}

		template <typename t_trie, typename Fn>
		void load_keys(
			t_trie &trie,
			std::istream &in,
			Fn callback
		)
		{
		}
	};
	
	
	template <bool t_enable_serialize>
	struct fast_trie_compact_as_tpl_serialize_trait
	{
		template <typename t_trie>
		std::size_t serialize(
			t_trie const &trie,
			std::ostream &out,
			sdsl::structure_tree_node *v,
			std::string name
		) const
		{
			return trie.m_trie.serialize(out, v, name);
		}

		template <typename t_trie>
		void load(
			t_trie &trie,
			std::istream &in
		)
		{
			trie.m_trie.load(in);
		}
	};
	
	
	template <>
	struct fast_trie_compact_as_tpl_serialize_trait <false>
	{
		template <typename t_trie>
		std::size_t serialize(
			t_trie const &trie,
			std::ostream &out,
			sdsl::structure_tree_node *v,
			std::string name
		) const
		{
			return 0;
		}

		template <typename t_trie>
		void load(
			t_trie &trie,
			std::istream &in
		)
		{
		}
	};
}}

#endif
