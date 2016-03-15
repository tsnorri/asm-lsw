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


#ifndef ASM_LSW_Y_FAST_TRIE_HH
#define ASM_LSW_Y_FAST_TRIE_HH

#include <asm_lsw/exception_tpl.hh>
#include <asm_lsw/fast_trie_common.hh>
#include <asm_lsw/util.hh>
#include <asm_lsw/y_fast_trie_base.hh>
#include <unordered_map>


namespace asm_lsw {
	
	template <typename t_key, typename t_value>
	class x_fast_trie;
}


namespace asm_lsw { namespace detail {

	template <typename t_key, typename t_value, typename t_access_key = map_adaptor_access_key <t_key>>
	using y_fast_trie_map_adaptor = map_adaptor <
		fast_trie_map_adaptor_map <t_value>::template type,
		t_key,
		t_value,
		t_access_key
	>;


	template <typename t_key, typename t_value>
	using y_fast_trie_spec = y_fast_trie_base_spec <
		t_key,
		t_value,
		y_fast_trie_map_adaptor,
		x_fast_trie <t_key, void>
	>;
}}	
	

namespace asm_lsw {

	template <typename t_key, typename t_value = void>
	class y_fast_trie : public y_fast_trie_base <detail::y_fast_trie_spec <t_key, t_value>>
	{
		template <typename, typename> friend class y_fast_trie_compact_as;

	public:
		typedef y_fast_trie_base <detail::y_fast_trie_spec <t_key, t_value>> base_class;
		typedef typename base_class::key_type key_type;
		typedef typename base_class::value_type value_type;
		typedef typename base_class::size_type size_type;
		typedef typename base_class::subtree_iterator subtree_iterator;
		typedef typename base_class::const_subtree_iterator const_subtree_iterator;
		typedef typename base_class::iterator iterator;
		typedef typename base_class::const_iterator const_iterator;
		
	protected:
		typedef typename base_class::representative_trie representative_trie;
		typedef typename base_class::subtree subtree;
		typedef typename base_class::subtree_map subtree_map;

	public:
		enum class error : uint32_t
		{
			no_error = 0,
			out_of_range
		};
		
	protected:
		key_type m_key_limit{std::numeric_limits <key_type>::max()};

	protected:
		void check_merge_subtree(typename subtree_map::map_type::iterator &st_it);
		void split_subtree(typename subtree_map::map_type::iterator &st_it);
		typename subtree_map::iterator find_subtree(key_type const key);
		void check_subtree_size(typename subtree_map::iterator st_it);
		
	public:
		y_fast_trie() = default;
		y_fast_trie(y_fast_trie const &) = default;
		y_fast_trie(y_fast_trie &&) = default;
		y_fast_trie &operator=(y_fast_trie const &) & = default;
		y_fast_trie &operator=(y_fast_trie &&) & = default;
		y_fast_trie(key_type const key_limit): m_key_limit(key_limit) {}

		key_type key_limit() const { return m_key_limit; }
		
		// Conditionally enable either.
		// (Return type of the first one is void == std::enable_if<...>::type.)
		template <typename T = value_type>
		typename std::enable_if<std::is_void<T>::value, void>::type
		insert(key_type const key);
		
		template <typename T = value_type>
		void insert(key_type const key, typename std::enable_if<!std::is_void<T>::value, T>::type const val); // FIXME: add emplace.

		bool erase(key_type const key);
		
		using base_class::find;
		using base_class::find_predecessor;
		using base_class::find_successor;
		
		bool find(key_type const key, subtree_iterator &iterator);
		bool find_predecessor(key_type const key, subtree_iterator &iterator, bool allow_equal = false);
		bool find_successor(key_type const key, subtree_iterator &iterator, bool allow_equal = false);
	};
	
	
	// FIXME: check this.
	template <typename t_key, typename t_value>
	auto y_fast_trie <t_key, t_value>::find_subtree(key_type const key) -> typename subtree_map::iterator
	{
		// Find the correct subtree. If the inserted key is smaller than any representative,
		// just use the first subtree.
		typename representative_trie::const_leaf_iterator it;
		bool res(false);
		if (key <= this->m_reps.min_key())
			res = this->m_reps.find(this->m_reps.min_key(), it);
		else
			res = this->m_reps.find_predecessor(key, it);
				
		assert(res);
				
		key_type const rep(it->first);
		auto st_it(this->m_subtrees.find(rep));
			
		return st_it;
	}
	
	
	// Check the size limit.
	// FIXME: check this.
	template <typename t_key, typename t_value>
	void y_fast_trie <t_key, t_value>::check_subtree_size(typename subtree_map::iterator st_it)
	{
		auto const log2M(std::log2(m_key_limit));
		if (log2M)
		{
			auto const size(st_it->second.size());
			if (2 * log2M < size)
				split_subtree(st_it);
		}
	}

	
	// FIXME: check this.
	template <typename t_key, typename t_value>
	template <typename T>
	typename std::enable_if <std::is_void <T>::value, void>::type
	y_fast_trie <t_key, t_value>::insert(key_type const key)
	{
		asm_lsw_assert(key <= m_key_limit, std::invalid_argument, error::out_of_range);
		++this->m_size;

		if (0 == this->m_reps.size())
		{
			this->m_reps.insert(key);
			this->m_subtrees.map().emplace(key, std::initializer_list <key_type>{key});
			return;
		}
		
		auto st_it(find_subtree(key));
		st_it->second.emplace(key);
		
		check_subtree_size(st_it);
	}

	
	// FIXME: check this.
	template <typename t_key, typename t_value>
	template <typename T>
	void y_fast_trie <t_key, t_value>::insert(
		key_type const key, typename std::enable_if <!std::is_void <T>::value, T>::type const val
	)
	{
		asm_lsw_assert(key <= m_key_limit, std::invalid_argument, error::out_of_range);
		++this->m_size;

		if (0 == this->m_reps.size())
		{
			this->m_reps.insert(key);
			this->m_subtrees.map().emplace(
				key,
				std::move(subtree{{key, std::move(val)}})
			);
			return;
		}
		
		auto st_it(find_subtree(key));
		st_it->second.emplace(key, val);
		
		check_subtree_size(st_it);
	}

	
	// FIXME: check this.
	template <typename t_key, typename t_value>
	bool y_fast_trie <t_key, t_value>::erase(key_type const key)
	{
		if (0 == this->m_reps.size())
			return false;
		
		key_type k1, k2;
		if (!this->find_nearest_subtrees(key, k1, k2))
			return false;
		
		{
			auto st_it(this->m_subtrees.find(k1));
			if (st_it->second.erase(key))
			{
				--this->m_size;
				check_merge_subtree(st_it);
				return true;
			}
		}
		
		// FIXME shouldn't be needed.
		if (k2 != k1)
		{
			auto st_it(this->m_subtrees.find(k2));
			if (st_it->second.erase(key))
			{
				assert(0);
				--this->m_size;
				check_merge_subtree(st_it);
				return true;
			}
		}
		
		return false;
	}
	
	
	// FIXME: check this (and then the base class methods).
	template <typename t_key, typename t_value>
	void y_fast_trie <t_key, t_value>::check_merge_subtree(typename subtree_map::map_type::iterator &st_it)
	{
		auto const log2M(std::log2(m_key_limit));
		auto &tree(st_it->second);
		if (! (tree.size() < log2M / 4))
			return;
		
		auto &map(this->m_subtrees.map());
		typename representative_trie::leaf_iterator it;
		this->m_reps.find(st_it->first, it);
		auto const next(it->second.next);
		if (next == st_it->first)
		{
			if (tree.size() == 0)
			{
				map.erase(st_it);
				this->m_reps.erase(it->first);
			}
			return;
		}
		
		auto target_it(map.find(next));
		subtree &target(target_it->second);
		subtree_iterator hint(tree.end());
		for (auto it(target.cbegin()), end(target.cend()); it != end; ++it)
			hint = tree.emplace_hint(hint, *it);
		map.erase(target_it);
		this->m_reps.erase(next);
		
		if (2 * log2M < tree.size())
			split_subtree(st_it);
	}
	
	
	// FIXME: check this (and then the base class methods).
	template <typename t_key, typename t_value>
	void y_fast_trie <t_key, t_value>::split_subtree(typename subtree_map::map_type::iterator &st_it)
	{
		using std::swap;

		auto const log2M(std::log2(m_key_limit));
		assert(log2M);
		
		// Create a new tree, move half of the contents.
		subtree t1, t2;
		swap(t1, st_it->second);
		subtree_iterator hint(t2.begin());

		auto it(t1.cbegin());
		for (util::remove_c_t<decltype(log2M)> i(0); i < log2M; ++i)
		{
			hint = t2.emplace_hint(hint, *it);
			++it;
		}
		assert(t2.size());
		t1.erase(t1.cbegin(), it);

		// Choose new representatives and store the trees.
		auto const t1_rep(this->iterator_key(t1.cbegin()));
		auto const t2_rep(this->iterator_key(t2.cbegin()));

		// In this class, subtree_map's map_type has non-hashed keys.
		this->m_reps.erase(st_it->first);
		this->m_reps.insert(t1_rep);
		this->m_reps.insert(t2_rep);
		
		auto &map(this->m_subtrees.map());
		if (! (t1_rep == st_it->first || t2_rep == st_it->first))
			map.erase(st_it);

		swap(map[t1_rep], t1);
		swap(map[t2_rep], t2);
	}
	
	
	template <typename t_key, typename t_value>
	bool y_fast_trie <t_key, t_value>::find(key_type const key, subtree_iterator &iterator)
	{
		return find(*this, key, iterator);
	}
	
	
	template <typename t_key, typename t_value>
	bool y_fast_trie <t_key, t_value>::find_predecessor(key_type const key, subtree_iterator &iterator, bool allow_equal)
	{
		return find_predecessor(*this, key, iterator, allow_equal);
	}
	
	
	template <typename t_key, typename t_value>
	bool y_fast_trie <t_key, t_value>::find_successor(key_type const key, subtree_iterator &iterator, bool allow_equal)
	{
		return find_successor(*this, key, iterator, allow_equal);
	}
}

#endif
