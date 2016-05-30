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


#include <asm_lsw/kn_path_label_matcher.hh>
#include <bandit/bandit.h>
#include <sdsl/csa_rao.hpp>
#include <sdsl/csa_rao_builder.hpp>
#include <sdsl/isa_lsw.hpp>

using namespace bandit;


struct pattern_match
{
	std::size_t length;
	std::size_t edit_distance;
	std::size_t lb;
	std::size_t rb;
	
	bool operator==(pattern_match const &other) const
	{
		return (length == other.length &&
				edit_distance == other.edit_distance &&
				lb == other.lb &&
				rb == other.rb);
	}
	
	bool operator<(pattern_match const &other) const
	{
		if (length == other.length)
		{
			if (edit_distance == other.edit_distance)
			{
				if (lb == other.lb)
					return rb < other.rb;
				else
					return (lb < other.lb);
			}
			else
			{
				return (edit_distance < other.edit_distance);
			}
		}
		else
		{
			return (length < other.length);
		}
	}
};
	
	
typedef std::map <std::size_t, std::multiset <pattern_match>> match_set;


struct pattern
{
	std::string pattern;
	uint8_t edit_distance;
	match_set matches;
};


struct pattern_set
{
	std::string text;
	std::vector <pattern> patterns;
};


template <typename t_matcher, typename t_pattern>
void run_typed_test(t_matcher &matcher, std::string const &text, t_pattern const &p)
{
	typedef typename t_matcher::cst_type cst_type;
	
	cst_type const &cst(matcher.cst());
	match_set matches;

	typename cst_type::node_type node{};
	typename cst_type::size_type match_length{};
	typename t_matcher::size_type pattern_length{};
	typename t_matcher::edit_distance_type edit_distance{};
		
	while (matcher.next_path_label(node, pattern_length, match_length, edit_distance))
	{
		pattern_match match{match_length, edit_distance, cst.lb(node), cst.rb(node)};
		matches[pattern_length].emplace(std::move(match));
	}
		
	auto const name(boost::format("should report matches correctly (text: '%s' pattern: '%s' k: %d)") % text % p.pattern % +matcher.edit_distance());
	it(boost::str(name).c_str(), [&](){
		AssertThat(matches, Equals(p.matches));
	});
}


template <typename t_cst>
void typed_tests()
{
	std::vector <pattern_set> tests{
		{
			"i", {
				{"ijj", 2, {
					{
						3, {
							{1, 2, 1, 1}
						}
					}
				}},
			}
		},
		
		{
			"mississippi", {
				{"ippi", 2, {
					{
						2, {
							{0, 2, 0, 11},
							{0, 2, 0, 11},
							{0, 2, 0, 11}
						}
					},
					{
						3, {
							{1, 2, 1, 4},
							{1, 2, 1, 4}
						}
					},
					{
						4, {
							{4, 0, 2, 2},
							{4, 2, 3, 4},
							{2, 2, 6, 6},
							{3, 1, 7, 7},
							{5, 1, 8, 8},
							{6, 2, 10, 10}
						}
					}
				}},
				
				{"iippi", 2, {
					{
						2, {
							{0, 2, 0, 11}
						}
					},
					{
						3, {
							{1, 2, 1, 4},
							{1, 2, 1, 4},
							{2, 2, 5, 5},
							{1, 2, 6, 7},
							{2, 2, 8, 9}
						}
					},
					{
						5, {
							{4, 1, 2, 2},
							{7, 2, 3, 3},
							{3, 2, 7, 7},
							{5, 1, 8, 8},
							{6, 2, 10, 10}
						}
					}
				}},
				
				{"issi", 2, {
					{
						1, {
							{1, 0, 1, 4}
						},
					},
					{
						2, {
							{0, 2, 0, 11},
							{1, 1, 8, 11}
						}
					},
					{
						3, {
							{1, 2, 1, 4}
						}
					},
					{
						4, {
							{4, 2, 2, 2},
							{4, 0, 3, 4},
							{5, 1, 5, 5},
							{2, 2, 8, 9},
							{5, 1, 9, 9},
							{3, 1, 10, 11}
						}
					}
				}},
				
				{"iissi", 2, {
					{
						2, {
							{1, 1, 1, 4},
							{0, 2, 0, 11}
						}
					},
					{
						3, {
							{1, 2, 1, 4},
							{2, 2, 6, 6},
							{1, 2, 8, 11},	// Everything that begins with the single letter 's'
							{1, 2, 8, 11}
						}
					},
					{
						5, {
							{4, 1, 3, 4},	// Delete one 'i' from the beginning
							{5, 1, 5, 5},	// Substitute the first 'i' with an 'm'
							{5, 1, 9, 9},	// Substitute the first 'i' with an 's'
							{3, 2, 10, 11}	// Delete two 'i' from the beginning
						}
					}
				}}
			}
		}
	};
	
	typedef t_cst cst_type;
	typedef asm_lsw::kn_path_label_matcher <cst_type, std::string, true> matcher_type;
	
	for (auto const &t : tests)
	{
		std::string input(t.text);
		std::string file("@test_input.iv8");
		sdsl::store_to_file(input.c_str(), file);
		
		cst_type cst;
		sdsl::construct(cst, file, 1);

		for (auto const &p : t.patterns)
		{
			matcher_type matcher(cst, p.pattern, p.edit_distance);
			run_typed_test(matcher, t.text, p);
		}
	}
}


go_bandit([](){
	describe("k1_matcher <cst_sada <>>:", [](){
		typed_tests <sdsl::cst_sada <>>();
	});

	describe("k1_matcher <sdsl::cst_sada <sdsl::csa_rao <sdsl::csa_rao_spec <0, 0>>, sdsl::lcp_support_sada <>>>:", [](){
		typed_tests <sdsl::cst_sada <sdsl::csa_rao <sdsl::csa_rao_spec <0, 0>>, sdsl::lcp_support_sada <>>>();
	});
});
