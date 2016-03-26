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

#ifndef ASM_LSW_K1_MATCHER_HELPER_HH
#define ASM_LSW_K1_MATCHER_HELPER_HH

#include <asm_lsw/bp_support_sparse.hh>
#include <asm_lsw/k1_matcher.hh>
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
	class k1_matcher <t_cst>::h_type
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
		typedef k1_matcher <t_cst> k1_matcher;
		
		// Indexed by identifiers from node_id().
		typedef unordered_map <typename cst_type::size_type, h_pair> h_map;
		typedef std::map <typename cst_type::size_type, h_pair> h_map_intermediate;

	protected:
		h_map m_maps;
		typename cst_type::size_type m_diff;

	protected:
		// Section 3.1, Lemma 17.
		static void construct_hl_hr(
			k1_matcher const &matcher,
			typename cst_type::node_type const v,
			typename cst_type::node_type const u,
			typename cst_type::size_type const log2n,
			h_pair &h
		)
		{
			auto const &cst(matcher.m_cst);
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
				auto const lcp_len(i == k ? plen_u : matcher.lcp_length(std::min(k, i), std::max(k, i)));
				
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
		h_type(k1_matcher const &matcher)
		{
			auto const &cst(matcher.m_cst);
			auto const &ce(matcher.m_ce);
			
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
				auto const v(matcher.node_inv_id(v_id));
				auto const u(matcher.node_inv_id(u_id));
				
				// u is now a core leaf node.
				assert(cst.is_leaf(u));
				h_pair h_u;
				construct_hl_hr(matcher, v, u, log2n, h_u);
				maps_i[u_id] = std::move(h_u);
			}
			
			typename h_map::template builder_type <h_map_intermediate> builder(maps_i);
			h_map maps_tmp(builder);
			m_maps = std::move(maps_tmp);
		}
	};


	// Def. 1, 2, Lemma 10.
	template <typename t_cst>
	class k1_matcher <t_cst>::f_type
	{
	public:
		typedef f_vector_type::size_type size_type;
		static f_vector_type::value_type const not_found{std::numeric_limits<f_vector_type::value_type>::max()};
		
	protected:
		typedef k1_matcher <t_cst> k1_matcher;

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
		f_type(k1_matcher const &matcher, pattern_type const &pattern) const
		{
			auto const &cst(matcher.m_cst);
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
				if (matcher.find_path(pattern, i, node))
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

	
	template <typename t_cst>
	struct k1_matcher <t_cst>::transform_gamma_v
	{
		typename gamma_type::kv_type operator()(typename gamma_intermediate_type::value_type &kv) const
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
}

#endif
