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

#include <asm_lsw/static_binary_tree.hh>
#include <bandit/bandit.h>
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/stream.hpp>
#include <iostream>

using namespace bandit;


template <typename t_value>
void static_binary_tree_tests(sdsl::int_vector <0> vec, t_value lower, t_value expected) // Copy.
{
	std::set <t_value> ref_set;
	for (auto const &v : vec)
		ref_set.insert(v);
	auto const max_value(*ref_set.crbegin());
	
	typedef asm_lsw::static_binary_tree <t_value> tree_type;
	auto const size(vec.size());
	tree_type tree(vec);
	
	it("preserves input vector size", [&](){
		AssertThat(vec.size(), Equals(size));
	});
	
	it("has working size()", [&](){
		AssertThat(tree.size(), Equals(size));
	});
	
	it("has working find()", [&](){
		for (t_value i(0); i < max_value; ++i)
		{
			if (ref_set.find(i) == ref_set.cend())
			{
				AssertThat(tree.find(i), Equals(tree.cend()));
			}
			else
			{
				auto const it(tree.find(i));
				auto const val(*it);
				AssertThat(it, Is().Not().EqualTo(tree.cend()));
				AssertThat(val, Equals(i));
			}
		}
	});
	
	it("has working lower_bound()", [&](){
		auto const it(tree.lower_bound(lower));
		AssertThat(it, Is().Not().EqualTo(tree.cend()));
		AssertThat(it, Equals(tree.find(expected)));
	});
	
	it("has working iterators", [&](){
		uint64_t i(0);
		uint64_t const count(vec.size());
		for (auto const &k : tree)
		{
			auto const expected(vec[i]);
			AssertThat(k, Equals(expected));
			++i;
		}
	});
	
	it("has working equality comparison operator", [&](){
		tree_type tree2;
		AssertThat(tree, Equals(tree));
		AssertThat(tree, Is().Not().EqualTo(tree2));
	});
	
	it("can serialize", [&](){
		std::size_t const buffer_size(4096);
		char buffer[buffer_size];

		boost::iostreams::basic_array <char> array(buffer, buffer_size);
		boost::iostreams::stream <boost::iostreams::basic_array <char>> iostream(array);
		
		auto const size(tree.size());
		tree.serialize(iostream);
		AssertThat(tree.size(), Equals(size));
		
		tree_type tree2;
		iostream.seekp(0);
		tree2.load(iostream);
		AssertThat(tree2, Equals(tree));
	});
}


go_bandit([](){
	{
		sdsl::int_vector <0> vec({0, 2, 8, 12, 15, 17, 22, 31, 55, 87, 99, 127, 200});
		
		describe("static_binary_tree <uint32_t> (1):", [&](){
			static_binary_tree_tests <uint32_t>(vec, 9, 12);
		});
		
		describe("static_binary_tree <uint8_t> (1):", [&](){
			static_binary_tree_tests <uint8_t>(vec, 9, 12);
		});
	}
	
	{
		sdsl::int_vector <0> vec({0, 2, 8, 12, 15, 17, 22, 31, 55, 87, 99, 127, 200, 201});
		
		describe("static_binary_tree <uint32_t> (2):", [&](){
			static_binary_tree_tests <uint32_t>(vec, 9, 12);
		});
		
		describe("static_binary_tree <uint8_t> (2):", [&](){
			static_binary_tree_tests <uint8_t>(vec, 9, 12);
		});
	}
});
