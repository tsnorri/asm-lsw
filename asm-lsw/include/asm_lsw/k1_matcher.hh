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


// FIXME: use integer functions for logarithm and power.
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
		class h_type
		{
		public:
			typedef std::map<
				typename csa_type::size_type,
				typename csa_type::size_type
			>															h_u_intermediate_type;
			typedef y_fast_trie_compact_as<
				typename csa_type::size_type,
				typename csa_type::size_type
			>															h_u_type;

			struct h_pair
			{
				std::unique_ptr <h_u_type> l;
				std::unique_ptr <h_u_type> r;
			};

		protected:
			// Indexed by identifiers from node_id().
			typedef unordered_map <typename cst_type::size_type, h_pair> h_map;
			typedef std::map <typename cst_type::size_type, h_pair> h_map_intermediate;

		protected:
			h_map m_maps;
			typename cst_type::size_type m_diff;

		protected:
			// Section 3.1, Lemma 17.
			static void construct_hl_hr(
				cst_type const &cst,
				lcp_rmq_type const &lcp_rmq,
				typename cst_type::node_type const v,
				typename cst_type::node_type const u,
				typename cst_type::size_type const log2n,
				h_pair &h
			)
			{
				auto const k(cst.lb(u));			// Left bound in SA i.e. SA index.
				auto const plen_u(cst.depth(u));
				auto const plen_v(cst.depth(v));
				
				auto const leaf_count(cst.size());
				typename cst_type::size_type i(0), j(0);
				h_u_intermediate_type hl_tmp, hr_tmp;
				
				while ((i = 1 + j * log2n) < leaf_count)
				{
					// Now i is an SA index s.t.
					//  i ∈  {i | i ≡ 1 (mod log₂²n)}
					//     = {i | i - 1 divisible by log₂²n}
					//     = {1 + j log₂²n | j ∈ ℕ}
					
					// Check |lcp(k, i)| ≥ plen(v).
					auto const lcp_len(i == k ? plen_u : lcp_length(cst, lcp_rmq, std::min(k, i), std::max(k, i)));
					
					// Check that LCP returns csa_type::size_type (for key).
					{
						typedef typename h_u_intermediate_type::key_type hu_key_type;
						static_assert(
							std::numeric_limits <decltype(lcp_len)>::max() <= std::numeric_limits <hu_key_type>::max(),
							""
						);
					}
					
					if (lcp_len >= plen_v)
					{
						if (i <= k)
							hl_tmp.insert(std::make_pair(lcp_len, i));
						else if (i > k)
							hr_tmp.insert(std::make_pair(lcp_len, i));
					}
					
					++j;
				}
				
				// Compress hl and hr.
				// Another option would be using a value transformer similar to transform_gamma_v.
				if (hl_tmp.size())
					h.l.reset(h_u_type::construct(hl_tmp, hl_tmp.cbegin()->first, hl_tmp.crbegin()->first));
				else
					h.l.reset(h_u_type::construct(hl_tmp, 0, 0));
				
				if (hr_tmp.size())
					h.r.reset(h_u_type::construct(hr_tmp, hr_tmp.cbegin()->first, hr_tmp.crbegin()->first));
				else
					h.r.reset(h_u_type::construct(hr_tmp, 0, 0));
			}

		public:
			h_map const &maps() const { return m_maps; }
			typename cst_type::size_type diff() const { return m_diff; }

			h_type() {}
			
			// Section 3.1, Lemma 17.
			h_type(cst_type const &cst, core_endpoints_type const &ce, lcp_rmq_type const &lcp_rmq)
			{
				auto const n(cst.size());						// Text length.
				auto const logn(sdsl::util::log2_floor(n));
				auto const log2n(logn * logn);
				assert(logn);
				m_diff = log2n;

				// Number of (opening) parentheses indicates the number of core paths.
				auto const &ce_bps(ce.bps());
				auto const count(ce_bps.rank(ce_bps.size() - 1));
				
				h_map_intermediate maps_i;
				for (util::remove_c_t<decltype(count)> i{0}; i < count; ++i)
				{
					// Find the index of each opening parenthesis and its counterpart,
					// then convert to sparse index.
					auto const ce_bps_begin(ce_bps.select(1 + i));
					auto const ce_bps_end(ce_bps.find_close(ce_bps_begin));
					auto const v_id(ce.to_sparse_idx(ce_bps_begin));
					auto const u_id(ce.to_sparse_idx(ce_bps_end));
					auto const v(node_inv_id(cst, v_id));
					auto const u(node_inv_id(cst, u_id));
					
					// u is now a core leaf node.
					assert(cst.is_leaf(u));
					h_pair h_u;
					construct_hl_hr(cst, lcp_rmq, v, u, log2n, h_u);
					maps_i[u_id] = std::move(h_u);
				}
				
				typename h_map::template builder_type <h_map_intermediate> builder(maps_i);
				h_map maps_tmp(builder);
				m_maps = std::move(maps_tmp);
			}
		};


		// Def. 1, 2, Lemma 10.
		class f_type
		{
		public:
			typedef f_vector_type::size_type size_type;
			static f_vector_type::value_type const not_found{std::numeric_limits<f_vector_type::value_type>::max()};

		protected:
			f_vector_type m_st;
			f_vector_type m_ed;

		public:
			// Return F_st[i].
			auto st(cst_type const &cst, f_vector_type::size_type const i) const -> f_vector_type::value_type
			{
				return (i < m_st.size() ? m_st[i] : 0);
			}
			
			
			// Return F_ed[i].
			auto ed(cst_type const &cst, f_vector_type::size_type const i) const -> f_vector_type::value_type
			{
				return (i < m_ed.size() ? m_ed[i] : cst.size());
			}


			f_type()
			{
			}


			// Construct the F arrays as defined in definitions 1 and 2 and lemma 10.
			// FIXME: calculate time complexity.
			f_type(cst_type const &cst, pattern_type const &pattern)
			{
				auto const m(pattern.size());
				pattern_type::size_type i(0);
				typename cst_type::node_type node;

				f_vector_type st(m, 0);
				f_vector_type ed(m, 0);

				// Start with i = 1 in pattern[i..m] (zero-based in code).
				// If not found in CST, fill with sentinels, set i = 2
				// and continue (Lam et al. 2.4 lemma 10).
				// FIXME: I'm not sure if this is done right since an upper limit for the whole pattern is O(|pattern| * |pattern| / 2 * t_SA). (There could be a smaller upper limit, though.)
				while (i < m)
				{
					// Resetting node is safe (in terms of time complexity) as long as a match hasn't been found.
					node = cst.root();
					if (find_path(cst, pattern, i, node))
					{
						// Find the SA indices of the leftmost and rightmost leaves.
						auto const lb(cst.lb(node));
						auto const rb(cst.rb(node));
						st[i] = lb;
						ed[i] = rb;
						++i;
						break;
					}
					else
					{
						st[i] = not_found;
						ed[i] = not_found;
						++i;
					}
				}

				// Continue with the suffix links.
				while (i < m)
				{
					node = cst.sl(node);
					
					// Remove characters from the end.
					while (true)
					{
						auto parent(cst.parent(node));
						if (cst.depth(parent) < m - i)
							break;
						
						node = parent;
					}
					
					st[i] = cst.lb(node);
					ed[i] = cst.rb(node);
					++i;
				}

				m_st = std::move(st);
				m_ed = std::move(ed);
			}
		};

	protected:
		struct transform_gamma_v
		{
			typename gamma_type::kv_type operator()(typename gamma_intermediate_type::value_type &kv)
			{
				if (kv.second.size())
				{
					typename gamma_type::mapped_type value(gamma_v_type::construct(
						kv.second,
						*kv.second.cbegin(),
						*kv.second.crbegin()
					));
					return std::make_pair(kv.first, std::move(value));
				}
				else
				{
					typename gamma_type::mapped_type value(gamma_v_type::construct(kv.second, 0, 0));
					return std::make_pair(kv.first, std::move(value));
				}
			}
		};
		
		
	public:
		static typename cst_type::size_type node_id(
			cst_type const &cst, typename cst_type::node_type const node
		)
		{
			return cst.bp_support.rank(node) - 1;
		}

		
		static typename cst_type::node_type node_inv_id(
			cst_type const &cst, typename cst_type::size_type const node_id
		)
		{
			return cst.bp_support.select(1 + node_id);
		}

		// Find the path for the given pattern.
		// Set node to the final node if found.
		// Time complexity O((|pattern| - start_idx) * t_SA)
		static bool find_path(
			cst_type const &cst,
			pattern_type const &pattern,
			pattern_type::size_type const start_idx,
			typename cst_type::node_type &node // inout
		)
		{
			auto const begin(pattern.cbegin() + start_idx);
			auto const end(pattern.cend());
			assert(begin < end);
			
			pattern_type::size_type char_pos(0);
			return (0 < sdsl::forward_search(cst, node, 0, begin, end, char_pos));
		}
		
		
		// Check if the given node is a side node as per section 2.5.
		// FIXME: calculate time complexity.
		static bool is_side_node(
			cst_type const &cst, core_nodes_type const &cn, typename cst_type::node_type const node
		)
		{
			auto nid(node_id(cst, node));
			return 0 == cn[nid];
		}
		
		
		static lcp_rmq_type::size_type lcp_length(
			cst_type const &cst,
			lcp_rmq_type const &lcp_rmq,
			lcp_rmq_type::size_type const l,
			lcp_rmq_type::size_type const r
		)
		{
			assert(l < r);
			auto const idx(lcp_rmq(l + 1, r));
			auto const retval(cst.lcp[idx]);
			return retval;
		}
		
		
		// Allow equal values for l and r.
		static lcp_rmq_type::size_type lcp_length_e(
			cst_type const &cst,
			lcp_rmq_type const &lcp_rmq,
			lcp_rmq_type::size_type const l,
			lcp_rmq_type::size_type const r
		)
		{
			assert(l < cst.csa.size());
			assert(r < cst.csa.size());
			
			if (l == r)
				return cst.depth(cst.select_leaf(l));
			
			return lcp_length(cst, lcp_rmq, l, r);
		}
		
		
		// Construct a bit vector core_nodes for listing core nodes. (Not included in the paper but needed for section 3.1.)
		// Space complexity: O(n) bits since the number of nodes in a suffix tree with |T| = n is 2n = O(n).
		// FIXME: calculate time complexity.
		static void construct_core_paths(cst_type const &cst, core_nodes_type &cn)
		{
			std::map <
				typename cst_type::node_type,
				typename cst_type::size_type
			> node_depths;
			sdsl::bit_vector core_nodes(cst.nodes(), 0); // Nodes with incoming core edges.
			
			// Count node depths from the bottom and choose the greatest.
			for (auto it(cst.begin_bottom_up()), end(cst.end_bottom_up()); it != end; ++it)
			{
				typename cst_type::node_type node(*it);
				if (cst.is_leaf(node))
				{
					auto res(node_depths.emplace(std::make_pair(node, 1)));
					assert(res.second); // Insertion should have happened.
				}
				else
				{
					// Not a leaf, iterate the children and choose.
					typename cst_type::size_type max_nd(0);
					typename cst_type::node_type argmax_nd(cst.root());
					
					auto children(cst.children(node));
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
					typename cst_type::size_type nid(node_id(cst, argmax_nd));
					core_nodes[nid] = 1;
					
					auto res(node_depths.emplace(std::make_pair(node, 1 + max_nd)));
					assert(res.second); // Insertion should have happened.
				}
			}
			
			cn = std::move(core_nodes);
		}
		
		
		// Core path endpoints from Lemma 15.
		// FIXME: calculate time complexity.
		static void construct_core_path_endpoints(cst_type const &cst, core_nodes_type const &cn, core_endpoints_type &ce)
		{
			auto const leaf_count(cst.size());
			auto const node_count(cst.nodes());
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
					auto const leaf(cst.select_leaf(1 + i));
					auto const leaf_id(node_id(cst, leaf));
					if (cn[leaf_id])
					{
						++path_count;
						closing[leaf_id] = 1;

						auto node(leaf);
						do
						{
							assert(cst.root() != node);
							node = cst.parent(node);
						}
						while (cn[node_id(cst, node)]);
						
						auto const nid(node_id(cst, node));
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


		static void construct_uncompressed_gamma_sets(cst_type const &cst, core_nodes_type const &cn, gamma_intermediate_type &gamma)
		{
			// Γ_v = {ISA[SA[i] + plen(u) + 1] | i ≡ 1 (mod log₂²n) and v_le ≤ i ≤ v_ri}.

			auto const n(cst.size());
			auto const logn(sdsl::util::log2_floor(n));
			auto const log2n(logn * logn);
			auto const &csa(cst.csa);
			auto const &isa(csa.isa);

			// cst.begin() returns an iterator that visits inner nodes twice.
			for (auto it(cst.begin_bottom_up()), end(cst.end_bottom_up()); it != end; ++it)
			{
				typename cst_type::node_type v(*it);
				if (is_side_node(cst, cn, v))
				{
					typename cst_type::node_type const u(cst.parent(v));

					// Find the SA indices of the leftmost and rightmost leaves.
					auto const lb(cst.lb(v));
					auto const rb(cst.rb(v));
					assert (lb <= rb); // FIXME: used to be lb < rb. Was this intentional?

					auto const v_id(node_id(cst, v));
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
						auto isa_idx(csa[i] + cst.depth(u) + 1);
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
		
		
		static void construct_gamma_sets(cst_type const &cst, core_nodes_type const &cn, gamma_type &gamma)
		{
			gamma_intermediate_type gamma_i;
			construct_uncompressed_gamma_sets(cst, cn, gamma_i);
			
			typename gamma_type::template builder_type <gamma_intermediate_type, transform_gamma_v> builder(gamma_i);
			gamma_type gamma_tmp(builder);
			gamma = std::move(gamma_tmp);
		}
		
		
		// Create the auxiliary data structures in order to perform range minimum queries on the LCP array.
		static void construct_lcp_rmq(cst_type const &cst, lcp_rmq_type &rmq)
		{
			lcp_rmq_type rmq_tmp(&cst.lcp);
			rmq = std::move(rmq_tmp);
		}
				
		
		// Get i from j = ISA[SA[i] + |P₁| + 1].
		static typename cst_type::size_type sa_idx_of_stored_isa_val(
			typename cst_type::csa_type const &csa,
			typename cst_type::csa_type::isa_type::value_type const isa_val,
			typename cst_type::size_type const pat1_len
		)
		{
			auto const isa_idx(csa[isa_val]);
			auto const sa_val(isa_idx - pat1_len - 1);
			auto const sa_idx(csa.isa[sa_val]);
			return sa_idx;
		}

		
		// Find one occurrence of the pattern (index i) s.t. st ≤ ISA[SA[i] + |P₁| + 1] ≤ ed.
		static bool find_pattern_occurrence(
			csa_type const &csa,
			pattern_type::size_type const pat1_len,
			typename csa_type::size_type const st,
			typename csa_type::size_type const ed,
			typename csa_type::size_type const lb,
			typename csa_type::size_type const rb,
			typename csa_type::size_type &i
		)
		{
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
				return find_pattern_occurrence(csa, pat1_len, st, ed, 1 + mid, rb, i);
			else if (ed < val)
				return find_pattern_occurrence(csa, pat1_len, st, ed, lb, mid - 1, i);
			else
			{
				assert(st <= val && val <= ed);
				i = mid;
				return true;
			}
		}
		
		
		// Find SA index i s.t. |lcp(i, k)| = |P₁| + q + 1 using binary search.
		template <template <typename> class t_cmp>
		static bool find_node_ilr_bin(
			cst_type const &cst,
			lcp_rmq_type const &lcp_rmq,
			typename csa_type::size_type const k,
			typename cst_type::size_type const r_len,
			typename csa_type::size_type const l,
			typename csa_type::size_type const r,
			typename csa_type::size_type &i
		)
		{
			t_cmp <typename csa_type::size_type> cmp;
			
			if (r < l)
				return false;

			auto const mid(l + (r - l) / 2);
			auto const res(lcp_length_e(cst, lcp_rmq, std::min(mid, k), std::max(mid, k)));

			// Tail recursion.
			// The first case prevents overflow for mid - 1.
			if (res != r_len && l == mid)
				return false;
			if (cmp(res, r_len))
				return find_node_ilr_bin <t_cmp>(cst, lcp_rmq, k, r_len, 1 + mid, r, i);
			else if (cmp(r_len, res))
				return find_node_ilr_bin <t_cmp>(cst, lcp_rmq, k, r_len, l, mid - 1, i);
			else
			{
				i = mid;
				return true;
			}
		}

		
		// Partition the range if needed since k has the longest lcp with itself.
		static bool find_node_ilr_bin_p(
			cst_type const &cst,
			lcp_rmq_type const &lcp_rmq,
			typename csa_type::size_type const k,
			typename cst_type::size_type const r_len,
			typename csa_type::size_type const l,
			typename csa_type::size_type const r,
			typename csa_type::size_type &i
		)
		{
			// Order is ascending up to k, then descending.
			if (l < k && k < r)
			{
				if (find_node_ilr_bin <std::less>(cst, lcp_rmq, k, r_len, l, k, i))
					return true;
				
				return find_node_ilr_bin <std::greater>(cst, lcp_rmq, k, r_len, 1 + k, r, i);
			}

			if (r < k)
				return find_node_ilr_bin <std::less>(cst, lcp_rmq, k, r_len, l, r, i);

			return find_node_ilr_bin <std::greater>(cst, lcp_rmq, k, r_len, l, r, i);
		}


		// Lemma 19 case 3 (with v being a core node).
		// Find a suitable range in H_u s.t. either 
		//  |lcp(j - log₂²n, k)| ≤ r_len ≤ |lcp(j, k)| or
		//  |lcp(j, k)| ≤ r_len ≤ |lcp(j + log₂²n, k)|
		// Then use the range to search for a suitable node.
		static bool find_node_ilr(
			cst_type const &cst,
			lcp_rmq_type const &lcp_rmq,
			h_type const &h,
			typename h_type::h_pair const &hx,
			typename csa_type::size_type const k,
			typename cst_type::size_type const r_len,	// cst.depth returns size_type.
			typename csa_type::size_type &i
		)
		{
			typename csa_type::size_type l(0), r(0);
			typename h_type::h_u_type::const_subtree_iterator it;
			auto const h_diff(h.diff());
			auto const lcp_rmq_size(lcp_rmq.size());
			auto const lcp_rmq_max(lcp_rmq_size ? lcp_rmq_size - 1 : 0);

			// Try i^l.
			if (hx.l->find_successor(r_len, it, true))
			{
				r = hx.l->iterator_value(it);
				l = ((h_diff <= r) ? (r - h_diff) : 0);
				// XXX Is l guaranteed to have |lcp(l, k)| ≤ r_len? (Case 3)

				if (find_node_ilr_bin_p(cst, lcp_rmq, k, r_len, l, r, i))
					return true;
			}

			// Try i^r.
			if (hx.r->find_predecessor(r_len, it, true))
			{
				l = hx.r->iterator_value(it);
				r = ((l + h_diff <= lcp_rmq_max) ? (l + h_diff) : lcp_rmq_max);
				// XXX Is r guaranteed to have r_len ≤ |lcp(r, k)|? (Case 3)

				if (find_node_ilr_bin_p(cst, lcp_rmq, k, r_len, l, r, i))
					return true;
			}

			// Try i^l again with another range.
			l = ((h_diff <= k) ? (k - h_diff) : 0);
			r = ((k + h_diff <= lcp_rmq_max) ? (k + h_diff) : lcp_rmq_max);
			if (find_node_ilr_bin_p(cst, lcp_rmq, k, r_len, l, r, i))
				return true;			

			return false;
		}


		// Use Lemma 11.
		static void extend_range(
			cst_type const &cst,
			pattern_type::size_type const pat1_len,
			typename csa_type::size_type const st,
			typename csa_type::size_type const ed,
			typename csa_type::size_type const v_le,
			typename csa_type::size_type const v_ri,
			typename csa_type::size_type &left,
			typename csa_type::size_type &right
		)
		{
			auto const &csa(cst.csa);
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
		static bool tree_search_side_node(
			cst_type const &cst,
			gamma_v_type const &gamma_v,
			typename cst_type::node_type const u,
			typename cst_type::node_type const v,
			typename csa_type::size_type const st,
			typename csa_type::size_type const ed,
			typename csa_type::size_type &i
		)
		{
			// v is a side node.
			auto const &csa(cst.csa);
			auto const pat1_len(cst.depth(u));
			auto const v_le(cst.lb(v));
			auto const v_ri(cst.rb(v));

			if (0 == gamma_v.size())
			{
				// Case 1.
				if (find_pattern_occurrence(csa, pat1_len, st, ed, v_le, v_ri, i))
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
					i = sa_idx_of_stored_isa_val(csa, isa_val, pat1_len);
#if 0
					auto const isa_val(*it);
					auto const isa_idx(csa[isa_val]);
					auto const sa_val(isa_idx - pat1_len - 1);
					auto const sa_idx(csa.isa[sa_val]);
					i = sa_idx;
#endif
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
						a = sa_idx_of_stored_isa_val(csa, a_val, pat1_len);
					}
					
					if (gamma_v.find_successor(ed, b_val_it))
					{
						auto const b_val(*b_val_it);
						b = sa_idx_of_stored_isa_val(csa, b_val, pat1_len);
					}
					
					if (find_pattern_occurrence(csa, pat1_len, st, ed, a, b, i))
						return true;
					
#if 0
					typename gamma_v_type::const_subtree_iterator a_it, b_it;
					typename csa_type::size_type a(st), b(ed);
					
					if (gamma_v.find_predecessor(st, a_it))
						a = *a_it;
					
					if (gamma_v.find_successor(ed, b_it))
						b = *b_it;
					
					// If extending was not possible, stop.
					if (st == a && ed == b)
						return false;
					
					if (find_pattern_occurrence(csa, pat1_len, a, b, v_le, v_ri, i))
						return true;
#endif
				}
			}

			return false;
		}
		
		
		// Lemma 19.
		static bool tree_search(
			cst_type const &cst,
			f_type const &f,
			gamma_type const &gamma,
			core_endpoints_type const &core_endpoints,
			lcp_rmq_type const &lcp_rmq,
			h_type const &h,
			pattern_type const &pattern,
			typename cst_type::node_type const u,
			typename cst_type::node_type core_path_beginning,
			typename cst_type::char_type const c,
			typename csa_type::size_type const st,
			typename csa_type::size_type const ed,
			typename csa_type::size_type &left,
			typename csa_type::size_type &right
		)
		{
			typename cst_type::size_type pos(0);
			auto const v(cst.child(u, c, pos));
			
			// Check if found.
			if (cst.root() == v)
				return false;
			
			typename csa_type::size_type i(0);
			
			auto const v_id(node_id(cst, v));
			auto const g_it(gamma.find(v_id));
			auto const pat1_len(cst.depth(u));
			auto const v_le(cst.lb(v));
			auto const v_ri(cst.rb(v));

			// If we diverged from the previous core path, update.
			auto core_path_beginning_id(0);
			if (core_endpoints_type::input_value::Opening == core_endpoints[v_id])
			{
				core_path_beginning = v;
				core_path_beginning_id = v_id;
			}
			else
			{
				core_path_beginning_id = node_id(cst, core_path_beginning);
			}
			
			// We still need to check the node type (core or side) in case
			// v doesn't begin a core path.
			if (gamma.cend() == g_it)
			{
				// No Γ_v with this node so v must be a core node. Find the
				// core leaf node x at the end of the core path.
				// Every core leaf node is supposed to have a set H associated with it.
				auto const x_id(core_endpoints.find_close(core_path_beginning_id));
				auto const h_it(h.maps().find(x_id));
				assert(h.maps().cend() != h_it);
				auto const &hx(h_it->second);
				if (hx.l->size() == 0 && hx.r->size() == 0)
				{
					// Case 1.
					// The number of leaves hanging from the core path is < log₂²n.
					if (!find_pattern_occurrence(cst.csa, pat1_len, st, ed, v_le, v_ri, i))
						return false;

					left = i;
					right = i;
					extend_range(cst, pat1_len, st, ed, v_le, v_ri, left, right);
					return true;
				}
				else
				{
					auto const &csa(cst.csa);
					auto const &isa(csa.isa);
					auto const x(node_inv_id(cst, x_id));
					assert(cst.is_leaf(x));
					
					// Get the suffix array index.
					auto const k(cst.id(x));
					auto const isa_idx(csa[k] + pat1_len + 1);
					
					// Check if the pattern may still be found in the suffix tree.
					if (! (isa_idx < isa.size()))
						return false;
					
					auto const isa_val(isa[isa_idx]);
					auto const q(lcp_length_e(cst, lcp_rmq, std::min(isa_val, st), std::max(isa_val, st)));
					auto const pat2_len(pattern.size() - 1 - pat1_len);
					if (q >= pat2_len)
					{
						// Case 2.
						// x corresponds to a suffix with P as its prefix.
						left = k;
						right = k;
						extend_range(cst, pat1_len, st, ed, v_le, v_ri, left, right);
						return true;
					}
					else
					{
						// Case 3.
						// Since x is a core leaf node, we have (some of) the indices
						// of the suitable LCP values stored in hx.l and hx.r.
						auto const r_len(pat1_len + q + 1);
						typename csa_type::size_type ilr(0);
						if (!find_node_ilr(cst, lcp_rmq, h, hx, k, r_len, ilr)) // ilr is an SA index.
							return false;
						
						// XXX Paper requires O(t_SA) time complexity for LCA but cst_sada gives O(rr_enclose).
						auto const leaf(cst.select_leaf(1 + ilr)); // Convert the suffix array index.

						// Find r and check if it completely matches P.
						auto const r(cst.lca(leaf, x));
						auto const r_depth(cst.depth(r));
						if (pat1_len + 1 + pat2_len <= r_depth)
						{
							left = cst.lb(r);
							right = cst.rb(r);
							extend_range(cst, pat1_len, st, ed, v_le, v_ri, left, right); // FIXME: not sure if needed.
							return true;
						}
						else
						{
							// Since the match was incomplete, the matching node must be r's descendant.
							// The chosen edge should be a side edge (Case 3).
							auto const ncc(pattern[r_depth]);
#ifndef NDEBUG
							{
								auto const s(cst.child(r, ncc));
								auto const node_type(core_endpoints[node_id(cst, s)]);
								assert(cst.is_leaf(s) || core_endpoints_type::input_value::Opening == node_type);
							}
#endif
							auto const r_st(f.st(cst, 1 + r_depth));
							auto const r_ed(f.ed(cst, 1 + r_depth));
							return tree_search(cst, f, gamma, core_endpoints, lcp_rmq, h, pattern, r, r, ncc, r_st, r_ed, left, right);
						}

						return false;
					}
				}
			}
			else if (tree_search_side_node(cst, *(g_it->second), u, v, st, ed, i))
			{
				left = i;
				right = i;
				extend_range(cst, pat1_len, st, ed, v_le, v_ri, left, right);
				return true;
			}
			else
			{
				return false;
			}
		}
		
		
		// Section 3.3.
		static void find_1_approximate_at_i(
			cst_type const &cst,
			f_type const &f,
			gamma_type const &gamma,
			core_endpoints_type const &core_endpoints,
			lcp_rmq_type const &lcp_rmq,
			h_type const &h,
			pattern_type const &pattern,
			typename cst_type::node_type const u,
			typename cst_type::node_type const core_path_beginning,
			typename f_type::size_type const i,
			typename cst_type::char_type const cc,
			csa_range_set &ranges
		)
		{
			typename csa_type::size_type left{0}, right{0};
			auto const st(f.st(cst, i));
			auto const ed(f.ed(cst, i));

			// [st, ed] = [F_st[j], F_ed[j]] = range(cst, pattern[j, m]) = range(cst, pattern[1 + i, m]),
			// where m is the pattern length. If pattern[1 + i, m] isn't a prefix of any suffix in cst,
			// there aren't any occurrences of P₁cP₂ (as defined in Lemma 19) in cst.
			if (st != f_type::not_found)
			{
				assert (ed != f_type::not_found);
				if (tree_search(cst, f, gamma, core_endpoints, lcp_rmq, h, pattern, u, core_path_beginning, cc, st, ed, left, right))
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
		
		
		static void find_1_approximate_continue_exact(
			cst_type const &cst,
			typename cst_type::node_type v,
			pattern_type const &pattern,
			pattern_type::size_type const patlen,
			pattern_type::size_type eidx,
			pattern_type::size_type pidx,
			csa_range_set &ranges
		)
		{
			// Repeatedly select the matching branch of the suffix tree until
			// the end of the pattern has been reached.
			while (true)
			{
				auto const len(cst.depth(v));
				while (eidx < len)
				{
					if (! (pidx < patlen))
						goto end;
					
					auto const ec(cst.edge(v, 1 + eidx));
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
				v = cst.child(v, pattern[pidx], char_pos);
				
				if (cst.root() == v)
					return;
				
				// The first character is known to match since an edge was found.
				++eidx;
				++pidx;
			}
			
		end:
			csa_range range(cst.lb(v), cst.rb(v));
			ranges.emplace(std::move(range));
		}
		

		// Section 3.3.
		static void find_1_approximate(
			cst_type const &cst,
			f_type const &f,
			gamma_type const &gamma,
			core_endpoints_type const &core_endpoints,
			lcp_rmq_type const &lcp_rmq,
			h_type const &h,
			pattern_type const &pattern,
			csa_range_set &ranges
		)
		{
			typename cst_type::node_type u(cst.root());
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
					find_1_approximate_at_i(
						cst, f, gamma, core_endpoints, lcp_rmq, h, pattern, u, core_path_beginning, 2 + i, cc, ranges
					);
				}
				
				// 2. Substitution.
				// Test with all the characteres in Sigma.
				for (typename cst_type::sigma_type c(0); c < cst.csa.sigma; ++c)
				{
					auto const cc(cst.csa.comp2char[c]);
					if (pattern[i] != cc)
					{
						find_1_approximate_at_i(
							cst, f, gamma, core_endpoints, lcp_rmq, h, pattern, u, core_path_beginning, 1 + i, cc, ranges
						);
					}
				}
				
				// 3. Insertion.
				// Allow insertion of the same character to the end of the string.
				for (typename cst_type::sigma_type c(0); c < cst.csa.sigma; ++c)
				{
					auto const cc(cst.csa.comp2char[c]);
					if (1 + i == pattern.size() || pattern[i] != cc)
					{
						find_1_approximate_at_i(
							cst, f, gamma, core_endpoints, lcp_rmq, h, pattern, u, core_path_beginning, i, cc, ranges
						);
					}
				}

				// 4. Exact match.
				auto const cc(pattern[i]);
				typename cst_type::size_type char_pos{0};
				auto const v(cst.child(u, cc, char_pos));
				
				// Character at i doesn't match; there won't be more
				// matches since k = 1.
				if (cst.root() == v)
					return;
				
				assert(i == cst.depth(u));
				auto k(i);
				auto const len(cst.depth(v));
				while (true)
				{
					// If the end of the pattern is reached, stop.
					if (patlen == k)
					{
						csa_range range(cst.lb(v), cst.rb(v));
						ranges.emplace(std::move(range));
						return;
					}
					
					// If the end of the edge was reached, stop.
					// (Obviously the match needs to be checked first.)
					if (! (k < len))
						break;

					auto const ec(cst.edge(v, 1 + k));
					auto const pc(pattern[k]);
					if (ec != pc)
					{
						// One mismatch was found; the current index is the only remaining one
						// for which mismatches need to be considered.
						
						// 1. Deletion. Remove character at pattern[k].
						find_1_approximate_continue_exact(cst, v, pattern, patlen, k, k + 1, ranges);
						
						// Skip if ec is the zero character.
						if (ec)
						{
							// 2. Insertion. Suppose ec was inserted into pattern before k.
							find_1_approximate_continue_exact(cst, v, pattern, patlen, k + 1, k, ranges);
							
							// 3. Substitution. Suppose pattern[k] was replaced with ec.
							find_1_approximate_continue_exact(cst, v, pattern, patlen, k + 1, k + 1, ranges);
						}
						
						return;
					}
					
					++k;
				}
				
				// No mismatches were found, continue.
				u = v;
				i = cst.depth(v);
				
				// Check if the current node starts a core path and update core_path_beginning.
				auto const u_id(node_id(cst, u));
				if (core_endpoints_type::input_value::Opening == core_endpoints[u_id])
					core_path_beginning = u;

				// In case the pattern length was reached with i, add the match
				// to ranges.
				// FIXME: shouldn't be reached now.
				if (cst.is_leaf(u))
				{
					assert(0);
					auto const idx(cst.id(u));
					ranges.emplace(idx, idx);
				}
			}
		}
	};
}

#endif
