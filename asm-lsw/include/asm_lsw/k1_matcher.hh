/*
 Copyright (c) 2015-2016 Tuukka Norri
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

#ifndef ASM_LSW_K1_MATCHER_HH
#define ASM_LSW_K1_MATCHER_HH

#include <asm_lsw/bp_support_sparse.hh>
#include <asm_lsw/x_fast_tries.hh>
#include <asm_lsw/y_fast_tries.hh>
#include <boost/functional/hash.hpp>
#include <boost/range/irange.hpp>
#include <sdsl/csa_rao.hpp>
#include <sdsl/cst_sada.hpp>
#include <sdsl/int_vector.hpp>
#include <sdsl/isa_lsw.hpp>
#include <unordered_set>


namespace asm_lsw {

	template <typename t_cst>
	class k1_matcher
	{
	protected:
		template <typename t_key, typename t_value>
		using unordered_map = map_adaptor_phf<map_adaptor_phf_spec<std::vector, pool_allocator, t_key, t_value>>;
		
	public:
		typedef sdsl::int_vector <>										int_vector_type;		// FIXME: make sure that this works with CSA's and CST's alphabet type.
		typedef sdsl::int_vector <>										pattern_type;			// FIXME: correct parameters.
		typedef int_vector_type											f_vector_type;
		typedef t_cst													cst_type;
		typedef typename cst_type::csa_type								csa_type;
		
		// Indexed by identifiers from node_id().
		typedef std::set <typename csa_type::value_type>				gamma_v_intermediate_type;
		typedef std::map <
			typename cst_type::size_type,
			gamma_v_intermediate_type
		>																gamma_intermediate_type;
			
		// Indexed by identifiers from node_id().
		typedef y_fast_trie_compact_as <typename csa_type::value_type>	gamma_v_type;
		typedef unordered_map <
			typename cst_type::size_type,
			std::unique_ptr <gamma_v_type>
		>																gamma_type;
		
		// Indexed by identifiers from node_id().
		typedef sdsl::bit_vector										core_nodes_type;
		typedef bp_support_sparse <>									core_endpoints_type;	// FIXME: check parameters.
		
		typedef sdsl::rmq_succinct_sct <>								lcp_rmq_type;			// Default parameters. FIXME: check that they yield O(t_SA) time complexity.
		typedef std::pair <
			typename csa_type::size_type,
			typename csa_type::size_type
		>																csa_range;
		typedef std::unordered_set <
			csa_range,
			boost::hash <csa_range>
		>																csa_range_set;

		static_assert(
			std::is_unsigned <typename cst_type::node_type>::value,
			"Unsigned integer required for cst_type::node_type."
		);
	
	public:
		class h_type;
		class f_type;

	protected:
		struct transform_gamma_v;
		
	protected:
		cst_type			m_cst;
		gamma_type			m_gamma;
		core_endpoints_type	m_ce;
		lcp_rmq_type		m_lcp_rmq;
		h_type				m_h;
		
	public:
		
		k1_matcher() {}
		
		k1_matcher(std::string const &input)
		{
			std::string file("@asm_lsw_k1_matcher_input.iv8");
			sdsl::store_to_file(input.c_str(), file);
			
			// CST
			cst_type cst;
			sdsl::construct(cst, file, 1);
			m_cst = std::move(cst);
			
			// Core path nodes
			core_nodes_type cn;
			construct_core_paths(cn);

			// Gamma
			gamma_type gamma;
			construct_gamma_sets(cn, gamma);
			m_gamma = std::move(gamma);
			
			// Core path endpoints
			core_endpoints_type ce;
			construct_core_path_endpoints(cn, ce);
			m_ce = std::move(ce);
			
			// LCP RMQ
			lcp_rmq_type lcp_rmq;
			construct_lcp_rmq(lcp_rmq);
			m_lcp_rmq = std::move(lcp_rmq);
			
			// H
			h_type h(*this);
			m_h = std::move(h);
		}
		
		
	protected:
		typename cst_type::size_type node_id(typename cst_type::node_type const node) const
		{
			return m_cst.bp_support.rank(node) - 1;
		}

		
		typename cst_type::node_type node_inv_id(typename cst_type::size_type const node_id) const
		{
			return m_cst.bp_support.select(1 + node_id);
		}

		bool find_path(
			pattern_type const &pattern,
			pattern_type::size_type const start_idx,
			typename cst_type::node_type &node // inout
		) const;
		
		bool is_side_node(core_nodes_type const &cn, typename cst_type::node_type const node) const;
		lcp_rmq_type::size_type lcp_length(lcp_rmq_type::size_type const l, lcp_rmq_type::size_type const r) const;
		lcp_rmq_type::size_type lcp_length_e(lcp_rmq_type::size_type const l, lcp_rmq_type::size_type const r) const;
		
		void construct_core_paths(core_nodes_type &cn) const;
		void construct_core_path_endpoints(core_nodes_type const &cn, core_endpoints_type &ce) const;
		void construct_uncompressed_gamma_sets(core_nodes_type const &cn, gamma_intermediate_type &gamma) const;
		void construct_gamma_sets(core_nodes_type const &cn, gamma_type &gamma) const;
		void construct_lcp_rmq(lcp_rmq_type &rmq) const;

		typename cst_type::size_type sa_idx_of_stored_isa_val(
			typename cst_type::csa_type::isa_type::value_type const isa_val,
			typename cst_type::size_type const pat1_len
		) const;
		
		bool find_pattern_occurrence(
			pattern_type::size_type const pat1_len,
			typename csa_type::size_type const st,
			typename csa_type::size_type const ed,
			typename csa_type::size_type const lb,
			typename csa_type::size_type const rb,
			typename csa_type::size_type &i
		) const;

		template <template <typename> class t_cmp>
		bool find_node_ilr_bin(
			typename csa_type::size_type const k,
			typename cst_type::size_type const r_len,
			typename csa_type::size_type const l,
			typename csa_type::size_type const r,
			typename csa_type::size_type &i
		) const;

		bool find_node_ilr_bin_p(
			typename csa_type::size_type const k,
			typename cst_type::size_type const r_len,
			typename csa_type::size_type const l,
			typename csa_type::size_type const r,
			typename csa_type::size_type &i
		) const;

		bool find_node_ilr(
			typename h_type::h_pair const &hx,
			typename csa_type::size_type const k,
			typename cst_type::size_type const r_len,	// cst.depth returns size_type.
			typename csa_type::size_type &i
		) const;

		void extend_range(
			pattern_type::size_type const pat1_len,
			typename csa_type::size_type const st,
			typename csa_type::size_type const ed,
			typename csa_type::size_type const v_le,
			typename csa_type::size_type const v_ri,
			typename csa_type::size_type &left,
			typename csa_type::size_type &right
		) const;
		
		bool tree_search_side_node(
			gamma_v_type const &gamma_v,
			typename cst_type::node_type const u,
			typename cst_type::node_type const v,
			typename csa_type::size_type const st,
			typename csa_type::size_type const ed,
			typename csa_type::size_type &i
		) const;

		bool tree_search(
			pattern_type const &pattern,
			f_type const &f,
			typename cst_type::node_type const u,
			typename cst_type::node_type core_path_beginning,
			typename cst_type::char_type const c,
			typename csa_type::size_type const st,
			typename csa_type::size_type const ed,
			typename csa_type::size_type &left,
			typename csa_type::size_type &right
		) const;
		
		void find_1_approximate_at_i(
			pattern_type const &pattern,
			f_type const &f,
			typename cst_type::node_type const u,
			typename cst_type::node_type const core_path_beginning,
			typename f_type::size_type const i,
			typename cst_type::char_type const cc,
			csa_range_set &ranges
		) const;
		
		void find_1_approximate_continue_exact(
			pattern_type const &pattern,
			typename cst_type::node_type v,
			pattern_type::size_type eidx,
			pattern_type::size_type pidx,
			csa_range_set &ranges
		) const;
		
	public:
		// Section 3.3.
		void find_1_approximate(pattern_type const &pattern, csa_range_set &ranges) const;
	};
	
	
	// Find the path for the given pattern.
	// Set node to the final node if found.
	// Time complexity O((|pattern| - start_idx) * t_SA)
	template <typename t_cst>
	bool k1_matcher <t_cst>::find_path(
		pattern_type const &pattern,
		pattern_type::size_type const start_idx,
		typename cst_type::node_type &node // inout
	) const
	{
		auto const begin(pattern.cbegin() + start_idx);
		auto const end(pattern.cend());
		assert(begin < end);
		
		pattern_type::size_type char_pos(0);
		return (0 < sdsl::forward_search(m_cst, node, 0, begin, end, char_pos));
	}
	
	
	// Check if the given node is a side node as per section 2.5.
	// FIXME: calculate time complexity.
	template <typename t_cst>
	bool k1_matcher <t_cst>::is_side_node(
		core_nodes_type const &cn,
		typename cst_type::node_type const node
	) const
	{
		auto nid(node_id(node));
		return 0 == cn[nid];
	}
	
	
	template <typename t_cst>
	auto k1_matcher <t_cst>::lcp_length(
		lcp_rmq_type::size_type const l,
		lcp_rmq_type::size_type const r
	) const -> lcp_rmq_type::size_type
	{
		assert(l < r);
		auto const idx(m_lcp_rmq(l + 1, r));
		auto const retval(m_cst.lcp[idx]);
		return retval;
	}
	
	
	// Allow equal values for l and r.
	template <typename t_cst>
	auto k1_matcher <t_cst>::lcp_length_e(
		lcp_rmq_type::size_type const l,
		lcp_rmq_type::size_type const r
	) const -> lcp_rmq_type::size_type
	{
		assert(l < m_cst.csa.size());
		assert(r < m_cst.csa.size());
		
		if (l == r)
			return m_cst.depth(m_cst.select_leaf(l));
		
		return lcp_length(l, r);
	}
	
	
	// Construct a bit vector core_nodes for listing core nodes. (Not included in the paper but needed for section 3.1.)
	// Space complexity: O(n) bits since the number of nodes in a suffix tree with |T| = n is 2n = O(n).
	// FIXME: calculate time complexity.
	template <typename t_cst>
	void k1_matcher <t_cst>::construct_core_paths(core_nodes_type &cn) const
	{
		std::map <
			typename cst_type::node_type,
			typename cst_type::size_type
		> node_depths;
		sdsl::bit_vector core_nodes(m_cst.nodes(), 0); // Nodes with incoming core edges.
		
		// Count node depths from the bottom and choose the greatest.
		for (auto it(m_cst.begin_bottom_up()), end(m_cst.end_bottom_up()); it != end; ++it)
		{
			typename cst_type::node_type node(*it);
			if (m_cst.is_leaf(node))
			{
				auto res(node_depths.emplace(std::make_pair(node, 1)));
				assert(res.second); // Insertion should have happened.
			}
			else
			{
				// Not a leaf, iterate the children and choose.
				typename cst_type::size_type max_nd(0);
				typename cst_type::node_type argmax_nd(m_cst.root());
				
				auto children(m_cst.children(node));
				for (auto const child : children)
				{
					auto d_it(node_depths.find(child));
					assert(node_depths.end() != d_it);
					
					if (max_nd < d_it->second)
					{
						max_nd = d_it->second;
						argmax_nd = d_it->first;
					}
					
					// d_it->first is not needed anymore.
					node_depths.erase(d_it);
				}
				
				// Update the bit vector.
				typename cst_type::size_type nid(node_id(argmax_nd));
				core_nodes[nid] = 1;
				
				auto res(node_depths.emplace(std::make_pair(node, 1 + max_nd)));
				assert(res.second); // Insertion should have happened.
			}
		}
		
		cn = std::move(core_nodes);
	}
	
	
	// Core path endpoints from Lemma 15.
	// FIXME: calculate time complexity.
	template <typename t_cst>
	void k1_matcher <t_cst>::construct_core_path_endpoints(core_nodes_type const &cn, core_endpoints_type &ce) const
	{
		auto const leaf_count(m_cst.size());
		auto const node_count(m_cst.nodes());
		size_t path_count(0);
		sdsl::bit_vector path_endpoints;
		sdsl::bit_vector mask;

		{
			sdsl::bit_vector opening(node_count, 0);
			sdsl::bit_vector closing(node_count, 0);
			
			// Find the '(' endpoint for each of the ')' endpoints in the leaves and count the paths.
			// The '(' endpoint will not have a bit set in cn.
			for (typename cst_type::size_type i(0); i < leaf_count; ++i)
			{
				auto const leaf(m_cst.select_leaf(1 + i));
				auto const leaf_id(node_id(leaf));
				if (cn[leaf_id])
				{
					++path_count;
					closing[leaf_id] = 1;

					auto node(leaf);
					do
					{
						assert(m_cst.root() != node);
						node = m_cst.parent(node);
					}
					while (cn[node_id(node)]);
					
					auto const nid(node_id(node));
					opening[nid] = 1;
				}
			}
			
			// Iterate the opening and closing vectors. If either has a bit set,
			// advance in the endpoint vector.
			{
				sdsl::bit_vector tmp(2 * path_count, 0);
				path_endpoints = std::move(tmp);
			}
			
			// Dense representation of the balanced parentheses.
			decltype(path_count) j{0};
			for (util::remove_c_t<decltype(node_count)> i{0}; i < node_count; ++i)
			{
				if (opening[i])
				{
					path_endpoints[j] = 1;
					++j;
				}
				else if (closing[i])
				{
					++j;
				}
			}
			assert(2 * path_count == j);

			// Mask of the sparse representation.
			// Somehow |= is not ok.
			for (util::remove_c_t<decltype(node_count)> i{0}; i < node_count; ++i)
				opening[i] = opening[i] | closing[i];

			mask = std::move(opening);
		}
		
		{
			core_endpoints_type tmp(std::move(path_endpoints), std::move(mask));
			ce = std::move(tmp);
		}
	}


	template <typename t_cst>
	void k1_matcher <t_cst>::construct_uncompressed_gamma_sets(core_nodes_type const &cn, gamma_intermediate_type &gamma) const
	{
		// Γ_v = {ISA[SA[i] + plen(u) + 1] | i ≡ 1 (mod log₂²n) and v_le ≤ i ≤ v_ri}.

		auto const n(m_cst.size());
		auto const logn(sdsl::util::log2_floor(n));
		auto const log2n(logn * logn);
		auto const &csa(m_cst.csa);
		auto const &isa(csa.isa);

		// cst.begin() returns an iterator that visits inner nodes twice.
		for (auto it(m_cst.begin_bottom_up()), end(m_cst.end_bottom_up()); it != end; ++it)
		{
			typename cst_type::node_type v(*it);
			if (is_side_node(cn, v))
			{
				typename cst_type::node_type const u(m_cst.parent(v));

				// Find the SA indices of the leftmost and rightmost leaves.
				auto const lb(m_cst.lb(v));
				auto const rb(m_cst.rb(v));
				assert (lb <= rb); // FIXME: used to be lb < rb. Was this intentional?

				auto const v_id(node_id(v));
				auto &gamma_v(gamma[v_id]); // Insert (with operator[]) if gamma_v doesn't exist yet.
				
				// Calculate a suitable starting index based on the following:
				//   {i | i ≡ 1 (mod log₂²n) and lb ≤ i ≤ rb}
				// = {i | i - 1 divisible by log₂²n and lb ≤ i ≤ rb}
				// = {i | i = 1 + j log₂²n, j ∈ ℕ and lb ≤ i ≤ rb}
				// Now lb ≤ 1 + j log₂²n <=> (lb - 1) / log₂²n ≤ j.
				// In case lb = 0, the inequality above yields 1 for the right side and j ← 0.
				typename cst_type::size_type i(0), j(0);
				if (lb)
					j = std::ceil(1.0 * (lb - 1) / log2n);
				
				while ((i = 1 + j * log2n) <= rb)
				{
					// cst.depth returns path label length.
					assert(lb <= i);
					auto isa_idx(csa[i] + m_cst.depth(u) + 1);
					if (! (isa.size() <= isa_idx))
					{
						auto val(isa[isa_idx]);
						gamma_v.insert(val);
					}
					++j;
				}
			}
		}
	}
	
	
	template <typename t_cst>
	void k1_matcher <t_cst>::construct_gamma_sets(core_nodes_type const &cn, gamma_type &gamma) const
	{
		gamma_intermediate_type gamma_i;
		construct_uncompressed_gamma_sets(cn, gamma_i);
		
		typename gamma_type::template builder_type <gamma_intermediate_type, transform_gamma_v> builder(gamma_i);
		gamma_type gamma_tmp(builder);
		gamma = std::move(gamma_tmp);
	}
	
	
	// Create the auxiliary data structures in order to perform range minimum queries on the LCP array.
	template <typename t_cst>
	void k1_matcher <t_cst>::construct_lcp_rmq(lcp_rmq_type &rmq) const
	{
		lcp_rmq_type rmq_tmp(&m_cst.lcp);
		rmq = std::move(rmq_tmp);
	}
			
	
	// Get i from j = ISA[SA[i] + |P₁| + 1].
	template <typename t_cst>
	auto k1_matcher <t_cst>::sa_idx_of_stored_isa_val(
		typename cst_type::csa_type::isa_type::value_type const isa_val,
		typename cst_type::size_type const pat1_len
	) const -> typename cst_type::size_type
	{
		auto const &csa(m_cst.csa);
		auto const isa_idx(csa[isa_val]);
		auto const sa_val(isa_idx - pat1_len - 1);
		auto const sa_idx(csa.isa[sa_val]);
		return sa_idx;
	}

	
	// Find one occurrence of the pattern (index i) s.t. st ≤ ISA[SA[i] + |P₁| + 1] ≤ ed.
	template <typename t_cst>
	bool k1_matcher <t_cst>::find_pattern_occurrence(
		pattern_type::size_type const pat1_len,
		typename csa_type::size_type const st,
		typename csa_type::size_type const ed,
		typename csa_type::size_type const lb,
		typename csa_type::size_type const rb,
		typename csa_type::size_type &i
	) const
	{
		auto const &csa(m_cst.csa);
		
		if (rb < lb)
			return false;
		
		auto const mid(lb + (rb - lb) / 2);
		auto const &isa(csa.isa);
		auto const isa_idx(csa[mid] + pat1_len + 1);
		
		// Make sure that the pattern isn't too long.
		if (isa.size() <= isa_idx)
		{
			// By Lemma 12 ISA[SA[i] + |P₁| + 1] is increasing for i ∈ [v_le, v_ri].
			assert(isa.size() <= csa[lb] + pat1_len + 1);
			return false;
		}
		
		auto const val(isa[isa_idx]);
		
		// Tail recursion.
		// The first case prevents overflow for mid - 1.
		if (st != val && lb == mid)
			return false;
		else if (val < st)
			return find_pattern_occurrence(pat1_len, st, ed, 1 + mid, rb, i);
		else if (ed < val)
			return find_pattern_occurrence(pat1_len, st, ed, lb, mid - 1, i);
		else
		{
			assert(st <= val && val <= ed);
			i = mid;
			return true;
		}
	}
	
	
	// Find SA index i s.t. |lcp(i, k)| = |P₁| + q + 1 using binary search.
	template <typename t_cst>
	template <template <typename> class t_cmp>
	bool k1_matcher <t_cst>::find_node_ilr_bin(
		typename csa_type::size_type const k,
		typename cst_type::size_type const r_len,
		typename csa_type::size_type const l,
		typename csa_type::size_type const r,
		typename csa_type::size_type &i
	) const
	{
		t_cmp <typename csa_type::size_type> cmp;
		
		if (r < l)
			return false;

		auto const mid(l + (r - l) / 2);
		auto const res(lcp_length_e(std::min(mid, k), std::max(mid, k)));

		// Tail recursion.
		// The first case prevents overflow for mid - 1.
		if (res != r_len && l == mid)
			return false;
		if (cmp(res, r_len))
			return find_node_ilr_bin <t_cmp>(k, r_len, 1 + mid, r, i);
		else if (cmp(r_len, res))
			return find_node_ilr_bin <t_cmp>(k, r_len, l, mid - 1, i);
		else
		{
			i = mid;
			return true;
		}
	}

	
	// Partition the range if needed since k has the longest lcp with itself.
	template <typename t_cst>
	bool k1_matcher <t_cst>::find_node_ilr_bin_p(
		typename csa_type::size_type const k,
		typename cst_type::size_type const r_len,
		typename csa_type::size_type const l,
		typename csa_type::size_type const r,
		typename csa_type::size_type &i
	) const
	{
		// Order is ascending up to k, then descending.
		if (l < k && k < r)
		{
			if (find_node_ilr_bin <std::less>(k, r_len, l, k, i))
				return true;
			
			return find_node_ilr_bin <std::greater>(k, r_len, 1 + k, r, i);
		}

		if (r < k)
			return find_node_ilr_bin <std::less>(k, r_len, l, r, i);

		return find_node_ilr_bin <std::greater>(k, r_len, l, r, i);
	}


	// Lemma 19 case 3 (with v being a core node).
	// Find a suitable range in H_u s.t. either 
	//  |lcp(j - log₂²n, k)| ≤ r_len ≤ |lcp(j, k)| or
	//  |lcp(j, k)| ≤ r_len ≤ |lcp(j + log₂²n, k)|
	// Then use the range to search for a suitable node.
	template <typename t_cst>
	bool k1_matcher <t_cst>::find_node_ilr(
		typename h_type::h_pair const &hx,
		typename csa_type::size_type const k,
		typename cst_type::size_type const r_len,	// cst.depth returns size_type.
		typename csa_type::size_type &i
	) const
	{
		typename csa_type::size_type l(0), r(0);
		typename h_type::h_u_type::const_subtree_iterator it;
		auto const h_diff(m_h.diff());
		auto const lcp_rmq_size(m_lcp_rmq.size());
		auto const lcp_rmq_max(lcp_rmq_size ? lcp_rmq_size - 1 : 0);

		// Try i^l.
		if (hx.l->find_successor(r_len, it, true))
		{
			r = hx.l->iterator_value(it);
			l = ((h_diff <= r) ? (r - h_diff) : 0);
			// XXX Is l guaranteed to have |lcp(l, k)| ≤ r_len? (Case 3)

			if (find_node_ilr_bin_p(k, r_len, l, r, i))
				return true;
		}

		// Try i^r.
		if (hx.r->find_predecessor(r_len, it, true))
		{
			l = hx.r->iterator_value(it);
			r = ((l + h_diff <= lcp_rmq_max) ? (l + h_diff) : lcp_rmq_max);
			// XXX Is r guaranteed to have r_len ≤ |lcp(r, k)|? (Case 3)

			if (find_node_ilr_bin_p(k, r_len, l, r, i))
				return true;
		}

		// Try i^l again with another range.
		l = ((h_diff <= k) ? (k - h_diff) : 0);
		r = ((k + h_diff <= lcp_rmq_max) ? (k + h_diff) : lcp_rmq_max);
		if (find_node_ilr_bin_p(k, r_len, l, r, i))
			return true;			

		return false;
	}


	// Use Lemma 11.
	template <typename t_cst>
	void k1_matcher <t_cst>::extend_range(
		pattern_type::size_type const pat1_len,
		typename csa_type::size_type const st,
		typename csa_type::size_type const ed,
		typename csa_type::size_type const v_le,
		typename csa_type::size_type const v_ri,
		typename csa_type::size_type &left,
		typename csa_type::size_type &right
	) const
	{
		auto const &csa(m_cst.csa);
		auto const &isa(csa.isa);
		
		while (true)
		{
			if (v_le == left)
				break;
			
			assert(v_le < left);
			auto const i(left - 1);
			
			auto const isa_idx(csa[i] + pat1_len + 1);
			if (! (isa_idx < isa.size()))
				break;
			
			if (! (st <= isa[isa_idx]))
				break;
			
			left = i;
		}
		
		while (true)
		{
			if (v_ri == right)
				break;
			
			assert(right < v_ri);
			auto const i(right + 1);
			
			auto const isa_idx(csa[i] + pat1_len + 1);
			if (! (isa_idx < isa.size()))
				break;
			
			if (! (isa[isa_idx] <= ed))
				break;
			
			right = i;
		}
	}


	// Lemma 19 (with v being a side node).
	template <typename t_cst>
	bool k1_matcher <t_cst>::tree_search_side_node(
		gamma_v_type const &gamma_v,
		typename cst_type::node_type const u,
		typename cst_type::node_type const v,
		typename csa_type::size_type const st,
		typename csa_type::size_type const ed,
		typename csa_type::size_type &i
	) const
	{
		// v is a side node.
		auto const &csa(m_cst.csa);
		auto const pat1_len(m_cst.depth(u));
		auto const v_le(m_cst.lb(v));
		auto const v_ri(m_cst.rb(v));

		if (0 == gamma_v.size())
		{
			// Case 1.
			if (find_pattern_occurrence(pat1_len, st, ed, v_le, v_ri, i))
				return true;
		}
		else
		{
			// Find a j s.t. st ≤ j ≤ ed by allowing equality and checking the upper bound.
			typename gamma_v_type::const_subtree_iterator it;
			if (gamma_v.find_successor(st, it, true) && *it <= ed)
			{
				// Case 2.
				// Get i from j = ISA[SA[i] + |P₁| + 1].
				auto const isa_val(*it);
				i = sa_idx_of_stored_isa_val(isa_val, pat1_len);
				return true;
			}
			else
			{
				// Case 3.
				// Extend the range slightly if possible. If not, the stored values
				// should be between st and ed in which case the method in case 2
				// would be applicable as the number of leaves is small enough.
				// XXX verify the statement above.
				typename gamma_v_type::const_subtree_iterator a_val_it, b_val_it;
				typename csa_type::size_type a(v_le), b(v_ri);
				
				if (gamma_v.find_predecessor(st, a_val_it))
				{
					auto const a_val(*a_val_it);
					a = sa_idx_of_stored_isa_val(a_val, pat1_len);
				}
				
				if (gamma_v.find_successor(ed, b_val_it))
				{
					auto const b_val(*b_val_it);
					b = sa_idx_of_stored_isa_val(b_val, pat1_len);
				}
				
				if (find_pattern_occurrence(pat1_len, st, ed, a, b, i))
					return true;
			}
		}

		return false;
	}
	
	
	// Lemma 19.
	template <typename t_cst>
	bool k1_matcher <t_cst>::tree_search(
		pattern_type const &pattern,
		f_type const &f,
		typename cst_type::node_type const u,
		typename cst_type::node_type core_path_beginning,
		typename cst_type::char_type const c,
		typename csa_type::size_type const st,
		typename csa_type::size_type const ed,
		typename csa_type::size_type &left,
		typename csa_type::size_type &right
	) const
	{
		typename cst_type::size_type pos(0);
		auto const v(m_cst.child(u, c, pos));
		
		// Check if found.
		if (m_cst.root() == v)
			return false;
		
		typename csa_type::size_type i(0);
		
		auto const v_id(node_id(v));
		auto const pat1_len(m_cst.depth(u));
		auto const v_le(m_cst.lb(v));
		auto const v_ri(m_cst.rb(v));

		// If we diverged from the previous core path, update.
		auto core_path_beginning_id(0);
		if (core_endpoints_type::input_value::Opening == m_ce[v_id])
		{
			core_path_beginning = v;
			core_path_beginning_id = v_id;
		}
		else
		{
			core_path_beginning_id = node_id(core_path_beginning);
		}
		
		// We still need to check the node type (core or side) in case
		// v doesn't begin a core path.
		auto const g_it(m_gamma.find(v_id));
		if (m_gamma.cend() == g_it)
		{
			// No Γ_v with this node so v must be a core node. Find the
			// core leaf node x at the end of the core path.
			// Every core leaf node is supposed to have a set H associated with it.
			auto const x_id(m_ce.find_close(core_path_beginning_id));
			auto const h_it(m_h.maps().find(x_id));
			assert(m_h.maps().cend() != h_it);
			auto const &hx(h_it->second);
			if (hx.l->size() == 0 && hx.r->size() == 0)
			{
				// Case 1.
				// The number of leaves hanging from the core path is < log₂²n.
				if (!find_pattern_occurrence(pat1_len, st, ed, v_le, v_ri, i))
					return false;

				left = i;
				right = i;
				extend_range(pat1_len, st, ed, v_le, v_ri, left, right);
				return true;
			}
			else
			{
				auto const &csa(m_cst.csa);
				auto const &isa(csa.isa);
				auto const x(node_inv_id(x_id));
				assert(m_cst.is_leaf(x));
				
				// Get the suffix array index.
				auto const k(m_cst.id(x));
				auto const isa_idx(csa[k] + pat1_len + 1);
				
				// Check if the pattern may still be found in the suffix tree.
				if (! (isa_idx < isa.size()))
					return false;
				
				auto const isa_val(isa[isa_idx]);
				auto const q(lcp_length_e(std::min(isa_val, st), std::max(isa_val, st)));
				auto const pat2_len(pattern.size() - 1 - pat1_len);
				if (q >= pat2_len)
				{
					// Case 2.
					// x corresponds to a suffix with P as its prefix.
					left = k;
					right = k;
					extend_range(pat1_len, st, ed, v_le, v_ri, left, right);
					return true;
				}
				else
				{
					// Case 3.
					// Since x is a core leaf node, we have (some of) the indices
					// of the suitable LCP values stored in hx.l and hx.r.
					auto const r_len(pat1_len + q + 1);
					typename csa_type::size_type ilr(0);
					if (!find_node_ilr(hx, k, r_len, ilr)) // ilr is an SA index.
						return false;
					
					// XXX Paper requires O(t_SA) time complexity for LCA but cst_sada gives O(rr_enclose).
					auto const leaf(m_cst.select_leaf(1 + ilr)); // Convert the suffix array index.

					// Find r and check if it completely matches P.
					auto const r(m_cst.lca(leaf, x));
					auto const r_depth(m_cst.depth(r));
					if (pat1_len + 1 + pat2_len <= r_depth)
					{
						left = m_cst.lb(r);
						right = m_cst.rb(r);
						extend_range(pat1_len, st, ed, v_le, v_ri, left, right); // FIXME: not sure if needed.
						return true;
					}
					else
					{
						// Since the match was incomplete, the matching node must be r's descendant.
						// The chosen edge should be a side edge (Case 3).
						// Every side edge should lead either to a leaf node or to a beginning of
						// a core path. XXX verify this.
						auto const next_cc(pattern[r_depth]);
#ifndef NDEBUG
						{
							auto const s(m_cst.child(r, next_cc));
							auto const node_type(m_ce[node_id(s)]);
							assert(m_cst.is_leaf(s) || core_endpoints_type::input_value::Opening == node_type);
						}
#endif
						auto const r_st(f.st(m_cst, 1 + r_depth));
						auto const r_ed(f.ed(m_cst, 1 + r_depth));
						return tree_search(pattern, f, r, r, next_cc, r_st, r_ed, left, right);
					}

					return false;
				}
			}
		}
		else if (tree_search_side_node(*(g_it->second), u, v, st, ed, i))
		{
			left = i;
			right = i;
			extend_range(pat1_len, st, ed, v_le, v_ri, left, right);
			return true;
		}
		else
		{
			return false;
		}
	}
	
	
	// Section 3.3.
	template <typename t_cst>
	void k1_matcher <t_cst>::find_1_approximate_at_i(
		pattern_type const &pattern,
		f_type const &f,
		typename cst_type::node_type const u,
		typename cst_type::node_type const core_path_beginning,
		typename f_type::size_type const i,
		typename cst_type::char_type const cc,
		csa_range_set &ranges
	) const
	{
		typename csa_type::size_type left{0}, right{0};
		auto const st(f.st(m_cst, i));
		auto const ed(f.ed(m_cst, i));

		// [st, ed] = [F_st[j], F_ed[j]] = range(cst, pattern[j, m]) = range(cst, pattern[1 + i, m]),
		// where m is the pattern length. If pattern[1 + i, m] isn't a prefix of any suffix in cst,
		// there aren't any occurrences of P₁cP₂ (as defined in Lemma 19) in cst.
		if (st != f_type::not_found)
		{
			assert (ed != f_type::not_found);
			if (tree_search(pattern, f, u, core_path_beginning, cc, st, ed, left, right))
			{
				csa_range range(left, right);
				ranges.emplace(std::move(range));
			}
		}
		else
		{
			assert (ed == f_type::not_found);
		}
	}
	
	
	template <typename t_cst>
	void k1_matcher <t_cst>::find_1_approximate_continue_exact(
		pattern_type const &pattern,
		typename cst_type::node_type v,
		pattern_type::size_type eidx,
		pattern_type::size_type pidx,
		csa_range_set &ranges
	) const
	{
		// Repeatedly select the matching branch of the suffix tree until
		// the end of the pattern has been reached.
		auto const patlen(pattern.size());
		while (true)
		{
			auto const len(m_cst.depth(v));
			while (eidx < len)
			{
				if (! (pidx < patlen))
					goto end;
				
				auto const ec(m_cst.edge(v, 1 + eidx));
				auto const pc(pattern[pidx]);
				if (ec != pc)
					return;
				
				++eidx;
				++pidx;
			}
			
			if (! (pidx < patlen))
				goto end;
			
			// Find the next edge, stop if not found.
			typename cst_type::size_type char_pos(0);
			v = m_cst.child(v, pattern[pidx], char_pos);
			
			if (m_cst.root() == v)
				return;
			
			// The first character is known to match since an edge was found.
			++eidx;
			++pidx;
		}
		
	end:
		csa_range range(m_cst.lb(v), m_cst.rb(v));
		ranges.emplace(std::move(range));
	}
	
	
	// Section 3.3.
	template <typename t_cst>
	void k1_matcher <t_cst>::find_1_approximate(pattern_type const &pattern, csa_range_set &ranges) const
	{
		f_type const f(*this, pattern);
		typename cst_type::node_type u(m_cst.root());
		typename cst_type::node_type core_path_beginning(u);
		auto const patlen(pattern.size());
		util::remove_c_t<decltype(patlen)> i(0);

		while (i < patlen)
		{
			// 1. Deletion
			// Needn't be checked if P[i] == P[i + 1].
			if (pattern[i] != pattern[1 + i])
			{
				auto const cc(pattern[1 + i]);
				find_1_approximate_at_i(pattern, f, u, core_path_beginning, 2 + i, cc, ranges);
			}
			
			// 2. Substitution.
			// Test with all the characteres in Sigma.
			for (typename cst_type::sigma_type c(0); c < m_cst.csa.sigma; ++c)
			{
				auto const cc(m_cst.csa.comp2char[c]);
				if (pattern[i] != cc)
					find_1_approximate_at_i(pattern, f, u, core_path_beginning, 1 + i, cc, ranges);
			}
			
			// 3. Insertion.
			// Allow insertion of the same character to the end of the string.
			for (typename cst_type::sigma_type c(0); c < m_cst.csa.sigma; ++c)
			{
				auto const cc(m_cst.csa.comp2char[c]);
				if (1 + i == pattern.size() || pattern[i] != cc)
					find_1_approximate_at_i(pattern, f, u, core_path_beginning, i, cc, ranges);
			}

			// 4. Exact match.
			auto const cc(pattern[i]);
			typename cst_type::size_type char_pos{0};
			auto const v(m_cst.child(u, cc, char_pos));
			
			// Character at i doesn't match; there won't be more
			// matches since k = 1.
			if (m_cst.root() == v)
				return;
			
			assert(i == m_cst.depth(u));
			auto k(i);
			auto const len(m_cst.depth(v));
			while (true)
			{
				// If the end of the pattern is reached, stop.
				if (patlen == k)
				{
					csa_range range(m_cst.lb(v), m_cst.rb(v));
					ranges.emplace(std::move(range));
					return;
				}
				
				// If the end of the edge was reached, stop.
				// (Obviously the match needs to be checked first.)
				if (! (k < len))
					break;

				auto const ec(m_cst.edge(v, 1 + k));
				auto const pc(pattern[k]);
				if (ec != pc)
				{
					// One mismatch was found; the current index is the only remaining one
					// for which mismatches need to be considered.
					
					// 1. Deletion. Remove character at pattern[k].
					find_1_approximate_continue_exact(pattern, v, k, k + 1, ranges);
					
					// Skip if ec is the zero character.
					if (ec)
					{
						// 2. Insertion. Suppose ec was inserted into pattern before k.
						find_1_approximate_continue_exact(pattern, v, k + 1, k, ranges);
						
						// 3. Substitution. Suppose pattern[k] was replaced with ec.
						find_1_approximate_continue_exact(pattern, v, k + 1, k + 1, ranges);
					}
					
					return;
				}
				
				++k;
			}
			
			// No mismatches were found, continue.
			u = v;
			i = m_cst.depth(v);
			
			// Check if the current node starts a core path and update core_path_beginning.
			auto const u_id(node_id(u));
			if (core_endpoints_type::input_value::Opening == m_ce[u_id])
				core_path_beginning = u;

			// In case the pattern length was reached with i, add the match
			// to ranges.
			// FIXME: shouldn't be reached now.
			if (m_cst.is_leaf(u))
			{
				assert(0);
				auto const idx(m_cst.id(u));
				ranges.emplace(idx, idx);
			}
		}
	}
}

#endif
