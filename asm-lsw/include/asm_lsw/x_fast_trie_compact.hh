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


#ifndef ASM_LSW_X_FAST_TRIE_COMPACT_HH
#define ASM_LSW_X_FAST_TRIE_COMPACT_HH

#include <asm_lsw/fast_trie_common.hh>
#include <asm_lsw/pool_allocator.hh>
#include <asm_lsw/util.hh>
#include <asm_lsw/x_fast_trie_base.hh>


namespace asm_lsw {
	template <typename t_key, typename t_value, typename t_hash>
	class x_fast_trie;
	
	
	// FIXME: move to namespace detail.
	// Use partial template specialization to apply a custom access_key
	// (which in turn uses the provided access_key) to pack two keys into
	// x_fast_trie_edge <t_key>.
	template <typename t_key, typename t_value, typename t_access_key>
	struct x_fast_trie_compact_map_adaptor_tpl
	{
	};
	
	
	// Specialization for the regular case, i.e. t_key is the map key.
	template <typename t_access_key, typename t_value>
	struct x_fast_trie_compact_map_adaptor_tpl <typename t_access_key::key_type, t_value, t_access_key>
	{
		using type = typename fast_trie_compact_map_adaptor <t_access_key>::template type <t_value>;
		static constexpr bool needs_custom_constructor() { return false; } // FIXME: should not be needed.
	};
	
	
	// Specialization for nodes / edges with a custom access_key.
	// FIXME: t_access_key isn't actually needed here, so this should be simplified.
	template <typename t_key, typename t_value, typename t_access_key>
	class x_fast_trie_compact_map_adaptor_tpl <x_fast_trie_node <t_key>, t_value, t_access_key>
	{
		class access_key : protected t_access_key
		{
		protected:
			uint8_t m_level{0};

		public:
			typedef x_fast_trie_node <t_key> key_type;
			typedef t_key accessed_type;
			
			access_key() = default;
			access_key(uint8_t level): m_level(level) {}
			
			accessed_type operator()(key_type const &node) const
			{
				// FIXME: verify the shift amount.
				return t_access_key::operator()(node[0].level_key(m_level));
			}
		};
		
	public:
		using type = typename fast_trie_compact_map_adaptor <access_key>::template type <t_value>;
		
		// Custom constructor not needed here, since the x_fast_trie_compact's constructor
		// handles everything.
		static constexpr bool needs_custom_constructor() { return false; }
	};
	
	
	// Fix access_key.
	template <typename t_access_key>
	struct x_fast_trie_compact_map_adaptor
	{
		template <typename t_key, typename t_value>
		using type = x_fast_trie_compact_map_adaptor_tpl <t_key, t_value, t_access_key>;
	};
	
	
	struct x_fast_trie_compact_lss_find_fn
	{
		template <typename t_lss, typename t_iterator>
		bool operator()(t_lss &lss, typename t_lss::level_idx_type const level, typename t_lss::key_type const key, t_iterator &it)
		{
			typename t_lss::key_type const current_key(lss.level_key(key, 1 + level));
			it = lss.m_lss[level].find_acc(current_key);
			if (lss.m_lss[level].cend() == it)
				return false;
	
			return true;
		}
	};
	
	
	template <typename t_key, typename t_value, typename t_access_key>
	using x_fast_trie_compact_spec = x_fast_trie_base_spec <
		t_key,
		t_value,
		x_fast_trie_compact_map_adaptor <t_access_key>::template type,
		x_fast_trie_compact_lss_find_fn
	>;
	
	
	// Uses perfect hashing instead of the one provided by STL.
	template <typename t_key, typename t_value = void, typename t_access_key = map_adaptor_access_key <t_key>>
	class x_fast_trie_compact : public x_fast_trie_base <x_fast_trie_compact_spec <t_key, t_value, t_access_key>>
	{
	protected:
		typedef x_fast_trie_base <x_fast_trie_compact_spec <t_key, t_value, t_access_key>> base_class;
		
		typedef typename base_class::level_idx_type level_idx_type;
		typedef typename base_class::level_map level_map;
		typedef typename base_class::leaf_link_map leaf_link_map;
		typedef typename base_class::node node;
		typedef typename base_class::edge edge;
		
	public:
		typedef typename base_class::key_type key_type;
		typedef typename base_class::value_type value_type;
		typedef typename base_class::leaf_iterator leaf_iterator;
		typedef typename base_class::const_leaf_iterator const_leaf_iterator;
		typedef typename base_class::iterator iterator;
		typedef typename base_class::const_iterator const_iterator;
		typedef t_access_key access_key_fn_type;
		
	public:
		x_fast_trie_compact() = default;
		x_fast_trie_compact(x_fast_trie_compact const &) = default;
		x_fast_trie_compact(x_fast_trie_compact &&) = default;
		x_fast_trie_compact &operator=(x_fast_trie_compact const &) & = default;
		x_fast_trie_compact &operator=(x_fast_trie_compact &&) & = default;
		
		template <typename t_hash>
		explicit x_fast_trie_compact(x_fast_trie <key_type, value_type, t_hash> &other);
	};
	
	
	template <typename t_key, typename t_value, typename t_access_key>
	template <typename t_hash>
	x_fast_trie_compact <t_key, t_value, t_access_key>::x_fast_trie_compact(x_fast_trie <key_type, value_type, t_hash> &other):
		x_fast_trie_compact()
	{
		typedef remove_ref_t <decltype(other)> other_adaptor_type;
		typedef typename level_map::template builder_type <
			typename other_adaptor_type::level_map::map_type
		> lss_builder_type;
		typedef typename leaf_link_map::template builder_type <
			typename other_adaptor_type::leaf_link_map::map_type
		> leaf_link_map_builder_type;
		typedef typename lss_builder_type::adaptor_type lss_adaptor_type;
		typedef typename leaf_link_map_builder_type::adaptor_type leaf_link_map_adaptor_type;
		
		// Create an empty allocator to be passed to the builders.
		pool_allocator <typename level_map::kv_type> allocator;
		
		// Create the builders.
		assert(remove_ref_t <decltype(other)>::s_levels == base_class::s_levels);
		
		std::vector <lss_builder_type> lss_builders;
		lss_builders.reserve(base_class::s_levels);
		for (level_idx_type i(0); i < base_class::s_levels; ++i)
		{
			typename level_map::access_key_fn_type acc(i);
			
			// Move the contents of the other map.
			auto &map(other.m_lss.level(i).map());
			lss_builder_type builder(map, allocator, acc);
		
			lss_builders.emplace_back(std::move(builder));
		}
		
		leaf_link_map_builder_type leaf_link_map_builder(other.m_leaf_links.map(), allocator);
		
		// Calculate the space needed.
		// Since new allocates space with correct alignment w.r.t. all fundamental types,
		// the starting address may be assumed to be zero.
		space_requirement sr;
		
		for (auto const &v : lss_builders)
			sr.add <typename level_map::kv_type>(v.element_count());
		
		sr.add <typename leaf_link_map::kv_type>(leaf_link_map_builder.element_count());
		
		// Allocate memory.
		allocator.allocate_pool(sr);
		
		// Create the adaptors.
		{
			std::size_t i(0);
			for (auto &builder : lss_builders)
			{
				lss_adaptor_type adaptor(builder);
				
				this->m_lss.level(i) = std::move(adaptor);
				++i;
			}
			
			leaf_link_map_adaptor_type adaptor(leaf_link_map_builder);
			this->m_leaf_links = std::move(adaptor);
		}
		
		// Min. value
		this->m_min = other.m_min;
		
		// The allocator shouldn't have any space left at this point.
		assert(0 == allocator.bytes_left());
	}
}

#endif
