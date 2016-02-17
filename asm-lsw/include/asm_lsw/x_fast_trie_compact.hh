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

#include <asm_lsw/pool_allocator.hh>
#include <asm_lsw/x_fast_trie_base.hh>


namespace asm_lsw {
	
	template <typename t_key, typename t_value>
	class x_fast_trie;
	
	// FIXME: move to namespace detail.
	// Fix the allocator parameter of the vector used by the adaptor.
	// Replace the second parameter if a different allocator is desired.
	template <template <typename ...> class t_vector, typename t_key, typename t_value>
	using x_fast_trie_compact_map_adaptor_spec = map_adaptor_phf_spec <t_vector, pool_allocator, t_key, t_value>;
	
	// Pass the adaptor parameters received from the trie to the specification.
	template <template <typename ...> class t_vector, typename t_key, typename t_value>
	using fast_trie_compact_map_adaptor = map_adaptor_phf <x_fast_trie_compact_map_adaptor_spec <t_vector, t_key, t_value>>;
	
	template <typename t_key, typename t_value>
	using x_fast_trie_compact_spec = x_fast_trie_base_spec <t_key, t_value, std::vector, fast_trie_compact_map_adaptor>;
	
	// Use perfect hashing instead of the one provided by STL.
	template <typename t_key, typename t_value = void>
	class x_fast_trie_compact : public x_fast_trie_base <x_fast_trie_compact_spec <t_key, t_value>>
	{
	protected:
		typedef x_fast_trie_base <x_fast_trie_compact_spec <t_key, t_value>> base_class;
		typedef typename base_class::lss lss;
		typedef typename base_class::level_map level_map;
		typedef typename base_class::leaf_link_map leaf_link_map;
		
	public:
		typedef typename base_class::key_type key_type;
		typedef typename base_class::value_type value_type;
		typedef typename base_class::iterator iterator;
		typedef typename base_class::const_iterator const_iterator;
		
	public:
		x_fast_trie_compact() = default;
		x_fast_trie_compact(x_fast_trie_compact const &) = default;
		x_fast_trie_compact(x_fast_trie_compact &&) = default;
		x_fast_trie_compact &operator=(x_fast_trie_compact const &) & = default;
		x_fast_trie_compact &operator=(x_fast_trie_compact &&) & = default;
		
		explicit x_fast_trie_compact(x_fast_trie <key_type, value_type> &other);
	};
	
	
	template <typename t_key, typename t_value>
	x_fast_trie_compact <t_key, t_value>::x_fast_trie_compact(x_fast_trie <key_type, value_type> &other):
		x_fast_trie_compact()
	{
		typedef typename std::remove_reference <decltype(other)>::type other_adaptor_type;
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
		typename lss::size_type const count(this->m_lss.size());
		assert(other.m_lss.size() == count);
		
		std::vector <lss_builder_type> lss_builders;
		lss_builders.reserve(count);
		for (typename lss::size_type i(0); i < count; ++i)
		{
			// Move the contents of the other map.
			auto &map(other.m_lss[i].map());
			lss_builder_type builder(map, allocator);
		
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
				this->m_lss[i] = std::move(adaptor);
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
