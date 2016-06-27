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

#ifndef ASM_LSW_KN_MATCHER_HH
#define ASM_LSW_KN_MATCHER_HH

#include <asm_lsw/cst_edge_pattern_pair.hh>
#include <asm_lsw/k1_matcher.hh>
#include <asm_lsw/k1_matcher_helper.hh>
#include <asm_lsw/kn_path_label_matcher.hh>
#include <sdsl/csa_rao.hpp>
#include <sdsl/cst_sada.hpp>


namespace asm_lsw {

	template <typename t_cst>
	class kn_matcher
	{
	public:
		typedef t_cst									cst_type;
		typedef typename cst_type::csa_type				csa_type;
		typedef k1_matcher <cst_type>					k1_matcher_type;
		typedef cst_edge_adaptor <cst_type>				k1_pattern_type;
		typedef typename k1_matcher_type::csa_ranges	csa_ranges;
		typedef std::size_t								size_type;
		
		static_assert(
			std::is_unsigned <typename cst_type::node_type>::value,
			"Unsigned integer required for cst_type::node_type."
		);
		
	protected:
		template <typename t_pattern>
		class match_cb
		{
		protected:
			cst_type const			*m_cst{nullptr};
			k1_matcher_type const	*m_matcher{nullptr};
			t_pattern const			*m_pattern{nullptr};
			csa_ranges				*m_ranges{nullptr};
			uint8_t					m_k{0};
			
		public:
			match_cb(
				k1_matcher_type const &matcher,
				t_pattern const &pattern,
				csa_ranges &ranges,
				uint8_t k
			):
				m_cst(&matcher.cst()),
				m_matcher(&matcher),
				m_pattern(&pattern),
				m_ranges(&ranges),
				m_k(k)
			{
			}
			
			template <typename t_size>
			void partial_match(typename cst_type::node_type const &node, t_size match_length, t_size pattern_start)
			{
				// Concatenate the matched edge with the end of the pattern and try the k = 1 matcher.
				cst_edge_pattern_pair <cst_type, t_pattern> new_pattern(
					*m_cst, node, match_length, *m_pattern, pattern_start
				);

#if 0
				auto vec(new_pattern.operator std::vector <typename decltype(new_pattern)::value_type>());
				std::cout << "Trying pattern: ";
				std::copy(vec.cbegin(), vec.cend(), std::ostream_iterator <char>(std::cout, ""));
				std::cout << std::endl;
#endif
				
				m_matcher->find_1_approximate(new_pattern, *m_ranges);
			}
			
			template <typename t_cost>
			void complete_match(
				typename cst_type::node_type const &node,
				typename cst_type::size_type match_length,
				t_cost edit_distance
			)
			{
				assert(edit_distance < m_k);
				
				// A good enough match was found, report it.
				auto const lb(m_cst->lb(node));
				auto const rb(m_cst->rb(node));
				m_ranges->emplace_back(lb, rb);
				
				// Check if the path label (with k - 1 differences) may be used
				// to find additional matches with one difference.
				if (edit_distance == m_k - 1)
				{
					cst_edge_adaptor <t_cst> edge_adaptor(*m_cst, node, match_length);
					m_matcher->find_1_approximate(edge_adaptor, *m_ranges);
				}

			}
		};
		
	protected:
		cst_type const	*m_cst{nullptr};
		k1_matcher_type	m_matcher;
		
	public:
		kn_matcher() {}
		
		kn_matcher(cst_type const &cst, bool construct_matcher_ivars = true):
			m_cst(&cst),
			m_matcher(cst, construct_matcher_ivars)
		{
		}
		
		cst_type const &cst() { return *m_cst; }

		template <typename t_pattern>
		void find_approximate(t_pattern const &pattern, uint8_t k, csa_ranges &ranges) const;
		
		size_type serialize(std::ostream &out, sdsl::structure_tree_node *v = nullptr, std::string name = "") const;
		void load(std::istream &in);
	};
	
	
	template <typename t_cst>
	template <typename t_pattern>
	void kn_matcher <t_cst>::find_approximate(
		t_pattern const &pattern, uint8_t k, csa_ranges &ranges
	) const
	{
		assert(k);
		
		auto const pattern_length(pattern.size());
		
		if (1 == k)
		{
			// The k = 1 matcher may be used directly.
			m_matcher.find_1_approximate(pattern, ranges);
		}
		else
		{
			// Pass each of the matched paths to the k = 1 matcher (Theorem 3).
			typedef match_cb <t_pattern> match_cb_type;
			typedef kn_path_label_matcher <cst_type, t_pattern, match_cb_type> pl_matcher_type;
			pl_matcher_type path_label_matcher(*m_cst, pattern, k - 1);
			match_cb_type cb(m_matcher, pattern, ranges, k);
			
			path_label_matcher.find_approximate(cb);
		}
	}
	
	
	template <typename t_cst>
	auto kn_matcher <t_cst>::serialize(std::ostream &out, sdsl::structure_tree_node *v, std::string name) const -> size_type
	{
		auto *child(sdsl::structure_tree::add_child(v, name, sdsl::util::class_name(*this)));
		size_type written_bytes(0);

		written_bytes += m_matcher.serialize(out, child, name);
		
		sdsl::structure_tree::add_size(child, written_bytes);
		return written_bytes;
	}
	
	
	template <typename t_cst>
	void kn_matcher <t_cst>::load(std::istream &in)
	{
		m_matcher.load(in);
	}
}

#endif
