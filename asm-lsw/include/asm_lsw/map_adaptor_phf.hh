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


#ifndef ASM_LSW_MAP_ADAPTOR_PHF_HH
#define ASM_LSW_MAP_ADAPTOR_PHF_HH

#include <asm_lsw/map_adaptor_helper.hh>
#include <asm_lsw/map_adaptor_phf_helper.hh>
#include <asm_lsw/phf_wrapper.hh>
#include <asm_lsw/util.hh>
#include <boost/core/enable_if.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <sdsl/int_vector.hpp>
#include <sdsl/rank_support.hpp>
#include <sdsl/select_support.hpp>
#include <type_traits>


namespace asm_lsw {
	
	template <template <typename ...> class, typename, typename, typename, typename, typename>
	class map_adaptor;
	
	template <typename t_spec, typename t_map, typename t_access_value_fn>
	class map_adaptor_phf_builder;


	template <
		template <typename ...> class t_vector,
		template <typename> class t_allocator,
		typename t_key,
		typename t_val,
		bool t_enable_serialize = false,
		typename t_access_key_fn = map_adaptor_access_key <t_key>
	>
	struct map_adaptor_phf_spec
	{
		template <typename ... Args> using vector_type = t_vector <Args ...>;
		template <typename ... Args> using allocator_type = t_allocator <Args ...>;
		typedef t_key key_type;
		typedef t_val value_type;
		typedef t_access_key_fn access_key_fn_type;
		
		template <template <typename> class t_new_allocator>
		using rebind_allocator = map_adaptor_phf_spec <t_vector, t_new_allocator, t_key, t_val, t_enable_serialize, t_access_key_fn>;
		
		enum { enable_serialize = t_enable_serialize };
	};
	
	
	template <typename t_spec>
	class map_adaptor_phf_base
	{
		template <typename> friend class map_adaptor_phf_base;

	protected:
		typedef sdsl::bit_vector used_indices_type;
		
	public:
		typedef detail::map_adaptor_phf_trait <t_spec>							trait_type;
		typedef typename t_spec::key_type										key_type;
		typedef typename t_spec::value_type										value_type; //FIXME: replace this with kv_type.
		typedef typename t_spec::value_type										mapped_type;
		typedef typename trait_type::kv_type									kv_type;
		typedef typename trait_type::const_iterator_return_type					const_iterator_return_type;
		typedef typename t_spec::template allocator_type <kv_type>				allocator_type;
		typedef typename t_spec::template vector_type <kv_type, allocator_type>	vector_type;
		typedef typename vector_type::size_type									size_type;
		typedef typename t_spec::access_key_fn_type								access_key_fn_type;
		
		template <template <typename> class t_new_allocator>
		using rebind_allocator = map_adaptor_phf_base <typename t_spec::template rebind_allocator <t_new_allocator>>;
		
		enum { can_serialize = t_spec::enable_serialize };

	protected:
		phf_wrapper m_phf;
		vector_type m_vector;
		used_indices_type m_used_indices;
		access_key_fn_type m_access_key_fn;
		size_type m_size{0};
		
	public:
		map_adaptor_phf_base() = default;
		map_adaptor_phf_base(map_adaptor_phf_base const &) = default;
		map_adaptor_phf_base(map_adaptor_phf_base &&) = default;
		map_adaptor_phf_base(
			phf_wrapper &phf,
			std::size_t const size,
			allocator_type const &alloc,
			access_key_fn_type const &access_key_fn
		):
			m_phf(std::move(phf)),
			m_vector(size, alloc),	// Default-insertion requires C++14.
			m_access_key_fn(access_key_fn)
		{
		}
		
		template <template <typename> class t_new_allocator>
		map_adaptor_phf_base(rebind_allocator <t_new_allocator> const &other):
			m_phf(other.m_phf),
			m_vector(other.m_vector.cbegin(), other.m_vector.cend()),
			m_used_indices(other.m_used_indices),
			m_access_key_fn(other.m_access_key_fn),
			m_size(other.m_size)
		{
		}
		
		map_adaptor_phf_base &operator=(map_adaptor_phf_base const &) & = default;
		map_adaptor_phf_base &operator=(map_adaptor_phf_base &&) & = default;
	};
	
	
	// FIXME: move to namespace detail.
	template <typename t_adaptor, typename t_it_val>
	class map_iterator_phf_tpl : public boost::iterator_facade <
		map_iterator_phf_tpl <t_adaptor, t_it_val>,
		t_it_val,
		boost::random_access_traversal_tag,
		t_it_val
	>
	{
		friend class boost::iterator_core_access;
		template <typename, typename> friend class map_iterator_phf_tpl;

	protected:
		typedef boost::iterator_facade <
			map_iterator_phf_tpl <t_adaptor, t_it_val>,
			t_it_val,
			boost::random_access_traversal_tag,
			t_it_val
		> base_class;
		
	public:
		typedef typename t_adaptor::vector_type::size_type size_type;
		typedef typename base_class::difference_type difference_type;
		
	protected:
		t_adaptor const *m_adaptor{nullptr};
		size_type m_idx{0};
		
	private:
		// For the converting constructor below.
		struct enabler {};
		
	public:
		map_iterator_phf_tpl() = default;
		
		map_iterator_phf_tpl(t_adaptor const *adaptor, size_type idx, bool convert = false):
			m_adaptor(adaptor),
			m_idx(idx)
		{
			if (convert && adaptor->m_vector.size())
			{
				auto const next(m_adaptor->m_used_indices_select1_support(1 + idx));
				m_idx = next;
			}
		}

		// Remove the conversion from iterator to const_iterator as per
		// http://www.boost.org/doc/libs/1_58_0/libs/iterator/doc/iterator_facade.html#telling-the-truth
		template <typename t_other_val>
		map_iterator_phf_tpl(
			map_iterator_phf_tpl <t_adaptor, t_other_val> const &other, typename boost::enable_if<
				boost::is_convertible<
					typename std::remove_reference <t_other_val>::type *,
					typename std::remove_reference <t_it_val>::type *
				>,
				enabler
			>::type = enabler()
		):
			m_adaptor(other.m_adaptor), m_idx(other.m_idx)
		{
		}
		
	private:
		void increment() { advance(1); }
		void decrement() { advance(-1); }
		
		void advance(difference_type n);
		
		template <typename t_other_val>
		difference_type distance_to(map_iterator_phf_tpl <t_adaptor, t_other_val> const &other) const;
		
		template <typename t_other_val>
		bool equal(map_iterator_phf_tpl <t_adaptor, t_other_val> const &other) const ASM_LSW_PURE;
		
		t_it_val dereference() const ASM_LSW_PURE;
	};
	

	template <typename t_spec>
	class map_adaptor_phf : public map_adaptor_phf_base <t_spec>
	{
		template <typename> friend class map_adaptor_phf;
		template <typename, typename> friend class map_iterator_phf_tpl;
		template <typename, typename, typename> friend class map_adaptor_phf_builder;

	protected:
		typedef map_adaptor_phf_base <t_spec>	base_class;
		
	public:
		typedef	typename base_class::trait_type							trait_type;
		typedef typename base_class::key_type							key_type;
		typedef typename base_class::value_type							value_type; // FIXME: replace kv_type with this.
		typedef typename base_class::mapped_type						mapped_type; // FIXME: replace value_type with this.
		typedef typename base_class::kv_type							kv_type;		// FIXME: replace.
		typedef typename base_class::const_iterator_return_type			const_iterator_return_type;	// FIXME: replace.
		typedef typename base_class::allocator_type						allocator_type;
		typedef typename base_class::vector_type						vector_type;
		typedef typename base_class::size_type							size_type;
		// If value_type is void, kv_type is the same as key_type.
		typedef map_iterator_phf_tpl <
			map_adaptor_phf,
			const_iterator_return_type
		>																const_iterator;
		typedef const_iterator											iterator;
		
		template <typename t_map, typename t_access_value_fn = detail::map_adaptor_phf_access_value <t_spec>>
		using builder_type = map_adaptor_phf_builder <t_spec, t_map, t_access_value_fn>;

		typedef typename base_class::access_key_fn_type					access_key_fn_type;
		typedef typename access_key_fn_type::accessed_type				accessed_type;
		
		template <template <typename> class t_new_allocator>
		using rebind_allocator = map_adaptor_phf <typename t_spec::template rebind_allocator <t_new_allocator>>;

		template <template <typename ...> class t_map, typename t_hash, typename t_key_equal>
		using mutable_map_adaptor_type = map_adaptor <t_map, key_type, value_type, access_key_fn_type, t_hash, t_key_equal>;
		
		typedef detail::map_adaptor_phf_tag		map_adaptor_phf_tag;

		static_assert(
			std::is_same <typename access_key_fn_type::key_type, key_type>::value,
			"access_key_fn's operator() should take key_type as parameter"
		);

	protected:
		typename base_class::used_indices_type::rank_0_type m_used_indices_rank0_support{};
		typename base_class::used_indices_type::select_1_type m_used_indices_select1_support{};
		
	protected:
		typename vector_type::size_type adapted_key(typename vector_type::size_type key) const { 
			if (this->m_phf.is_valid())
				return PHF::hash(&this->m_phf.get(), key);
			else
				return key;
		}
		
		template <typename t_adaptor>
		static size_type find_index(t_adaptor &adaptor, key_type const &key);

		template <typename t_adaptor>
		static size_type find_index_acc(
			t_adaptor &adaptor,
			accessed_type const &key
		);
			
		size_type serialize_common(std::ostream &out, sdsl::structure_tree_node *child) const;
		void load_common(std::istream &in);

	public:
		map_adaptor_phf() = default;
		map_adaptor_phf(map_adaptor_phf const &other);
		map_adaptor_phf(map_adaptor_phf &&other);
		
		template <template <typename> class t_new_allocator>
		map_adaptor_phf(rebind_allocator <t_new_allocator> const &);
		
		template <typename t_map, typename t_access_value_fn>
		explicit map_adaptor_phf(map_adaptor_phf_builder <t_spec, t_map, t_access_value_fn> &builder);
		
		// Move the values in map to the one owned by this adaptor.
		template <typename t_map, typename t_access_value_fn>
		map_adaptor_phf(
			t_map &map,
			std::size_t const size,
			phf_wrapper &phf,
			allocator_type const &alloc,
			access_key_fn_type const &access_key_fn,
			t_access_value_fn &access_value_fn
		);

		// Move the values in map to the one owned by this adaptor.
		// Create an allocator for this adaptor only.
		// Disable the constructor for map_adaptor_phf.
		template <
			typename t_map,
			typename std::enable_if <false == detail::is_map_adaptor_phf <t_map>::value> * = nullptr
		>
		explicit map_adaptor_phf(t_map &map);
		
		map_adaptor_phf &operator=(map_adaptor_phf const &) &;
		map_adaptor_phf &operator=(map_adaptor_phf &&other) &;
		
		template <typename t_other_spec>
		auto operator==(
			map_adaptor_phf <t_other_spec> const &other
		) const -> typename std::enable_if <
			std::is_same <
				mapped_type, typename map_adaptor_phf <t_other_spec>::value_type
			>::value, bool
		>::type
		{ /* FIXME: what about access_key? */ return trait_type::contains(*this, other) && trait_type::contains(other, *this); }
		
		vector_type const &map() const							{ return this->m_vector; }
		vector_type &map()										{ return this->m_vector; }
		size_type size() const									{ return this->m_size; }
		access_key_fn_type const &access_key_fn() const			{ return this->m_access_key_fn; }

		const_iterator find(key_type const &key)				{ return const_iterator(this, find_index(*this, key)); }
		const_iterator find(key_type const &key) const			{ return const_iterator(this, find_index(*this, key)); }
		const_iterator find_acc(accessed_type const &key)		{ return const_iterator(this, find_index_acc(*this, key)); }
		const_iterator find_acc(accessed_type const &key) const	{ return const_iterator(this, find_index_acc(*this, key)); }
		const_iterator begin()									{ return const_iterator(this, 0, true); }
		const_iterator begin() const							{ return const_iterator(this, 0, true); }
		const_iterator cbegin() const							{ return const_iterator(this, 0, true); }
		const_iterator end()									{ return const_iterator(this, this->m_vector.size()); }
		const_iterator end() const								{ return const_iterator(this, this->m_vector.size()); }
		const_iterator cend() const								{ return const_iterator(this, this->m_vector.size()); }
		kv_type const &operator[](size_type const idx) const	{ return this->m_vector[idx]; }
		
		// Enable _keys variants if the container is a set-type. For load and serialize,
		// require enable_serialize since implementing the required function in value_type
		// and possibly access_key_fn might not be trivial.
		
		template <typename Fn, bool t_dummy = true>
		auto serialize_keys(
			std::ostream &out,
			Fn value_callback,
			sdsl::structure_tree_node *v,
			std::string name
		) const -> typename std::enable_if <trait_type::is_map_type && t_dummy, std::size_t>::type;
	
		template <bool t_dummy = true>
		auto serialize(
			std::ostream &out,
			sdsl::structure_tree_node *v = nullptr,
			std::string name = ""
		) const -> typename std::enable_if <t_spec::enable_serialize && t_dummy, std::size_t>::type;
	
		template <typename Fn, bool t_dummy = true>
		auto load_keys(
			std::istream &in,
			Fn value_callback
		) -> typename std::enable_if <trait_type::is_map_type && t_dummy>::type;
			
		template <bool t_dummy = true>
		auto load(std::istream &in) -> typename std::enable_if <t_spec::enable_serialize && t_dummy>::type;
	};


	template <typename t_spec, typename t_map, typename t_access_value_fn = detail::map_adaptor_phf_access_value <t_spec>>
	class map_adaptor_phf_builder
	{
	protected:
		template <typename> struct post_init_map;
		template <typename> friend struct post_init_map;

	public:
		typedef map_adaptor_phf <t_spec>						adaptor_type;
		typedef typename adaptor_type::trait_type				trait_type;
		typedef typename adaptor_type::key_type					key_type;
		typedef typename adaptor_type::value_type				value_type;
		typedef typename adaptor_type::kv_type					kv_type;
		typedef typename adaptor_type::allocator_type			allocator_type;
		typedef typename adaptor_type::vector_type				vector_type;
		typedef typename adaptor_type::size_type				size_type;
		typedef typename adaptor_type::access_key_fn_type		access_key_fn_type;
		typedef typename adaptor_type::accessed_type			accessed_type;

		template <template <typename ...> class t_adaptor_map, typename t_hash, typename t_key_equal>
		using mutable_map_adaptor_type = typename adaptor_type::template mutable_map_adaptor_type <t_adaptor_map, t_hash, t_key_equal>;
		
	protected:
		phf_wrapper				m_phf;
		allocator_type			m_allocator;	// FIXME: always copy?
		access_key_fn_type		m_access_key_fn;	// FIXME: always copy?
		t_access_value_fn		m_access_value_fn;	// FIXME: add a constructor parameter for this.
		std::size_t				m_element_count{0};
		t_map					&m_map;
		
	public:
		template <typename t_vec>
		void fill_with_map_keys(t_vec &dst, t_map const &map, accessed_type &max_value)
		{
			typename t_vec::size_type i{0};
			for (auto const &kv : map)
			{
				auto const val(this->m_access_key_fn(trait_type::key(kv)));
				dst[i++] = val;
				max_value = std::max(max_value, val);
			}
		}
		
		std::size_t element_count() const
		{
			return m_element_count;
		}
		
		phf_wrapper &phf() { return m_phf; }
		allocator_type &allocator() { return m_allocator; }
		t_map &map() { return m_map; }
		access_key_fn_type &access_key_fn() { return m_access_key_fn; }
		t_access_value_fn &access_value_fn() { return m_access_value_fn; }

		map_adaptor_phf_builder();
		explicit map_adaptor_phf_builder(t_map &map);
		map_adaptor_phf_builder(t_map &map, allocator_type const &allocator);
		map_adaptor_phf_builder(t_map &map, access_key_fn_type const &access_key_fn);
		map_adaptor_phf_builder(
			t_map &map,
			access_key_fn_type const &access_key_fn,
			allocator_type const &allocator
		);
	
	protected:
		void post_init(t_map &map, bool const call_post_init_map, bool const create_allocator);

		template <typename T>
		struct post_init_map
		{
			void operator()(map_adaptor_phf_builder &builder, T const &) {}
		};

		template <template <typename ...> class t_adaptor_map, typename t_hash, typename t_key_equal>
		struct post_init_map <mutable_map_adaptor_type <t_adaptor_map, t_hash, t_key_equal>>
		{
			void operator()(
				map_adaptor_phf_builder &builder,
				mutable_map_adaptor_type <t_adaptor_map, t_hash, t_key_equal> const &adaptor
			)
			{
				builder.m_access_key_fn = adaptor.access_key_fn();
			}
		};
	};
	
	
	template <
		typename t_spec,
		template <typename ...> class t_map,
		typename t_key,
		typename t_val,
		typename t_access_key,
		typename t_hash,
		typename t_key_equal
	>
	auto operator==(
		map_adaptor_phf <t_spec> const &map1,
		map_adaptor <t_map, t_key, t_val, t_access_key, t_hash, t_key_equal> const &map2
	) -> typename std::enable_if <
		std::is_same <
			typename util::remove_ref_t <decltype(map1)>::mapped_type,
			typename util::remove_ref_t <decltype(map2)>::mapped_type
		>::value, bool
	>::type
	{
		/* FIXME: what about access_key? */
		typedef detail::map_adaptor_phf_trait <t_spec> trait_type;
		return trait_type::contains(map1, map2) && trait_type::contains(map2, map1);
	}
	
	template <
		typename t_spec,
		template <typename ...> class t_map,
		typename t_key,
		typename t_val,
		typename t_access_key,
		typename t_hash,
		typename t_key_equal
	>
	auto operator==(
		map_adaptor <t_map, t_key, t_val, t_access_key, t_hash, t_key_equal> const &map1,
		map_adaptor_phf <t_spec> const &map2
	) -> typename std::enable_if <
		std::is_same <
			typename util::remove_ref_t <decltype(map1)>::mapped_type,
			typename util::remove_ref_t <decltype(map2)>::mapped_type
		>::value, bool
	>::type
	{
		/* FIXME: what about access_key? */
		typedef detail::map_adaptor_phf_trait <t_spec> trait_type;
		return trait_type::contains(map1, map2) && trait_type::contains(map2, map1);
	}

	
	
	template <typename t_adaptor, typename t_it_val>
	void map_iterator_phf_tpl <t_adaptor, t_it_val>::advance(difference_type n)
	{
		assert(m_adaptor);

		// Convert from vector index to object order and advance.
		auto const idx(m_idx - m_adaptor->m_used_indices_rank0_support(m_idx));
		auto const k(n + 1 + idx);
		auto const next(m_adaptor->m_used_indices_select1_support(k));
		m_idx = next;
	}
	
	
	template <typename t_adaptor, typename t_it_val>
	template <typename t_other_val>
	auto map_iterator_phf_tpl <t_adaptor, t_it_val>::distance_to(map_iterator_phf_tpl <t_adaptor, t_other_val> const &other) const -> difference_type
	{
		assert(m_adaptor == other.m_adaptor);
		return other.m_idx - m_idx;
	}
	
	
	template <typename t_adaptor, typename t_it_val>
	template <typename t_other_val>
	auto map_iterator_phf_tpl <t_adaptor, t_it_val>::equal(map_iterator_phf_tpl <t_adaptor, t_other_val> const &other) const -> bool
	{
		return (this->m_adaptor == other.m_adaptor &&
				this->m_idx == other.m_idx);
	}
	
	
	template <typename t_adaptor, typename t_it_val>
	auto map_iterator_phf_tpl <t_adaptor, t_it_val>::dereference() const -> t_it_val
	{
		// std::pair <key_type, mapped_type> apparently isn't convertible to
		// std::pair <key_type, mapped_type const>, so the set type container
		// returns a reference from m_vector and the map type container
		// constructs a pair the second element of which is a reference to the
		// mapped value.
		return t_adaptor::trait_type::dereference(m_adaptor->m_vector[m_idx]);
	}

	
	template <typename t_spec>
	template <typename t_adaptor>
	auto map_adaptor_phf <t_spec>::find_index(t_adaptor &adaptor, key_type const &key) -> size_type
	{
		auto const acc(adaptor.m_access_key_fn(key));
		return find_index_acc(adaptor, acc);
	}
	
	
	template <typename t_spec>
	template <typename t_adaptor>
	auto map_adaptor_phf <t_spec>::find_index_acc(t_adaptor &adaptor, accessed_type const &acc) -> size_type
	{
		// In case the user passes a non-existent key, the hash function may return a colliding value.
		// To alleviate this, check both that a value has been stored for the adapted key and that the
		// found value matches the given one.
		auto const adapted_key(adaptor.adapted_key(acc));
		if (adapted_key < adaptor.m_vector.size() && adaptor.m_used_indices[adapted_key])
		{
			auto const found_acc(adaptor.m_access_key_fn(trait_type::key(adaptor.m_vector[adapted_key])));
			if (found_acc == acc)
				return adapted_key;
		}
			
		return adaptor.m_vector.size();
	}
	
	
	template <typename t_spec>
	map_adaptor_phf <t_spec>::map_adaptor_phf(map_adaptor_phf const &other):
		base_class(other),
		m_used_indices_rank0_support(other.m_used_indices_rank0_support),
		m_used_indices_select1_support(other.m_used_indices_select1_support)
	{
		m_used_indices_rank0_support.set_vector(&this->m_used_indices);
		m_used_indices_select1_support.set_vector(&this->m_used_indices);
	}
	
	
	template <typename t_spec>
	template <template <typename> class t_new_allocator>
	map_adaptor_phf <t_spec>::map_adaptor_phf(rebind_allocator <t_new_allocator> const &other):
		base_class(other),
		m_used_indices_rank0_support(other.m_used_indices_rank0_support),
		m_used_indices_select1_support(other.m_used_indices_select1_support)
	{
		m_used_indices_rank0_support.set_vector(&this->m_used_indices);
		m_used_indices_select1_support.set_vector(&this->m_used_indices);
	}
	
	
	template <typename t_spec>
	map_adaptor_phf <t_spec>::map_adaptor_phf(map_adaptor_phf &&other):
		base_class(std::move(other)),
		m_used_indices_rank0_support(std::move(other.m_used_indices_rank0_support)),
		m_used_indices_select1_support(std::move(other.m_used_indices_select1_support))
	{
		m_used_indices_rank0_support.set_vector(&this->m_used_indices);
		m_used_indices_select1_support.set_vector(&this->m_used_indices);
	}


	template <typename t_spec>
	auto map_adaptor_phf <t_spec>::operator=(map_adaptor_phf const &other) & -> map_adaptor_phf &
	{
		if (&other != this)
		{
			base_class::operator=(other);
			m_used_indices_rank0_support = other.m_used_indices_rank0_support;
			m_used_indices_rank0_support.set_vector(&this->m_used_indices);
			m_used_indices_select1_support = other.m_used_indices_select1_support;
			m_used_indices_select1_support.set_vector(&this->m_used_indices);
		}
		return *this;
	}


	template <typename t_spec>
	auto map_adaptor_phf <t_spec>::operator=(map_adaptor_phf &&other) & -> map_adaptor_phf &
	{
		if (&other != this)
		{
			base_class::operator=(std::move(other));
			m_used_indices_rank0_support = std::move(other.m_used_indices_rank0_support);
			m_used_indices_rank0_support.set_vector(&this->m_used_indices);
			m_used_indices_select1_support = std::move(other.m_used_indices_select1_support);
			m_used_indices_select1_support.set_vector(&this->m_used_indices);
		}
		return *this;
	}
	
	
	template <typename t_spec>
	auto map_adaptor_phf <t_spec>::serialize_common(
		std::ostream &out,
		sdsl::structure_tree_node *child
	) const -> size_type
	{
		std::size_t written_bytes(0);
		
		written_bytes += this->m_phf.serialize(out, child, "phf");
		written_bytes += this->m_used_indices.serialize(out, child, "used_indices");
		written_bytes += this->m_access_key_fn.serialize(out, child, "access_key");
		written_bytes += m_used_indices_rank0_support.serialize(out, child, "used_indices_r0_support");
		written_bytes += m_used_indices_select1_support.serialize(out, child, "used_indices_s1_support");
		written_bytes += sdsl::write_member(this->m_size, out, child, "size");
		
		return written_bytes;
	}
	
	
	template <typename t_spec>
	void map_adaptor_phf <t_spec>::load_common(std::istream &in)
	{
		this->m_phf.load(in);
		this->m_used_indices.load(in);
		this->m_access_key_fn.load(in);
		m_used_indices_rank0_support.load(in);
		m_used_indices_select1_support.load(in);
		sdsl::read_member(this->m_size, in);
		
		m_used_indices_rank0_support.set_vector(&this->m_used_indices);
		m_used_indices_select1_support.set_vector(&this->m_used_indices);
		
		this->m_vector.resize(this->m_used_indices.size() - 1);
	}
	
	
	template <typename t_spec>
	template <typename Fn, bool t_dummy>
	auto map_adaptor_phf <t_spec>::serialize_keys(
		std::ostream &out,
		Fn value_callback,
		sdsl::structure_tree_node *v,
		std::string name
	) const -> typename std::enable_if <trait_type::is_map_type && t_dummy, std::size_t>::type
	{
		auto *child(sdsl::structure_tree::add_child(v, name, sdsl::util::class_name(*this)));
		size_type written_bytes(0);

		written_bytes += serialize_common(out, child);
		written_bytes += trait_type::serialize_keys(this->m_vector, this->m_used_indices, value_callback, out, child);
		
		sdsl::structure_tree::add_size(child, written_bytes);
		return written_bytes;
	}
	
	
	template <typename t_spec>
	template <bool t_dummy>
	auto map_adaptor_phf <t_spec>::serialize(
		std::ostream &out,
		sdsl::structure_tree_node *v,
		std::string name
	) const -> typename std::enable_if <t_spec::enable_serialize && t_dummy, std::size_t>::type
	{
		auto *child(sdsl::structure_tree::add_child(v, name, sdsl::util::class_name(*this)));
		size_type written_bytes(0);

		written_bytes += serialize_common(out, child);
		written_bytes += trait_type::serialize(this->m_vector, this->m_used_indices, out, child);
		
		sdsl::structure_tree::add_size(child, written_bytes);
		return written_bytes;
	}
	
	
	template <typename t_spec>
	template <typename Fn, bool t_dummy>
	auto map_adaptor_phf <t_spec>::load_keys(
		std::istream &in,
		Fn value_callback
	) -> typename std::enable_if <trait_type::is_map_type && t_dummy>::type
	{
		load_common(in);
		trait_type::load_keys(this->m_vector, this->m_used_indices, value_callback, in);
	}
	
	
	template <typename t_spec>
	template <bool t_dummy>
	auto map_adaptor_phf <t_spec>::load(
		std::istream &in
	) -> typename std::enable_if <t_spec::enable_serialize && t_dummy>::type
	{
		load_common(in);
		trait_type::load(this->m_vector, this->m_used_indices, in);
	}
	
	
	template <typename t_spec>
	template <
		typename t_map,
		typename std::enable_if <false == detail::is_map_adaptor_phf <t_map>::value> *
	>
	map_adaptor_phf <t_spec>::map_adaptor_phf(t_map &map):
		map_adaptor_phf()
	{
		map_adaptor_phf_builder <t_spec, t_map> builder(map);
		*this = map_adaptor_phf(builder);
	}
	
	
	template <typename t_spec>
	template <typename t_map, typename t_access_value_fn>
	map_adaptor_phf <t_spec>::map_adaptor_phf(map_adaptor_phf_builder <t_spec, t_map, t_access_value_fn> &builder):
		map_adaptor_phf(
			builder.map(),
			builder.element_count(),
			builder.phf(),
			builder.allocator(),
			builder.access_key_fn(),
			builder.access_value_fn()
		)
	{
	}
	
	
	template <typename t_spec>
	template <typename t_map, typename t_access_value_fn>
	map_adaptor_phf <t_spec>::map_adaptor_phf(
		t_map &map,
		std::size_t size,
		phf_wrapper &phf,
		allocator_type const &alloc,
		access_key_fn_type const &access_key_fn,
		t_access_value_fn &access_value_fn
	): base_class(phf, size, alloc, access_key_fn)
	{
		if (size)
		{
			// Reserve the required space for m_used_indices.
			// Make select1(count) return a value that may be used in the end() itearator.
			{
				assert(size < 1 + size);
				decltype(this->m_used_indices) tmp(1 + size, 0);
				tmp[size] = 1;
				this->m_used_indices = std::move(tmp);
			}
			
			// Create the mapping. Since all possible values are known at this time,
			// PHF::hash will return unique values and no sublists are needed.
			// FIXME: since the template parameter of PHF::hash is independent of that of PHF::init,
			// the function may return incorrect hashes thus making its use rather error-prone.
			// For now, using the same type with PHF::init and adapted_key() needs to be ensured.
			{
				auto const map_size(map.size());
				for (auto &kv : map)
				{
					auto const key(this->m_access_key_fn(trait_type::key(kv)));
					auto const hash(this->adapted_key(key));
					assert(!this->m_used_indices[hash]);
					
					this->m_vector[hash] = std::move(access_value_fn(kv));
					this->m_used_indices[hash] = 1;
					++this->m_size;
				}
			}
			
			// Set up rank0 support.
			{
				decltype(m_used_indices_rank0_support) tmp(&this->m_used_indices);
				m_used_indices_rank0_support = std::move(tmp);
			}
			
			// Set up select1 support.
			{
				decltype(m_used_indices_select1_support) tmp(&this->m_used_indices);
				m_used_indices_select1_support = std::move(tmp);
			}
		}
	}
	
	
	template <typename t_spec, typename t_map, typename t_access_value_fn>
	map_adaptor_phf_builder <t_spec, t_map, t_access_value_fn>::map_adaptor_phf_builder(t_map &map):
		m_map(map)
	{
		post_init(map, true, true);
	}
	
	
	template <typename t_spec, typename t_map, typename t_access_value_fn>
	map_adaptor_phf_builder <t_spec, t_map, t_access_value_fn>::map_adaptor_phf_builder(
		t_map &map, allocator_type const &allocator
	):
		m_allocator(allocator),
		m_map(map)
	{
		post_init(map, true, false);
	}
	
	
	template <typename t_spec, typename t_map, typename t_access_value_fn>
	map_adaptor_phf_builder <t_spec, t_map, t_access_value_fn>::map_adaptor_phf_builder(
		t_map &map, access_key_fn_type const &access_key_fn
	):
		m_access_key_fn(access_key_fn),
		m_map(map)
	{
		post_init(map, false, true);
	}
	
	
	template <typename t_spec, typename t_map, typename t_access_value_fn>
	map_adaptor_phf_builder <t_spec, t_map, t_access_value_fn>::map_adaptor_phf_builder(
		t_map &map,
		access_key_fn_type const &access_key_fn,
		allocator_type const &allocator
	):
		m_allocator(allocator),
		m_access_key_fn(access_key_fn),
		m_map(map)
	{
		post_init(map, false, false);
	}


	template <typename t_spec, typename t_map, typename t_access_value_fn>
	void map_adaptor_phf_builder <t_spec, t_map, t_access_value_fn>::post_init(
		t_map &map, bool const call_post_init_map, bool const create_allocator
	)
	{
		if (0 == map.size())
			return;
		
		if (call_post_init_map)
		{
			post_init_map <t_map> pi;
			pi(*this, map);
		}

		// Make a copy of the keys to generate the hash function.
		// FIXME: check integrality of key_type.
		typedef typename vector_type::size_type key_type;
		accessed_type max_value(0);
		sdsl::int_vector <0> keys(map.size(), 0, std::numeric_limits <key_type>::digits);
		fill_with_map_keys(keys, map, max_value);
		
		// FIXME: consider the parameters below (4, 80, 0).
		std::size_t const loading_factor(80);
		int st(PHF::init <key_type, false>(
			&this->m_phf.get(), reinterpret_cast <key_type *>(keys.data()), keys.size(), 4, loading_factor, 0
		));
		assert(0 == st); // FIXME: throw instead on error. (Or do this in a wrapper for phf.)
		
		// Compact.
		PHF::compact(&this->m_phf.get());

		// If the hashed values take more space than the actual keys,
		// skip hashing.
		if (max_value < 1 + max_value && 1 + max_value <= m_phf.get().m)
		{
			m_element_count = 1 + max_value;
			m_phf = phf_wrapper();
		}
		else
		{
			m_element_count = m_phf.get().m;
		}

		if (create_allocator)
		{
			// FIXME: assumes that allocator can take the element count.
			m_allocator = std::move(allocator_type(element_count()));
		}
	}
}

#endif
