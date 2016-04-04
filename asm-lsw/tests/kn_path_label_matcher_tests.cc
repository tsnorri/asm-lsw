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


enum class match_type : int
{
	match_lte = 0,
	match_exact = 1
};


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


struct pattern
{
	std::string pattern;
	uint8_t edit_distance;
	match_type mt;
	std::set <pattern_match> matches;
};


struct pattern_set
{
	std::string text;
	std::vector <pattern> patterns;
};

template <typename t_matcher>
bool next_path_label(
	t_matcher &matcher,
	match_type const mt,
	typename t_matcher::cst_type::node_type &node,
	typename t_matcher::cst_type::size_type &length,
	typename t_matcher::edit_distance_type &edit_distance
)
{
	if (match_type::match_lte == mt)
		return matcher.next_path_label(node, length, edit_distance);
	else if (match_type::match_exact == mt)
		return matcher.next_path_label_exact(node, length, edit_distance);
	else
	{
		assert(0);
		return false;
	}
}


template <typename t_cst>
void typed_tests()
{
	typedef t_cst cst_type;
	
	std::vector <pattern_set> tests{
		{
			"i", {
				{"ijj", 2, match_type::match_lte, {
					{1, 2, 1, 1}
				}},
					
				{"ijj", 2, match_type::match_exact, {
					{1, 2, 1, 1}
				}}
			}
		},
		
		{
			"mississippi", {
				{"ippi", 2, match_type::match_lte, {
					{4, 0, 2, 2},
					{4, 2, 3, 4},
					{3, 1, 7, 7},
					{5, 1, 8, 8},
					{6, 2, 10, 10}
				}},
				
				{"ippi", 2, match_type::match_exact, {
					{2, 2, 2, 2},
					{4, 2, 3, 4},
					{2, 2, 7, 7},
					{4, 2, 8, 8},
					{6, 2, 10, 10}
				}},

				{"iippi", 2, match_type::match_lte, {
					{4, 1, 2, 2},
					{7, 2, 3, 3},
					{5, 1, 8, 8},
					{6, 2, 10, 10}
				}},
				
				{"iippi", 2, match_type::match_exact, {
					{3, 2, 2, 2},
					{7, 2, 3, 3},
					{4, 2, 8, 8},
					{6, 2, 10, 10}
				}},
				
				{"issi", 2, match_type::match_lte, {
					{4, 2, 2, 2},
					{4, 0, 3, 4},
					{5, 1, 5, 5},
					{2, 2, 8, 9},
					{5, 1, 9, 9},
					{3, 1, 10, 11}
				}},
				
				{"issi", 2, match_type::match_exact, {
					{4, 2, 2, 2},
					{2, 2, 3, 4},
					{4, 2, 5, 5},
					{2, 2, 8, 9},
					{2, 2, 10, 11}
				}},
				
				{"iissi", 2, match_type::match_lte, {
					{4, 1, 3, 4},
					{5, 1, 5, 5},
					{5, 1, 9, 9},
					{3, 2, 10, 11},
					{6, 2, 11, 11} // Actually overlaps the previous match.
				}},
				
				{"iissi", 2, match_type::match_exact, {
					{3, 2, 3, 4},
					{4, 2, 5, 5},
					{4, 2, 9, 9},
					{3, 2, 10, 11}
				}}
			}
		}
	};
	
	for (auto const &t : tests)
	{
		std::string input(t.text);
		std::string file("@test_input.iv8");
		sdsl::store_to_file(input.c_str(), file);
		
		cst_type cst;
		sdsl::construct(cst, file, 1);
		
		for (auto const &p : t.patterns)
		{
			typedef asm_lsw::kn_path_label_matcher <cst_type, std::string> matcher_type;
			matcher_type matcher(cst, p.pattern, p.edit_distance);
			std::set <pattern_match> matches;
			
			typename cst_type::node_type node{};
			typename cst_type::size_type length{};
			typename matcher_type::edit_distance_type edit_distance{};
			
			while (next_path_label(matcher, p.mt, node, length, edit_distance))
			{
				pattern_match match{length, edit_distance, cst.lb(node), cst.rb(node)};
				matches.emplace(std::move(match));
			}
			
			auto const name(boost::format("should report matches correctly (text: '%s' pattern: '%s' k: %d match type: %d)") % t.text % p.pattern % +p.edit_distance % int(p.mt));
			it(boost::str(name).c_str(), [&](){
				AssertThat(matches, Equals(p.matches));
			});
		}
	}
}


go_bandit([](){
	describe("k1_matcher <cst_sada <>>:", [](){
		typed_tests <sdsl::cst_sada <>>();
	});

	describe("k1_matcher <sdsl::cst_sada <sdsl::csa_rao <sdsl::csa_rao_spec <4, 0>>, sdsl::lcp_support_sada <>>>:", [](){
		typed_tests <sdsl::cst_sada <sdsl::csa_rao <sdsl::csa_rao_spec <4, 0>>, sdsl::lcp_support_sada <>>>();
	});
});