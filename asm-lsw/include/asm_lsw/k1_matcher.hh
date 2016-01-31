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
#include <sdsl/csa_rao.hpp>
#include <sdsl/cst_sada.hpp>
#include <sdsl/int_vector.hpp>


namespace asm_lsw {

	class k1_matcher
	{
	public:
		struct h_type;
		struct h_pair;
		
		typedef sdsl::int_vector<>										int_vector_type;		// FIXME: make sure that this works with CSA's and CST's alphabet type.
		typedef std::string												pattern_type;
		typedef int_vector_type											f_type;
		typedef sdsl::csa_rao<>											csa_type;				// FIXME: correct parameters.
		typedef sdsl::cst_sada<csa_type>								cst_type;
		typedef y_fast_trie<csa_type::value_type>						gamma_v_type;
		typedef std::unordered_map<cst_type::node_type, gamma_v_type>	gamma_type;				// FIXME: use a map with perfect hashing instead.
		typedef sdsl::rrr_vector										core_nodes_type;
		typedef bp_support_sparse										core_endpoints_type;	// FIXME: check parameters.
		typedef sdsl::rmq_succinct_sct									lcp_rmq_type;			// Default parameters. FIXME: check that they yield O(t_SA) time complexity.
		typedef std::unordered_map<cst_type::node_type, h_pair>			h_map;					// FIXME: use a map with perfect hashing instead.
		typedef y_fast_trie<csa_type::size_type, csa_type::size_type>	h_u_type;

		static_assert(std::is_integral<cst_type::node_type>::value, "Integer required for cst_type::node_type.");
		
	public:
		struct h_pair
		{
			h_u_type l;
			h_u_type r;
		};
		
		struct h_type
		{
			h_map maps;
			cst_type::size_type diff;
		};

	public:
		static f_type::value_type const not_found{std::numeric_limits<f_type::value_type>::max()};

	protected:
		cst_type			m_cst;
		f_type				m_f_st;				// Def. 1, 2, lemma 10. // FIXME: move m_f_st, m_f_ed inside the search function?
		f_type				m_f_ed;				// Def. 1, 2, lemma 10.
		gamma_type			m_gamma;			// Lemma 16.
		lcp_rmq_type		m_lcp_rmq;
		core_nodes_type		m_core_nodes;		// FIXME: remove since (probably) not needed after constructing the data structures.
		core_endpoints_type	m_core_endpoints;

	protected:
		// Find the path for the given pattern.
		// Set node to the final node if found.
		// Time complexity O((|pattern| - start_idx) * t_SA)
		static bool find_path(
			cst_type const &cst,
			pattern_type const &pattern,
			pattern_type::size_type const start_idx,
			cst_type::node_type &node // inout
		)
		{
			cst_type::node_type node;
			auto const begin(pattern.cbegin() + start_idx);
			pattern_type::size_type char_pos(0);

			for (auto it(begin), end(pattern.cend()); it != end; ++it)
			{
				// node is inout.
				if (0 == sdsl::forward_search(cst, node, it - begin, *it, char_pos))
					return false;
			}

			return true;
		}
		
		
		// Check if the given node is a side node as per section 2.5.
		// FIXME: calculate time complexity.
		static bool is_side_node(cst_type const &cst, cst_type::node_type const node)
		{
			auto node_id(cst.id(node));
			return m_core_nodes[node_id];
		}
		
		
		// Return F_st[i].
		// FIXME: calculate time complexity.
		auto f_st(f_type::size_type const i) const -> f_type::value_type
		{
			return (i < m_f_st.size() ? m_f_st[i] : 0);
		}
		
		
		// Return F_ed[i].
		// FIXME: calculate time complexity.
		auto f_ed(f_type::size_type const i) const -> f_type::value_type
		{
			return (i < m_f_ed.size() ? m_f_ed[i] : m_cst.size());
		}
		
		
		// Construct a bit vector core_nodes for listing core nodes. (Not included in the paper but needed for section 3.1.)
		// Space complexity: O(n) bits since the number of nodes in a suffix tree with |T| = n is 2n = O(n).
		// FIXME: calculate time complexity.
		static void construct_core_paths(cst_type const &cst, core_nodes_type &cn)
		{
			std::unordered_map<cst_type::node_type, cst_type::size_type> node_depths;
			sdsl::bit_vector core_nodes(cst.nodes(), 0); // Nodes with incoming core edges.
			
			// Count node depths from the bottom and choose the greatest.
			for (auto it(cst.begin_bottom_up()), end(cst.end_bottom_up()); it != end; ++it)
			{
				cst::node_type node(*it);
				if (cst.is_leaf(node))
				{
					auto res(node_depths.emplace(std::make_pair(node, 1)));
					assert(res.second); // Insertion should have happened.
				}
				else
				{
					// Not a leaf, iterate the children and choose.
					cst_type::size_type max_nd(0);
					cst_type::node_type argmax_nd(cst.root());
					
					auto children(cst.children(node));
					for (auto it(children.begin()), end(children.end()); it != end; ++it)
					{
						auto d_it(node_depths.find(*it));
						assert(node_depths.end() != d_it);
						
						if (max_nd < d_it.second)
						{
							max_nd = d_it.second;
							argmax_nd = d_it.first;
						}
						
						// d_it->first is not needed anymore.
						node_depths.erase(d_it);
					}
					
					// Update the bit vector.
					core_nodes[argmax_nd] = 1;
					
					auto res(node_depths.emplace(std::make_pair(node, 1 + max_nd)));
					assert(res.second); // Insertion should have happened.
				}
			}
			
			core_nodes_type tmp_cn(core_nodes);
			cn = std::move(tmp_cn);
		}
		
		
		// Core path endpoints from Lemma 15.
		// FIXME: calculate time complexity.
		static void construct_core_path_endpoints(cst_type const &cst, core_nodes_type const &cn, core_endpoints_type const &ce)
		{
			auto const leaf_count(cst.size());
			auto const node_count(cst.nodes());
			size_type path_count(0);
			bit_vector path_endpoints;
			bit_vector mask;

			{
				bit_vector opening(node_count, 0);
				bit_vector closing(node_count, 0);
				
				// Find the '(' endpoint for each of the ')' endpoints in the leaves and count the paths.
				// The '(' endpoint will not have a bit set in cn.
				// Documentation for inv_id: identifiers are in range [0, nodes() - 1].
				for (cst::size_type i(0); i < leaf_count; ++i)
				{
					cst::node_type node(cst.inv_id(i));
					if (cn[node])
					{
						++path_count;
						closing[node] = 1;

						do
						{
							assert(cst.root() != node);
							node = cst.parent(node);
						}
						while (cn[node]);
						
						opening[node] = 1;
					}
				}
				
				// Balanced parentheses representation.
				// Iterate the opening and closing vectors. If either has a bit set,
				// advance in the endpoint vector.
				{
					bit_vector tmp(2 * path_count, 0);
					path_endpoints = std::move(tmp);
				}
				
				// Dense representation of the balanced parentheses.
				decltype(path_count) j{0};
				for (decltype(node_count) i{0}; i < node_count; ++i)
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
				for (decltype(node_count) i{0}; i < node_count; ++i)
					opening[i] |= closing[i];

				mask = std::move(opening);
			}
			
			{
				core_endpoints_type tmp(std::move(path_endpoints), std::move(mask));
				ce = std::move(tmp);
			}
		}


		// Construct the F arrays as defined in definitions 1 and 2 and lemma 10.
		// FIXME: calculate time complexity.
		static void construct_f_arrays(cst_type const &cst, pattern_type const &pattern, f_type &f_st, f_type &f_ed)
		{
			auto const m(pattern.size());
			pattern_type::size_type i(0);
			cst_type::node_type node;

			f_st = f_type(m, 0);
			f_ed = f_type(m, 0);

			// Start with i = 1 in pattern[i..m] (zero-based in code).
			// If not found in CST, fill with sentinels, set i = 2
			// and continue (Lam et al. 2.4 lemma 10).
			// FIXME: I'm not sure if this is done right since an upper limit for the whole pattern is O(|pattern| * |pattern| / 2 * t_SA). (There could be a smaller upper limit, though.)
			while (i < m)
			{
				if (find_path(cst, pattern, i, node))
				{
					// Find the SA indices of the leftmost and rightmost leaves.
					f_st[i] = cst.lb(node);
					f_ed[i] = cst.rb(node);
					break;
				}
				else
				{
					f_st[i] = not_found;
					f_ed[i] = not_found;
				}

				++i;
			}

			// Continue with the suffix links. Either i == m or a match was found. In the latter case, every substring
			// of the pattern must
			// FIXME: complete the sentence above.
			while (i < m)
			{
				node = cst.sl(node);
				f_st[i] = cst.lb(node);
				f_ed[i] = cst.rb(node);
				++i;
			}
		}


		static void construct_gamma_sets(cst_type const &cst, gamma_type &gamma)
		{
			// Γ_v = {ISA[SA[i] + plen(u) + 1] | i ≡ 1 (mod log₂²n) and v_le ≤ i ≤ v_ri}.

			auto const n(cst.size());
			auto const logn(std::log2(n));
			auto const log2n(std::pow(logn, 2)); // FIXME: check integrality.
			auto const &csa(cst.csa);
			auto const &isa(csa.isa);

			for (auto it(cst.begin()), end(cst.end()); it != end; ++it)
			{
				if (is_side_node(cst, *it))
				{
					cst_type::node_type const v(*it);
					cst_type::node_type const u(cst.parent(v));

					// Find the SA indices of the leftmost and rightmost leaves.
					auto const lb(cst.lb(v));
					auto const rb(cst.rb(v));
					assert (lb <= rb); // FIXME: used to be lb < rb. Was this intentional?

					auto &gamma_v(gamma[v]); // Insert (with operator[]) if gamma_v doesn't exist yet.
					
					// Calculate a suitable starting index based on the following:
					//   {i | i ≡ 1 (mod log₂²n) and lb ≤ i ≤ rb}
					// = {i | i - 1 divisible by log₂²n and lb ≤ i ≤ rb}
					// = {i | i = 1 + j log₂²n, j ∈ ℕ and lb ≤ i ≤ rb}
					// Now lb ≤ 1 + j log₂²n <=> (lb - 1) / log₂²n ≤ j.
					// In case lb = 0, the inequality above yields 1 for the right side and j ← 0.
					cst::size_type i(0), j(0);
					if (lb)
						j = std::ceil((lb - 1) / log2n); // FIXME: verify integrality.
					
					while ((i = 1 + j * log2n) <= rb)
					{
						// cst.depth returns path label length.
						auto val(isa[csa[i] + cst.depth(u) + 1]);
						gamma_v.insert(val);
						
						++j;
					}
				}
			}
		}
		
		
		// Create the auxiliary data structures in order to perform range minimum queries on the LCP array.
		static void construct_lcp_rmq(cst_type const &cst, lcp_rmq_type &rmq)
		{
			lcp_rmq_type rmq_tmp(cst.lcp);
			rmq = std::move(rmq_tmp);
		}
		
		
		// Section 3.1, Lemma 17.
		static void construct_hl_hr(
									cst_type const &cst,
									core_endpoints_type const &ce,
									lcp_rmq_type const &lcp_rmq,
									cst_type::node_type const u,
									cst_type::size_type const log2n,
									h_u_type &hl,
									h_u_type &hr)
		{
			auto const v(ce.find_open(u));					// Core path beginning.
			auto const k(cst.lb(u));						// Left bound (leftmost leaf index) in SA.
			auto const plen_v(cst.depth(v));				// FIXME: check that this actually is plen(v).
			
			auto const leaf_count(cst.size());
			cst::size_type i(0), j(0);
			while ((i = 1 + j * log2n) < leaf_count)
			{
				// Now i is an SA index s.t.
				//  i ∈  {i | i ≡ 1 (mod log₂²n)}
				//     = {i | i - 1 divisible by log₂²n}
				//     = {1 + j log₂²n | j ∈ ℕ}
				
				// Check |lcp(k, i)| ≥ plen(v).
				auto const lcp_len(lcp_rmq(k, i));
				if (lcp_len >= plen_v)
				{
					if (i <= k)
						hl.insert(lcp_len, i);
					else if (i > k)
						hr.insert(lcp_len, i);
				}
				
				++j;
			}
		}
		
		
		// Section 3.1, Lemma 17.
		static void construct_h_arrays(cst_type const &cst, core_endpoints_type const &ce, lcp_rmq_type const &lcp_rmq, h_type &h)
		{
			auto const n(cst.size());						// Text length.
			auto const logn(std::log2(n));
			auto const log2n(std::pow(logn, 2));			// FIXME: check integrality.
			h.diff = log2n;

			auto const &bp(ce.bp());
			auto const count(bp.rank(bp.size() - 1)); // Number of (opening) parentheses.
			for (decltype(count) i{1}; i <= count; ++i)
			{
				// Find the index of each opening parenthesis and its counterpart,
				// then convert to sparse index.
				auto const bp_begin(bp.select(i));
				auto const bp_end(bp.find_close(bp_begin));
				auto const u(ce.to_sparse_idx(bp_end));
				
				// u is now a core leaf node.
				h_pair h_u;
				construct_hl_hr(cst, ce, lcp_rmq, u, log2n, h_u.l, h_u.r);
				h.maps[u] = h_u;
			}
		}
		
		
		// Find one occurrence of the pattern (index i) s.t. st ≤ ISA[SA[i] + |P₁| + 1] ≤ ed.
		static bool find_pattern_occurrence(
			csa_type const &csa,
			pattern_type::size_type const pat1_len,
			f_type::value_type const st,
			f_type::value_type const ed,
			csa_type::size_type const lb,
			csa_type::size_type const rb,
			csa_type::size_type &i
		)
		{
			if (rb < lb)
				return false;
			
			auto const mid((rb - lb) / 2);
			auto const &isa(csa.isa);
			auto const isa_idx(csa[mid] + pat1_len + 1);
			auto const val(isa[isa_idx]);
			
			// Tail recursion.
			if (val < st)
				return find_pattern_occurrence(csa, pat1_len, st, ed, mid, rb, i);
			else if (ed < val)
				return find_pattern_occurrence(csa, pat1_len, st, ed, lb, mid, i);
			else
			{
				i = mid;
				return true;
			}
		}
		
		
		static void find_range_in_h(
			h_type const &h,
			h_u_type const &hu,
			cst_type::size_type const k,
			cst_type::size_type const r_len,
			h_u_type::value_type &l,
			h_u_type::value_type &r
		)
		{
			h_u_type::leaf_iterator it;
			if (hu.find_predecessor(r_len, it, true))
			{
				l = hu.dereference(it);
				if (hu.find_successor(r_len, it, true))
					r = hu.dereference(it);
				else
					r = l + h.diff;
			}
			else if (hu.find_successor(r_len, it, true));
			{
				r = hu.dereference(it);
				l = r - h.diff;
			}
			else
			{
				l = k - h.diff;
				r = k + diff;
			}
		}
		
		
		// Lemma 19.
		static void tree_search(
			cst_type const &cst,
			gamma_type const &gamma,
			core_endpoints_type const &core_endpoints,
			lcp_rmq_type const &lcp_rmq,
			h_type const &h,
			cst_type::node_type const u,
			cst_type::node_type const core_path_beginning,
			cst_type::char_type c,
			f_type::value_type const st,
			f_type::value_type const ed
		)
		{
			cst_type::size_type pos(0);
			auto const v(cst.child(u, c, pos));
			
			// Check if found.
			if (cst.root() == v)
				return;
			
			csa_type::size_type i(0);
			
			auto const &gamma_v(gamma.find(v));
			if (gamma.cend() == gamma_v)
			{
				// No Γ_v with this node so it must be a core node. Find the
				// core leaf node x at the end of the core path.
				cst_type::node_type x(core_endpoints.find_close(core_path_beginning));
				auto const &hx(h.maps[x]);
				if (hx.l.size() == 0 && hx.r.size() == 0)
				{
					// Case 1.
					// The number of leaves hanging from the core path is < log₂²n.
					auto const pat1_len(cst.depth(u));
					auto const v_le(cst.lb(v));
					auto const v_ri(cst.rb(v));
					if (!find_pattern_occurrence(cst.csa, pat1_len, st, ed, v_le, v_ri, i))
						return;
				}
				else
				{
					auto const &csa(cst.csa);
					auto const &isa(csa.isa);
					auto const k(cst.id(x));
					auto const q(lcp_rmq(isa[csa[k] + pat1_len + 1], st));
					auto const pat2_len(/* FIXME: write me. */);
					if (q >= pat2_len)
					{
						// Case 2.
						// x corresponds to a suffix with P as its prefix.
						i = k;
					}
					else
					{
						// Case 3.
						// Since x is a core leaf node, we have (some of) the indices
						// of the suitable lcp values stored in hx.l and hx.r.
						auto const r_len(pat1_len + q + 1);
						h_u_type::value_type jll{0}, jlr{0}, jrl{0}, jrr{0};

						find_range_in_h(h, hx.l, k, r_len, jll, jlr);
						find_range_in_h(h, hx.r, k, r_len, jrl, jrr);
						
						// FIXME: complete me.
					}
				}
				
				// FIXME: complete me.
			}
			else
			{
				// v is a side node.
				auto const pat1_len(cst.depth(u));
				auto const v_le(cst.lb(v));
				auto const v_ri(cst.rb(v));

				if (0 == gamma_v.size())
				{
					// Case 1.
					if (!find_pattern_occurrence(cst.csa, pat1_len, st, ed, v_le, v_ri, i))
						return;
				}
				else
				{
					// Find an i s.t. st ≤ i ≤ ed by allowing equality and checking the upper bound.
					decltype(gamma_v)::const_subtree_iterator it;
					if (gamma_v.find_successor(st, it, true) && *it <= ed)
					{
						// Case 2.
						// Get i from ISA[SA[i] + |P₁| + 1].
						auto const isa_val(*it);
						auto const isa_idx(csa[val]);
						auto const sa_val(isa_idx - pat1_len - 1);
						auto const sa_idx(csa.isa[sa_val]);
						i = sa_idx;
					}
					else
					{
						// Case 3.
						// Extend the range slightly if possible.
						decltype(gamma_v)::const_subtree_iterator a_it, b_it;
						f_type::value_type a(st), b(ed);
						
						if (gamma_v.find_predecessor(st, a_it))
							a = *a_it;
						
						if (gamma_v.find_successor(ed, b_it))
							b = *b_it;
						
						// If extending was not possible, stop.
						if (st == a && ed == b)
							return;
						
						if (!find_pattern_occurrence(cst.csa, pat1_len, a, b, v_le, v_ri, i))
							return;
					}
				}
			}
			
			// FIXME: complete me.
		}
		
		
		// Section 3.3.
		static void find_1_mismatch()
		{
			// FIXME: complete me.
		}
	};
}


void test(std::string const &input)
{
	std::string filename("@input.iv8");
	sdsl::store_to_file(input.c_str(), file);

	sdsl::csa_rao<t_spec> csa;
	sdsl::construct(csa, file, 1);
	std::cout << "---------" << std::endl;

	sdsl::csa_rao<t_spec> csa2;
	csa2 = std::move(csa);
}

#endif
