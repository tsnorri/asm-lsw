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


#ifndef ASM_LSW_Y_FAST_TRIE_COMPACT_HH
#define ASM_LSW_Y_FAST_TRIE_COMPACT_HH

#include <asm_lsw/fast_trie_common.hh>
#include <asm_lsw/y_fast_trie_base.hh>
#include <asm_lsw/y_fast_trie_helper.hh>
#include <vector>


namespace asm_lsw {
	
	template <typename t_spec>
	class map_adaptor_phf;
	
	template <typename t_key, typename t_value, bool t_enable_serialize>
	class x_fast_trie_compact;
	
	template <typename t_key, typename t_value>
	class y_fast_trie;
}


namespace asm_lsw { namespace detail {

	// Fix t_enable_serialize in order to pass it only from the compact trie.
	template <bool t_enable_serialize>
	struct y_fast_trie_compact_map_adaptor_trait
	{
		template <typename t_key, typename t_value, typename t_access_key = map_adaptor_access_key <t_key>>
		using map_type = fast_trie_compact_map_adaptor <t_key, t_value, t_enable_serialize, t_access_key>;
	};
	

	template <typename t_key, typename t_value, bool t_enable_serialize>
	using y_fast_trie_compact_spec = y_fast_trie_base_spec <
		t_key,
		t_value,
		y_fast_trie_compact_map_adaptor_trait <t_enable_serialize>::template map_type,
		x_fast_trie_compact <t_key, void, std::is_void <t_value>::value>,
		typename y_fast_trie_compact_subtree_trait <t_key, t_value, t_enable_serialize>::subtree_type
	>;
}}


namespace asm_lsw {

	// Use perfect hashing instead of the one provided by STL.
	template <typename t_key, typename t_value = void, bool t_enable_serialize = false>
	class y_fast_trie_compact : public y_fast_trie_base <detail::y_fast_trie_compact_spec <t_key, t_value, t_enable_serialize>>
	{
	public:
		typedef y_fast_trie_base <detail::y_fast_trie_compact_spec <t_key, t_value, t_enable_serialize>> base_class;
		typedef typename base_class::key_type				key_type;
		typedef typename base_class::value_type				value_type;
		typedef typename base_class::size_type				size_type;
		typedef typename base_class::subtree_iterator		subtree_iterator;
		typedef typename base_class::const_subtree_iterator	const_subtree_iterator;
		typedef typename base_class::iterator				iterator;
		typedef typename base_class::const_iterator			const_iterator;
		
	protected:
		typedef typename base_class::trait					trait;
		typedef typename base_class::subtree_map			subtree_map;
		
	public:
		y_fast_trie_compact() = default;
		y_fast_trie_compact(y_fast_trie_compact const &) = default;
		y_fast_trie_compact(y_fast_trie_compact &&) = default;
		y_fast_trie_compact &operator=(y_fast_trie_compact const &) & = default;
		y_fast_trie_compact &operator=(y_fast_trie_compact &&) & = default;
		
		explicit y_fast_trie_compact(y_fast_trie <key_type, value_type> &other):
			base_class(other.m_reps, other.m_subtrees.map(), other.m_size)
		{
		}
		
		using base_class::find;
		using base_class::find_predecessor;
		using base_class::find_successor;
		using base_class::find_subtree_exact;
		
		template <typename Fn, bool t_dummy = true>
		auto serialize_keys(
			std::ostream &out,
			Fn value_callback,
			sdsl::structure_tree_node *v = nullptr,
			std::string name = ""
		) const -> typename std::enable_if <trait::is_map_type && t_dummy, std::size_t>::type;
	
		template <bool t_dummy = true>
		auto serialize(
			std::ostream &out,
			sdsl::structure_tree_node *v = nullptr,
			std::string name = ""
		) const -> typename std::enable_if <t_enable_serialize && t_dummy, std::size_t>::type;
	
		template <typename Fn, bool t_dummy = true>
		auto load_keys(
			std::istream &in, Fn value_callback
		) -> typename std::enable_if <trait::is_map_type && t_dummy>::type;
	
		template <bool t_dummy = true>
		auto load(std::istream &in) -> typename std::enable_if <t_enable_serialize && t_dummy>::type;
	};
	
	
	template <typename t_key, typename t_value, bool t_enable_serialize>
	template <typename Fn, bool t_dummy>
	auto y_fast_trie_compact <t_key, t_value, t_enable_serialize>::serialize_keys(
		std::ostream &out,
		Fn serialize_subtree,
		sdsl::structure_tree_node *v,
		std::string name
	) const -> typename std::enable_if <trait::is_map_type && t_dummy, std::size_t>::type
	{
		auto *child(sdsl::structure_tree::add_child(v, name, sdsl::util::class_name(*this)));
		std::size_t written_bytes(0);
		
		written_bytes += sdsl::write_member(this->m_size, out, child, "size");
		written_bytes += this->m_reps.serialize(out, child, "reps");
		written_bytes += this->m_subtrees.serialize_keys(out, serialize_subtree, child, "subtrees");
		
		sdsl::structure_tree::add_size(child, written_bytes);
		return written_bytes;
	}
	
	
	template <typename t_key, typename t_value, bool t_enable_serialize>
	template <bool t_dummy>
	auto y_fast_trie_compact <t_key, t_value, t_enable_serialize>::serialize(
		std::ostream &out,
		sdsl::structure_tree_node *v,
		std::string name
	) const -> typename std::enable_if <t_enable_serialize && t_dummy, std::size_t>::type
	{
		// FIXME: repeating the previous function.
		auto *child(sdsl::structure_tree::add_child(v, name, sdsl::util::class_name(*this)));
		std::size_t written_bytes(0);
		
		written_bytes += sdsl::write_member(this->m_size, out, child, "size");
		written_bytes += this->m_reps.serialize(out, child, "reps");
		written_bytes += this->m_subtrees.serialize(out, child, "subtrees");
		
		sdsl::structure_tree::add_size(child, written_bytes);
		return written_bytes;
	}
	
	
	template <typename t_key, typename t_value, bool t_enable_serialize>
	template <typename Fn, bool t_dummy>
	auto y_fast_trie_compact <t_key, t_value, t_enable_serialize>::load_keys(
		std::istream &in,
		Fn load_subtree
	) -> typename std::enable_if <trait::is_map_type && t_dummy>::type
	{
		sdsl::read_member(this->m_size, in);
		this->m_reps.load(in);
		this->m_subtrees.load_keys(in, load_subtree);
	}
	
	
	template <typename t_key, typename t_value, bool t_enable_serialize>
	template <bool t_dummy>
	auto y_fast_trie_compact <t_key, t_value, t_enable_serialize>::load(
		std::istream &in
	) -> typename std::enable_if <t_enable_serialize && t_dummy>::type
	{
		sdsl::read_member(this->m_size, in);
		this->m_reps.load(in);
		this->m_subtrees.load(in);
	}
}

#endif
