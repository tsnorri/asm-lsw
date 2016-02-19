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


template <typename, collection_kind>
struct test_specialization
{
};

template <typename t_value>
struct test_specialization <t_value, collection_kind::set>
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
};

template <typename t_value>
struct test_specialization <t_value, collection_kind::map>
{
	typedef t_value value_type;

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


template <collection_kind t_collection_kind, template <typename ...> class t_map, typename t_key, typename t_value>
void test_adaptors(t_map <t_key, t_value> const &test_values)
{
	test_specialization <t_value, t_collection_kind> test_spec;
	typedef typename decltype(test_spec)::value_type value_type;

	{
		asm_lsw::map_adaptor <t_map, t_key, value_type> adaptor;
		auto tv_copy(test_values);
		describe(boost::str(boost::format("%s:") % typeid(adaptor).name()).c_str(), [&](){
			adaptor = std::move(decltype(adaptor)(tv_copy));
			common_tests(adaptor, test_values, test_spec);
		});
	}

	{
		typedef asm_lsw::map_adaptor_phf_spec <std::vector, asm_lsw::pool_allocator, t_key, value_type> adaptor_spec;
		auto tv_copy(test_values);
		asm_lsw::map_adaptor_phf_builder <adaptor_spec, decltype(test_values)> builder(tv_copy);
		
		describe(boost::str(boost::format("%s:") % typeid(typename decltype(builder)::adaptor_type).name()).c_str(), [&](){
			typename decltype(builder)::adaptor_type adaptor(builder);
			common_tests(adaptor, test_values, test_spec);
		});
	}
}


go_bandit([](){
	describe("std::unordered_set<uint8_t>:", [](){
		std::unordered_set<uint8_t> set{1, 5, 99, 33, 87, 107, 200};
		test_adaptors <collection_kind::set>(set);
	});

	describe("std::unordered_set<uint32_t>:", [](){
		std::unordered_set<uint32_t> set{100, 29813, 1938, 213193, 38921, 547385, 6598, 5462, 54325};
		test_adaptors <collection_kind::set>(set);
	});

	describe("std::unordered_map<uint8_t, uint8_t>:", [](){
		std::unordered_map<uint8_t, uint8_t> map{
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
		std::unordered_map<uint32_t, uint32_t> map{
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
});
