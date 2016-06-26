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


#include <asm_lsw/k1_matcher.hh>
#include <asm_lsw/k1_matcher_helper.hh>
#include <bandit/bandit.h>
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/stream.hpp>
#include <sdsl/csa_rao.hpp>
#include <sdsl/csa_rao_builder.hpp>

using namespace bandit;


template <typename t_range_set>
struct pattern_range
{
	std::string pattern;
	t_range_set ranges;
};


template <typename t_range_set>
struct input_pattern
{
	std::string input;
	std::vector <pattern_range <t_range_set>> patterns;
};


template <typename t_matcher, typename t_input_pattern>
void serialize(t_input_pattern const &ip, std::ostream &ostream, std::size_t const buffer_size)
{
	// Create the CST and the matcher.
	typename t_matcher::cst_type temp_cst;
	t_matcher temp_matcher(ip.input, temp_cst);
	
	// Serialize temp_cst and temp_matcher.
	auto const serialized_cst_size(temp_cst.serialize(ostream));
	auto const serialized_matcher_size(temp_matcher.serialize(ostream));
	
	assert(serialized_cst_size + serialized_matcher_size <= buffer_size);
}


template <typename t_matcher>
void load(typename t_matcher::cst_type &cst, t_matcher &matcher, std::istream &istream)
{
	// Load the CST.
	cst.load(istream);
	
	// Load the matcher into another variable and move.
	t_matcher temp_matcher_2(cst, false);
	temp_matcher_2.load(istream);
	matcher = std::move(temp_matcher_2);
}


template <typename t_matcher, bool t_serialize, typename t_input_pattern>
void typed_tests(t_input_pattern const &ip)
{
	describe(("input '" + ip.input + "'").c_str(), [&](){
		
		typename t_matcher::cst_type cst;
		t_matcher matcher;
		
		if (t_serialize)
		{
			// Back the stream with an array.
			std::size_t const buffer_size(16384);
			std::vector <char> buffer(buffer_size);
			
			boost::iostreams::basic_array <char> array(buffer.data(), buffer_size);
			boost::iostreams::stream <boost::iostreams::basic_array <char>> iostream(array);
			
			serialize <t_matcher>(ip, iostream, buffer_size);
			iostream.seekp(0);
			load(cst, matcher, iostream);
		}
		else
		{
			t_matcher temp_matcher(ip.input, cst);
			matcher = std::move(temp_matcher);
		}
		
		for (auto const &p : ip.patterns)
		{
			it(("reports results correctly for " + p.pattern).c_str(), [&](){
				sdsl::int_vector <0> pattern(p.pattern.size());
				{
					decltype(pattern)::size_type i(0);
					for (auto const c : p.pattern)
					{
						pattern[i] = c;
						++i;
					}
				}
			
				typename t_matcher::csa_ranges ranges;
				matcher.find_1_approximate(pattern, ranges);
				asm_lsw::util::post_process_ranges(ranges);
				
				AssertThat(ranges, Equals(p.ranges));
			});
		}
	});
}


template <typename t_cst, bool t_serialize>
void typed_tests()
{
	typedef asm_lsw::k1_matcher <t_cst> k1_matcher;
	typedef input_pattern <typename k1_matcher::csa_ranges> input_pattern;
	
	std::vector <input_pattern> input_patterns{
		{
			"abcdefg", {
				{"abcdefg",		{{1, 2}}},
				{"abcdefgh",	{{1, 1}}},
				{"abcefg",		{{1, 1}}},
				{"abcefgh",		{}}
			}
		},
		{
			"ababac", {
				{"abcbac",		{{1, 1}}},
				{"ababac",		{{1, 1}, {4, 4}}},
				{"ababacc",		{{1, 1}}}
			}
		},
		{
			"abracadabracadabra", {
				{"abra",		{{2, 4}, {9, 11}, {14, 15}}},
				{"aca",			{{5, 8}, {12, 13}, {17, 18}}}
			}
		},
		{
			"abbbaabbbab", {
				{"bbbb",		{{3, 4}, {9, 11}}},
				{"bbb",			{{3, 4}, {7, 11}}},
				{"bab",			{{1, 4}, {6, 11}}}
			}
		},
		{
			"mississippi", {
				{"miss",		{{3, 5}, {9, 9}}},
				{"issi",		{{3, 5}, {9, 11}}},
				{"sis",			{{3, 5}, {8, 11}}}
			}
		}
	};
	
	for (auto const &ip : input_patterns)
		typed_tests <k1_matcher, t_serialize>(ip);
}


go_bandit([](){
	describe("k1_matcher <cst_sada <>> (without serialization):", [](){
		typed_tests <sdsl::cst_sada <>, false>();
	});

	describe("k1_matcher <sdsl::cst_sada <sdsl::csa_rao <sdsl::csa_rao_spec <4, 0>>, sdsl::lcp_support_sada <>>> (without serialization):", [](){
		typed_tests <sdsl::cst_sada <sdsl::csa_rao <sdsl::csa_rao_spec <4, 0>>, sdsl::lcp_support_sada <>>, false>();
	});
	
	describe("k1_matcher <cst_sada <>> (with serialization):", [](){
		typed_tests <sdsl::cst_sada <>, true>();
	});

	describe("k1_matcher <sdsl::cst_sada <sdsl::csa_rao <sdsl::csa_rao_spec <4, 0>>, sdsl::lcp_support_sada <>>> (with serialization):", [](){
		typed_tests <sdsl::cst_sada <sdsl::csa_rao <sdsl::csa_rao_spec <4, 0>>, sdsl::lcp_support_sada <>>, true>();
	});
});
