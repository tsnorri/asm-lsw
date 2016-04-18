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
#include <asm_lsw/util.hh>
#include <boost/format.hpp>
#include <cstdint>
#include <sdsl/cst_sada.hpp>
#include <sdsl/int_vector.hpp>
#include <sdsl/util.hpp>


namespace asm_lsw {

	template <
		typename t_cst,
		typename t_pattern_vector,
		bool t_match_pattern_prefix
	>
	class kn_path_label_matcher
	{
	public:
		typedef t_cst cst_type;
		typedef t_pattern_vector pattern_vector_type;
		
		static_assert(
			std::is_unsigned <typename cst_type::node_type>::value,
			"Unsigned integer required for cst_type::node_type."
		);

	protected:
		cst_type const *m_cst;
		pattern_vector_type const *m_pattern;
		typename cst_type::node_type m_node;
		typename cst_type::node_type m_previous_match;
		matrix <0> m_e;
		uint8_t m_k{0};
		
	public:
		typedef decltype(m_e)::size_type	size_type;
		typedef decltype(m_e)::value_type	edit_distance_type;
		
	protected:
		void reset(bool allocate_matrix)
		{
			assert(m_cst);
			
			auto const patlen(m_pattern->size());
			auto const bits(sdsl::util::log2_ceil(1 + patlen));
			auto const default_cost(decltype(m_e)::max_value(bits));
			
			if (allocate_matrix)
			{
 				// At most 2k + 1 row entries to be filled in each column, two additional for sentinels.
				decltype(patlen) const effective_rows(3 + 2 * m_k);
				auto const rows(std::min(effective_rows, 1 + patlen));
				decltype(m_e) e(rows, 1 + m_k + patlen, bits, default_cost);
				
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
		
		
		bool find_next_node(/* inout */ typename cst_type::node_type &node_ref) const
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
			auto const limit(util::min(max_entries + p_idx, patlen));

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
		
		
		bool compare_path_label(
			size_type const row,
			/* inout */ size_type &k0,
			/* out */ typename cst_type::node_type &node_ref,
			/* out */ typename cst_type::size_type &length_ref,
			/* out */ edit_distance_type &cost_ref
		)
		{
			auto const patlen(m_pattern->size());
			auto const ncol(m_e.columns());

			// Find the rightmost column that may be read.
			// Save k0 to spare some iterations in case
			// pattern ranges shorter than patlen are allowed.
			while (k0 < ncol)
			{
				auto const idx(ncol - k0 - 1);
				auto const pad(column_pad(idx));
		
				if (pad <= row)
					break;
				++k0;
			}
			
			// Check the given row, find the leftmost minimum or equal index.
			auto k(k0);

			// Read until the leftmost column is reached.
			while (k < ncol)
			{
				auto idx(ncol - k - 1);
				auto const pad(column_pad(idx));
				auto const last_entry_row(pad + 1 + 2 * m_k);
			
				if (last_entry_row < row)
					break;
			
				auto cost(m_e(row - pad, idx));
				if (cost <= m_k)
				{
					// Check if the ending character was matched. If this is the case,
					// take the previous character.
					auto const c(m_cst->edge(m_node, idx));
					if (0 == c)
					{
						--idx;
						auto const pad(column_pad(idx));
						auto new_cost(m_e(row - pad, idx));
						assert(new_cost <= cost);
						cost = new_cost;
					}
					
					// Find the lowest node that contains the index in question.
					auto node(m_node);
					auto const root(m_cst->root());
					while (true)
					{
						auto const parent(m_cst->parent(node));
						if (m_cst->depth(parent) < idx || node == root)
							break;
				
						node = parent;
					}
					
					// In case of prefix matching, report all matches as otherwise
					// the prefix length would need to be taken into account.
					if (t_match_pattern_prefix || node != m_previous_match)
					{
						m_previous_match = node;
						node_ref = node;
						length_ref = idx;
						cost_ref = cost;
						return true;
					}
					
					return false;
				}
				
				++k;
			}
			return false;
		}


	public:
		cst_type const &cst() const { return *m_cst; }
		uint8_t edit_distance() const { return m_k; }
		
		
		kn_path_label_matcher():
			m_cst(nullptr),
			m_k(0)
		{
		}
		
		
		kn_path_label_matcher(t_cst const &cst, pattern_vector_type const &pattern, uint8_t const k):
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
			typename cst_type::size_type &match_length_ref,
			edit_distance_type &cost_ref
		)
		{
			size_type pattern_length(0);
			return next_path_label(node_ref, pattern_length, match_length_ref, cost_ref);
		}

		
		bool next_path_label(
			/* out */ typename cst_type::node_type &node_ref,
			/* out */ size_type &pattern_length_ref,
			/* out */ typename cst_type::size_type &match_length_ref,
			/* out */ edit_distance_type &cost_ref
		)
		{
			assert(m_cst);

			// m_node is initialized to the node '$',
			// which would be the only one in case the tree were empty
			// and thus would not be handled.
			assert(m_cst->size());
			
			// Calculating a new column in m_e takes O(k) time (Lemma 23)
			// as there are entries on 2k + 1 rows at most.
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
						auto const patlen(m_pattern->size());
						
						// ncol - k0 - 1 equals to rightmost column index (updated in compare_path_label),
						// not related to m_k.
						size_type k0(ncol - j - 1);
						
						if (t_match_pattern_prefix)
						{
							// Don't check with an empty pattern (i.e. patlen - l - 1)
							// as this case is handled as a deletion.
							size_type l(0);
							while (l < patlen)
							{
								if (compare_path_label(patlen - l, k0, node_ref, match_length_ref, cost_ref))
								{
									pattern_length_ref = patlen - l;
									return true;
								}
								
								++l;
							}
							
							// If too many mismatches were found or if the pattern were shortened,
							// continue in the outer loop.
							break;
						}
						else
						{
							if (compare_path_label(patlen, k0, node_ref, match_length_ref, cost_ref))
							{
								pattern_length_ref = patlen;
								return true;
							}

							// If too many mismatches were found, continue in the outer loop.
							break;
						}
					} // if (j == ncol - 1 || !can_continue_branch || (j == depth && m_cst->is_leaf(m_node)))
							 
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
				} // while (true) [update columns]
			} // while (true) [find the next node]
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
