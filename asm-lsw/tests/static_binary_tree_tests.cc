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
#include <asm_lsw/util.hh>
#include <bandit/bandit.h>
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/stream.hpp>
#include <iostream>

using namespace bandit;


template <typename t_key, typename t_value>
struct trait
{
	typedef std::map <t_key, t_value> ref_set_type;
	
	template <typename t_it>
	t_key key(t_it const &it) { return it->first; }
	
	template <typename T>
	std::pair <t_key const, t_value> transform_vector_val(T const &kv)
	{
		std::pair <t_key const, t_value> retval(kv.first, kv.second);
		return retval;
	}
	
	template <typename T, typename U>
	void compare(T const &val, U const &ref_val)
	{
		AssertThat(val.first, Equals(ref_val.first));
		AssertThat(val.second, Equals(ref_val.second));
	}
};


template <typename t_key>
struct trait <t_key, void>
{
	typedef std::set <t_key> ref_set_type;

	template <typename t_it>
	t_key key(t_it const &it) { return *it; }
	
	t_key const &transform_vector_val(t_key const &key)
	{
		return key;
	}
	
	template <typename T, typename U>
	void compare(T const &val, U const &ref_val)
	{
		AssertThat(val, Equals(ref_val));
	}
};


template <typename t_key, typename t_value, typename t_vec>
void static_binary_tree_tests(t_vec vec, t_key lower, t_key expected) // Copy.
{
	typedef trait <t_key, t_value> trait_type;
	trait_type trait;
	
	typename trait_type::ref_set_type ref_set;
	for (auto const &v : vec)
		ref_set.insert(v);
	auto const max_key(trait.key(ref_set.crbegin()));
	
	typedef asm_lsw::static_binary_tree <t_key, t_value, true> tree_type;
	auto const size(vec.size());
	tree_type tree(vec);
	
	it("preserves input vector size", [&](){
		AssertThat(vec.size(), Equals(size));
	});
	
	it("has working size()", [&](){
		AssertThat(tree.size(), Equals(size));
	});
	
	it("has working find()", [&](){
		for (t_key i(0); i < max_key; ++i)
		{
			auto const ref_it(ref_set.find(i));
			if (ref_set.cend() == ref_it)
			{
				AssertThat(tree.find(i), Equals(tree.cend()));
			}
			else
			{
				auto const it(tree.find(i));
				auto const val(*it);
				auto const ref_val(*ref_it);
				AssertThat(it, Is().Not().EqualTo(tree.cend()));
				trait.compare(val, ref_val);
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
			auto const val(vec[i]);
			auto const expected(trait.transform_vector_val(val));
			trait.compare(k, expected);
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
		std::size_t written_bytes(tree.serialize(iostream));
		assert(written_bytes <= buffer_size);
		AssertThat(tree.size(), Equals(size));
		
		tree_type tree2;
		iostream.seekp(0);
		tree2.load(iostream);
		AssertThat(tree2, Equals(tree));
	});
}


template <typename T>
void fill_map_1(std::vector <std::pair <T, T>> &out_vec)
{
	asm_lsw::util::remove_ref_t <decltype(out_vec)> vec{
		{0, 123},
		{2, 23},
		{8, 1},
		{12, 5},
		{15, 97},
		{17, 42},
		{22, 32},
		{31, 54},
		{55, 81},
		{87, 133},
		{99, 100},
		{127, 52},
		{200, 28}
	};
	out_vec = std::move(vec);
}


template <typename T>
void fill_map_2(std::vector <std::pair <T, T>> &out_vec)
{
	asm_lsw::util::remove_ref_t <decltype(out_vec)> vec{
		{0, 123},
		{2, 23},
		{8, 1},
		{12, 5},
		{15, 97},
		{17, 42},
		{22, 32},
		{31, 54},
		{55, 81},
		{87, 133},
		{99, 100},
		{127, 52},
		{200, 28},
		{201, 45}
	};
	out_vec = std::move(vec);
}


go_bandit([](){
	{
		sdsl::int_vector <0> vec({0, 2, 8, 12, 15, 17, 22, 31, 55, 87, 99, 127, 200});
		
		describe("static_binary_tree <uint32_t, void, true> (1):", [&](){
			static_binary_tree_tests <uint32_t, void>(vec, 9, 12);
		});
		
		describe("static_binary_tree <uint8_t, void, true> (1):", [&](){
			static_binary_tree_tests <uint8_t, void>(vec, 9, 12);
		});
	}
	
	{
		sdsl::int_vector <0> vec({0, 2, 8, 12, 15, 17, 22, 31, 55, 87, 99, 127, 200, 201});
		
		describe("static_binary_tree <uint32_t, void, true> (2):", [&](){
			static_binary_tree_tests <uint32_t, void>(vec, 9, 12);
		});
		
		describe("static_binary_tree <uint8_t, void, true> (2):", [&](){
			static_binary_tree_tests <uint8_t, void>(vec, 9, 12);
		});
	}
	
	{
		{
			describe("static_binary_tree <uint32_t, uint32_t, true> (1):", [&](){
				std::vector <std::pair <uint32_t, uint32_t>> vec;
				fill_map_1(vec);
				static_binary_tree_tests <uint32_t, uint32_t>(vec, 9, 12);
			});
		
			describe("static_binary_tree <uint8_t, uint8_t, true> (1):", [&](){
				std::vector <std::pair <uint8_t, uint8_t>> vec;
				fill_map_1(vec);
				static_binary_tree_tests <uint8_t, uint8_t>(vec, 9, 12);
			});
		}
	
		{
			describe("static_binary_tree <uint32_t, uint32_t, true> (2):", [&](){
				std::vector <std::pair <uint32_t, uint32_t>> vec;
				fill_map_2(vec);
				static_binary_tree_tests <uint32_t, uint32_t>(vec, 9, 12);
			});
		
			describe("static_binary_tree <uint8_t, uint8_t, true> (2):", [&](){
				std::vector <std::pair <uint8_t, uint8_t>> vec;
				fill_map_2(vec);
				static_binary_tree_tests <uint8_t, uint8_t>(vec, 9, 12);
			});
		}
	}
});
