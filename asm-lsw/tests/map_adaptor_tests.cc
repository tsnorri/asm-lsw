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
#include <bandit/bandit.h>
#include <boost/format.hpp>
#include <typeinfo>

using namespace bandit;

template <typename t_adaptor, typename t_map>
void common_tests(t_adaptor const &adaptor, t_map const &test_values)
{
	it("can report its size", [&](){
		AssertThat(adaptor.size(), Equals(test_values.size()));
	});

	it("can find objects", [&](){
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
	});

	it("can iterate objects", [&](){
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
	});
}


template <typename t_adaptor, template <typename ...> class t_map, typename t_key, typename t_value>
void test_one_adaptor(t_adaptor &adaptor, t_map <t_key, t_value> const &test_values)
{
	describe(boost::str(boost::format("%s:") % typeid(adaptor).name()).c_str(), [&](){
		auto tv_copy(test_values);
		adaptor = std::move(t_adaptor(tv_copy));
		common_tests(adaptor, test_values);
	});
}


template <template <typename ...> class t_map, typename t_key, typename t_value>
void test_adaptors(t_map <t_key, t_value> const &test_values)
{
	{
		asm_lsw::map_adaptor <t_map, t_key, t_value> adaptor;
		test_one_adaptor(adaptor, test_values);
	}

	{
		asm_lsw::map_adaptor_phf <std::vector, t_key, t_value> adaptor;
		test_one_adaptor(adaptor, test_values);
	}
}


go_bandit([](){
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
		test_adaptors(map);
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
		test_adaptors(map);
	});
});
