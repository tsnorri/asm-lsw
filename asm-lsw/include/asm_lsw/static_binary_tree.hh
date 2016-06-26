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

#include <asm_lsw/static_binary_tree_helper.hh>
#include <asm_lsw/util.hh>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/iterator/reverse_iterator.hpp>
#include <sdsl/int_vector.hpp>
#include <sdsl/rank_support.hpp>


namespace asm_lsw { namespace detail {
	
	template <typename t_tree, typename t_it_val, typename t_it_ref>
	class static_binary_tree_iterator_tpl : public boost::iterator_facade <
		static_binary_tree_iterator_tpl <t_tree, t_it_val, t_it_ref>,
		t_it_val,
		boost::bidirectional_traversal_tag,
		t_it_ref
	>
	{
		friend class boost::iterator_core_access;

	protected:
		typedef boost::iterator_facade <
			static_binary_tree_iterator_tpl <t_tree, t_it_val, t_it_ref>,
			t_it_val,
			boost::bidirectional_traversal_tag,
			t_it_ref
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
		
		bool equal(static_binary_tree_iterator_tpl <t_tree, t_it_val, t_it_ref> const &other) const ASM_LSW_PURE;
		
		t_it_ref dereference() const;
	};
}}


namespace asm_lsw
{
	template <typename t_key, typename t_mapped = void, bool t_enable_serialize = false>
	class static_binary_tree
	{
		template <typename, typename, typename> friend class detail::static_binary_tree_iterator_tpl;
		template <typename, typename, bool> friend class detail::static_binary_tree_helper;
		
		
	public:
		typedef detail::static_binary_tree_helper <t_key, t_mapped, t_enable_serialize>	helper_type;
		typedef typename helper_type::key_type											key_type;
		typedef typename helper_type::mapped_type										mapped_type;
		typedef typename helper_type::value_type										value_type;
		typedef typename helper_type::size_type											size_type;
		
		typedef detail::static_binary_tree_iterator_tpl <
			static_binary_tree,
			typename helper_type::iterator_val const,
			typename helper_type::iterator_ref const
		>																				const_iterator;
		typedef boost::reverse_iterator <const_iterator>								const_reverse_iterator;
		typedef const_iterator															iterator;
		typedef const_reverse_iterator													reverse_iterator;
		
		enum { is_map_type = helper_type::is_map_type };
		
	protected:
		helper_type						m_helper;
		sdsl::bit_vector				m_used_indices;
		sdsl::bit_vector::rank_1_type	m_used_indices_r1_support;
		size_type						m_leftmost_idx{0};
		size_type						m_past_end_idx{0};
		
	protected:
		template <typename t_collection>
		static std::vector <value_type> move_to_vector(t_collection &collection);
		
		// Ahnentafel system.
		size_type left_child(size_type const i) const ASM_LSW_CONST { return 2 * i + 1; }
		size_type right_child(size_type const i) const ASM_LSW_CONST { return 2 * i + 2; }
		size_type parent(size_type const i) const ASM_LSW_CONST { return (i - 1) / 2; }
		
		bool left_child_c(size_type &i /* inout */) const;
		bool right_child_c(size_type &i /* inout */) const;
		bool parent_c(size_type &i /* inout */) const ASM_LSW_CONST;
		
		bool find_leftmost(size_type &i /* inout */) const;
		
		void fill_used_indices(size_type const target, size_type const begin, size_type const end);

		template <typename t_vector>
		void fill_tree(t_vector const &input_vec, size_type const target, size_type const begin, size_type const end);
		
		bool lower_bound_subtree(key_type const &key, size_type idx, const_iterator &it /* out */) const;
		
		size_type serialize_common(std::ostream &out, sdsl::structure_tree_node *child) const;
		void load_common(std::istream &in);
		
	public:
		static_binary_tree() {}
		
		// Check that the passed collection satisfies SequenceContainer (for operator[]).
		template <
			typename t_vector,
			typename std::enable_if <util::is_sequence_container <t_vector>::value>::type * = nullptr
		>
		explicit static_binary_tree(t_vector &input_vec);
		
		template <
			typename t_vector,
			typename std::enable_if <util::is_sequence_container <t_vector>::value>::type * = nullptr
		>
		explicit static_binary_tree(t_vector &&input_vec):
			static_binary_tree(input_vec) // Convert to lvalue, constructor moves contents anyway.
		{
		}
		
		// Not SequenceContainer, move the contents to a vector first.
		template <
			typename t_collection,
			typename std::enable_if <!util::is_sequence_container <t_collection>::value>::type * = nullptr
		>
		explicit static_binary_tree(t_collection &collection):
			static_binary_tree(move_to_vector(collection))
		{
		}
		
		template <
			typename t_collection,
			typename std::enable_if <!util::is_sequence_container <t_collection>::value>::type * = nullptr
		>
		explicit static_binary_tree(t_collection &&collection):
			static_binary_tree(collection) // Convert to lvalue.
		{
		}
		
		static_binary_tree(static_binary_tree const &);
		static_binary_tree(static_binary_tree &&);
		static_binary_tree &operator=(static_binary_tree const &) &;
		static_binary_tree &operator=(static_binary_tree &&) &;
		
		bool empty() const { return 0 == size(); }
		size_type size() const { return m_helper.size(); }
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
		
		template <typename t_other_key, typename t_other_value, bool t_other_enable_serialize>
		bool operator==(
			static_binary_tree <t_other_key, t_other_value, t_other_enable_serialize> const &other
		) const
		{
			return std::equal(cbegin(), cend(), other.cbegin(), other.cend());
		}

		template <typename Fn, bool t_dummy = true>
		auto serialize_keys(
			std::ostream &out,
			Fn value_callback,
			sdsl::structure_tree_node *v = nullptr,
			std::string name = ""
		) const -> typename std::enable_if <is_map_type && t_dummy, size_type>::type;
	
		template <bool t_dummy = true>
		auto serialize(
			std::ostream &out,
			sdsl::structure_tree_node *v = nullptr,
			std::string name = ""
		) const -> typename std::enable_if <t_enable_serialize && t_dummy, size_type>::type;
	
		template <typename Fn, bool t_dummy = true>
		auto load_keys(
			std::istream &in,
			Fn value_callback
		) -> typename std::enable_if <is_map_type && t_dummy>::type;
	
		template <bool t_dummy = true>
		auto load(
			std::istream &in
		) -> typename std::enable_if <t_enable_serialize && t_dummy>::type;
		
		void print() const;
	};
	
	
	// FIXME: most of these could be made automatic by creating a base class without the r1 support.
	template <typename t_key, typename t_mapped, bool t_enable_serialize>
	static_binary_tree <t_key, t_mapped, t_enable_serialize>::static_binary_tree (
		static_binary_tree const &other
	):
		m_helper(other.m_helper),
		m_used_indices(other.m_used_indices),
		m_used_indices_r1_support(other.m_used_indices_r1_support),
		m_leftmost_idx(other.m_leftmost_idx),
		m_past_end_idx(other.m_past_end_idx)
	{
		m_used_indices_r1_support.set_vector(&this->m_used_indices);
	}
	
	
	template <typename t_key, typename t_mapped, bool t_enable_serialize>
	static_binary_tree <t_key, t_mapped, t_enable_serialize>::static_binary_tree (
		static_binary_tree &&other
	):
		m_helper(std::move(other.m_helper)),
		m_used_indices(std::move(other.m_used_indices)),
		m_used_indices_r1_support(std::move(other.m_used_indices_r1_support)),
		m_leftmost_idx(std::move(other.m_leftmost_idx)),
		m_past_end_idx(std::move(other.m_past_end_idx))
	{
		m_used_indices_r1_support.set_vector(&this->m_used_indices);
	}
	
	
	template <typename t_key, typename t_mapped, bool t_enable_serialize>
	auto static_binary_tree <t_key, t_mapped, t_enable_serialize>::operator=(
		static_binary_tree const &other
	) & -> static_binary_tree &
	{
		if (&other != this)
		{
			static_binary_tree tmp(other); // Copy
			*this = std::move(tmp);
		}
		return *this;
	}
	
	
	template <typename t_key, typename t_mapped, bool t_enable_serialize>
	auto static_binary_tree <t_key, t_mapped, t_enable_serialize>::operator=(
		static_binary_tree &&other
	) & -> static_binary_tree &
	{
		if (&other != this)
		{
			m_helper = std::move(other.m_helper);
			m_used_indices = std::move(other.m_used_indices);
			m_used_indices_r1_support = std::move(other.m_used_indices_r1_support);
			m_leftmost_idx = std::move(other.m_leftmost_idx);
			m_past_end_idx = std::move(other.m_past_end_idx);
				
			m_used_indices_r1_support.set_vector(&this->m_used_indices);
		}
		return *this;
	}
	
	
	template <typename t_key, typename t_mapped, bool t_enable_serialize>
	template <typename t_collection>
	auto static_binary_tree <t_key, t_mapped, t_enable_serialize>::move_to_vector(
		t_collection &collection
	) -> std::vector <value_type>
	{
		std::vector <value_type> retval;
		retval.reserve(collection.size());
		
		for (auto &kv : collection)
			retval.emplace_back(std::move(kv));
		
		return retval;
	}
	
	
	template <typename t_key, typename t_mapped, bool t_enable_serialize>
	void static_binary_tree <t_key, t_mapped, t_enable_serialize>::fill_used_indices(
		size_type const target, size_type const lb, size_type const rb
	)
	{
		size_type const mid(lb + (rb - lb) / 2);
		m_used_indices[target] = 1;
		
		if (lb < mid)
			fill_used_indices(left_child(target), lb, mid - 1);
		
		if (mid < rb)
			fill_used_indices(right_child(target), 1 + mid, rb);
	}
	
	
	template <typename t_key, typename t_mapped, bool t_enable_serialize>
	template <typename t_vector>
	void static_binary_tree <t_key, t_mapped, t_enable_serialize>::fill_tree(
		t_vector const &input_vec, size_type const target, size_type const lb, size_type const rb
	)
	{
		size_type const mid(lb + (rb - lb) / 2);
		size_type pos(m_used_indices_r1_support.rank(target));
		m_helper.value(pos) = std::move(input_vec[mid]);
		
		if (lb < mid)
			fill_tree(input_vec, left_child(target), lb, mid - 1);
		
		if (mid < rb)
			fill_tree(input_vec, right_child(target), 1 + mid, rb);
	}
	
	
	template <typename t_key, typename t_mapped, bool t_enable_serialize>
	template <
		typename t_vector,
		typename std::enable_if <util::is_sequence_container <t_vector>::value>::type *
	>
	static_binary_tree <t_key, t_mapped, t_enable_serialize>::static_binary_tree(
		t_vector &input_vec
	)
	{
		auto const size(input_vec.size());
		if (size)
		{
			{
				helper_type h(size);
				m_helper = std::move(h);
			}
			
			{
				size_type complete_size(sdsl::util::upper_power_of_2(1 + size) - 1);
				sdsl::bit_vector vec(complete_size, 0);
				m_used_indices = std::move(vec);
			}
			
			fill_used_indices(0, 0, size - 1);
			sdsl::bit_vector::rank_1_type used_indices_r1_support(&m_used_indices);
			m_used_indices_r1_support = std::move(used_indices_r1_support);
			
			m_helper.sort(input_vec);
			fill_tree(input_vec, 0, 0, size - 1);
			
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
			auto const found_key(m_helper.key(key_idx));
			if (key < found_key)
			{
				if (!left_child_c(idx))
					return cend();
			}
			else if (found_key < key)
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
		auto const found_key(m_helper.key(key_idx));
		if (found_key < key)
		{
			// The found value is less than key. Try to find a greater value.
			if (right_child_c(idx))
				return lower_bound_subtree(key, idx, it);
			
			return false;
		}
		else
		{
			auto next_idx(idx);
			// The found value is not less than key. Continue by trying to find a lesser value.
			if (! (left_child_c(next_idx) && lower_bound_subtree(key, next_idx, it)))
				it = const_iterator(*this, idx);
			return true;
		}
	}
	
	
	template <typename t_key, typename t_mapped, bool t_enable_serialize>
	auto static_binary_tree <t_key, t_mapped, t_enable_serialize>::serialize_common(
		std::ostream &out,
		sdsl::structure_tree_node *child
	) const -> size_type
	{
		size_type written_bytes(0);
		written_bytes += m_used_indices.serialize(out, child, "used_indices");
		written_bytes += m_used_indices_r1_support.serialize(out, child, "used_indices_r1_support");
		written_bytes += write_member(m_leftmost_idx, out, child, "leftmost_idx");
		written_bytes += write_member(m_past_end_idx, out, child, "past_end_idx");
		return written_bytes;
	}
	
	
	template <typename t_key, typename t_mapped, bool t_enable_serialize>
	template <typename Fn, bool t_dummy>
	auto static_binary_tree <t_key, t_mapped, t_enable_serialize>::serialize_keys(
		std::ostream &out,
		Fn value_callback,
		sdsl::structure_tree_node *v,
		std::string name
	) const -> typename std::enable_if <is_map_type && t_dummy, size_type>::type
	{
		auto *child(sdsl::structure_tree::add_child(v, name, sdsl::util::class_name(*this)));
		std::size_t written_bytes(0);
		
		written_bytes += m_helper.serialize_keys(out, value_callback, child, "helper");
		written_bytes += serialize_common(out, child);
		
		sdsl::structure_tree::add_size(child, written_bytes);
		return written_bytes;
	}
	
	
	template <typename t_key, typename t_mapped, bool t_enable_serialize>
	template <bool t_dummy>
	auto static_binary_tree <t_key, t_mapped, t_enable_serialize>::serialize(
		std::ostream &out, sdsl::structure_tree_node *v, std::string name
	) const -> typename std::enable_if <t_enable_serialize && t_dummy, size_type>::type
	{
		auto *child(sdsl::structure_tree::add_child(v, name, sdsl::util::class_name(*this)));
		std::size_t written_bytes(0);
		
		written_bytes += m_helper.serialize(out, child, "helper");
		written_bytes += serialize_common(out, child);
		
		sdsl::structure_tree::add_size(child, written_bytes);
		return written_bytes;
	}
	
	
	template <typename t_key, typename t_mapped, bool t_enable_serialize>
	void static_binary_tree <t_key, t_mapped, t_enable_serialize>::load_common(std::istream &in)
	{
		m_used_indices.load(in);
		m_used_indices_r1_support.load(in);
		sdsl::read_member(m_leftmost_idx, in);
		sdsl::read_member(m_past_end_idx, in);
		
		m_used_indices_r1_support.set_vector(&this->m_used_indices);
	}
	
	
	template <typename t_key, typename t_mapped, bool t_enable_serialize>
	template <typename Fn, bool t_dummy>
	auto static_binary_tree <t_key, t_mapped, t_enable_serialize>::load_keys(
		std::istream &in,
		Fn value_callback
	) -> typename std::enable_if <is_map_type && t_dummy>::type
	{
		m_helper.load_keys(in, value_callback);
		load_common(in);
	}
	
	
	template <typename t_key, typename t_mapped, bool t_enable_serialize>
	template <bool t_dummy>
	auto static_binary_tree <t_key, t_mapped, t_enable_serialize>::load(
		std::istream& in
	) -> typename std::enable_if <t_enable_serialize && t_dummy>::type
	{
		m_helper.load(in);
		load_common(in);
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
		
		m_helper.print(*this);
	}
}


namespace asm_lsw { namespace detail {

	template <typename t_tree, typename t_it_val, typename t_it_ref>
	void static_binary_tree_iterator_tpl <t_tree, t_it_val, t_it_ref>::increment()
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
	
	
	template <typename t_tree, typename t_it_val, typename t_it_ref>
	void static_binary_tree_iterator_tpl <t_tree, t_it_val, t_it_ref>::decrement()
	{
		size_type prev_idx(m_idx);
		
		// If the current node has a left child, it must have been
		// the previous one, unless that node has a right child.
		if (m_tree->left_child_c(prev_idx))
		{
			while (true)
			{
				if (!m_tree->right_child_c(prev_idx))
					break;
			}
			
			m_idx = prev_idx;
			return;
		}
		
		// Otherwise, go to parent and skip if the node was the left child.
		size_type cur(m_idx);
		while (m_tree->parent_c(prev_idx))
		{
			if (m_tree->right_child(prev_idx) == cur)
			{
				m_idx = prev_idx;
				return;
			}
			
			cur = prev_idx;
		}
	}
	
	
	template <typename t_tree, typename t_it_val, typename t_it_ref>
	bool static_binary_tree_iterator_tpl <t_tree, t_it_val, t_it_ref>::equal(
		static_binary_tree_iterator_tpl <t_tree, t_it_val, t_it_ref> const &other
	) const
	{
		return (this->m_tree == other.m_tree &&
				this->m_idx == other.m_idx);
	}
	
	
	template <typename t_tree, typename t_it_val, typename t_it_ref>
	auto static_binary_tree_iterator_tpl <t_tree, t_it_val, t_it_ref>::dereference() const -> t_it_ref
	{
		assert(m_idx < m_tree->m_used_indices.size());
		size_type const key_idx(m_tree->m_used_indices_r1_support.rank(m_idx));
		return m_tree->m_helper.dereference(key_idx);
	}
}}

#endif
