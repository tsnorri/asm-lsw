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
#include <asm_lsw/util.hh>
#include <asm_lsw/x_fast_trie_base_helper.hh>
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

	protected:
		class lss_access;
	
	public:
		typedef typename t_spec::key_type key_type;
		typedef typename t_spec::value_type value_type; // FIXME: change to something meaningful (pair for mutable).
		typedef typename t_spec::value_type mapped_type;
		static std::size_t const s_levels{std::numeric_limits <key_type>::digits};

	protected:
		typedef detail::x_fast_trie_edge <key_type> edge;
		typedef detail::x_fast_trie_node <key_type> node;
		typedef detail::x_fast_trie_leaf_link <key_type, value_type> leaf_link;
		typedef detail::x_fast_trie_trait<key_type, value_type> trait;
		typedef typename t_spec::template map_adaptor_trait <node, void, typename node::access_key> level_map_trait;
		typedef typename t_spec::template map_adaptor_trait <key_type, leaf_link> leaf_link_map_trait;
		typedef typename level_map_trait::type level_map;
		typedef typename leaf_link_map_trait::type leaf_link_map;
		typedef typename t_spec::lss_find_fn lss_find_fn;

	public:
		typedef typename lss_access::level_idx_type level_idx_type;
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

		class lss_access
		{
			friend lss_find_fn;
			
		protected:
			template <bool, bool> struct custom_initialization;
			template <bool, bool> friend struct custom_initialization;

		public:
			typedef x_fast_trie_base::level_map level_map;
			typedef x_fast_trie_base::key_type key_type;
			typedef x_fast_trie_base::edge edge;
			typedef x_fast_trie_base::node node;
			typedef std::array <level_map, s_levels> lss;
			typedef typename lss::size_type level_idx_type;

		protected:
			lss m_lss;

		protected:
			template <typename t_lss_acc, typename t_iterator>
			static bool find_node(t_lss_acc &lss, key_type const key, level_idx_type const level, t_iterator &node);

			key_type level_key(key_type const key, level_idx_type const level) const { return key >> level; }
			
		protected:
			template <bool t_check, bool t_dummy = false>
			struct custom_initialization
			{
				void operator()(lss_access &acc) {}
			};
			
			template <bool t_dummy>
			struct custom_initialization <true, t_dummy>
			{
				void operator()(lss_access &acc) { level_map_trait::construct_map_adaptors(acc.m_lss); }
			};

		public:
			lss_access()
			{
				custom_initialization <level_map_trait::needs_custom_constructor()> cs;
				cs(*this);
			}

			typename level_map::size_type level_size(level_idx_type const idx) const;
			level_map &level(level_idx_type const);
			level_map const &level(level_idx_type const) const;

			typename lss::const_reverse_iterator crbegin() const { return m_lss.crbegin(); }
			typename lss::const_reverse_iterator crend() const { return m_lss.crend(); }

			bool find_node(key_type const key, level_idx_type const level, typename level_map::iterator &node);
			bool find_node(key_type const key, level_idx_type const level, typename level_map::const_iterator &node) const;

			void update_levels(key_type const key);
			void erase_key(key_type const key, key_type const prev, key_type const next);
		};

	protected:
		lss_access m_lss;
		leaf_link_map m_leaf_links;
		key_type m_min{};

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
		static bool find_nearest(t_trie &trie, key_type const key, t_iterator &it);
		
		template <typename t_trie, typename t_iterator>
		static bool find_lowest_ancestor(t_trie &trie, key_type const key, t_iterator &it, level_idx_type &level);
		
		template <typename t_trie, typename t_iterator>
		static bool find_lowest_ancestor(
			t_trie &trie,
			key_type const key,
			t_iterator &ancestor,
			level_idx_type &level,
			level_idx_type const bottom,
			level_idx_type const top
		);
		
	public:
		x_fast_trie_base() = default;
		x_fast_trie_base(x_fast_trie_base const &) = default;
		x_fast_trie_base(x_fast_trie_base &&) = default;
		x_fast_trie_base &operator=(x_fast_trie_base const &) & = default;
		x_fast_trie_base &operator=(x_fast_trie_base &&) & = default;

		std::size_t constexpr key_size() const { return sizeof(key_type); }
		size_type size() const { return m_leaf_links.size(); }
		bool contains(key_type const key) const;
		key_type min_key() const { return m_min; } // Returns a meaningful value if the tree is not empty.
		key_type max_key() const;
		
		const_leaf_iterator begin() const	{ return m_leaf_links.cbegin(); }
		const_leaf_iterator end() const		{ return m_leaf_links.cend(); }
		const_leaf_iterator cbegin() const	{ return m_leaf_links.cbegin(); }
		const_leaf_iterator cend() const	{ return m_leaf_links.cend(); }
		
		bool find(key_type const key, const_leaf_iterator &it) const;
		bool find_predecessor(key_type const key, const_leaf_iterator &pred, bool allow_equal = false) const;
		bool find_successor(key_type const key, const_leaf_iterator &succ, bool allow_equal = false) const;
		bool find_nearest(key_type const key, const_leaf_iterator &it) const;
		bool find_lowest_ancestor(key_type const key, typename level_map::const_iterator &it, level_idx_type &level) const;
		bool find_node(key_type const key, level_idx_type const level, typename level_map::const_iterator &node) const;
		
		typename trait::key_type const iterator_key(const_leaf_iterator const &it) const { trait t; return t.key(it); };		
		typename trait::value_type const &iterator_value(const_leaf_iterator const &it) const { trait t; return t.value(it); };

		void print() const;
	};


	template <typename t_spec>
	auto x_fast_trie_base <t_spec>::lss_access::level_size(level_idx_type const idx) const -> typename level_map::size_type
	{
		assert(0 <= idx);
		assert(idx < s_levels);
		return m_lss[idx].size();
	}


	template <typename t_spec>
	auto x_fast_trie_base <t_spec>::lss_access::level(level_idx_type const idx) -> level_map &
	{
		assert(0 <= idx);
		assert(idx < s_levels);
		return m_lss[idx];
	}


	template <typename t_spec>
	auto x_fast_trie_base <t_spec>::lss_access::level(level_idx_type const idx) const -> level_map const &
	{
		assert(0 <= idx);
		assert(idx < s_levels);
		return m_lss[idx];
	}


	template <typename t_spec>
	auto x_fast_trie_base <t_spec>::max_key() const -> key_type
	{
		const_leaf_iterator it;
		bool const status(find(m_min, it));
		assert(status);
		return it->second.prev;
	}


	template <typename t_spec>
	template <typename t_lss_acc, typename t_iterator>
	bool x_fast_trie_base <t_spec>::lss_access::find_node(
		t_lss_acc &lss,
		key_type const key,
		level_idx_type const level,
		t_iterator &node
	)
	{
		assert(0 <= level);
		assert(level < s_levels);
		
		lss_find_fn fn;
		return fn(lss, level, key, node);
	}


	template <typename t_spec>
	bool x_fast_trie_base <t_spec>::lss_access::find_node(
		key_type const key,
		level_idx_type const level,
		typename level_map::iterator &node
	)
	{
		return find_node(*this, key, level, node);
	}


	template <typename t_spec>
	bool x_fast_trie_base <t_spec>::lss_access::find_node(
		key_type const key,
		level_idx_type const level,
		typename level_map::const_iterator &node
	) const
	{
		return find_node(*this, key, level, node);
	}


	template <typename t_spec>
	void x_fast_trie_base <t_spec>::lss_access::update_levels(key_type const key)
	{
		// Update the level map.
		for (level_idx_type i(0); i < s_levels; ++i)
		{
			level_idx_type const level_idx(s_levels - i);
			key_type const lk(level_key(key, level_idx));
			key_type const nlk(level_key(key, level_idx - 1));
			key_type const next_branch(0x1 & nlk);
			key_type const other_branch(!next_branch);
			
			typename level_map::iterator node_it;
			if (find_node(key, level_idx - 1, node_it))
			{
				node node(*node_it);
				node[next_branch] = edge(key);
				if (node.edge_is_descendant_ptr(level_idx - 1, other_branch))
				{
					auto desc(node[other_branch].key());
					node[other_branch] = edge(other_branch ? std::max(desc, key) : std::min(desc, key));
				}
				
				auto &level_map(level(level_idx - 1));
				auto pos(level_map.map().erase(node_it));
				level_map.map().emplace_hint(pos, std::move(node));
			}
			else
			{
				node node(key, key);
				level_idx_type lss_idx(level_idx - 1);
				this->m_lss[lss_idx].map().emplace(std::move(node));
			}
		}
	}


	template <typename t_spec>
	void x_fast_trie_base <t_spec>::lss_access::erase_key(key_type const key, key_type const prev, key_type const next)
	{
		// Walk from the leaf to the root and remove all nodes the other (not current)
		// edge of which is a descendant pointer.
		level_idx_type i(0);
		bool is_minmax[2]{false, false};	// Whether the left or the right branch contains a min/max.
		bool origin_branch{0};				// The branch at the end of which the deleted node was the other branch not being a descendant pointer.
		while (i < s_levels)
		{
			typename level_map::iterator node_it;
			bool const status(find_node(key, i, node_it));
			assert(status);
			node node(*node_it);

			key_type const nlk(level_key(key, i));
			bool const target_branch(0x1 & nlk);
			bool const other_branch(!target_branch);

			if (node.edge_is_descendant_ptr(i, other_branch))
				m_lss[i].map().erase(node_it);
			else
			{
				// If the other branch is not a descendant pointer, there must be a min/max in that branch.
				is_minmax[other_branch] = true;
				
				origin_branch = target_branch;
				
				// Create a descendant pointer in place of the erased branch.
				auto &level_map(level(i));
				node[target_branch] = edge(target_branch ? prev : next);
				auto pos(level_map.map().erase(node_it));
				level_map.map().emplace_hint(pos, std::move(node));
				break;
			}
			
			++i;
		}
		
		++i;
		while (i < s_levels)
		{
			typename level_map::iterator node_it;
			bool const status(find_node(key, i, node_it));
			assert(status);
			node node(*node_it);
			auto &level_map(level(i));

			key_type const nlk(level_key(key, i));
			bool const target_branch(0x1 & nlk);
			bool const other_branch(!target_branch);

			// The not-origin branch will point to the other side (min / max)
			// of the subtree and hence needn't be updated.
			if (other_branch == origin_branch)
			{
				if (node.edge_is_descendant_ptr(i, other_branch))
				{
					node[other_branch] = edge(other_branch ? prev : next);
					auto pos(level_map.map().erase(node_it));
					level_map.map().emplace_hint(pos, std::move(node));
				}
				else
				{
					// If the other branch is not a descendant pointer, there must be a min/max in that branch.
					is_minmax[other_branch] = true;
					if (is_minmax[0] && is_minmax[1])
						break;
				}
			}
			
			++i;
		}
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
		if (0 == trie.size())
			return false;
		
		t_iterator it;
		bool const status(find(trie, key, it));

		if (allow_equal && status)
		{
			pred = it;
			return true;
		}
		else if (!status)
		{
			bool const status(find_nearest(trie, key, it));
			assert(status);
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
		if (0 == trie.size())
			return false;

		t_iterator it;
		bool const status(find(trie, key, it));
		
		if (allow_equal && status)
		{
			succ = it;
			return true;
		}
		else if (!status)
		{
			bool const status(find_nearest(trie, key, it));
			assert(status);
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
	bool x_fast_trie_base <t_spec>::find_nearest(key_type const key, const_leaf_iterator &it) const
	{
		return find_nearest(*this, key, it);
	}


	template <typename t_spec>
	template <typename t_trie, typename t_iterator>
	bool x_fast_trie_base <t_spec>::find_nearest(t_trie &trie, key_type const key, t_iterator &nearest)
	{
		if (0 == trie.size())
			return false;

		// FIXME: can lowest ancestor return false?
		level_idx_type level(0);
		typename iterator_type <level_map, t_trie>::type it;
		bool const status(find_lowest_ancestor(trie, key, it, level));
		assert(status);
		key_type const next_branch(0x1 & (key >> level));
		auto const &node(*it);
		
		assert(1 == level || node.edge_is_descendant_ptr(level, next_branch));

		edge const &edge(node[next_branch]);
		nearest = trie.m_leaf_links.find(edge.key());
		assert(trie.m_leaf_links.cend() != nearest);
		assert(edge.key() == nearest->first);
		return true;
	}


	template <typename t_spec>
	bool x_fast_trie_base <t_spec>::find_lowest_ancestor(
		key_type const key,
		typename level_map::const_iterator &it,
		level_idx_type &level) const
	{
		return find_lowest_ancestor(*this, key, it, level);
	}


	template <typename t_spec>
	template <typename t_trie, typename t_iterator>
	bool x_fast_trie_base <t_spec>::find_lowest_ancestor(
		t_trie &trie,
		key_type const key,
		t_iterator &it,
		level_idx_type &level
	)
	{
		return find_lowest_ancestor(trie, key, it, level, 0, s_levels - 1);
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
		
		if (!trie.m_lss.find_node(key, mid, ancestor))
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
		return m_lss.find_node(key, level, node);
	}


	template <typename t_spec>
	void x_fast_trie_base <t_spec>::print() const
	{
		std::cerr
			<< "LSS:" << std::endl
			<< "[level]: key [0x1 & key]: left key (left value) [d if descendant] right key (right value) [d if descendant]" << std::endl;

		{
			util::remove_c_t <decltype(s_levels)> i(0);
			for (auto lss_it(m_lss.crbegin()), lss_end(m_lss.crend()); lss_it != lss_end; ++lss_it)
			{
				auto const level(s_levels - i - 1);
				std::cerr << boost::format("[%02x]:") % level;
				assert(1 + level == lss_it->access_key_fn().level());
				for (auto node_it(lss_it->cbegin()), node_end(lss_it->cend()); node_it != node_end; ++node_it)
				{
					// In case of the PHF map adaptor, node_it->first points to the
					// first item in the pair (which is the key as expected),
					// not the vector index.
					auto const &node(*node_it);
					auto const &lhs(node[0]);
					auto const &rhs(node[1]);
					auto const key(lss_it->access_key_fn()(node));
					auto const llk(lhs.level_key(level));
					auto const rlk(rhs.level_key(level));
					auto const lk(lhs.key());
					auto const rk(rhs.key());
					bool const l_is_descendant(node.edge_is_descendant_ptr(level, 0));
					bool const r_is_descendant(node.edge_is_descendant_ptr(level, 1));

					assert(lhs.level_key(1 + level) == key);
					assert(rhs.level_key(1 + level) == key);

					std::cerr << boost::format("\t(%02x [%02x]: %02x (%02x)") % (+key) % (+(0x1 & key)) % (+llk) % (+lk);
					if (l_is_descendant)
						std::cerr << " [d] ";
					else
						std::cerr << "     ";

					std::cerr << boost::format("%02x (%02x)") % (+rlk) % (+rk);
					if (r_is_descendant)
						std::cerr << " [d] ";
					else
						std::cerr << "     ";
					std::cerr << ")" << std::endl;
				}
	
				std::cerr << std::endl;
				++i;
			}
		}
		
		std::cerr << "Leaves:" << std::endl;
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
