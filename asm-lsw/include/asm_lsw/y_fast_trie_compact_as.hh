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


#ifndef ASM_LSW_Y_FAST_TRIE_COMPACT_AS_HH
#define ASM_LSW_Y_FAST_TRIE_COMPACT_AS_HH

#include <asm_lsw/util.hh>
#include <asm_lsw/y_fast_trie_compact.hh>


namespace asm_lsw {

	// FIXME: move to namespace detail.
	template <typename t_key, typename t_value>
	struct y_fast_trie_compact_as_subtree_it_val { typedef std::pair <t_key, t_value const &> type; };
	
	template <typename t_key>
	struct y_fast_trie_compact_as_subtree_it_val <t_key, void> { typedef t_key type; };
	
	template <typename t_it_val, typename t_max_key, typename t_value>
	class y_fast_trie_compact_as_subtree_iterator_tpl : public boost::iterator_facade <
		y_fast_trie_compact_as_subtree_iterator_tpl <t_it_val, t_max_key, t_value>,
		t_it_val,
		boost::random_access_traversal_tag,
		t_it_val // Not really a reference, gets constructed on-demand.
	>
	{
		friend class boost::iterator_core_access;

	protected:
		typedef boost::iterator_facade <
			y_fast_trie_compact_as_subtree_iterator_tpl <t_it_val, t_max_key, t_value>,
			t_it_val,
			boost::random_access_traversal_tag,
			t_it_val
		> base_class;
			
		template <typename t_tpl_value, bool t_dummy = false>
		struct dereference_fn
		{
			t_it_val operator()(t_it_val const &it_val, t_max_key const offset)
			{
				t_it_val retval(offset + it_val.first, it_val.second);
				return retval;
			}
		};
		
		template <bool t_dummy>
		struct dereference_fn <void, t_dummy>
		{
			t_it_val operator()(t_it_val const &it_val, t_max_key const offset) { return it_val + offset; }
		};
			
		typedef detail::y_fast_trie_base_subtree_iterator_wrapper <t_it_val> iterator_type;

	public:
		typedef typename std::size_t size_type;
		typedef typename base_class::difference_type difference_type;

	protected:
		std::unique_ptr <iterator_type> m_it;
		t_max_key m_offset{0};

	private:
		// For the converting constructor below.
		struct enabler {};
		
	public:
		y_fast_trie_compact_as_subtree_iterator_tpl() = default;
		
		y_fast_trie_compact_as_subtree_iterator_tpl(iterator_type &it, t_max_key offset):
			m_it(&it),
			m_offset(offset)
		{
		}
		
		// Remove the conversion from iterator to const_iterator as per
		// http://www.boost.org/doc/libs/1_58_0/libs/iterator/doc/iterator_facade.html#telling-the-truth
		template <typename t_other_val>
		y_fast_trie_compact_as_subtree_iterator_tpl(
			y_fast_trie_compact_as_subtree_iterator_tpl <t_other_val, t_max_key, t_value> &&other, typename boost::enable_if<
				boost::is_convertible<t_other_val *, t_it_val *>, enabler
			>::type = enabler()
		):
			m_it(std::move(other.m_it)),
			m_offset(other.m_offset)
		{
		}

	private:
		void increment() { advance(1); }
		void decrement() { advance(-1); }
		
		void advance(difference_type n) { m_it->advance(n); }
		
		template <typename t_other_val>
		difference_type distance_to(
			y_fast_trie_compact_as_subtree_iterator_tpl <t_other_val, t_max_key, t_value> const &other
		) const
		{
			return m_it->distance_to(*other.m_it);
		}
		
		template <typename t_other_val>
		bool equal(
			y_fast_trie_compact_as_subtree_iterator_tpl <t_other_val, t_max_key, t_value> const &other
		) const
		{
			return m_it->equal(*other.m_it);
		}
		
		t_it_val dereference() const
		{
			dereference_fn <t_value> dfn;
			t_it_val it_val(m_it->dereference());
			t_it_val retval(dfn(it_val, m_offset));
			return retval;
		}
	};
	
	
	template <typename t_max_key, typename t_value = void>
	class y_fast_trie_compact_as
	{
	public:
		typedef t_max_key key_type;
		typedef t_value value_type;
		typedef std::size_t size_type;
		typedef detail::y_fast_trie_trait<key_type, value_type> trait;
		
		typedef y_fast_trie_compact_as_subtree_iterator_tpl <
			typename y_fast_trie_compact_as_subtree_it_val <key_type, value_type>::type const,
			key_type,
			value_type
		> const_subtree_iterator;
		
		typedef const_subtree_iterator const_iterator;
		
	protected:
		key_type m_offset{0};
	
	protected:
		template <typename t_dst, typename t_src, typename T = typename t_dst::mapped_type>
		struct fill_trie
		{
			static_assert(std::is_same <
				typename util::remove_ref_t <t_src>::mapped_type,
				typename util::remove_ref_t <t_dst>::mapped_type
			>::value, "");

			void operator()(t_dst &dst, t_src const &src, key_type const offset) const
			{
				for (auto const &kv : src)
				{
					assert(kv.first - offset < std::numeric_limits <typename t_dst::key_type>::max());
					typename t_dst::key_type key(kv.first - offset);
					typename t_dst::value_type value(kv.second);
					dst.insert(key, value);
				}
			}
		};

		template <typename t_dst, typename t_src>
		struct fill_trie <t_dst, t_src, void>
		{
			void operator()(t_dst &dst, t_src const &src, key_type const offset) const
			{
				for (auto const &k : src)
				{
					assert(k - offset < std::numeric_limits <typename t_dst::key_type>::max());
					typename t_dst::key_type key(k - offset);
					dst.insert(key);
				}
			}
		};
		
		template <typename t_ret_key, typename t_key>
		static y_fast_trie_compact_as *construct_specific(y_fast_trie <t_key, t_value> &, key_type const);
		
		y_fast_trie_compact_as(key_type const offset):
			m_offset(offset)
		{
		}
		
	public:
		template <typename t_key>
		static y_fast_trie_compact_as *construct(y_fast_trie <t_key, t_value> &);
		
		key_type offset() const { return m_offset; }

		virtual size_type key_size() const = 0;
		virtual size_type size() const = 0;

		virtual bool contains(key_type const key) const = 0;
		virtual key_type min_key() const = 0;
		virtual key_type max_key() const = 0;
		
		virtual bool find(key_type const key, const_subtree_iterator &iterator) const = 0;
		virtual bool find_predecessor(key_type const key, const_subtree_iterator &iterator, bool allow_equal = false) const = 0;
		virtual bool find_successor(key_type const key, const_subtree_iterator &iterator, bool allow_equal = false) const = 0;

		virtual void print() const = 0;

		typename trait::key_type const iterator_key(const_subtree_iterator const &it) const { trait t; return t.key(it); }
		typename trait::value_type const &iterator_value(const_subtree_iterator const &it) const { trait t; return t.value(it); }
	};
	
	
	template <typename t_max_key, typename t_key, typename t_value>
	class y_fast_trie_compact_as_tpl : public y_fast_trie_compact_as <t_max_key, t_value>
	{
	protected:
		typedef y_fast_trie_compact_as <t_max_key, t_value> base_class;
		typedef y_fast_trie_compact <t_key, t_value> trie_type;
		
		typedef detail::y_fast_trie_base_subtree_iterator_wrapper <
			typename y_fast_trie_compact_as_subtree_it_val <t_max_key, t_value>::type const
		> subtree_iterator_wrapper;
		
		typedef typename base_class::size_type size_type;
		typedef typename base_class::key_type key_type;
		typedef typename base_class::const_subtree_iterator const_subtree_iterator;
		
	protected:
		trie_type m_trie;
		
	public:
		y_fast_trie_compact_as_tpl(trie_type &trie, t_max_key offset):
			base_class(offset),
			m_trie(std::move(trie))
		{
		}
		
		virtual size_type key_size() const override					{ return sizeof(t_key); }
		virtual size_type size() const override						{ return m_trie.size(); }
		
		virtual bool contains(key_type const key) const override	{ return m_trie.contains(key - this->m_offset); }
		virtual key_type min_key() const override					{ return this->m_offset + m_trie.min_key(); }
		virtual key_type max_key() const override					{ return this->m_offset + m_trie.max_key(); }

		virtual void print() const override							{ m_trie.print(); }

		virtual bool find(key_type const key, const_subtree_iterator &iterator) const override;
		virtual bool find_predecessor(key_type const key, const_subtree_iterator &iterator, bool allow_equal = false) const override;
		virtual bool find_successor(key_type const key, const_subtree_iterator &iterator, bool allow_equal = false) const override;
		
	protected:
		bool check_find_result(bool const, typename trie_type::const_subtree_iterator, const_subtree_iterator &) const;
	};
	
	
	template <typename t_max_key, typename t_key, typename t_value>
	bool y_fast_trie_compact_as_tpl <t_max_key, t_key, t_value>::check_find_result(
		bool const res,
		typename trie_type::const_subtree_iterator it,
		const_subtree_iterator &out_it
	) const
	{
		if (res)
		{
			subtree_iterator_wrapper *wrapper(subtree_iterator_wrapper::construct(it));
			out_it = const_subtree_iterator(*wrapper, this->m_offset);
			return true;
		}
		return false;
	}
	
	
	template <typename t_max_key, typename t_key, typename t_value>
	bool y_fast_trie_compact_as_tpl <t_max_key, t_key, t_value>::find(
		key_type const key, const_subtree_iterator &out_it
	) const
	{
		if (key < this->m_offset)
			return false;
		else if (std::numeric_limits <typename trie_type::key_type>::max() < key - this->m_offset)
			return false;
		
		typename trie_type::const_subtree_iterator it;
		return check_find_result(m_trie.find(key - this->m_offset, it), it, out_it);
	}
	
	
	template <typename t_max_key, typename t_key, typename t_value>
	bool y_fast_trie_compact_as_tpl <t_max_key, t_key, t_value>::find_predecessor(
		key_type const key, const_subtree_iterator &out_it, bool allow_equal
	) const
	{
		if (key < this->m_offset)
			return false;
		else if (std::numeric_limits <typename trie_type::key_type>::max() < key - this->m_offset)
			return find(max_key(), out_it);

		typename trie_type::const_subtree_iterator pred;
		return check_find_result(m_trie.find_predecessor(key - this->m_offset, pred, allow_equal), pred, out_it);
	}


	template <typename t_max_key, typename t_key, typename t_value>
	bool y_fast_trie_compact_as_tpl <t_max_key, t_key, t_value>::find_successor(
		key_type const key, const_subtree_iterator &out_it, bool allow_equal
	) const
	{
		if (key < this->m_offset)
			return find(min_key(), out_it);
		else if (std::numeric_limits <typename trie_type::key_type>::max() < key - this->m_offset)
			return false;

		typename trie_type::const_subtree_iterator succ;
		return check_find_result(m_trie.find_successor(key - this->m_offset, succ, allow_equal), succ, out_it);
	}
	
	
	template <typename t_max_key, typename t_value>
	template <typename t_ret_key, typename t_key>
	y_fast_trie_compact_as <t_max_key, t_value> *y_fast_trie_compact_as <t_max_key, t_value>::construct_specific(
		y_fast_trie <t_key, t_value> &trie,
		key_type const offset
	)
	{
		// Set the domain limit.
		y_fast_trie <t_ret_key, t_value> temp_trie(trie.max_key() - offset);
		
		fill_trie <decltype(temp_trie), typename util::remove_ref_t <decltype(trie)>::subtree> ft;

		auto const proxy(trie.subtree_map_proxy());
		for (auto const &st : proxy)
			ft(temp_trie, st.second, offset);
		
		y_fast_trie_compact <t_ret_key, t_value> ct(temp_trie);
		return new y_fast_trie_compact_as_tpl <t_max_key, t_ret_key, t_value>(ct, offset);
	}


	template <typename t_max_key, typename t_value>
	template <typename t_key>
	y_fast_trie_compact_as <t_max_key, t_value> *y_fast_trie_compact_as <t_max_key, t_value>::construct(
		y_fast_trie <t_key, t_value> &trie
	)
	{
		if (0 == trie.size())
			return y_fast_trie_compact_as::construct_specific <uint8_t>(trie, 0);
		
		auto const min(trie.min_key());
		auto const max(trie.max_key());
		auto const ret_max(max - min);

		if (ret_max <= std::numeric_limits <uint8_t>::max())
			return y_fast_trie_compact_as::construct_specific <uint8_t>(trie, min);
		else if (ret_max <= std::numeric_limits <uint16_t>::max())
			return y_fast_trie_compact_as::construct_specific <uint16_t>(trie, min);
		else if (ret_max <= std::numeric_limits <uint32_t>::max())
			return y_fast_trie_compact_as::construct_specific <uint32_t>(trie, min);
		else if (ret_max <= std::numeric_limits <uint64_t>::max())
			return y_fast_trie_compact_as::construct_specific <uint64_t>(trie, min);

		assert(0);
	}
}

#endif
