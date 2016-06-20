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

#ifndef ASM_LSW_STATIC_BINARY_TREE_HH
#define ASM_LSW_STATIC_BINARY_TREE_HH

#include <asm_lsw/util.hh>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/iterator/reverse_iterator.hpp>
#include <sdsl/int_vector.hpp>
#include <sdsl/rank_support.hpp>


namespace asm_lsw { namespace detail {
	
	template <typename t_tree>
	class static_binary_tree_iterator_tpl : public boost::iterator_facade <
		static_binary_tree_iterator_tpl <t_tree>,
		typename t_tree::key_type,
		boost::random_access_traversal_tag,
		typename t_tree::key_type
	>
	{
		friend class boost::iterator_core_access;

	protected:
		typedef boost::iterator_facade <
			static_binary_tree_iterator_tpl <t_tree>,
			typename t_tree::key_type,
			boost::bidirectional_traversal_tag,
			typename t_tree::key_type
		> base_class;
		
	public:
		typedef typename t_tree::size_type size_type;
		typedef typename base_class::difference_type difference_type;
		
	protected:
		t_tree const *m_tree{nullptr};
		size_type m_idx;
		
		
	private:
		// For the converting constructor below.
		struct enabler {};
		
	public:
		static_binary_tree_iterator_tpl() = default;
		
		static_binary_tree_iterator_tpl(t_tree const &tree, size_type idx):
			m_tree(&tree),
			m_idx(idx)
		{
		}
		
	private:
		void increment();
		void decrement();
		
		bool equal(static_binary_tree_iterator_tpl <t_tree> const &other) const ASM_LSW_PURE;
		
		typename t_tree::key_type dereference() const;
	};
}}


namespace asm_lsw
{
	template <typename t_key, typename t_mapped = void, bool t_enable_serialize = false>
	class static_binary_tree
	{
		template <typename> friend class detail::static_binary_tree_iterator_tpl;
		
	public:
		typedef t_key															key_type;
		typedef t_mapped														mapped_type;
		typedef sdsl::int_vector <std::numeric_limits <t_key>::digits>			key_vector_type;
		typedef typename key_vector_type::size_type								size_type;
		
		typedef detail::static_binary_tree_iterator_tpl <static_binary_tree>	const_iterator;
		typedef boost::reverse_iterator <const_iterator>						const_reverse_iterator;
		
		
	protected:
		key_vector_type					m_keys;
		sdsl::bit_vector				m_used_indices;
		sdsl::bit_vector::rank_1_type	m_used_indices_r1_support;
		size_type						m_leftmost_idx{0};
		size_type						m_past_end_idx{0};
		
	protected:
		// Ahnentafel system.
		size_type left_child(size_type const i) const ASM_LSW_CONST { return 2 * i + 1; }
		size_type right_child(size_type const i) const ASM_LSW_CONST { return 2 * i + 2; }
		size_type parent(size_type const i) const ASM_LSW_CONST { return (i - 1) / 2; }
		
		bool left_child_c(size_type &i /* inout */) const;
		bool right_child_c(size_type &i /* inout */) const;
		bool parent_c(size_type &i /* inout */) const ASM_LSW_CONST;
		
		bool find_leftmost(size_type &i /* inout */) const;
		
		template <typename t_int_vector>
		void fill_used_indices(t_int_vector const &keys, size_type const target, size_type const begin, size_type const end);

		template <typename t_int_vector>
		void fill_keys(t_int_vector const &keys, size_type const target, size_type const begin, size_type const end);
		
		bool lower_bound_subtree(key_type const &key, size_type idx, const_iterator &it /* out */) const;
		
	public:
		static_binary_tree() {}
		
		template <typename t_int_vector>
		static_binary_tree(t_int_vector &keys);
		
		bool empty() const { return 0 == size(); }
		size_type size() const { return m_keys.size(); }
		const_iterator find(key_type const &key) const;
		const_iterator lower_bound(key_type const &key) const;
		
		const_iterator cbegin() const			{ return const_iterator(*this, m_leftmost_idx); }
		const_iterator cend() const				{ return const_iterator(*this, m_past_end_idx); }
		const_reverse_iterator crbegin() const	{ return const_reverse_iterator(cend()); }
		const_reverse_iterator crend() const	{ return const_reverse_iterator(cbegin()); }
		const_iterator begin() const			{ return cbegin(); }
		const_iterator end() const				{ return cend(); }
		const_reverse_iterator rbegin() const	{ return crbegin(); }
		const_reverse_iterator rend() const		{ return crend(); }
		
		template <typename t_other_key>
		bool operator==(static_binary_tree <t_other_key> const &other) const { return std::equal(cbegin(), cend(), other.cbegin(), other.cend()); }

		size_type serialize(std::ostream& out, sdsl::structure_tree_node *v = nullptr, std::string name = "") const;
		void load(std::istream& in);
		
		void print() const;
	};
	
	
	template <typename t_key, typename t_mapped, bool t_enable_serialize>
	template <typename t_int_vector>
	void static_binary_tree <t_key, t_mapped, t_enable_serialize>::fill_used_indices(
		t_int_vector const &keys, size_type const target, size_type const lb, size_type const rb
	)
	{
		size_type const mid(lb + (rb - lb) / 2);
		m_used_indices[target] = 1;
		
		if (lb < mid)
			fill_used_indices(keys, left_child(target), lb, mid - 1);
		
		if (mid < rb)
			fill_used_indices(keys, right_child(target), 1 + mid, rb);
	}
	
	
	template <typename t_key, typename t_mapped, bool t_enable_serialize>
	template <typename t_int_vector>
	void static_binary_tree <t_key, t_mapped, t_enable_serialize>::fill_keys(
		t_int_vector const &keys, size_type const target, size_type const lb, size_type const rb
	)
	{
		size_type const mid(lb + (rb - lb) / 2);
		size_type pos(m_used_indices_r1_support.rank(target));
		m_keys[pos] = keys[mid];
		
		if (lb < mid)
			fill_keys(keys, left_child(target), lb, mid - 1);
		
		if (mid < rb)
			fill_keys(keys, right_child(target), 1 + mid, rb);
	}
	
	
	template <typename t_key, typename t_mapped, bool t_enable_serialize>
	template <typename t_int_vector>
	static_binary_tree <t_key, t_mapped, t_enable_serialize>::static_binary_tree(
		t_int_vector &keys
	)
	{
		auto const size(keys.size());
		if (size)
		{
			{
				key_vector_type vec(size, 0);
				m_keys = std::move(vec);
			}
			
			{
				size_type complete_size(sdsl::util::upper_power_of_2(1 + size) - 1);
				sdsl::bit_vector vec(complete_size, 0);
				m_used_indices = std::move(vec);
			}
			
			std::sort(keys.begin(), keys.end());
			fill_used_indices(keys, 0, 0, size - 1);
			
			sdsl::bit_vector::rank_1_type used_indices_r1_support(&m_used_indices);
			m_used_indices_r1_support = std::move(used_indices_r1_support);
			fill_keys(keys, 0, 0, size - 1);
			
			// For iterators.
			find_leftmost(m_leftmost_idx);
			
			while (true)
			{
				if (!right_child_c(m_past_end_idx))
					break;
			}
			
			// The next node would be the right child of the current one.
			m_past_end_idx = right_child(m_past_end_idx);
		}
	}
	
	
	template <typename t_key, typename t_mapped, bool t_enable_serialize>
	bool static_binary_tree <t_key, t_mapped, t_enable_serialize>::left_child_c(
		size_type &i /* inout */
	) const
	{
		size_type j(left_child(i));
		if (j < m_used_indices.size() && m_used_indices[j])
		{
			i = j;
			return true;
		}
		return false;
	}
	
	
	template <typename t_key, typename t_mapped, bool t_enable_serialize>
	bool static_binary_tree <t_key, t_mapped, t_enable_serialize>::right_child_c(
		size_type &i /* inout */
	) const
	{
		size_type j(right_child(i));
		if (j < m_used_indices.size() && m_used_indices[j])
		{
			i = j;
			return true;
		}
		return false;
	}
	
	
	template <typename t_key, typename t_mapped, bool t_enable_serialize>
	bool static_binary_tree <t_key, t_mapped, t_enable_serialize>::parent_c(
		size_type &i /* inout */
	) const
	{
		if (i)
		{
			i = parent(i);
			return true;
		}
		return false;
	}
	
	
	template <typename t_key, typename t_mapped, bool t_enable_serialize>
	bool static_binary_tree <t_key, t_mapped, t_enable_serialize>::find_leftmost(
		size_type &i /* inout */
	) const
	{
		bool found(false);
		while (true)
		{
			if (!left_child_c(i))
				break;
			
			found = true;
		}
		return found;
	}
	
	
	template <typename t_key, typename t_mapped, bool t_enable_serialize>
	auto static_binary_tree <t_key, t_mapped, t_enable_serialize>::find(
		key_type const &key
	) const -> const_iterator
	{
		if (empty())
			return cend();
		
		size_type idx(0);
		while (true)
		{
			size_type key_idx(m_used_indices_r1_support.rank(idx));
			if (key < m_keys[key_idx])
			{
				if (!left_child_c(idx))
					return cend();
			}
			else if (m_keys[key_idx] < key)
			{
				if (!right_child_c(idx))
					return cend();
			}
			else
			{
				return const_iterator(*this, idx);
			}
		}
	}
	
	
	template <typename t_key, typename t_mapped, bool t_enable_serialize>
	auto static_binary_tree <t_key, t_mapped, t_enable_serialize>::lower_bound(
		key_type const &key
	) const -> const_iterator
	{
		if (empty())
			return cend();
		
		const_iterator retval(cend());
		lower_bound_subtree(key, 0, retval);
		return retval;
	}
	
	
	template <typename t_key, typename t_mapped, bool t_enable_serialize>
	bool static_binary_tree <t_key, t_mapped, t_enable_serialize>::lower_bound_subtree(
		key_type const &key, size_type idx, const_iterator &it /* out */
	) const
	{
		if (!m_used_indices[idx])
			return false;
		
		size_type key_idx(m_used_indices_r1_support.rank(idx));
		if (key < m_keys[key_idx])
		{
			// The found value is not less than key. Continue by trying to find a lesser value.
			if (! (left_child_c(idx) && lower_bound_subtree(key, idx, it)))
				it = const_iterator(*this, idx);
			return true;
		}
		else
		{
			// The found value is less than key. Try to find a greater value.
			if (right_child_c(idx))
				return lower_bound_subtree(key, idx, it);
			
			return false;
		}
	}
	
	
	template <typename t_key, typename t_mapped, bool t_enable_serialize>
	auto static_binary_tree <t_key, t_mapped, t_enable_serialize>::serialize(
		std::ostream &out, sdsl::structure_tree_node *v, std::string name
	) const -> size_type
	{
		auto *child(sdsl::structure_tree::add_child(v, name, sdsl::util::class_name(*this)));
		std::size_t written_bytes(0);
		
		written_bytes += m_keys.serialize(out, child, "keys");
		written_bytes += m_used_indices.serialize(out, child, "used_indices");
		written_bytes += m_used_indices_r1_support.serialize(out, child, "used_indices_r1_support");
		written_bytes += write_member(m_leftmost_idx, out, child, "leftmost_idx");
		written_bytes += write_member(m_past_end_idx, out, child, "past_end_idx");
		
		sdsl::structure_tree::add_size(child, written_bytes);
		return written_bytes;
	}
	
	
	template <typename t_key, typename t_mapped, bool t_enable_serialize>
	void static_binary_tree <t_key, t_mapped, t_enable_serialize>::load(
		std::istream& in
	)
	{
		m_keys.load(in);
		m_used_indices.load(in);
		m_used_indices_r1_support.load(in);
		sdsl::read_member(m_leftmost_idx, in);
		sdsl::read_member(m_past_end_idx, in);
		
		m_used_indices_r1_support.set_vector(&this->m_used_indices);
	}
	
	
	template <typename t_key, typename t_mapped, bool t_enable_serialize>
	void static_binary_tree <t_key, t_mapped, t_enable_serialize>::print() const
	{
		size_type const count(m_used_indices.size());
		
		std::cerr << "Indices:";
		for (size_type i(0); i < count; ++i)
			std::cerr << " " << std::hex << std::setw(2) << std::setfill('0') << i;
		std::cerr << std::endl;
		
		std::cerr << "Used:   ";
		for (size_type i(0); i < count; ++i)
			std::cerr << " " << std::hex << std::setw(2) << std::setfill('0') << +(m_used_indices[i]);
		std::cerr << std::endl;
		
		std::cerr << "Keys: ";
		auto it(m_keys.cbegin());
		for (size_type i(0); i < count; ++i)
		{
			if (m_used_indices[i])
			{
				std::cerr << " " << std::hex << std::setw(2) << std::setfill('0') << +(*it);
				++it;
			}
			else
			{
				std::cerr << "   ";
			}
		}
		std::cerr << std::endl;

	}
}


namespace asm_lsw { namespace detail {

	template <typename t_tree>
	void static_binary_tree_iterator_tpl <t_tree>::increment()
	{
		size_type next_idx(m_idx);
		if (m_tree->right_child_c(next_idx))
		{
			m_tree->find_leftmost(next_idx);
			m_idx = next_idx;
			return;
		}
		
		size_type cur(m_idx);
		while (m_tree->parent_c(next_idx))
		{
			// If we came from the left branch, stop.
			// Otherwise, continue.
			if (m_tree->left_child(next_idx) == cur)
			{
				m_idx = next_idx;
				return;
			}
			
			cur = next_idx;
		}
		
		// Reached when coming from the right branch to the root.
		m_idx = m_tree->m_past_end_idx;
	}
	
	
	template <typename t_tree>
	void static_binary_tree_iterator_tpl <t_tree>::decrement()
	{
		size_type prev_idx(m_idx);
		
		// If the current node has a left child, it must have been
		// the previous one, unless that node has a right child.
		if (m_tree->left_child_c(prev_idx))
		{
			m_tree->right_child_c(prev_idx);
			m_idx = prev_idx;
			return;
		}
		
		// Otherwise, go to parent and skip if the node was the right child.
		size_type cur(m_idx);
		while (m_tree->parent_c(prev_idx))
		{
			if (m_tree->left_child(prev_idx) == cur)
			{
				m_idx = prev_idx;
				return;
			}
			
			cur = prev_idx;
		}
	}
	
	
	template <typename t_tree>
	bool static_binary_tree_iterator_tpl <t_tree>::equal(
		static_binary_tree_iterator_tpl <t_tree> const &other
	) const
	{
		return (this->m_tree == other.m_tree &&
				this->m_idx == other.m_idx);
	}
	
	
	template <typename t_tree>
	auto static_binary_tree_iterator_tpl <t_tree>::dereference() const -> typename t_tree::key_type
	{
		assert(m_idx < m_tree->m_used_indices.size());
		size_type const key_idx(m_tree->m_used_indices_r1_support.rank(m_idx));
		auto const retval(m_tree->m_keys[key_idx]);
		return retval;
	}
}}

#endif
