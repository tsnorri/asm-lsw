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
#include <bandit/bandit.h>

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
void typed_tests(t_input_pattern const &ip)
{
	describe(("input '" + ip.input + "'").c_str(), [&](){
		std::string file("@testinput.iv8");
		sdsl::store_to_file(ip.input.c_str(), file);
		
		typename t_matcher::cst_type cst;
		sdsl::construct(cst, file, 1);
		
		// Core path nodes
		typename t_matcher::core_nodes_type cn;
		t_matcher::construct_core_paths(cst, cn);

		// Gamma
		typename t_matcher::gamma_type gamma;
		t_matcher::construct_gamma_sets(cst, cn, gamma);
		
		// Core path endpoints
		typename t_matcher::core_endpoints_type ce;
		t_matcher::construct_core_path_endpoints(cst, cn, ce);
		
		// LCP RMQ
		typename t_matcher::lcp_rmq_type lcp_rmq;
		t_matcher::construct_lcp_rmq(cst, lcp_rmq);
		
		// H
		typename t_matcher::h_type h(cst, ce, lcp_rmq);
		
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
			
				typename t_matcher::f_type f(cst, pattern);

				typename t_matcher::csa_range_set ranges;
				t_matcher::find_1_approximate(cst, f, gamma, ce, lcp_rmq, h, pattern, ranges);
			
				AssertThat(ranges, Equals(p.ranges));
			});
		}
	});
}


template <typename t_cst>
void typed_tests()
{
	typedef asm_lsw::k1_matcher <t_cst> k1_matcher;
	typedef input_pattern <typename k1_matcher::csa_range_set> input_pattern;
	
	std::vector <input_pattern> input_patterns{
		{
			"abcdefg", {
				{"abcdefg",		{{1, 1}, {2, 2}}},
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
		}
	};
	
	for (auto const &ip : input_patterns)
		typed_tests <k1_matcher>(ip);
}


go_bandit([](){
	describe("k1_matcher <cst_sada <>>:", [](){
		typed_tests <sdsl::cst_sada <>>();
	});
});
