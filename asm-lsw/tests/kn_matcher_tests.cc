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
#include <asm_lsw/kn_matcher.hh>
#include <asm_lsw/util.hh>
#include <bandit/bandit.h>
#include <sdsl/csa_rao.hpp>
#include <sdsl/csa_rao_builder.hpp>
#include <sdsl/isa_lsw.hpp>

using namespace bandit;


struct pattern_set
{
	std::string text;
	std::vector <std::string> patterns;
};


template <typename t_cst, typename t_ranges>
class path_label_matcher_cb {
	
protected:
	t_cst const	*m_cst{nullptr};
	t_ranges	*m_ranges{nullptr};
	
public:
	path_label_matcher_cb(t_cst const &cst, t_ranges &ranges):
		m_cst(&cst),
		m_ranges(&ranges)
	{
	}
	
	template <typename t_size>
	void partial_match(typename t_cst::node_type const &node, t_size match_length, t_size pattern_start) {}
	
	template <typename t_size, typename t_cost>
	void complete_match(typename t_cst::node_type const &node, t_size match_length, t_cost cost)
	{
		m_ranges->emplace_back(m_cst->lb(node), m_cst->rb(node));
	}
};



static uint8_t const min_k(1U);
static uint8_t const max_k(5U);


template <typename t_cst>
void typed_tests()
{
	typedef t_cst cst_type;
	
	std::vector <pattern_set> tests{
		{
			"alabar a la alabarda", {"alabar"}
		},

		{
			"i", {"ijj"}
		},
		
		{
			"mississippi", {"ippi", "iippi", "issi", "iissi"}
		},
		
		{
			"ixxixxxixxxxi", {"ixixxixxxi"}
		},

		{
			"ixxixxxiabc", {"iixxxiabc"}
		},

		{
			"ixxxixxxxiabc", {"iixxxxiabc"}
		},
		
		{
			"abracadabra", {"abra"}
		},
		
		{
			"alabar a la alabarda", {"ala", "alabar", "bard"}
		},
		
		{
			"vesihiisi sihisi hississx", {"hiisi", "sihisi"}
		},
		
		{
			"Entel tentel, Zwei Regimentel Gehn zu Tische, Fangen Fische, Zuckerkxnig los!", {"tentel", "Fische"}
		},
		
		{
			"Liru laru loru moni turha poru ratki riidaksi muuttuu", {"liru", "laru", "loru", "poru"}
		},
		
		{
			"CCAGGCGGGCTCGCCACGTCGGCTAATCCTGGTACATTTTGTAAACAATGTTCAGAAGAAAATTTGTGATAGAAGGACGAGTCACCGCGTACTAATAGCAACAACGATCGGCCGCACCATCCATTGTCGTGGTGACGCTCGGATTACACGGGAAAGGTGCTTGTGTCCCGACAGGCTAGGATATAATCCTGAGGCGTTACCCCAATCGTTCAGCGTGGGATTTGCTACAACTCCTGAGCGCTACATGTACGAAACCATGTTATGTATGCACAAGGCCGACAATAGGACGTAGCCTTGAAGTTAGTACGTAGCGTGGTCGCATAAGTACAGTAGATCCTCCCCGCGCATCCTATTTATTAAGTTAATTCTACAGCAATACGATCATATGCGGATCCGCAGTGGCCGGTAGACACACGTCTACCCCGCTGCTCAATGACCGGGACTAAAGAGGCGAAGATTATGGTGTGTGACCCGTTATGCTCGAGTTCGGTCAGAGCGTCATTGCGAGTAGTCGTTTGCTTTCTCAAACTCCGAGCGATTAAGCGTGACAGCCCCAGGGAACCCACAAAACGTGATCGCAGTCCATCCGATCATACACAGAAAGGAAGGTCCCCATACACCGACGCACCAGTTTACACGCCGTATGCATAAACGAGCCGCACGAACCAGAGAGCTTGAAGTGGACCTCTAGTTCCTCTACAAAGAACAGGTTGACCTGTCGCGAAGATGCCTTACCTAGATGCAATGACGGACGTATTCCTTTTGCCTCAACGGCTCCTGCTTTCGCTGAAATCCAAGACAGGCAACAGAAACCGCCTTTCGAAAGTGAGTCCTTCGTCTGTGACTAACTGTGCCAAATCGTCTTGCAAACTCCTAATCCAGTTTAACTCACCAAATTATAGCCATACAGACCCTAATTTCATATCATATCACGCGATTAGCCTCTGCTTAATTTCTGTGCTCAAGGGTTTTGGTCCGCCCGAGTGATGCTGCCAATTAG",
			{
				"GGCTCGCCACGTCGG",
				"GACGCTCGGATTACACGGGAAAGGTGC",
				"CGCGCATCCTATTTATTAAGTTAATTCTACAGCAAT",
				"AAGACAGGCAACAGAAACCGCCTTTCGAAAGTGAGTCCTTCGTCTGTGAC",
				"TATGGTGTGTGACCCGTTATGCTCGAGTTCGGTCAGAGCGTCATTGCGAGT",
				"TTTAACTCACCAAATTATAGCCATACAGACCCTAATTTCATATCATATCACGCGATTAGCCTCTGCTTAATTTCTGTGCTCAAGGGTTTTGG"
			}
		},
		
		{
			"GATACTTTGGTCATGTTTCCGTTGTAGGAGTGAACCCACTTGCCTTTGCGTCTTAATACCAATGAAAAACCTATGCACTTTGTACAGGGTACCATCGGGATTCTGAACCCTCAGATAGTGGGGATCCCGGGTATAGACCTTTATCTGCGGTCCAACTTAGGCATAAACCTCCATGCTACCTTGTCAGACCCACCCTGCACGAGGTAAATATGGGACGCGTCCGACCTGGCTCCTGGCGTTCTACGCCGCCACGTGTTCGTTAACTGTTGATTGGTAGCACAAAAGTAATACCATGGTCCTTGAAATTCGGCTCAGTTAGTTCGAGCGTAATGTCACAAATGGCGCAGAACGGCAATGAGTGTTTGACACTAGGTGGTGTTCAGTTCGGTAACGGAGAGACTGTGCGGCATACTTAATTATACATTTGAAACGCGCCCAAGTGACGCTAGGCAAGTCAGAGCAGGTTCCCGTGTTAGCTTAAGGGTAAACATACAAGTCGATTGAAGATGGGTAGGGGGCTTCAATTCGTCCAGCACTCTACGGTACCTCCGAGAGCAAGTAGGGCACCCTGTAGTTCGAAGCGGAACTATTTCGTGGGGCGAGCCCACATCGTCTCTTCTGCGGATGACTTAACACGTTAGGGAGGTGGAGTTGATTCGAACGATGGTTATAAATCAAAAAAACGGAACGCTGTCTGGAGGATGAATCTAACGGTGCGTAACTCGATCACTCACTCGCTATTCGAACTGCGCGAAAGTTCCCAGCGCTCATACACTTGGTTCCGAGGCCTGTCCTGATATATGAACCCAAACTAGAGCGGGGCTGTTGACGTTTGGAGTTGAAAAAATCTAATATTCCAATCGGCTTCAACGTGCACCACCGCAGGCGGCTGACGAGGGGCTCACACCGAGAAAGTAGACTGTTGCGCGTTGGGGGTAGCGCCGGCTAACAAAGACGCCTGGTACAGCAGGAGTATCAAACCCGTACAAAGGCAACATCC",
			{
				"CTTGGTTCCGAG",
				"AAATGGCGCAGAACGGC",
				"TTCCGAGGCCTGTCCTGATATATG",
				"AACGGAACGCTGTCTGGAGGATGAATCTAA",
				"ATAAATCAAAAAAACGGAACGCTGTCTGGAGG"
			}
		}
	};
	
	typedef asm_lsw::kn_matcher <cst_type> matcher_type;
	typedef path_label_matcher_cb <cst_type, typename matcher_type::csa_ranges> path_label_matcher_cb_type;
	typedef asm_lsw::kn_path_label_matcher <cst_type, std::string, path_label_matcher_cb_type> path_label_matcher_type;
	
	for (auto const &t : tests)
	{
		std::string input(t.text);
		std::string file("@test_input.iv8");
		sdsl::store_to_file(input.c_str(), file);
		
		cst_type cst;
		sdsl::construct(cst, file, 1);
		
#if 0
		{
			std::cout << "Suffixes: " << std::endl;
			sdsl::csXprintf(std::cout, "%T", cst.csa, '$');
			std::cout << std::endl;

			std::cout << "Children:" << std::endl;
			auto node_count(cst.nodes());
			for (decltype(node_count) i(0); i < node_count; ++i)
			{
				auto const inv_id(cst.bp_support.select(1 + i));
				if (!cst.is_leaf(inv_id))
				{
					std::cout << '\t' << inv_id << ':' << std::endl;
					auto const proxy(cst.children(inv_id));
					for (auto const k : proxy)
					{
						std::cout << "\t\t" << k << ":\t'";
						for (typename cst_type::size_type t(0), len(cst.depth(k)); t < len; ++t)
							std::cout << cst.edge(k, 1 + t);
						std::cout << '\'' << std::endl;
					}
				}
			}
			std::cout << std::endl;
		}
#endif
		
		matcher_type matcher(cst);
		
		for (auto const &pattern : t.patterns)
		{
			for (uint8_t k(min_k), k_limit(asm_lsw::util::min(pattern.size(), 1 + max_k)); k < k_limit; ++k)
			{
				describe((boost::format("k-differences with k = %d") % +k).str().c_str(), [&](){
					typename matcher_type::csa_ranges ranges, pl_ranges;
					
					{
						// Check with the path label matcher.
						path_label_matcher_type pl_matcher(cst, pattern, k);
						path_label_matcher_cb_type pl_cb(cst, pl_ranges);
						pl_matcher.find_approximate(pl_cb);
					}
					
					{
						// The former should be equal to the result of the k = n matcher.
						matcher.find_approximate(pattern, k, ranges);
					}
					
					asm_lsw::util::post_process_ranges(ranges);
					asm_lsw::util::post_process_ranges(pl_ranges);
					
					auto const name((boost::format("should report matches correctly (text: '%s' pattern: '%s'") % t.text % pattern).str());
					it(name.c_str(), [&](){
						AssertThat(ranges, Equals(pl_ranges));
					});
				});
			}
		}
	}
	
	{
		path_label_matcher_type dummy;
		dummy.print_e();
		dummy.print_p();
	}
}


go_bandit([](){
#if 0
	describe("k1_matcher <cst_sada <>>:", [](){
		typed_tests <sdsl::cst_sada <>>();
	});
#endif

	describe("k1_matcher <sdsl::cst_sada <sdsl::csa_rao <sdsl::csa_rao_spec <0, 0>>, sdsl::lcp_support_sada <>>>:", [](){
		typed_tests <sdsl::cst_sada <sdsl::csa_rao <sdsl::csa_rao_spec <0, 0>>, sdsl::lcp_support_sada <>>>();
	});
});
