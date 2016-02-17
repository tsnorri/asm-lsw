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


#ifndef ASM_LSW_X_FAST_TRIE_BASE_HH
#define ASM_LSW_X_FAST_TRIE_BASE_HH

#include <asm_lsw/map_adaptors.hh>
#include <asm_lsw/x_fast_trie_helper.hh>
#include <boost/format.hpp>
#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <type_traits>


namespace asm_lsw {
	
	// TODO: for t_key or t_value equal to or larger than some threshold (e.g. pointer size, unsigned long long), use a specialization that stores pointers in edge and leaf_link?

	template <typename t_spec>
	class x_fast_trie_base
	{
		static_assert(std::is_integral <typename t_spec::key_type>::value, "Unsigned integer required.");
		static_assert(!std::is_signed <typename t_spec::key_type>::value, "Unsigned integer required.");
		
		template <typename, typename> friend class x_fast_trie_compact;
	
	public:
		typedef typename t_spec::key_type key_type;
		typedef typename t_spec::value_type value_type;
		static int const s_levels{std::numeric_limits <key_type>::digits};

	protected:
		typedef x_fast_trie_edge <key_type> edge;
		typedef std::array <edge, 2> node;
		typedef x_fast_trie_leaf_link <key_type, value_type> leaf_link;
		typedef x_fast_trie_trait<key_type, value_type> trait;
		typedef typename t_spec::template map_adaptor_type <key_type, node> level_map;
		typedef typename t_spec::template map_adaptor_type <key_type, leaf_link> leaf_link_map;
		typedef std::array <level_map, s_levels> lss;
		typedef typename lss::size_type level_idx_type;

	public:
		typedef typename leaf_link_map::iterator leaf_iterator;
		typedef typename leaf_link_map::const_iterator const_leaf_iterator;
		typedef typename leaf_link_map::size_type size_type;
		typedef leaf_iterator iterator;
		typedef const_leaf_iterator const_iterator;

	protected:
		// Conditionally use iterator or const_iterator.
		template <typename t_container, typename t_check, typename t_enable = void>
		struct iterator_type
		{
			typedef typename t_container::iterator type;
		};
		
		template <typename t_container, typename t_check>
		struct iterator_type <t_container, t_check, typename std::enable_if <std::is_const <t_check>::value>::type>
		{
			typedef typename t_container::const_iterator type;
		};

	protected:
		lss m_lss;
		leaf_link_map m_leaf_links;
		key_type m_min;

	protected:
		// Moving functionality to the static member functions is needed for avoiding code duplication
		// and maintaining const correctness in the find member functions.
		// Another option would have been to const_cast m_lss and m_leaf_links in the const versions of the functions
		// but this seems safer.
		
		template <typename t_trie, typename t_iterator>
		static bool find(t_trie &trie, key_type const key, t_iterator &it);
		
		template <typename t_trie, typename t_iterator>
		static bool find_predecessor(t_trie &trie, key_type const key, t_iterator &pred, bool allow_equal);
		
		template <typename t_trie, typename t_iterator>
		static bool find_successor(t_trie &trie, key_type const key, t_iterator &succ, bool allow_equal);
		
		template <typename t_trie, typename t_iterator>
		static void find_nearest(t_trie &trie, key_type const key, t_iterator &it);
		
		template <typename t_trie, typename t_iterator>
		static void find_lowest_ancestor(t_trie &trie, key_type const key, t_iterator &it, level_idx_type &level);
		
		template <typename t_trie, typename t_iterator>
		static bool find_lowest_ancestor(
			t_trie &trie,
			key_type const key,
			t_iterator &ancestor,
			level_idx_type &level,
			level_idx_type const bottom,
			level_idx_type const top
		);
		
		template <typename t_trie, typename t_iterator>
		static bool find_node(t_trie &trie, key_type const key, level_idx_type const level, t_iterator &node);
		
	public:
		x_fast_trie_base() = default;
		x_fast_trie_base(x_fast_trie_base const &) = default;
		x_fast_trie_base(x_fast_trie_base &&) = default;
		x_fast_trie_base &operator=(x_fast_trie_base const &) & = default;
		x_fast_trie_base &operator=(x_fast_trie_base &&) & = default;

		size_type size() const { return m_leaf_links.size(); }
		auto level_key(key_type const key, level_idx_type const level) const -> key_type;
		bool contains(key_type const key) const;
		key_type min_key() const { return m_min; } // Returns a meaningful value if the tree is not empty.
		
		const_leaf_iterator cbegin() const { return m_leaf_links.cbegin(); }
		const_leaf_iterator cend() const { return m_leaf_links.cend(); }
		
		bool find(key_type const key, const_leaf_iterator &it) const;
		bool find_predecessor(key_type const key, const_leaf_iterator &pred, bool allow_equal = false) const;
		bool find_successor(key_type const key, const_leaf_iterator &succ, bool allow_equal = false) const;
		void find_nearest(key_type const key, const_leaf_iterator &it) const;
		void find_lowest_ancestor(key_type const key, typename level_map::const_iterator &it, level_idx_type &level) const;
		bool find_node(key_type const key, level_idx_type const level, typename level_map::const_iterator &node) const;
		
		typename trait::key_type const iterator_key(leaf_iterator const &it) const { trait t; return t.key(it); };
		typename trait::key_type const iterator_key(const_leaf_iterator const &it) const { trait t; return t.key(it); };		
		typename trait::value_type const &iterator_value(const_leaf_iterator const &it) const { trait t; return t.value(it); };

		void print() const;
	};
	
	
	template <typename t_spec>
	auto x_fast_trie_base <t_spec>::level_key(key_type const key, level_idx_type const level) const -> key_type
	{
		return (key >> level);
	}


	template <typename t_spec>
	bool x_fast_trie_base <t_spec>::contains(key_type const key) const
	{
		const_leaf_iterator it;
		return find(key, it);
	}

	
	template <typename t_spec>
	bool x_fast_trie_base <t_spec>::find(key_type const key, const_leaf_iterator &it) const
	{
		return find(*this, key, it);
	}


	template <typename t_spec>
	template <typename t_trie, typename t_iterator>
	bool x_fast_trie_base <t_spec>::find(t_trie &trie, key_type const key, t_iterator &it)
	{
		it = trie.m_leaf_links.find(key);
		if (trie.m_leaf_links.cend() == it)
			return false;
		
		return true;
	}


	template <typename t_spec>
	bool x_fast_trie_base <t_spec>::find_predecessor(key_type const key, const_leaf_iterator &pred, bool allow_equal) const
	{
		return find_predecessor(*this, key, pred, allow_equal);
	}


	template <typename t_spec>
	template <typename t_trie, typename t_iterator>
	bool x_fast_trie_base <t_spec>::find_predecessor(t_trie &trie, key_type const key, t_iterator &pred, bool allow_equal)
	{
		t_iterator it;
		bool const status(find(trie, key, it));

		if (allow_equal && status)
		{
			pred = it;
			return true;
		}
		else if (!status)
		{
			find_nearest(trie, key, it);
		}
		
		if (it->first < key)
		{
			pred = it;
			return true;
		}
		else
		{
			auto prev(it->second.prev);
			if (prev < key)
			{
				pred = trie.m_leaf_links.find(prev);
				return true;
			}
		}
		return false;
	}


	template <typename t_spec>
	bool x_fast_trie_base <t_spec>::find_successor(key_type const key, const_leaf_iterator &succ, bool allow_equal) const
	{
		return find_successor(*this, key, succ, allow_equal);
	}


	template <typename t_spec>
	template <typename t_trie, typename t_iterator>
	bool x_fast_trie_base <t_spec>::find_successor(t_trie &trie, key_type const key, t_iterator &succ, bool allow_equal)
	{
		t_iterator it;
		bool const status(find(trie, key, it));
		
		if (allow_equal && status)
		{
			succ = it;
			return true;
		}
		else if (!status)
		{
			find_nearest(trie, key, it);
		}
		
		if (it->first > key)
		{
			succ = it;
			return true;
		}
		else
		{
			auto next(it->second.next);
			if (key < next)
			{
				succ = trie.m_leaf_links.find(next);
				return true;
			}
		}
		return false;
	}


	template <typename t_spec>
	void x_fast_trie_base <t_spec>::find_nearest(key_type const key, const_leaf_iterator &it) const
	{
		find_nearest(*this, key, it);
	}


	template <typename t_spec>
	template <typename t_trie, typename t_iterator>
	void x_fast_trie_base <t_spec>::find_nearest(t_trie &trie, key_type const key, t_iterator &nearest)
	{
		level_idx_type level(0);
		typename iterator_type <level_map, t_trie>::type it;
		find_lowest_ancestor(trie, key, it, level);
		
		key_type const next_branch(0x1 & (key >> level));
		edge const &node(it->second[next_branch]);
		
		assert(1 == level || node.is_descendant());
		
		nearest = trie.m_leaf_links.find(node.key());
		assert(trie.m_leaf_links.cend() != nearest);
		assert(node.key() == nearest->first);
	}


	template <typename t_spec>
	void x_fast_trie_base <t_spec>::find_lowest_ancestor(
		key_type const key,
		typename level_map::const_iterator &it,
		level_idx_type &level) const
	{
		find_lowest_ancestor(*this, key, it, level);
	}


	template <typename t_spec>
	template <typename t_trie, typename t_iterator>
	void x_fast_trie_base <t_spec>::find_lowest_ancestor(
		t_trie &trie,
		key_type const key,
		t_iterator &it,
		level_idx_type &level
	)
	{
		level_idx_type const levels(trie.m_lss.size());
		find_lowest_ancestor(trie, key, it, level, 0, levels - 1);
	}


	template <typename t_spec>
	template <typename t_trie, typename t_iterator>
	bool x_fast_trie_base <t_spec>::find_lowest_ancestor(
		t_trie &trie,
		key_type const key,
		t_iterator &ancestor,
		level_idx_type &level,
		level_idx_type const bottom,
		level_idx_type const top
	)
	{
		if (bottom > top)
			return false;
		
		level_idx_type const mid((bottom + top) / 2);
		
		if (!find_node(trie, key, mid, ancestor))
		{
			// Candidate hasn't been found yet, conditionally return false.
			return find_lowest_ancestor(trie, key, ancestor, level, 1 + mid, top);
		}

		// Potential lowest ancestor was found, return true.
		level = mid;
		if (bottom < mid)
			find_lowest_ancestor(trie, key, ancestor, level, bottom, mid - 1);
		return true;
	}
	

	template <typename t_spec>
	bool x_fast_trie_base <t_spec>::find_node(
		key_type const key,
		level_idx_type const level,
		typename level_map::const_iterator &node
	) const
	{
		return find_node(*this, key, level, node);
	}


	template <typename t_spec>
	template <typename t_trie, typename t_iterator>
	bool x_fast_trie_base <t_spec>::find_node(
		t_trie &trie,
		key_type const key,
		level_idx_type const level,
		t_iterator &node
	)
	{
		key_type const current_key(trie.level_key(key, 1 + level));
		
		auto it(trie.m_lss[level].find(current_key));
		if (trie.m_lss[level].cend() == it)
			return false;
		
		node = it;
		return true;
	}


	template <typename t_spec>
	void x_fast_trie_base <t_spec>::print() const
	{
		std::cerr << "LSS:\n[level]: key [0x1 & key]: left (d if descendant) right (d if descendant)\n";

		{
			auto size(m_lss.size());
			decltype(size) i(0);
			for (auto lss_it(m_lss.crbegin()), lss_end(m_lss.crend()); lss_it != lss_end; ++lss_it)
			{
				std::cerr << boost::format("\t[%02x]:") % (size - i - 1);
				for (auto node_it(lss_it->cbegin()), node_end(lss_it->cend()); node_it != node_end; ++node_it)
				{
					// In case of the PHF map adaptor, node_it->first points to the
					// first item in the pair (which is the key as expected),
					// not the vector index.
					auto const key(node_it->first);
					auto const left(node_it->second[0].key());
					auto const right(node_it->second[1].key());
					bool const l_is_descendant(node_it->second[0].is_descendant());
					bool const r_is_descendant(node_it->second[1].is_descendant());
					std::cerr << boost::format(" (%02x [%02x]: %02x") % (+key) % (+(0x1 & key)) % (+left);
					if (l_is_descendant)
						std::cerr << " (d) ";
					else
						std::cerr << "     ";

					std::cerr << boost::format("%02x") % (+right);
					if (r_is_descendant)
						std::cerr << " (d) ";
					else
						std::cerr << "     ";
					std::cerr << ")";
				}
	
				std::cerr << "\n";
				++i;
			}
		}
		
		std::cerr << "Leaves:\n";
		{
			for (auto it(m_leaf_links.cbegin()), end(m_leaf_links.cend()); it != end; ++it)
			{
				auto const key(it->first);
				auto const prev(it->second.prev);
				auto const next(it->second.next);
				std::cerr << boost::format("\t(%02x: %02x %02x)\n") % (+key) % (+prev) % (+next);
			}
		}
		
		{
			auto const min(m_min);
			std::cerr << boost::format("Min leaf: %02x\n") % (+min);
		}
	}
}

#endif
