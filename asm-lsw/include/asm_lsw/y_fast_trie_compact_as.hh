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

#include <asm_lsw/fast_trie_compact_as_helper.hh>
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

	
	template <typename t_max_key, typename t_value = void, bool t_enable_serialize = false>
	class y_fast_trie_compact_as
	{
	protected:
		typedef detail::fast_trie_compact_as_trait <t_value> as_trait;
		
	public:
		typedef t_max_key key_type;
		typedef t_value value_type; // FIXME: replace.
		typedef t_value mapped_type;
		typedef std::size_t size_type;

		typedef detail::y_fast_trie_trait <key_type, value_type> trait;
		
		typedef y_fast_trie_compact_as_subtree_iterator_tpl <
			typename y_fast_trie_compact_as_subtree_it_val <key_type, value_type>::type const,
			key_type,
			value_type
		> const_subtree_iterator;
		
		typedef const_subtree_iterator const_iterator;
		
		typedef typename as_trait::serialize_value_callback_type serialize_value_callback_type;
		typedef typename as_trait::load_value_callback_type load_value_callback_type;
		
		typedef detail::y_fast_trie_tag y_fast_trie_tag;
		
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

			void operator()(t_dst &dst, t_src &src, key_type const offset) const
			{
				for (auto &kv : src)
				{
					assert(kv.first - offset < std::numeric_limits <typename t_dst::key_type>::max());
					typename t_dst::key_type key(kv.first - offset);
					typename t_dst::value_type value(std::move(kv.second));
					dst.insert(key, std::move(value));
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

		// FIXME: make t_src const & if t_value is void.
		struct fill_with_trie
		{
			template <typename t_dst, typename t_src>
			void operator()(
				t_dst &dst,
				t_src &src,
				key_type const offset
			) const
			{
				fill_trie <t_dst, typename util::remove_ref_t <t_src>::subtree> ft;
				auto proxy(src.subtree_map_proxy());
				for (auto &st : proxy)
					ft(dst, st.second, offset);
			}
		};

		// FIXME: make t_src const & if t_value is void.
		struct fill_with_collection
		{
			template <typename t_dst, typename t_src>
			void operator()(
				t_dst &dst,
				t_src &src,
				key_type const offset
			) const
			{
				fill_trie <t_dst, t_src> ft;
				ft(dst, src, offset);
			}
		};
		
	protected:
		template <typename t_ret_key, typename t_collection, typename t_fill>
		static y_fast_trie_compact_as *construct_specific(t_collection &, t_fill const &, key_type const, key_type const);

		template <typename t_collection, typename t_fill>
		static y_fast_trie_compact_as *construct(
			t_collection &collection,
			t_fill const &fill,
			t_max_key const min,
			t_max_key const max
		);

		y_fast_trie_compact_as(key_type const offset = 0):
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
		
		static y_fast_trie_compact_as *construct_from_size(std::size_t const key_size);
		
	public:
		template <typename t_key>
		static y_fast_trie_compact_as *construct(y_fast_trie <t_key, t_value> &);
		
		template <typename t_collection>
		static y_fast_trie_compact_as *construct(t_collection &, key_type const min, key_type const max);

		key_type offset() const { return m_offset; }

		virtual size_type key_size() const = 0;
		virtual size_type size() const = 0;

		virtual bool contains(key_type const key) const = 0;
		virtual key_type min_key() const = 0;
		virtual key_type max_key() const = 0;
		
		virtual bool find(key_type const key, const_subtree_iterator &iterator) const = 0;
		virtual bool find_predecessor(key_type const key, const_subtree_iterator &iterator, bool allow_equal = false) const = 0;
		virtual bool find_successor(key_type const key, const_subtree_iterator &iterator, bool allow_equal = false) const = 0;
		virtual bool find_subtree_min(key_type const key, const_subtree_iterator &iterator) const = 0;
		virtual bool find_subtree_max(key_type const key, const_subtree_iterator &iterator) const = 0;
		virtual bool find_next_subtree_key(key_type &key /* inout */) const = 0;

		virtual void print() const = 0;

		typename trait::key_type const iterator_key(const_subtree_iterator const &it) const { trait t; return t.key(it); }
		typename trait::value_type const &iterator_value(const_subtree_iterator const &it) const { trait t; return t.value(it); }
		
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
			y_fast_trie_compact_as
		>::type *;
	
		template <bool t_dummy = true>
		static auto load(
			std::istream &in
		) -> typename std::enable_if <
			t_enable_serialize && t_dummy,
			y_fast_trie_compact_as
		>::type *;
	};
	
	
	template <typename t_max_key, typename t_key, typename t_value, bool t_enable_serialize = false>
	class y_fast_trie_compact_as_tpl final : public y_fast_trie_compact_as <t_max_key, t_value, t_enable_serialize>
	{
		template <typename>
		friend struct detail::fast_trie_compact_as_tpl_value_trait;
		
		template <bool>
		friend struct detail::fast_trie_compact_as_tpl_serialize_trait;
		
		template <typename, typename, bool>
		friend class y_fast_trie_compact_as;
		
	protected:
		typedef y_fast_trie_compact_as <t_max_key, t_value, t_enable_serialize> base_class;
		typedef y_fast_trie_compact <t_key, t_value, t_enable_serialize> trie_type;
		
		typedef detail::y_fast_trie_base_subtree_iterator_wrapper <
			typename y_fast_trie_compact_as_subtree_it_val <t_max_key, t_value>::type const
		> subtree_iterator_wrapper;
		
		typedef typename base_class::size_type						size_type;
		typedef typename base_class::key_type						key_type;
		typedef typename base_class::value_type						value_type;
		typedef typename base_class::serialize_value_callback_type	serialize_value_callback_type;
		typedef typename base_class::load_value_callback_type		load_value_callback_type;
		typedef typename base_class::const_subtree_iterator			const_subtree_iterator;
		
		typedef detail::y_fast_trie_tag y_fast_trie_tag;
		
	protected:
		trie_type m_trie;
		
	protected:
		y_fast_trie_compact_as_tpl() = default;
		
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

		virtual void print() const override							{ std::cout << "Offset: " << this->m_offset << std::endl; m_trie.print(); }

		virtual bool find(key_type const key, const_subtree_iterator &iterator) const override;
		virtual bool find_predecessor(key_type const key, const_subtree_iterator &iterator, bool allow_equal = false) const override;
		virtual bool find_successor(key_type const key, const_subtree_iterator &iterator, bool allow_equal = false) const override;
		virtual bool find_subtree_min(key_type const key, const_subtree_iterator &iterator) const override;
		virtual bool find_subtree_max(key_type const key, const_subtree_iterator &iterator) const override;
		virtual bool find_next_subtree_key(key_type &key /* inout */) const override;
		
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
		
		bool check_find_result(bool const, typename trie_type::const_subtree_iterator, const_subtree_iterator &) const;
	};
	
	
	template <typename t_max_key, typename t_value, bool t_enable_serialize>
	auto y_fast_trie_compact_as <t_max_key, t_value, t_enable_serialize>::construct_from_size(
		std::size_t const key_size
	) -> y_fast_trie_compact_as *
	{
		switch (key_size)
		{
			case 1:
				return new y_fast_trie_compact_as_tpl <t_max_key, uint8_t, t_value, t_enable_serialize>();
				
			case 2:
				return new y_fast_trie_compact_as_tpl <t_max_key, uint16_t, t_value, t_enable_serialize>();
				
			case 4:
				return new y_fast_trie_compact_as_tpl <t_max_key, uint32_t, t_value, t_enable_serialize>();
				
			case 8:
				return new y_fast_trie_compact_as_tpl <t_max_key, uint64_t, t_value, t_enable_serialize>();
				
			default:
				assert(0); // Not implemented.
				return nullptr;
		}
	}
	
	
	template <typename t_max_key, typename t_value, bool t_enable_serialize>
	template <bool t_dummy>
	auto y_fast_trie_compact_as <t_max_key, t_value, t_enable_serialize>::serialize_keys(
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
	auto y_fast_trie_compact_as <t_max_key, t_value, t_enable_serialize>::serialize(
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
	auto y_fast_trie_compact_as <t_max_key, t_value, t_enable_serialize>::load_keys(
		std::istream &in,
		load_value_callback_type value_callback
	) -> typename std::enable_if <
		trait::is_map_type && as_trait::is_map_type && t_dummy,
		y_fast_trie_compact_as
	>::type *
	{
		std::size_t key_size(0);
		sdsl::read_member(key_size, in);
		y_fast_trie_compact_as *retval(construct_from_size(key_size));
		sdsl::read_member(retval->m_offset, in);
		retval->load_keys_(in, value_callback);
		return retval;
	}
	
	
	template <typename t_max_key, typename t_value, bool t_enable_serialize>
	template <bool t_dummy>
	auto y_fast_trie_compact_as <t_max_key, t_value, t_enable_serialize>::load(
		std::istream &in
	) -> typename std::enable_if <
		t_enable_serialize && t_dummy,
		y_fast_trie_compact_as
	>::type *
	{
		std::size_t key_size(0);
		sdsl::read_member(key_size, in);
		y_fast_trie_compact_as *retval(construct_from_size(key_size));
		sdsl::read_member(retval->m_offset, in);
		retval->load_(in);
		return retval;
	}
	
	
	template <typename t_max_key, typename t_key, typename t_value, bool t_enable_serialize>
	bool y_fast_trie_compact_as_tpl <t_max_key, t_key, t_value, t_enable_serialize>::check_find_result(
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
	
	
	template <typename t_max_key, typename t_key, typename t_value, bool t_enable_serialize>
	bool y_fast_trie_compact_as_tpl <t_max_key, t_key, t_value, t_enable_serialize>::find(
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
	
	
	template <typename t_max_key, typename t_key, typename t_value, bool t_enable_serialize>
	bool y_fast_trie_compact_as_tpl <t_max_key, t_key, t_value, t_enable_serialize>::find_predecessor(
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


	template <typename t_max_key, typename t_key, typename t_value, bool t_enable_serialize>
	bool y_fast_trie_compact_as_tpl <t_max_key, t_key, t_value, t_enable_serialize>::find_successor(
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

	
	template <typename t_max_key, typename t_key, typename t_value, bool t_enable_serialize>
	bool y_fast_trie_compact_as_tpl <t_max_key, t_key, t_value, t_enable_serialize>::find_subtree_min(
		key_type const key, const_subtree_iterator &out_it
	) const
	{
		if (key < this->m_offset)
			return false;

		typename trie_type::const_subtree_iterator it;
		return check_find_result(m_trie.find_subtree_min(key - this->m_offset, it), it, out_it);
	}

	
	template <typename t_max_key, typename t_key, typename t_value, bool t_enable_serialize>
	bool y_fast_trie_compact_as_tpl <t_max_key, t_key, t_value, t_enable_serialize>::find_subtree_max(
		key_type const key, const_subtree_iterator &out_it
	) const
	{
		if (key < this->m_offset)
			return false;
		
		typename trie_type::const_subtree_iterator it;
		return check_find_result(m_trie.find_subtree_max(key - this->m_offset, it), it, out_it);
	}

	
	template <typename t_max_key, typename t_key, typename t_value, bool t_enable_serialize>
	bool y_fast_trie_compact_as_tpl <t_max_key, t_key, t_value, t_enable_serialize>::find_next_subtree_key(
		key_type &key /* inout */
	) const
	{
		if (key < this->m_offset)
			return false;
		
		typename trie_type::key_type tmp_key(key - this->m_offset);
		if (m_trie.find_next_subtree_key(tmp_key))
		{
			key = tmp_key + this->m_offset;
			return true;
		}
		
		return false;
	}
	
	
	template <typename t_max_key, typename t_key, typename t_value, bool t_enable_serialize>
	auto y_fast_trie_compact_as_tpl <t_max_key, t_key, t_value, t_enable_serialize>::serialize_keys_(
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
	auto y_fast_trie_compact_as_tpl <t_max_key, t_key, t_value, t_enable_serialize>::serialize_(
		std::ostream &out,
		sdsl::structure_tree_node *v,
		std::string name
	) const -> size_type
	{
		detail::fast_trie_compact_as_tpl_serialize_trait <t_enable_serialize> trait;
		return trait.serialize(*this, out, v, name);
	}


	template <typename t_max_key, typename t_key, typename t_value, bool t_enable_serialize>
	void y_fast_trie_compact_as_tpl <t_max_key, t_key, t_value, t_enable_serialize>::load_keys_(
		std::istream &in,
		load_value_callback_type value_callback
	)
	{
		detail::fast_trie_compact_as_tpl_value_trait <value_type> trait;
		trait.load_keys(*this, in, value_callback);
	}


	template <typename t_max_key, typename t_key, typename t_value, bool t_enable_serialize>
	void y_fast_trie_compact_as_tpl <t_max_key, t_key, t_value, t_enable_serialize>::load_(
		std::istream &in
	)
	{
		detail::fast_trie_compact_as_tpl_serialize_trait <t_enable_serialize> trait;
		trait.load(*this, in);
	}
	
	
	template <typename t_max_key, typename t_value, bool t_enable_serialize>
	template <typename t_ret_key, typename t_collection, typename t_fill>
	y_fast_trie_compact_as <t_max_key, t_value, t_enable_serialize> *
	y_fast_trie_compact_as <t_max_key, t_value, t_enable_serialize>::construct_specific(
		t_collection &collection,
		t_fill const &fill,
		key_type const offset,
		key_type const limit
	)
	{
		// Set the domain limit.
		y_fast_trie <t_ret_key, t_value> temp_trie(limit);
		
		fill(temp_trie, collection, offset);
		y_fast_trie_compact <t_ret_key, t_value, t_enable_serialize> ct(temp_trie);
		return new y_fast_trie_compact_as_tpl <t_max_key, t_ret_key, t_value, t_enable_serialize>(ct, offset);
	}


	// FIXME: change the trie type to const or non-const based on t_value (void or non-void).
	template <typename t_max_key, typename t_value, bool t_enable_serialize>
	template <typename t_key>
	y_fast_trie_compact_as <t_max_key, t_value, t_enable_serialize> *
	y_fast_trie_compact_as <t_max_key, t_value, t_enable_serialize>::construct(
		y_fast_trie <t_key, t_value> &trie
	)
	{
		fill_with_trie fill;
		if (0 == trie.size())
			return y_fast_trie_compact_as::construct_specific <uint8_t>(trie, fill, 0, 0);
		
		auto const min(trie.min_key());
		auto const max(trie.max_key());
		return construct(trie, fill, min, max);
	}


	// FIXME: change the collection type to const or non-const based on t_value (void or non-void).
	template <typename t_max_key, typename t_value, bool t_enable_serialize>
	template <typename t_collection>
	y_fast_trie_compact_as <t_max_key, t_value, t_enable_serialize> *
	y_fast_trie_compact_as <t_max_key, t_value, t_enable_serialize>::construct(
		t_collection &collection,
		t_max_key const min,
		t_max_key const max
	)
	{
		fill_with_collection fill;
		if (0 == collection.size())
			return y_fast_trie_compact_as::construct_specific <uint8_t>(collection, fill, 0, 0);

#ifndef NDEBUG
		{
			util::iterator_access <t_value> acc;
			auto const res(std::minmax_element(collection.cbegin(), collection.cend()));
			assert(acc.key(res.first) == min);
			assert(acc.key(res.second) == max);
		}
#endif

		return construct(collection, fill, min, max);
	}


	// FIXME: change the trie type to const or non-const based on t_value (void or non-void).
	template <typename t_max_key, typename t_value, bool t_enable_serialize>
	template <typename t_collection, typename t_fill>
	y_fast_trie_compact_as <t_max_key, t_value, t_enable_serialize> *
	y_fast_trie_compact_as <t_max_key, t_value, t_enable_serialize>::construct(
		t_collection &collection,
		t_fill const &fill,
		t_max_key const min,
		t_max_key const max
	)
	{
		auto const ret_max(max - min);
		if (ret_max <= std::numeric_limits <uint8_t>::max())
			return y_fast_trie_compact_as::construct_specific <uint8_t>(collection, fill, min, ret_max);
		else if (ret_max <= std::numeric_limits <uint16_t>::max())
			return y_fast_trie_compact_as::construct_specific <uint16_t>(collection, fill, min, ret_max);
		else if (ret_max <= std::numeric_limits <uint32_t>::max())
			return y_fast_trie_compact_as::construct_specific <uint32_t>(collection, fill, min, ret_max);
		else if (ret_max <= std::numeric_limits <uint64_t>::max())
			return y_fast_trie_compact_as::construct_specific <uint64_t>(collection, fill, min, ret_max);

		// Require one of the types to match.
		assert(0);
	}
}

#endif
