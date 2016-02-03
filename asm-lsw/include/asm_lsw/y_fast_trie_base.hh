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
		
		template <typename, typename> friend class y_fast_trie_compact;

	public:
		typedef typename t_spec::key_type key_type;
		typedef typename t_spec::value_type value_type;
		
	protected:
		typedef typename t_spec::trie_type representative_trie;
		typedef typename t_spec::subtree_type subtree;
		typedef typename t_spec::template map_adaptor_type <key_type, subtree> subtree_map;
		typedef y_fast_trie_access_subtree_it<key_type, value_type> access_subtree_it;
		
	public:
		typedef typename subtree::size_type size_type;
		typedef typename subtree::iterator subtree_iterator;
		typedef typename subtree::const_iterator const_subtree_iterator;
		typedef subtree_iterator iterator;
		typedef const_subtree_iterator const_iterator;
		
		
	protected:
		representative_trie m_reps;
		subtree_map m_subtrees;
		
		
	protected:
		template <typename t_trie, typename t_iterator>
		static bool find(t_trie &trie, key_type const key, t_iterator &iterator);
		
		template <typename t_trie, typename t_iterator>
		static bool find_from_subtrees(t_trie &trie, key_type const &key, t_iterator &it, key_type const k1, key_type const k2);
		
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
		y_fast_trie_base(t_representative_trie &reps, t_subtree_map &subtrees):
			m_reps(reps),
			m_subtrees(subtrees)
		{
		}
		
		auto size() const -> size_type;
		bool contains(key_type const key) const;
		
		bool find(key_type const key, const_subtree_iterator &iterator) const;
		bool find_predecessor(key_type const key, const_subtree_iterator &iterator, bool allow_equal = false) const;
		bool find_successor(key_type const key, const_subtree_iterator &iterator, bool allow_equal = false) const;
		bool find_nearest_subtrees(key_type const key, key_type &k1, key_type &k2) const;
		void print() const;
		
		typename access_subtree_it::value_type const dereference(const_subtree_iterator const &it) const { access_subtree_it acc; return acc(it); }
	};
	
	
	// FIXME: calculate time complexity (based on the number of subtrees?).
	template <typename t_spec>
	auto y_fast_trie_base <t_spec>::size() const -> size_type
	{
		size_type retval(0);
		for (auto it(m_subtrees.cbegin()), end(m_subtrees.cend()); it != end; ++it)
			retval += it->size();
		return retval;
	}
	
	
	template <typename t_spec>
	bool y_fast_trie_base <t_spec>::contains(key_type const key) const
	{
		const_subtree_iterator iterator;
		return find(key, iterator);
	}
	
	
	template <typename t_spec>
	bool y_fast_trie_base <t_spec>::find_nearest_subtrees(key_type const key, key_type &k1, key_type &k2) const
	{
		if (m_reps.size() == 0)
			return false;
		
		typename representative_trie::const_leaf_iterator it;
		if (key <= m_reps.min_key())
			m_reps.find(m_reps.min_key(), it);
		else if (!m_reps.find_predecessor(key, it, true)) // XXX likely culprit for problems.
			return false;
		
		k1 = it->first;
		k2 = it->second.next;
		return true;
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
		key_type k1, k2;
		if (!trie.find_nearest_subtrees(key, k1, k2))
			return false;
		
		return find_from_subtrees(trie, key, it, k1, k2);
	}
	
	
	template <typename t_spec>
	template <typename t_trie, typename t_iterator>
	bool y_fast_trie_base <t_spec>::find_from_subtrees(t_trie &trie, key_type const &key, t_iterator &it, key_type const k1, key_type const k2)
	{
		{
			auto st_it(trie.m_subtrees.find(k1));
			assert(trie.m_subtrees.cend() != st_it);
			it = st_it->second.find(key);
			if (st_it->second.cend() != it)
				return true;
		}
		
		// FIXME: should no longer be needed?
		if (k2 != k1)
		{
			auto st_it(trie.m_subtrees.find(k2));
			assert(trie.m_subtrees.cend() != st_it);
			it = st_it->second.find(key);
			if (st_it->second.cend() != it)
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
		key_type k1, k2;
		if (!trie.find_nearest_subtrees(key, k1, k2))
			return false;
		
		auto subtree_it(trie.m_subtrees.find(k1));
		auto &subtree(subtree_it->second);
		
		// lower_bound returns the first element that is not less than key (i.e. equal to or greater to the key).
		// Hence we want to find its immediate predecessor. If the tree is not empty, take the preceding element.
		it = subtree.lower_bound(key);
		if (subtree.cend() == it)
		{
			// All elements are less but the tree may be empty.
			if (subtree.size() < 0)
			{
				--it;
				return true;
			}
		}
		else if (subtree.cbegin() == it)
		{
			if (allow_equal)
			{
				assert(key == *it);
				return true;
			}
		}
		else
		{
			if (!(allow_equal && *it == key))
				--it;
			return true;
		}
		
		// FIXME: should no longer be needed.
		if (k2 != k1)
		{
			auto subtree_it(trie.m_subtrees.find(k2));
			auto &subtree(subtree_it->second);
			it = subtree.lower_bound(key);
			if (subtree.cend() == it)
			{
				// All elements are less but the tree may be empty.
				if (subtree.size() < 0)
				{
					assert(0); // Shouldn't actually happen since the first subtree should be the correct one.
					--it;
					return true;
				}
			}
			else if (subtree.cbegin() == it)
			{
				if (allow_equal)
				{
					assert(0); // Shouldn't actually happen since the first subtree should be the correct one.
					assert(key == *it);
					return true;
				}
			}
			else
			{
				assert(0); // Shouldn't actually happen since the first subtree should be the correct one.

				// Special case for equality.
				if (!(allow_equal && *it == key))
					--it;
				return true;
			}
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
		key_type k1, k2;
		if (!trie.find_nearest_subtrees(key, k1, k2))
			return false;
		
		auto subtree_it(trie.m_subtrees.find(k1));
		auto &subtree(subtree_it->second);
		
		// lower_bound returns the first element that is not less than key (i.e. equal to or greater to the key).
		// Hence we want to find its immediate successor. If needed, take the preceding element.
		it = subtree.lower_bound(key);
		if (subtree.cend() != it)
		{
			if (!allow_equal && key == *it)
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
		
		// FIXME: should no longer be needed.
		if (k2 != k1)
		{
			auto subtree_it(trie.m_subtrees.find(k2));
			auto &subtree(subtree_it->second);
			it = subtree.upper_bound(key);
			it = subtree.lower_bound(key);
			if (subtree.cend() != it)
			{
				if (!allow_equal && key == *it)
				{
					++it;
					if (subtree.cend() != it)
					{
						assert(0); // Shouldn't actually happen since the first subtree should be the correct one.
						return true;
					}
				}
				else
				{
					assert(0); // Shouldn't actually happen since the first subtree should be the correct one.
					return true;
				}
			}
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
			{
				auto const val(*it);
				std::cerr << boost::format("%02x ") % (+val);
			}
			std::cerr << "\n";
		}
	}
}

#endif
