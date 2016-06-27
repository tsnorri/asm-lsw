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


#ifndef ASM_LSW_X_FAST_TRIE_COMPACT_AS_HH
#define ASM_LSW_X_FAST_TRIE_COMPACT_AS_HH

#include <asm_lsw/fast_trie_compact_as_helper.hh>
#include <asm_lsw/x_fast_trie_compact.hh>


namespace asm_lsw {

	// FIXME: move to namespace detail.
	template <typename t_trie, typename t_it_val>
	class x_fast_trie_compact_as_leaf_link_iterator_tpl : public boost::iterator_facade <
		x_fast_trie_compact_as_leaf_link_iterator_tpl <t_trie, t_it_val>,
		t_it_val,
		boost::random_access_traversal_tag,
		t_it_val // Not really a reference, gets constructed on-demand.
	>
	{
		friend class boost::iterator_core_access;
		friend t_trie;

	protected:
		typedef boost::iterator_facade <
			x_fast_trie_compact_as_leaf_link_iterator_tpl <t_trie, t_it_val>,
			t_it_val,
			boost::random_access_traversal_tag,
			t_it_val
		> base_class;
		
	public:
		typedef typename t_trie::size_type size_type;
		typedef typename base_class::difference_type difference_type;

	protected:
		t_trie const *m_trie;
		size_type m_idx{0};
		
	private:
		// For the converting constructor below.
		struct enabler {};
		
	public:
		x_fast_trie_compact_as_leaf_link_iterator_tpl() = default;
		
		x_fast_trie_compact_as_leaf_link_iterator_tpl(t_trie const &trie, size_type idx):
			m_trie(&trie),
			m_idx(idx)
		{
		}
		
		// Remove the conversion from iterator to const_iterator as per
		// http://www.boost.org/doc/libs/1_58_0/libs/iterator/doc/iterator_facade.html#telling-the-truth
		template <typename t_other_val>
		x_fast_trie_compact_as_leaf_link_iterator_tpl(
			x_fast_trie_compact_as_leaf_link_iterator_tpl <t_trie, t_other_val> const &other, typename boost::enable_if<
				boost::is_convertible<t_other_val *, t_it_val *>, enabler
			>::type = enabler()
		):
			m_trie(other.m_trie), m_idx(other.m_idx)
		{
		}
		
	private:
		void increment() { advance(1); }
		void decrement() { advance(-1); }
		
		void advance(difference_type n) { m_idx += n; }
		
		template <typename t_other_val>
		difference_type distance_to(x_fast_trie_compact_as_leaf_link_iterator_tpl <t_trie, t_other_val> const &other) const;
		
		template <typename t_other_val>
		bool equal(x_fast_trie_compact_as_leaf_link_iterator_tpl <t_trie, t_other_val> const &other) const;
		
		t_it_val dereference() const;
	};
	
	
	template <typename t_max_key, typename t_value = void, bool t_enable_serialize = false>
	class x_fast_trie_compact_as
	{
		template <typename, typename>
		friend class x_fast_trie_compact_as_leaf_link_iterator_tpl;

	protected:
		typedef detail::x_fast_trie_trait<t_max_key, t_value> trait;
		typedef detail::fast_trie_compact_as_trait <t_value> as_trait;

	public:
		typedef t_max_key key_type;
		typedef t_value value_type; // FIXME: replace.
		typedef t_value mapped_type;
		typedef std::size_t size_type;

		template <typename T, bool t_dummy = false> struct value_ref { typedef T const &type; };
		template <bool t_dummy> struct value_ref <void, t_dummy> { typedef void type; };

		typedef std::pair <key_type, detail::x_fast_trie_leaf_link <
			key_type,
			typename value_ref <value_type>::type>
		> leaf_it_val;
			
		typedef std::pair <key_type const, detail::x_fast_trie_leaf_link <
			key_type,
			typename value_ref <value_type>::type>
		> const_leaf_it_val;
			
		typedef typename as_trait::serialize_value_callback_type serialize_value_callback_type;
		typedef typename as_trait::load_value_callback_type load_value_callback_type;

		typedef x_fast_trie_compact_as_leaf_link_iterator_tpl <
			x_fast_trie_compact_as,
			const_leaf_it_val
		>							const_leaf_iterator;
		typedef const_leaf_iterator	const_iterator;
		
		typedef detail::x_fast_trie_tag x_fast_trie_tag;

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
					typename t_dst::value_type value(kv.second.value);
					dst.insert(key, value);
				}
			}
		};

		template <typename t_dst, typename t_src>
		struct fill_trie <t_dst, t_src, void>
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
					dst.insert(key);
				}
			}
		};
		
	protected:
		template <typename t_ret_key, typename t_key>
		static x_fast_trie_compact_as *construct_specific(x_fast_trie <t_key, t_value> &, key_type const);

		x_fast_trie_compact_as(key_type const offset = 0):
			m_offset(offset)
		{
		}
		
		virtual size_type serialize_keys_(
			std::ostream &out,
			serialize_value_callback_type value_callback,
			sdsl::structure_tree_node *v,
			std::string name
		) const = 0;
	
		virtual size_type serialize_(
			std::ostream &out,
			sdsl::structure_tree_node *v,
			std::string name
		) const = 0;
	
		virtual void load_keys_(
			std::istream &in,
			load_value_callback_type value_callback
		) = 0;
	
		virtual void load_(std::istream &in) = 0;
		
		static x_fast_trie_compact_as *construct_from_size(std::size_t const key_size);
		
	public:
		virtual ~x_fast_trie_compact_as() {}
		
		template <typename t_key>
		static x_fast_trie_compact_as *construct(x_fast_trie <t_key, t_value> &);

		key_type offset() const { return m_offset; }
		
		virtual size_type key_size() const = 0;
		virtual size_type size() const = 0;
		virtual bool contains(key_type const key) const = 0;
		virtual key_type min_key() const = 0;
		virtual key_type max_key() const = 0;
		
		virtual const_leaf_iterator cbegin() const = 0;
		virtual const_leaf_iterator cend() const = 0;
		const_leaf_iterator begin() const { return cbegin(); }
		const_leaf_iterator end() const { return cend(); };
		
		virtual bool find(key_type const key, const_leaf_iterator &it) const = 0;
		virtual bool find_predecessor(key_type const key, const_leaf_iterator &pred, bool allow_equal = false) const = 0;
		virtual bool find_successor(key_type const key, const_leaf_iterator &succ, bool allow_equal = false) const = 0;
		
		template <typename t_other_max_key, typename t_other_value, bool t_other_enable_serialize>
		bool operator==(x_fast_trie_compact_as <t_other_max_key, t_other_value, t_other_enable_serialize> const &other) const
		{
			return std::equal(cbegin(), cend(), other.cbegin(), other.cend());
		}
		
		virtual void print() const = 0;
		
		typename trait::key_type const iterator_key(const_leaf_iterator const &it) const { trait t; return t.key(it); }
		typename trait::value_type const &iterator_value(const_leaf_iterator const &it) const { trait t; return t.value(it); }
		
		template <bool t_dummy = true>
		auto serialize_keys(
			std::ostream &out,
			serialize_value_callback_type value_callback,
			sdsl::structure_tree_node *v,
			std::string name
		) const -> typename std::enable_if <trait::is_map_type && as_trait::is_map_type && t_dummy, std::size_t>::type;
	
		template <bool t_dummy = true>
		auto serialize(
			std::ostream &out,
			sdsl::structure_tree_node *v = nullptr,
			std::string name = ""
		) const -> typename std::enable_if <t_enable_serialize && t_dummy, std::size_t>::type;
	
		template <bool t_dummy = true>
		static auto load_keys(
			std::istream &in,
			load_value_callback_type value_callback
		) -> typename std::enable_if <
			trait::is_map_type && as_trait::is_map_type && t_dummy,
			x_fast_trie_compact_as
		>::type *;
	
		template <bool t_dummy = true>
		static auto load(
			std::istream &in
		) -> typename std::enable_if <
			t_enable_serialize && t_dummy,
			x_fast_trie_compact_as
		>::type *;

	protected:
		virtual leaf_it_val leaf_link(size_type idx) const = 0;
		const_leaf_it_val const_leaf_link(size_type idx) const
		{
			auto ll(leaf_link(idx));
			return const_leaf_it_val(ll.first, ll.second);
		}
	};


	template <typename t_max_key, typename t_key, typename t_value, bool t_enable_serialize = false>
	class x_fast_trie_compact_as_tpl final : public x_fast_trie_compact_as <t_max_key, t_value, t_enable_serialize>
	{
		template <typename>
		friend struct detail::fast_trie_compact_as_tpl_value_trait;
		
		template <bool>
		friend struct detail::fast_trie_compact_as_tpl_serialize_trait;
		
		template <typename, typename, bool>
		friend class x_fast_trie_compact_as;
		
	protected:
		typedef x_fast_trie_compact_as <t_max_key, t_value, t_enable_serialize> base_class;
		typedef x_fast_trie_compact <t_key, t_value, t_enable_serialize> trie_type;

		typedef typename base_class::leaf_it_val leaf_it_val;

	public:
		typedef typename base_class::size_type size_type;
		typedef typename base_class::key_type key_type;
		typedef typename base_class::value_type value_type;
		typedef typename base_class::mapped_type mapped_type;
		typedef typename base_class::serialize_value_callback_type serialize_value_callback_type;
		typedef typename base_class::load_value_callback_type load_value_callback_type;
		typedef typename base_class::const_leaf_iterator const_leaf_iterator;

		typedef detail::x_fast_trie_tag x_fast_trie_tag;

	protected:
		trie_type m_trie;

	protected:
		x_fast_trie_compact_as_tpl(): base_class(0) {}

	public:
		x_fast_trie_compact_as_tpl(trie_type &trie, t_max_key offset):
			base_class(offset),
			m_trie(std::move(trie))
		{
		}

		virtual size_type key_size() const override					{ return sizeof(t_key); }
		virtual size_type size() const override						{ return m_trie.size(); }
		virtual bool contains(key_type const key) const override	{ return m_trie.contains(key - this->m_offset); }
		virtual key_type min_key() const override					{ return this->m_offset + m_trie.min_key(); }
		virtual key_type max_key() const override					{ return this->m_offset + m_trie.max_key(); }

		virtual const_leaf_iterator cbegin() const override			{ return const_leaf_iterator(*this, 0); }
		virtual const_leaf_iterator cend() const override			{ return const_leaf_iterator(*this, size()); }

		virtual bool find(key_type const key, const_leaf_iterator &it) const override;
		virtual bool find_predecessor(key_type const key, const_leaf_iterator &pred, bool allow_equal = false) const override;
		virtual bool find_successor(key_type const key, const_leaf_iterator &succ, bool allow_equal = false) const override;
		
		virtual void print() const override							{ m_trie.print(); }
		
	protected:
		virtual size_type serialize_keys_(
			std::ostream &out,
			serialize_value_callback_type value_callback,
			sdsl::structure_tree_node *v,
			std::string name
		) const override;
	
		virtual size_type serialize_(
			std::ostream &out,
			sdsl::structure_tree_node *v,
			std::string name
		) const override;
	
		virtual void load_keys_(
			std::istream &in,
			load_value_callback_type value_callback
		) override;
	
		virtual void load_(std::istream &in) override;
		
		virtual leaf_it_val leaf_link(size_type idx) const override;
		bool check_find_result(bool const, typename trie_type::const_leaf_iterator const &, const_leaf_iterator &) const;
	};


	template <typename t_trie, typename t_it_val>
	template <typename t_other_val>
	auto x_fast_trie_compact_as_leaf_link_iterator_tpl <t_trie, t_it_val>::distance_to(
		x_fast_trie_compact_as_leaf_link_iterator_tpl <t_trie, t_other_val> const &other
	) const -> difference_type
	{
		assert(m_trie == other.m_trie);
		return other.m_idx - m_idx;
	}
	
	
	template <typename t_trie, typename t_it_val>
	template <typename t_other_val>
	bool x_fast_trie_compact_as_leaf_link_iterator_tpl <t_trie, t_it_val>::equal(
		x_fast_trie_compact_as_leaf_link_iterator_tpl <t_trie, t_other_val> const &other
	) const
	{
		return (this->m_trie == other.m_trie &&
				this->m_idx == other.m_idx);
	}
	
	
	template <typename t_trie, typename t_it_val>
	auto x_fast_trie_compact_as_leaf_link_iterator_tpl <t_trie, t_it_val>::dereference() const -> t_it_val
	{
		return m_trie->const_leaf_link(m_idx);
	}
	
	
	template <typename t_max_key, typename t_value, bool t_enable_serialize>
	auto x_fast_trie_compact_as <t_max_key, t_value, t_enable_serialize>::construct_from_size(
		std::size_t const key_size
	) -> x_fast_trie_compact_as *
	{
		switch (key_size)
		{
			case 1:
				return new x_fast_trie_compact_as_tpl <t_max_key, uint8_t, t_value, t_enable_serialize>();
				
			case 2:
				return new x_fast_trie_compact_as_tpl <t_max_key, uint16_t, t_value, t_enable_serialize>();
				
			case 4:
				return new x_fast_trie_compact_as_tpl <t_max_key, uint32_t, t_value, t_enable_serialize>();
				
			case 8:
				return new x_fast_trie_compact_as_tpl <t_max_key, uint64_t, t_value, t_enable_serialize>();
				
			default:
				assert(0); // Not implemented.
				return nullptr;
		}
	}
	
	
	template <typename t_max_key, typename t_value, bool t_enable_serialize>
	template <bool t_dummy>
	auto x_fast_trie_compact_as <t_max_key, t_value, t_enable_serialize>::serialize_keys(
		std::ostream &out,
		serialize_value_callback_type value_callback,
		sdsl::structure_tree_node *v,
		std::string name
	) const -> typename std::enable_if <trait::is_map_type && as_trait::is_map_type && t_dummy, std::size_t>::type
	{
		auto *child(sdsl::structure_tree::add_child(v, name, sdsl::util::class_name(*this)));
		size_type written_bytes(0);
		
		std::size_t const key_size(this->key_size());
		written_bytes += sdsl::write_member(key_size, out, child, "key_size");
		written_bytes += sdsl::write_member(m_offset, out, child, "offset");
		written_bytes += serialize_keys_(out, value_callback, v, name);
		
		return written_bytes;
	}
	
	
	template <typename t_max_key, typename t_value, bool t_enable_serialize>
	template <bool t_dummy>
	auto x_fast_trie_compact_as <t_max_key, t_value, t_enable_serialize>::serialize(
		std::ostream &out,
		sdsl::structure_tree_node *v,
		std::string name
	) const -> typename std::enable_if <t_enable_serialize && t_dummy, std::size_t>::type
	{
		auto *child(sdsl::structure_tree::add_child(v, name, sdsl::util::class_name(*this)));
		size_type written_bytes(0);
			
		std::size_t const key_size(this->key_size());
		written_bytes += sdsl::write_member(key_size, out, child, "key_size");
		written_bytes += sdsl::write_member(m_offset, out, child, "offset");
		written_bytes += serialize_(out, v, name);
			
		return written_bytes;
	}
	
	
	template <typename t_max_key, typename t_value, bool t_enable_serialize>
	template <bool t_dummy>
	auto x_fast_trie_compact_as <t_max_key, t_value, t_enable_serialize>::load_keys(
		std::istream &in,
		load_value_callback_type value_callback
	) -> typename std::enable_if <
		trait::is_map_type && as_trait::is_map_type && t_dummy,
		x_fast_trie_compact_as
	>::type *
	{
		std::size_t key_size(0);
		sdsl::read_member(key_size, in);
		x_fast_trie_compact_as *retval(construct_from_size(key_size));
		sdsl::read_member(retval->m_offset, in);
		retval->load_keys_(in, value_callback);
		return retval;
	}
	
	
	template <typename t_max_key, typename t_value, bool t_enable_serialize>
	template <bool t_dummy>
	auto x_fast_trie_compact_as <t_max_key, t_value, t_enable_serialize>::load(
		std::istream &in
	) -> typename std::enable_if <
		t_enable_serialize && t_dummy,
		x_fast_trie_compact_as
	>::type *
	{
		std::size_t key_size(0);
		sdsl::read_member(key_size, in);
		x_fast_trie_compact_as *retval(construct_from_size(key_size));
		sdsl::read_member(retval->m_offset, in);
		retval->load_(in);
		return retval;
	}


	template <typename t_max_key, typename t_key, typename t_value, bool t_enable_serialize>
	bool x_fast_trie_compact_as_tpl <t_max_key, t_key, t_value, t_enable_serialize>::check_find_result(
		bool const res,
		typename trie_type::const_leaf_iterator const &it,
		const_leaf_iterator &out_it
	) const
	{
		if (res)
		{
			auto const distance(it - m_trie.cbegin());
			out_it = const_leaf_iterator(*this, distance);
			return true;
		}
		return false;
	}


	template <typename t_max_key, typename t_key, typename t_value, bool t_enable_serialize>
	bool x_fast_trie_compact_as_tpl <t_max_key, t_key, t_value, t_enable_serialize>::find(
		key_type const key, const_leaf_iterator &out_it
	) const
	{
		if (key < this->m_offset)
			return false;
		else if (std::numeric_limits <typename trie_type::key_type>::max() < key - this->m_offset)
			return false;
		
		typename trie_type::const_leaf_iterator it;
		return check_find_result(m_trie.find(key - this->m_offset, it), it, out_it);
	}


	template <typename t_max_key, typename t_key, typename t_value, bool t_enable_serialize>
	bool x_fast_trie_compact_as_tpl <t_max_key, t_key, t_value, t_enable_serialize>::find_predecessor(
		key_type const key, const_leaf_iterator &out_it, bool allow_equal
	) const
	{
		if (key < this->m_offset)
			return false;
		else if (std::numeric_limits <typename trie_type::key_type>::max() < key - this->m_offset)
			return find(max_key(), out_it);

		typename trie_type::const_leaf_iterator pred;
		return check_find_result(m_trie.find_predecessor(key - this->m_offset, pred, allow_equal), pred, out_it);
	}


	template <typename t_max_key, typename t_key, typename t_value, bool t_enable_serialize>
	bool x_fast_trie_compact_as_tpl <t_max_key, t_key, t_value, t_enable_serialize>::find_successor(
		key_type const key, const_leaf_iterator &out_it, bool allow_equal
	) const
	{
		if (key < this->m_offset)
			return find(min_key(), out_it);
		else if (std::numeric_limits <typename trie_type::key_type>::max() < key - this->m_offset)
			return false;

		typename trie_type::const_leaf_iterator succ;
		return check_find_result(m_trie.find_successor(key - this->m_offset, succ, allow_equal), succ, out_it);
	}


	template <typename t_max_key, typename t_key, typename t_value, bool t_enable_serialize>
	auto x_fast_trie_compact_as_tpl <t_max_key, t_key, t_value, t_enable_serialize>::leaf_link(
		size_type const idx
	) const -> leaf_it_val
	{
		auto const &leaf_link(m_trie.leaf_link(idx));
		typename leaf_it_val::second_type ret_link(leaf_link.second);
		ret_link.prev += this->m_offset;
		ret_link.next += this->m_offset;
		leaf_it_val retval{this->m_offset + leaf_link.first, ret_link};
		return retval;
	}
	
	
	template <typename t_max_key, typename t_key, typename t_value, bool t_enable_serialize>
	auto x_fast_trie_compact_as_tpl <t_max_key, t_key, t_value, t_enable_serialize>::serialize_keys_(
		std::ostream &out,
		serialize_value_callback_type value_callback,
		sdsl::structure_tree_node *v,
		std::string name
	) const -> size_type
	{
		detail::fast_trie_compact_as_tpl_value_trait <value_type> trait;
		return trait.serialize_keys(*this, out, value_callback, v, name);
	}


	template <typename t_max_key, typename t_key, typename t_value, bool t_enable_serialize>
	auto x_fast_trie_compact_as_tpl <t_max_key, t_key, t_value, t_enable_serialize>::serialize_(
		std::ostream &out,
		sdsl::structure_tree_node *v,
		std::string name
	) const -> size_type
	{
		detail::fast_trie_compact_as_tpl_serialize_trait <t_enable_serialize> trait;
		return trait.serialize(*this, out, v, name);
	}


	template <typename t_max_key, typename t_key, typename t_value, bool t_enable_serialize>
	void x_fast_trie_compact_as_tpl <t_max_key, t_key, t_value, t_enable_serialize>::load_keys_(
		std::istream &in,
		load_value_callback_type value_callback
	)
	{
		detail::fast_trie_compact_as_tpl_value_trait <value_type> trait;
		trait.load_keys(*this, in, value_callback);
	}


	template <typename t_max_key, typename t_key, typename t_value, bool t_enable_serialize>
	void x_fast_trie_compact_as_tpl <t_max_key, t_key, t_value, t_enable_serialize>::load_(
		std::istream &in
	)
	{
		detail::fast_trie_compact_as_tpl_serialize_trait <t_enable_serialize> trait;
		trait.load(*this, in);
	}


	template <typename t_max_key, typename t_value, bool t_enable_serialize>
	template <typename t_ret_key, typename t_key>
	auto x_fast_trie_compact_as <t_max_key, t_value, t_enable_serialize>::construct_specific(
		x_fast_trie <t_key, t_value> &trie,
		key_type const offset
	) -> x_fast_trie_compact_as *
	{
		x_fast_trie <t_ret_key, t_value> temp_trie;
		fill_trie <decltype(temp_trie), decltype(trie)> ft;
		ft(temp_trie, trie, offset);
		x_fast_trie_compact <t_ret_key, t_value, t_enable_serialize> ct(temp_trie);
		return new x_fast_trie_compact_as_tpl <t_max_key, t_ret_key, t_value, t_enable_serialize>(ct, offset);
	}


	template <typename t_max_key, typename t_value, bool t_enable_serialize>
	template <typename t_key>
	auto x_fast_trie_compact_as <t_max_key, t_value, t_enable_serialize>::construct(
		x_fast_trie <t_key, t_value> &trie
	) -> x_fast_trie_compact_as *
	{
		if (0 == trie.size())
			return x_fast_trie_compact_as::construct_specific <uint8_t>(trie, 0);
		
		auto const min(trie.min_key());
		auto const max(trie.max_key());
		auto const ret_max(max - min);

		if (ret_max <= std::numeric_limits <uint8_t>::max())
			return x_fast_trie_compact_as::construct_specific <uint8_t>(trie, min);
		else if (ret_max <= std::numeric_limits <uint16_t>::max())
			return x_fast_trie_compact_as::construct_specific <uint16_t>(trie, min);
		else if (ret_max <= std::numeric_limits <uint32_t>::max())
			return x_fast_trie_compact_as::construct_specific <uint32_t>(trie, min);
		else if (ret_max <= std::numeric_limits <uint64_t>::max())
			return x_fast_trie_compact_as::construct_specific <uint64_t>(trie, min);

		assert(0);
	}
}

#endif
