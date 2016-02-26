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
#include <boost/core/enable_if.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <sdsl/int_vector.hpp>
#include <sdsl/rank_support.hpp>
#include <sdsl/select_support.hpp>


namespace asm_lsw {

	template <template <typename ...> class, typename, typename, typename, typename, typename>
	class map_adaptor;
	
	template <typename t_spec, typename t_map>
	class map_adaptor_phf_builder;


	template <
		template <typename ...> class t_vector,
		template <typename> class t_allocator,
		typename t_key,
		typename t_val,
		typename t_access_key_fn = map_adaptor_access_key <t_key>
	>
	struct map_adaptor_phf_spec
	{
		template <typename ... Args> using vector_type = t_vector <Args ...>;
		template <typename ... Args> using allocator_type = t_allocator <Args ...>;
		typedef t_key key_type;
		typedef t_val value_type;
		typedef t_access_key_fn access_key_fn_type;
	};
	
	
	template <typename t_spec>
	class map_adaptor_phf_base
	{
	protected:
		typedef sdsl::bit_vector used_indices_type;
		
	public:
		typedef detail::map_adaptor_phf_trait <t_spec>							trait_type;
		typedef typename t_spec::key_type										key_type;
		typedef typename t_spec::value_type										value_type;
		typedef typename trait_type::kv_type									kv_type;
		typedef typename t_spec::template allocator_type <kv_type>				allocator_type;
		typedef typename t_spec::template vector_type <kv_type, allocator_type>	vector_type;
		typedef typename vector_type::size_type									size_type;
		typedef typename t_spec::access_key_fn_type								access_key_fn_type;
		
	protected:
		phf_wrapper m_phf;
		vector_type m_vector;
		used_indices_type m_used_indices;
		access_key_fn_type m_access_key_fn;
		size_type m_size{0};
		
	public:
		map_adaptor_phf_base() = default;
		map_adaptor_phf_base(map_adaptor_phf_base const &) = delete;
		map_adaptor_phf_base(map_adaptor_phf_base &&) = default;
		map_adaptor_phf_base(
			phf_wrapper &phf,
			allocator_type const &alloc,
			access_key_fn_type const &access_key_fn
		):
			m_phf(std::move(phf)),
			m_vector(m_phf.get().m, kv_type(), alloc),
			m_access_key_fn(access_key_fn)
		{
		}
		
		map_adaptor_phf_base &operator=(map_adaptor_phf_base const &) & = delete;
		map_adaptor_phf_base &operator=(map_adaptor_phf_base &&) & = default;
	};
	
	
	template <typename t_adaptor, typename t_it_val>
	class map_iterator_phf_tpl : public boost::iterator_facade <
		map_iterator_phf_tpl <t_adaptor, t_it_val>,
		t_it_val,
		boost::random_access_traversal_tag
	>
	{
		friend class boost::iterator_core_access;
		template <typename, typename> friend class map_iterator_phf_tpl;

	protected:
		typedef boost::iterator_facade <
			map_iterator_phf_tpl <t_adaptor, t_it_val>,
			t_it_val,
			boost::random_access_traversal_tag
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
			if (convert)
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
				boost::is_convertible<t_other_val *, t_it_val *>, enabler
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
		bool equal(map_iterator_phf_tpl <t_adaptor, t_other_val> const &other) const;
		
		t_it_val &dereference() const;
	};
	

	template <typename t_spec>
	class map_adaptor_phf : public map_adaptor_phf_base <t_spec>
	{
		template <typename, typename> friend class map_iterator_phf_tpl;
		template <typename, typename> friend class map_adaptor_phf_builder;

	protected:
		typedef map_adaptor_phf_base <t_spec> base_class;
		
	public:
		typedef	typename base_class::trait_type							trait_type;
		typedef typename base_class::key_type							key_type;
		typedef typename base_class::value_type							value_type;
		typedef typename base_class::kv_type							kv_type;
		typedef typename base_class::allocator_type						allocator_type;
		typedef typename base_class::vector_type						vector_type;
		typedef typename base_class::size_type							size_type;
		// If value_type is void, kv_type is the same as key_type.
		typedef map_iterator_phf_tpl <map_adaptor_phf, kv_type>			iterator;
		typedef map_iterator_phf_tpl <map_adaptor_phf, kv_type const>	const_iterator;
		
		template <typename t_map>
		using builder_type = map_adaptor_phf_builder <t_spec, t_map>;

		typedef typename base_class::access_key_fn_type					access_key_fn_type;
		typedef typename access_key_fn_type::accessed_type				accessed_type;

		template <template <typename ...> class t_map, typename t_hash, typename t_key_equal>
		using mutable_map_adaptor_type = map_adaptor <t_map, key_type, value_type, access_key_fn_type, t_hash, t_key_equal>;

		static_assert(
			std::is_same <typename access_key_fn_type::key_type, key_type>::value,
			"access_key_fn's operator() should take key_type as parameter"
		);

	protected:
		typename base_class::used_indices_type::rank_0_type m_used_indices_rank0_support;
		typename base_class::used_indices_type::select_1_type m_used_indices_select1_support;
		
	protected:
		typename vector_type::size_type adapted_key(typename vector_type::size_type key) const { 
			return PHF::hash(&this->m_phf.get(), key);
		}
		
		template <typename t_adaptor>
		static size_type find_index(t_adaptor &adaptor, key_type const &key);

		template <typename t_adaptor>
		static size_type find_index_acc(
			t_adaptor &adaptor,
			accessed_type const &key
		);

	public:
		map_adaptor_phf() = default;
		map_adaptor_phf(map_adaptor_phf const &) = delete;
		map_adaptor_phf(map_adaptor_phf &&other);
		
		template <typename t_map>
		explicit map_adaptor_phf(map_adaptor_phf_builder <t_spec, t_map> &builder);
		
		// Move the values in map to the one owned by this adaptor.
		template <typename t_map>
		map_adaptor_phf(
			t_map &map,
			phf_wrapper &phf,
			allocator_type const &alloc,
			access_key_fn_type const &access_key_fn
		);

		// Move the values in map to the one owned by this adaptor.
		// Create an allocator for this adaptor only.
		template <typename t_map>
		explicit map_adaptor_phf(t_map &map);
		
		map_adaptor_phf &operator=(map_adaptor_phf const &) & = delete;
		map_adaptor_phf &operator=(map_adaptor_phf &&other) &;
	
		vector_type const &map() const							{ return this->m_vector; }
		vector_type &map()										{ return this->m_vector; }
		size_type size() const									{ return this->m_size; }
		access_key_fn_type const &access_key_fn() const			{ return this->m_access_key_fn; }

		iterator find(key_type const &key)						{ return iterator(this, find_index(*this, key)); }
		const_iterator find(key_type const &key) const			{ return const_iterator(this, find_index(*this, key)); }
		iterator find_acc(accessed_type const &key)				{ return iterator(this, find_index_acc(*this, key)); }
		const_iterator find_acc(accessed_type const &key) const	{ return const_iterator(this, find_index_acc(*this, key)); }
		iterator begin()										{ return iterator(this, 0, true); }
		const_iterator begin() const							{ return const_iterator(this, 0, true); }
		const_iterator cbegin() const							{ return const_iterator(this, 0, true); }
		iterator end()											{ return iterator(this, this->m_vector.size()); }
		const_iterator end() const								{ return const_iterator(this, this->m_vector.size()); }
		const_iterator cend() const								{ return const_iterator(this, this->m_vector.size()); }
	};


	template <typename t_spec, typename t_map>
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

		template <template <typename ...> class t_adaptor_map, typename t_hash, typename t_key_equal>
		using mutable_map_adaptor_type = typename adaptor_type::template mutable_map_adaptor_type <t_adaptor_map, t_hash, t_key_equal>;
		
	protected:
		phf_wrapper				m_phf;
		allocator_type			m_allocator;	// FIXME: always copy?
		access_key_fn_type		m_access_key_fn;	// FIXME: always copy?
		t_map					&m_map;
		
	public:
		template <typename t_vec>
		void fill_with_map_keys(t_vec &dst, t_map const &map)
		{
			typename t_vec::size_type i{0};
			for (auto const &kv : map)
				dst[i++] = this->m_access_key_fn(trait_type::key(kv));
		}
		
		std::size_t element_count() const
		{
			return m_phf.get().m;
		}
		
		phf_wrapper &phf() { return m_phf; }
		allocator_type &allocator() { return m_allocator; }
		t_map &map() { return m_map; }
		access_key_fn_type &access_key_fn() { return m_access_key_fn; }

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
		return std::abs(m_idx - other.m_idx);
	}
	
	
	template <typename t_adaptor, typename t_it_val>
	template <typename t_other_val>
	auto map_iterator_phf_tpl <t_adaptor, t_it_val>::equal(map_iterator_phf_tpl <t_adaptor, t_other_val> const &other) const -> bool
	{
		return (this->m_adaptor == other.m_adaptor &&
				this->m_idx == other.m_idx);
	}
	
	
	template <typename t_adaptor, typename t_it_val>
	auto map_iterator_phf_tpl <t_adaptor, t_it_val>::dereference() const -> t_it_val &
	{
		return m_adaptor->m_vector[m_idx];
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
		auto const adapted_key(adaptor.adapted_key(acc));
		if (adapted_key < adaptor.m_vector.size())
		{
			auto const found_acc(adaptor.m_access_key_fn(trait_type::key(adaptor.m_vector[adapted_key])));
			if (found_acc == acc)
				return adapted_key;
		}
			
		return adaptor.m_vector.size();
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
	auto map_adaptor_phf <t_spec>::operator=(map_adaptor_phf &&other) & -> map_adaptor_phf &
	{
		base_class::operator=(std::move(other));
		m_used_indices_rank0_support = std::move(other.m_used_indices_rank0_support);
		m_used_indices_rank0_support.set_vector(&this->m_used_indices);
		m_used_indices_select1_support = std::move(other.m_used_indices_select1_support);
		m_used_indices_select1_support.set_vector(&this->m_used_indices);
		return *this;
	}


	template <typename t_spec>
	template <typename t_map>
	map_adaptor_phf <t_spec>::map_adaptor_phf(t_map &map):
		map_adaptor_phf()
	{
		map_adaptor_phf_builder <t_spec, t_map> builder(map);
		*this = map_adaptor_phf(builder);
	}
	
	
	template <typename t_spec>
	template <typename t_map>
	map_adaptor_phf <t_spec>::map_adaptor_phf(map_adaptor_phf_builder <t_spec, t_map> &builder):
		map_adaptor_phf(builder.map(), builder.phf(), builder.allocator(), builder.access_key_fn())
	{
	}
	
	
	template <typename t_spec>
	template <typename t_map>
	map_adaptor_phf <t_spec>::map_adaptor_phf(
		t_map &map,
		phf_wrapper &phf,
		allocator_type const &alloc,
		access_key_fn_type const &access_key_fn
	): base_class(phf, alloc, access_key_fn)
	{
		// Reserve the required space for m_used_indices.
		// Make select1(count) return a value that may be used in the end() itearator.
		auto const size(this->m_phf.get().m);
		
		{
			decltype(this->m_used_indices) tmp(1 + size, 0);
			tmp[size] = 1;
			this->m_used_indices = std::move(tmp);
		}
		
		// Create the mapping. Since all possible values are known at this time,
		// PHF::hash will return unique values and no sublists are needed.
		// FIXME: since the template parameter of PHF::hash is independent of that of PHF::init,
		// the function may return incorrect hashes thus making its use rather error-prone.
		// For now, using the same type with PHF::init and adapted_key() needs to be ensured.
		for (auto const &kv : map)
		{
			auto const key(this->m_access_key_fn(trait_type::key(kv)));
			auto const hash(this->adapted_key(key));
			assert(!this->m_used_indices[hash]);

			this->m_vector[hash] = std::move(trait_type::kv(kv));
			this->m_used_indices[hash] = 1;
			++this->m_size;
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
	
	
	template <typename t_spec, typename t_map>
	map_adaptor_phf_builder <t_spec, t_map>::map_adaptor_phf_builder(t_map &map):
		m_map(map)
	{
		post_init(map, true, true);
	}
	
	
	template <typename t_spec, typename t_map>
	map_adaptor_phf_builder <t_spec, t_map>::map_adaptor_phf_builder(t_map &map, allocator_type const &allocator):
		m_allocator(allocator),
		m_map(map)
	{
		post_init(map, true, false);
	}
	
	
	template <typename t_spec, typename t_map>
	map_adaptor_phf_builder <t_spec, t_map>::map_adaptor_phf_builder(t_map &map, access_key_fn_type const &access_key_fn):
		m_access_key_fn(access_key_fn),
		m_map(map)
	{
		post_init(map, false, true);
	}
	
	
	template <typename t_spec, typename t_map>
	map_adaptor_phf_builder <t_spec, t_map>::map_adaptor_phf_builder(
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


	template <typename t_spec, typename t_map>
	void map_adaptor_phf_builder <t_spec, t_map>::post_init(t_map &map, bool const call_post_init_map, bool const create_allocator)
	{
		if (call_post_init_map)
		{
			post_init_map <t_map> pi;
			pi(*this, map);
		}

		// Make a copy of the keys to generate the hash function.
		// FIXME: check integrality of key_type.
		typedef typename vector_type::size_type key_type;
		sdsl::int_vector <0> keys(map.size(), 0, std::numeric_limits <key_type>::digits);
		fill_with_map_keys(keys, map);
		
		// FIXME: consider the parameters below (4, 80, 0).
		int st(PHF::init <key_type, false>(
			&this->m_phf.get(), reinterpret_cast <key_type *>(keys.data()), keys.size(), 4, 80, 0
		));
		assert(0 == st); // FIXME: throw instead on error. (Or do this in a wrapper for phf.)
		
		// Compact.
		PHF::compact(&this->m_phf.get());

		if (create_allocator)
		{
			// FIXME: assumes that allocator can take the element count.
			m_allocator = std::move(allocator_type(element_count()));
		}
	}
}

#endif
