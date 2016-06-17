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


#ifndef ASM_LSW_Y_FAST_TRIE_BASE_HH
#define ASM_LSW_Y_FAST_TRIE_BASE_HH

#include <asm_lsw/map_adaptor.hh>
#include <asm_lsw/y_fast_trie_helper.hh>
#include <boost/format.hpp>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <type_traits>


namespace asm_lsw {
	
	template <typename t_spec>
	class y_fast_trie_base
	{
		static_assert(std::is_integral <typename t_spec::key_type>::value, "Unsigned integer required.");
		static_assert(!std::is_signed <typename t_spec::key_type>::value, "Unsigned integer required.");
		
		template <typename, typename, bool> friend class y_fast_trie_compact;
		template <typename, typename, typename, bool> friend class y_fast_trie_compact_as_tpl;

	public:
		typedef typename t_spec::key_type key_type;
		typedef typename t_spec::value_type value_type; // FIXME: change to something meaningful (pair for mutable).
		typedef typename t_spec::value_type mapped_type;
		
	protected:
		typedef typename t_spec::trie_type representative_trie_type;
		typedef typename t_spec::subtree_type subtree;
		typedef typename t_spec::template map_adaptor <key_type, subtree> subtree_map;
		typedef detail::y_fast_trie_trait<key_type, value_type> trait;
		
	public:
		typedef size_t size_type;
		typedef typename subtree_map::const_iterator const_subtree_map_iterator;
		
		typedef typename subtree::iterator subtree_iterator;
		typedef typename subtree::const_iterator const_subtree_iterator;
		typedef typename subtree::reverse_iterator reverse_subtree_iterator;
		typedef typename subtree::const_reverse_iterator const_reverse_subtree_iterator;
		typedef subtree_iterator iterator;
		typedef const_subtree_iterator const_iterator;
		typedef reverse_subtree_iterator reverse_iterator;
		typedef const_reverse_subtree_iterator const_reverse_iterator;
		
		
	protected:
		representative_trie_type m_reps;
		subtree_map m_subtrees;
		size_type m_size{0};
		
		
	protected:
		template <typename t_trie, typename t_iterator>
		static bool find(t_trie &trie, key_type const key, t_iterator &iterator);
		
		template <typename t_trie, typename t_iterator>
		static bool find_predecessor(t_trie &trie, key_type const key, t_iterator &it, bool allow_equal);
		
		template <typename t_trie, typename t_iterator>
		static bool find_successor(t_trie &trie, key_type const key, t_iterator &it, bool allow_equal);
		
		
	public:
		y_fast_trie_base() = default;
		y_fast_trie_base(y_fast_trie_base const &) = default;
		y_fast_trie_base(y_fast_trie_base &&) = default;
		y_fast_trie_base &operator=(y_fast_trie_base const &) & = default;
		y_fast_trie_base &operator=(y_fast_trie_base &&) & = default;
		
		template <typename t_representative_trie, typename t_subtree_map>
		y_fast_trie_base(t_representative_trie &reps, t_subtree_map &subtrees, size_type size):
			m_reps(reps),
			m_subtrees(subtrees),
			m_size(size)
		{
		}
		
		representative_trie_type const &representative_trie() const { return m_reps; }
		
		std::size_t constexpr key_size() const { return sizeof(key_type); }
		size_type size() const;
		bool contains(key_type const key) const;
		key_type min_key() const { return m_reps.min_key(); } // Returns a meaningful value if the tree is not empty.
		key_type max_key() const;
		bool find_next_subtree_key(key_type &key /* inout */) const;

		bool find(key_type const key, const_subtree_iterator &iterator) const;
		bool find_predecessor(key_type const key, const_subtree_iterator &iterator, bool allow_equal = false) const;
		bool find_successor(key_type const key, const_subtree_iterator &iterator, bool allow_equal = false) const;
		bool find_subtree_min(key_type const key, const_subtree_iterator &iterator) const;
		bool find_subtree_max(key_type const key, const_subtree_iterator &iterator) const;
		bool find_subtree_exact(key_type const key, const_subtree_map_iterator &iterator) const;
		void print() const;

		y_fast_trie_subtree_map_proxy <subtree_map> subtree_map_proxy() { return y_fast_trie_subtree_map_proxy <subtree_map>(&m_subtrees); }
		y_fast_trie_subtree_map_proxy <subtree_map const> subtree_map_proxy() const { return y_fast_trie_subtree_map_proxy <subtree_map const>(&m_subtrees); }

		typename subtree_map::const_iterator subtree_cbegin() const { return m_subtrees.cbegin(); }
		typename subtree_map::const_iterator subtree_cend() const { return m_subtrees.cend(); }
		
		typename trait::key_type const iterator_key(const_subtree_iterator const &it) const { trait t; return t.key(it); }
		typename trait::value_type const &iterator_value(const_subtree_iterator const &it) const { trait t; return t.value(it); }
		typename trait::key_type const iterator_key(const_reverse_subtree_iterator const &it) const { trait t; return t.key(it); }
		typename trait::value_type const &iterator_value(const_reverse_subtree_iterator const &it) const { trait t; return t.value(it); }
	};
	
	
	template <typename t_spec>
	auto y_fast_trie_base <t_spec>::size() const -> size_type
	{
		return m_size;
	}
	
	
	template <typename t_spec>
	bool y_fast_trie_base <t_spec>::contains(key_type const key) const
	{
		const_subtree_iterator iterator;
		return find(key, iterator);
	}
	
	
	template <typename t_spec>
	auto y_fast_trie_base <t_spec>::max_key() const -> key_type
	{
		if (this->size() == 0)
			return 0;
		
		auto const max(this->m_reps.max_key());
		auto st_it(this->m_subtrees.find(max));
		assert(this->m_subtrees.cend() != st_it);
		auto const &subtree(st_it->second);
		assert(subtree.size());
		return this->iterator_key(subtree.crbegin());
	}
	
	
	template <typename t_spec>
	bool y_fast_trie_base <t_spec>::find_subtree_min(key_type const key, const_subtree_iterator &iterator) const
	{
		const_subtree_map_iterator st_it;
		if (!find_subtree_exact(key, st_it))
			return false;
		
		auto &subtree(st_it->second);
		if (0 == subtree.size())
			return false;
		
		iterator = subtree.cbegin();
		return true;
	}

	
	template <typename t_spec>
	bool y_fast_trie_base <t_spec>::find_subtree_max(key_type const key, const_subtree_iterator &iterator) const
	{
		const_subtree_map_iterator st_it;
		if (!find_subtree_exact(key, st_it))
			return false;
		
		auto &subtree(st_it->second);
		if (0 == subtree.size())
			return false;
		
		iterator = --subtree.cend();
		return true;
	}
	
	
	template <typename t_spec>
	bool y_fast_trie_base <t_spec>::find_subtree_exact(key_type const key, const_subtree_map_iterator &iterator) const
	{
		auto it(m_subtrees.find(key));
		if (m_subtrees.cend() == it)
			return false;
		
		iterator = it;
		return true;
	}
	
	
	template <typename t_spec>
	bool y_fast_trie_base <t_spec>::find_next_subtree_key(key_type &key /* inout */) const
	{
		typename representative_trie_type::const_leaf_iterator it;
		if (m_reps.find_successor(key, it, false))
		{
			key = it->first;
			return true;
		}
		
		return false;
	}
	
	
	template <typename t_spec>
	bool y_fast_trie_base <t_spec>::find(key_type const key, const_subtree_iterator &iterator) const
	{
		return find(*this, key, iterator);
	}
	
	
	template <typename t_spec>
	template <typename t_trie, typename t_iterator>
	bool y_fast_trie_base <t_spec>::find(t_trie &trie, key_type const key, t_iterator &it)
	{
		typename representative_trie_type::const_leaf_iterator leaf_it;
		if (!trie.m_reps.find_predecessor(key, leaf_it, true))
			return false;
		
		key_type const st_key(leaf_it->first);
		auto st_it(trie.m_subtrees.find(st_key));
		assert(trie.m_subtrees.cend() != st_it);
		auto &subtree(st_it->second);
		auto val_it(subtree.find(key));
		if (subtree.cend() != val_it)
		{
			it = val_it;
			return true;
		}
		
		return false;
	}
	
	
	template <typename t_spec>
	bool y_fast_trie_base <t_spec>::find_predecessor(key_type const key, const_subtree_iterator &iterator, bool allow_equal) const
	{
		return find_predecessor(*this, key, iterator, allow_equal);
	}
	
	
	template <typename t_spec>
	template <typename t_trie, typename t_iterator>
	bool y_fast_trie_base <t_spec>::find_predecessor(t_trie &trie, key_type const key, t_iterator &it, bool allow_equal)
	{
		typename representative_trie_type::const_leaf_iterator leaf_it;
		if (!trie.m_reps.find_predecessor(key, leaf_it, allow_equal))
			return false;

		key_type const k1(leaf_it->first);
		auto subtree_it(trie.m_subtrees.find(k1));
		auto &subtree(subtree_it->second);
		
		// lower_bound returns the first element that is not less than key (i.e. equal to or greater to the key).
		// Hence we want to find its immediate predecessor. If the tree is not empty, take the preceding element.
		it = subtree.lower_bound(key);
		if (subtree.cend() == it)
		{
			// All elements are less but the tree may be empty.
			if (0 < subtree.size())
			{
				--it;
				return true;
			}
		}
		else if (subtree.cbegin() == it)
		{
			if (allow_equal)
			{
				assert(key == trie.iterator_key(it));
				return true;
			}

			// Since allow_equal were tested when calling find_predecessor for the representatives,
			// there must not be another subtree that contains a suitable value.
			return false;
		}
		else
		{
			if (!(allow_equal && trie.iterator_key(it) == key))
				--it;
			return true;
		}
		
		return false;
	}
	
	
	template <typename t_spec>
	bool y_fast_trie_base <t_spec>::find_successor(key_type const key, const_subtree_iterator &iterator, bool allow_equal) const
	{
		return find_successor(*this, key, iterator, allow_equal);
	}
	
	
	template <typename t_spec>
	template <typename t_trie, typename t_iterator>
	bool y_fast_trie_base <t_spec>::find_successor(t_trie &trie, key_type const key, t_iterator &it, bool allow_equal)
	{
		typename representative_trie_type::const_leaf_iterator leaf_it;
		if (!trie.m_reps.find_predecessor(key, leaf_it, true))
		{
			if (!trie.size())
				return false;
			
			key_type const min(trie.min_key());
			const_subtree_map_iterator st_it;
			auto const status(trie.find_subtree_exact(min, st_it));
			assert(status);
			assert(st_it->second.size());
			it = st_it->second.begin();
			return true;
		}

		key_type const k1(leaf_it->first);
		auto subtree_it(trie.m_subtrees.find(k1));
		auto &subtree(subtree_it->second);
		
		// lower_bound returns the first element that is not less than key (i.e. equal to or greater to the key).
		// Hence we want to find its immediate successor. If needed, take the preceding element.
		it = subtree.lower_bound(key);
		if (subtree.cend() != it)
		{
			if (!allow_equal && key == trie.iterator_key(it))
			{
				++it;
				if (subtree.cend() != it)
					return true;
			}
			else
			{
				return true;
			}
		}
		
		// If key is the rightmost value of a subtree, the minimum (i.e. representative)
		// of the next subtree is the successor.
		key_type const k2(leaf_it->second.next);
		if (k1 < k2)
		{
			assert(key < k2);
			auto subtree_it(trie.m_subtrees.find(k2));
			assert(subtree_it != trie.m_subtrees.cend());
			auto &subtree(subtree_it->second);
			auto min_it(subtree.cbegin());
			assert(subtree.cend() != min_it);
			it = min_it;
			return true;
		}
		
		return false;
	}
	
	
	template <typename t_spec>
	void y_fast_trie_base <t_spec>::print() const
	{
		std::cerr << boost::format("Representatives (%02x):\n\t") % m_reps.size();
		for (auto it(m_reps.cbegin()), end(m_reps.cend()); it != end; ++it)
		{
			auto const key(it->first);
			std::cerr << std::hex << +key << " ";
		}

		std::cerr << "\n";
		
		std::cerr << boost::format("Subtrees (%02x):\n") % m_subtrees.size();
		for (auto st_it(m_subtrees.cbegin()), st_end(m_subtrees.cend()); st_it != st_end; ++st_it)
		{
			auto const key(st_it->first);
			std::cerr << boost::format("\t%02x: ") % (+key);
			for (auto it(st_it->second.cbegin()), end(st_it->second.cend()); it != end; ++it)
				std::cerr << boost::format("%02x ") % (+iterator_value(it));

			std::cerr << "\n";
		}
	}
}

#endif
