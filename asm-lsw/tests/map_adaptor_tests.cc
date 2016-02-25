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


#include <asm_lsw/map_adaptors.hh>
#include <asm_lsw/pool_allocator.hh>
#include <asm_lsw/util.hh>
#include <bandit/bandit.h>
#include <boost/format.hpp>
#include <typeinfo>
#include <unordered_map>
#include <unordered_set>

using namespace bandit;


enum collection_kind
{
	map,
	set
};


struct pair
{
	uint32_t a;
	uint32_t b;
	
	bool operator==(pair const &other) const { return a == other.a && b == other.b; }
};


struct pair_access_key
{
	typedef uint32_t accessed_type;
	typedef pair key_type;
	
	accessed_type operator()(key_type const &pair) const { return pair.a; }
};


template <typename t_key, typename t_access_key = asm_lsw::map_adaptor_access_key <t_key>>
struct test_traits
{
	static_assert(std::is_same <t_key, typename t_access_key::key_type>::value, "");
	typedef t_access_key access_key;
	typedef typename access_key::accessed_type accessed_type;
	typedef typename asm_lsw::map_adaptor_hash <access_key, std::hash <accessed_type>> hash;
	typedef typename asm_lsw::map_adaptor_key_equal <access_key, std::equal_to <accessed_type>> key_equal;
};


template <typename t_access_key>
struct test_traits <pair, t_access_key>
{
	typedef pair_access_key access_key;
	typedef typename asm_lsw::map_adaptor_hash <access_key, std::hash <uint32_t>> hash;
	typedef typename asm_lsw::map_adaptor_key_equal <access_key, std::equal_to <uint32_t>> key_equal;
};


template <typename t_key, typename t_value, typename t_access_key = asm_lsw::map_adaptor_access_key <t_key>>
struct test_types
{
	typedef test_traits <t_key, t_access_key> trait_type;
	
	typedef std::unordered_set <
		t_key,
		typename trait_type::hash,
		typename trait_type::key_equal
	> unordered_set_type;
	
	typedef std::unordered_map <
		t_key,
		t_value,
		typename trait_type::hash,
		typename trait_type::key_equal
	> unordered_map_type;
};


template <typename, collection_kind>
struct test_specialization
{
};


// Tests for set-type containers.
template <typename t_map>
struct test_specialization <t_map, collection_kind::set>
{
	typedef void value_type;

	template <typename t_adaptor, typename t_set>
	void test_find(t_adaptor const &adaptor, t_set const &test_values)
	{
		auto tv_copy(test_values);
		for (auto const &kv : test_values)
		{
			auto const it(adaptor.find(kv));
			AssertThat(it, Is().Not().EqualTo(adaptor.cend()));
			AssertThat(*it, Equals(kv));
			tv_copy.erase(kv);
		}
		AssertThat(tv_copy.size(), Equals(0));
	}

	template <typename t_adaptor, typename t_set>
	void test_iterate(t_adaptor const &adaptor, t_set const &test_values)
	{
		auto tv_copy(test_values);
		for (auto const &kv : adaptor)
		{
			auto it(tv_copy.find(kv));
			AssertThat(it, Is().Not().EqualTo(tv_copy.end()));
			AssertThat(*it, Equals(kv));
			tv_copy.erase(kv);
		}
		AssertThat(tv_copy.size(), Equals(0));	
	}
	
	template <typename t_adaptor, typename t_set>
	void test_access_key(t_adaptor const &adaptor, t_set const &test_values)
	{
		// Assumed stateless.
		typename test_traits <typename t_adaptor::key_type>::access_key acc;
			
		auto tv_copy(test_values);
		for (auto const &kv : test_values)
		{
			auto acc_key(acc(kv));
			auto const it(adaptor.find_acc(acc_key));
			AssertThat(it, Is().Not().EqualTo(adaptor.cend()));
			AssertThat(*it, Equals(kv));
			tv_copy.erase(kv);
		}
		AssertThat(tv_copy.size(), Equals(0));
	}
};


// Tests for map-type containers.
template <typename t_tv_map>
struct test_specialization <t_tv_map, collection_kind::map>
{
	typedef typename t_tv_map::mapped_type value_type;

	template <typename t_adaptor, typename t_map>
	void test_find(t_adaptor const &adaptor, t_map const &test_values)
	{
		auto tv_copy(test_values);
		for (auto const &kv : test_values)
		{
			auto const it(adaptor.find(kv.first));
			AssertThat(it, Is().Not().EqualTo(adaptor.cend()));
			AssertThat(it->first, Equals(kv.first));
			AssertThat(it->second, Equals(kv.second));
			tv_copy.erase(kv.first);
		}
		AssertThat(tv_copy.size(), Equals(0));
	}

	template <typename t_adaptor, typename t_map>
	void test_iterate(t_adaptor const &adaptor, t_map const &test_values)
	{
		auto tv_copy(test_values);
		for (auto const &kv : adaptor)
		{
			auto it(tv_copy.find(kv.first));
			AssertThat(it, Is().Not().EqualTo(tv_copy.end()));
			AssertThat(it->first, Equals(kv.first));
			AssertThat(it->second, Equals(kv.second));
			tv_copy.erase(it);
		}
		AssertThat(tv_copy.size(), Equals(0));	
	}
	
	
	template <typename t_adaptor, typename t_map>
	void test_access_key(t_adaptor const &adaptor, t_map const &test_values)
	{
		// Not implemented.
	}
};


template <typename t_adaptor, typename t_map, typename t_test_specialization>
void common_tests(t_adaptor const &adaptor, t_map const &test_values, t_test_specialization &test_spec)
{
	it("can report its size", [&](){
		AssertThat(adaptor.size(), Equals(test_values.size()));
	});

	it("can find objects", [&](){
		test_spec.test_find(adaptor, test_values);
	});

	it("can iterate objects", [&](){
		test_spec.test_iterate(adaptor, test_values);
	});
}


template <typename t_adaptor, typename t_map, typename t_test_specialization>
void phf_tests(t_adaptor const &adaptor, t_map const &test_values, t_test_specialization &test_spec)
{
	it("can find objects with access_key", [&](){
		test_spec.test_access_key(adaptor, test_values);
	});
}


// Copy the test values and create one instance of each map adaptor. Then run the tests.
template <collection_kind t_collection_kind, template <typename ... > class t_map, class ... t_args>
void test_adaptors(t_map <t_args ...> const &test_values)
{
	test_specialization <t_map <t_args ...>, t_collection_kind> test_spec;
	typedef typename t_map <t_args ...>::key_type key_type;
	typedef typename decltype(test_spec)::value_type value_type;
	typedef typename test_traits <key_type>::access_key access_key_fn_type;

	{
		asm_lsw::map_adaptor <t_map, key_type, value_type, access_key_fn_type> adaptor;
		auto tv_copy(test_values);
		describe(boost::str(boost::format("%s:") % typeid(adaptor).name()).c_str(), [&](){
			adaptor = std::move(decltype(adaptor)(tv_copy));
			common_tests(adaptor, test_values, test_spec);
		});
	}

	{
		typedef asm_lsw::map_adaptor_phf_spec <
			std::vector, asm_lsw::pool_allocator, key_type, value_type, typename test_traits <key_type>::access_key
		> adaptor_spec;
		auto tv_copy(test_values);
		asm_lsw::map_adaptor_phf_builder <adaptor_spec, decltype(test_values)> builder(tv_copy);
		
		describe(boost::str(boost::format("%s:") % typeid(typename decltype(builder)::adaptor_type).name()).c_str(), [&](){
			typename decltype(builder)::adaptor_type adaptor(builder);
			common_tests(adaptor, test_values, test_spec);
			phf_tests(adaptor, test_values, test_spec);
		});
	}
}


go_bandit([](){
	describe("basic", [](){
		uint8_t const k_shift_amt(4);

		class access_key
		{
		protected:
			uint8_t m_shift_amt{0};

		public:
			typedef uint8_t key_type;
			typedef uint8_t accessed_type;
			
			access_key(uint8_t amt): m_shift_amt(amt) {}

			uint8_t shift_amt() const { return m_shift_amt; }

			accessed_type operator()(key_type const &key) const
			{
				AssertThat(m_shift_amt, Is().Not().EqualTo(0));
				auto retval(key >> m_shift_amt);
				return retval;
			}
		};

		access_key acc(k_shift_amt);
		AssertThat(acc(0x1f), Equals(0x1));

		typedef test_types <uint8_t, void, access_key>::unordered_set_type set_type;
		set_type::hasher hash(acc);
		set_type::key_equal key_equal(acc);
		set_type set({0x8, 0x11, 0x22, 0x33, 0x44, 0x57}, 6, hash, key_equal);

		asm_lsw::map_adaptor <std::unordered_set, uint8_t, void, access_key> adaptor(set, acc);

		it("should use access_key_fn", [&](){
			for (uint8_t i(0x0); i <= 0x5; ++i)
			{
				auto const it(adaptor.find(i * 0x10));
				AssertThat(it, Is().Not().EqualTo(adaptor.cend()));
			}
		});

		it("should propagate access_key_fn", [&](){
			asm_lsw::map_adaptor_phf_builder <
				asm_lsw::map_adaptor_phf_spec <std::vector, asm_lsw::pool_allocator, uint8_t, void, access_key>,
				decltype(adaptor)
			> builder(adaptor, acc);
			decltype(builder)::adaptor_type adaptor_phf(builder);
			AssertThat(adaptor_phf.access_key_fn().shift_amt(), Equals(k_shift_amt));
			for (uint8_t i(0x0); i <= 0x5; ++i)
			{
				auto const it(adaptor_phf.find(i * 0x10));
				AssertThat(it, Is().Not().EqualTo(adaptor_phf.cend()));
			}
		});
	});

	describe("std::unordered_set<uint8_t>:", [](){
		test_types <uint8_t, void>::unordered_set_type set{1, 5, 99, 33, 87, 107, 200};
		test_adaptors <collection_kind::set>(set);
	});

	describe("std::unordered_set<uint32_t>:", [](){
		test_types <uint32_t, void>::unordered_set_type set{100, 29813, 1938, 213193, 38921, 547385, 6598, 5462, 54325};
		test_adaptors <collection_kind::set>(set);
	});

	describe("std::unordered_map<uint8_t, uint8_t>:", [](){
		test_types <uint8_t, uint8_t>::unordered_map_type map{
			{1, 2},
			{5, 12},
			{99, 3},
			{33, 5},
			{87, 6},
			{107, 15},
			{200, 100}
		};
		test_adaptors <collection_kind::map>(map);
	});

	describe("std::unordered_map<uint32_t, uint32_t>:", [](){
		test_types <uint32_t, uint32_t>::unordered_map_type map{
			{100, 132},
			{29813, 2313},
			{1938, 982},
			{213193, 8392},
			{38921, 23},
			{547385, 3424},
			{6598, 42784},
			{5462, 49835},
			{54325, 1312}
		};
		test_adaptors <collection_kind::map>(map);
	});
	
	describe("std::unordered_set<pair>:", [](){
		test_types <pair, void>::unordered_set_type set{
			{100, 132},
			{29813, 2313},
			{1938, 982},
			{213193, 8392},
			{38921, 23},
			{547385, 3424},
			{6598, 42784},
			{5462, 49835},
			{54325, 1312}
		};
		test_adaptors <collection_kind::set>(set);
	});
});
