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

#ifndef ASM_LSW_KN_PATH_LABEL_MATCHER_HH
#define ASM_LSW_KN_PATH_LABEL_MATCHER_HH

#include <asm_lsw/matrix.hh>
#include <boost/format.hpp>
#include <cstdint>
#include <sdsl/cst_sada.hpp>
#include <sdsl/int_vector.hpp>
#include <sdsl/util.hpp>


namespace asm_lsw {

	template <typename t_cst, typename t_pattern_vector>
	class kn_path_label_matcher
	{
	public:
		typedef t_cst cst_type;
		
		static_assert(
			std::is_unsigned <typename cst_type::node_type>::value,
			"Unsigned integer required for cst_type::node_type."
		);

	protected:
		cst_type const *m_cst;
		t_pattern_vector const *m_pattern;
		typename cst_type::node_type m_node;
		typename cst_type::node_type m_previous_match;
		matrix <0> m_e;
		uint8_t m_k{0};
		
	public:
		typedef decltype(m_e)::size_type	size_type;
		typedef decltype(m_e)::value_type	edit_distance_type;
		
	protected:
		class exact_cmp
		{
		protected:
			size_type			m_idx{0};
			bool				m_found_less{false};
			bool				m_found_exact{false};
			
		public:
			exact_cmp(edit_distance_type const k)
			{
			}
			
			void operator()(edit_distance_type const cost, edit_distance_type const k, size_type const idx)
			{
				if (cost < k)
					m_found_less = true;
				else if (cost == k)
				{
					m_found_exact = true;
					m_idx = idx;
				}
			}
			
			bool found() const { return m_found_exact; }
			size_type index() const { assert(m_found_exact); return m_idx; }
			edit_distance_type cost(edit_distance_type const k) const { assert(m_found_exact); return k; }
			bool can_continue_branch() const { assert(!m_found_exact); return m_found_less; }
			
			// Once a match has been located, no reason to go through the remaining subtrees.
			constexpr bool can_prune() const { return true; }
		};
		
		
		class lte_cmp
		{
		protected:
			edit_distance_type	m_min_cost{0};
			size_type			m_idx{0};
			bool				m_found{false};
			
		public:
			lte_cmp(edit_distance_type const k):
				m_min_cost(1 + k)
			{
			}
			
			void operator()(edit_distance_type const cost, edit_distance_type const k, size_type const idx)
			{
				if (cost <= m_min_cost)
				{
					m_found = true;
					m_min_cost = cost;
					m_idx = idx;
				}
			}

			bool found() const { return m_found; }
			size_type index() const { return m_idx; }
			edit_distance_type cost(edit_distance_type const k) const { assert(m_found); return m_min_cost; }
			
			// No need to continue in the current branch.
			constexpr bool can_continue_branch() const { return false; }
			
			// Any of the subtrees may contain a better match.
			constexpr bool can_prune() const { return false; }
		};
		
	protected:
		void reset(bool allocate_matrix)
		{
			auto const patlen(m_pattern->size());
			auto const patlen_log2(sdsl::util::log2_ceil(1 + patlen));
			auto const patlen_bits(sdsl::util::upper_power_of_2(patlen_log2));
			auto const bits(std::min <decltype(patlen_bits)>(8, patlen_bits));
			auto const default_cost(decltype(m_e)::max_value(bits));
			
			if (allocate_matrix)
			{
				// 2k + 1 row entries to be filled in each column, two additional for sentinels.
				decltype(patlen) const effective_rows(3 + 2 * m_k);
				auto const rows(std::min(effective_rows, 1 + patlen));
				decltype(m_e) e(rows, 1 + patlen + m_k, bits, default_cost);
				
				m_e = std::move(e);
			}
			else
			{
				m_e.fill(default_cost);
			}
			
			m_node = m_cst->child(m_cst->root(), 0);
			m_previous_match = m_cst->root();
			
			// Start with zero.
			m_e(0, 0) = 0;
			
			// Fill the first column of the matrix.
			size_type i(1), nrow(m_e.rows()), ncol(m_e.columns());
			while (i < nrow && i <= m_k)
			{
				// Indel cost is 1.
				m_e(i, 0) = i;
				++i;
			}
			
			// Fill the first row.
			i = 1;
			while (i < ncol && i <= m_k)
			{
				// Indel cost is 1.
				m_e(0, i) = i;
				++i;
			}
		}
		
		
		size_type column_pad(size_type column)
		{
			if (column < m_k + 1)
				return 0;
			else
				return column - m_k - 1;
		}
		
		
		size_type text_start(size_type column)
		{
			if (column < m_k + 1)
				return 0;
			else
				return column - m_k - 1;
		}
		
		
		// node_ref is inout.
		bool find_next_node(typename cst_type::node_type &node_ref) const
		{
			auto const root(m_cst->root());
			auto node(node_ref);
			assert(root != node);
			
			while (true)
			{
				auto const sibling(m_cst->sibling(node));
				if (root == sibling)
				{
					node = m_cst->parent(node);
					if (root == node)
						return false;
					
					continue;
				}

				node_ref = sibling;
				return true;
			}
		}

		
		// j is the column index.
		bool fill_column(size_type const j)
		{
			assert(j);
			auto const pad0(column_pad(j));
			auto const pad1(column_pad(j - 1));
			auto const ec(m_cst->edge(m_node, j)); // 1-based indexing.
			auto p_idx(text_start(j));
			
			auto const patlen(m_pattern->size());
			auto const max_entries(1 + 2 * m_k);
			typedef typename std::common_type <decltype(patlen), decltype(max_entries)>::type limit_type;
			auto const limit(std::min <limit_type>(max_entries + p_idx, patlen));

			edit_distance_type min_cost(std::numeric_limits <edit_distance_type>::max());
			
			while (p_idx < limit)
			{
				auto const pc((*m_pattern)[p_idx]);
				
				// Cost for both insertion and deletion is 1.
				auto const i(1 + p_idx);
				
				auto const       up(m_e(i - 1	- pad0,	j)		+ 1);
				auto const     left(m_e(i		- pad1,	j - 1)	+ 1);
				auto const diagonal(m_e(i - 1	- pad1,	j - 1)	+ (pc == ec ? 0 : 1));
				
				auto const cost(std::min({up, left, diagonal}));
				min_cost = std::min(cost, min_cost);
				m_e(i - pad0, j) = cost;
				
				++p_idx;
			}
			
			return (min_cost <= m_k);
		}
		
		
		template <typename t_cmp>
		bool next_path_label(
			typename cst_type::node_type &node_ref,
			typename cst_type::size_type &length_ref,
			edit_distance_type &cost_ref
		)
		{
			// Calculating a new column in m_e takes O(k) time (Lemma 23)
			// as there are entries on 2k + 1 rows at most.
			auto const patlen(m_pattern->size());
			auto const ncol(m_e.columns());
			
			while (true)
			{
				if (m_cst->root() == m_node)
					return false;
				
				if (!find_next_node(m_node))
				{
					m_node = m_cst->root();
					return false;
				}
				
				// parent is the LCA of the previous node and the current one.
				// m_e needs to be updated starting from its depth.
				auto const parent(m_cst->parent(m_node));
				auto const start(m_cst->depth(parent));
				auto depth(m_cst->depth(m_node));

				// Update the columns.
				auto const i(column_pad(1 + start));
				auto j(1 + start);
				while (true)
				{
					bool can_continue_branch(fill_column(j));
					
					// Make sure that the current branch is handled by checking leaves.
					if (j == ncol - 1 || !can_continue_branch || (j == depth && m_cst->is_leaf(m_node)))
					{
						// Check the last row, find the leftmost minimum or equal index.
						t_cmp cmp(m_k);
						
						// Find the rightmost column that may be read.
						size_type k(ncol - j - 1); // Not related to m_k.
						while (k < ncol)
						{
							auto const idx(ncol - k - 1);
							auto const pad(column_pad(idx));
							
							if (pad <= patlen)
								break;
							++k;
						}
						
						// Read until the leftmost column is reached.
						while (k < ncol)
						{
							auto const idx(ncol - k - 1);
							auto const pad(column_pad(idx));
							auto const last_entry_idx(pad + 1 + 2 * m_k);
							
							if (last_entry_idx < patlen)
								break;
							
							auto const cost(m_e(patlen - pad, idx));
							cmp(cost, m_k, idx);

							++k;
						}
						
						if (cmp.found())
						{
							// Find the lowest node that contains the index in question.
							auto node(m_node);
							while (true)
							{
								auto const parent(m_cst->parent(node));
								if (m_cst->depth(parent) < cmp.index())
									break;
								
								node = parent;
							}
							
							// In case of exact matching, the subtree may be skipped if
							// a suitable parent was found since a better match needn't
							// be searched.
							if (cmp.can_prune())
								m_node = node;
							
							if (node != m_previous_match)
							{
								m_previous_match = node;
								node_ref = node;
								length_ref = cmp.index();
								cost_ref = cmp.cost(m_k);
								return true;
							}
						}
						else if (!can_continue_branch || !cmp.can_continue_branch())
						{
							// If too many mismatches were found, continue in the outer loop.
							break;
						}
						
						// If sufficiently (or too) few mismatches were found, continue in this branch.
					}
							 
					if (j == depth)
					{
						auto const child(m_cst->select_child(m_node, 1));
						
						// If the current node is a leaf, continue in the
						// outer loop.
						if (m_cst->root() == child)
							break;
						
						m_node = child;
						depth = m_cst->depth(m_node);
					}
					
					++j;
				}
			}
		}


	public:
		cst_type const &cst() { return *m_cst; }

		
		kn_path_label_matcher(t_cst const &cst, t_pattern_vector const &pattern, uint8_t const k):
			m_cst(&cst),
			m_pattern(&pattern),
			m_k(k)
		{
			reset(true);
		}
		
		
		void reset()
		{
			reset(false);
		}
		
		
		bool next_path_label(
			typename cst_type::node_type &node_ref,
			typename cst_type::size_type &length_ref,
			edit_distance_type &cost_ref
		)
		{
			return next_path_label <lte_cmp>(node_ref, length_ref, cost_ref);
		}
		
		
		bool next_path_label_exact(
			typename cst_type::node_type &node_ref,
			typename cst_type::size_type &length_ref,
			edit_distance_type &cost_ref
		)
		{
			return next_path_label <exact_cmp>(node_ref, length_ref, cost_ref);
		}

		
		void print_matrix()
		{
			std::cout << std::endl;
			size_type const rows(m_e.rows()), cols(m_e.columns());

			std::cout << "  ";
			for (size_type i(0); i < cols; ++i)
				std::cout << boost::format(" %02u") % i;
			std::cout << std::endl;
			
			for (size_type i(0); i < rows; ++i)
			{
				std::cout << boost::format("%02u") % i;
				for (size_type j(0); j < cols; ++j)
				{
					auto const pad(column_pad(j));
					if (i < pad || pad + rows <= i)
						std::cout << "   ";
					else
						std::cout << boost::format(" %02u") % m_e(i - pad, j);
				}
				std::cout << std::endl;
			}
		}
	};
}

#endif
