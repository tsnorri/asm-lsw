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


#ifndef ASM_LSW_UTIL_HH
#define ASM_LSW_UTIL_HH

#include <algorithm>
#include <limits>
#include <sdsl/io.hpp>
#include <type_traits>

#define DO_PRAGMA(x) _Pragma (#x)
#define TODO(x) DO_PRAGMA(message ("TODO - " #x))

#ifdef HAVE_ATTRIBUTE_PURE
#define ASM_LSW_PURE __attribute__((pure))
#else
#define ASM_LSW_PURE
#endif

#ifdef HAVE_ATTRIBUTE_CONST
#define ASM_LSW_CONST __attribute__((const))
#else
#define ASM_LSW_CONST
#endif


namespace asm_lsw { namespace util {
	// Preserve C++11 compatibility.
	template <typename T>
	using remove_c_t = typename std::remove_const <T>::type;

	template <typename T>
	using remove_ref_t = typename std::remove_reference <T>::type;

	template <typename T>
	using remove_c_ref_t = remove_c_t <remove_ref_t <T>>;

	template <typename t_value>
	struct iterator_access
	{
		template <typename t_iterator>
		auto key(t_iterator const &it) const -> decltype(it->first) & { return it->first; }
	};

	template <>
	struct iterator_access <void>
	{
		template <typename t_iterator>
		auto key(t_iterator const &it) const -> decltype(*it) & { return *it; }
	};
	
	
	template <typename T, typename U>
	ASM_LSW_CONST auto min(T const a, U const b) -> typename std::common_type <T, U>::type
	{
		typedef typename std::common_type <T, U>::type return_type;
		return std::min <return_type>(a, b);
	}
	
	
	template <typename t_ranges>
	void post_process_ranges(t_ranges &ranges)
	{
		std::sort(ranges.begin(), ranges.end());
		
		auto out_it(ranges.begin()), it(ranges.begin()), end(ranges.end());
		if (it != end)
		{
			while (true)
			{
				auto val(*it);
				auto first(val.first);
				auto last(val.second);
				
				auto n_it(it);
				while (true)
				{
					++n_it;
					if (n_it == end)
					{
						val.first = first;
						val.second = last;
						*out_it = val;
						++out_it;
						ranges.resize(std::distance(ranges.begin(), out_it));
						return;
					}
					
					auto n_first(n_it->first);
					auto n_last(n_it->second);
					
					assert(first <= n_first);
					if (n_first <= 1 + last)
					{
						if (last < n_last)
							last = n_last;
					}
					else
					{
						it = n_it;
						break;
					}
				}
				
				val.first = first;
				val.second = last;
				*out_it = val;
				++out_it;
			}
		}
	}
	
	
	// Choose either write_member or serialize. The latter probably isn't needed much.
	template <typename t_value, bool t_has_serialize = sdsl::has_serialize <t_value>::value>
	struct serialize_value_fn {};

	template <typename t_value>
	struct serialize_value_fn <t_value, true>
	{
		std::size_t operator()(t_value const &value, std::ostream &out, sdsl::structure_tree_node *node)
		{
			return value.serialize(out, node, "");
		}
	};
	
	template <typename t_value>
	struct serialize_value_fn <t_value, false>
	{
		std::size_t operator()(t_value const &value, std::ostream &out, sdsl::structure_tree_node *node)
		{
			return sdsl::write_member_nn(value, out);
		}
	};
	
	
	// Choose either read_member or load. The latter probably isn't needed much.
	template <typename t_value, bool t_has_load = sdsl::has_load <t_value>::value>
	struct load_value_fn {};

	template <typename t_value>
	struct load_value_fn <t_value, true>
	{
		void operator()(t_value &value, std::istream &in)
		{
			value.load(in);
		}
	};
	
	template <typename t_value>
	struct load_value_fn <t_value, false>
	{
		void operator()(t_value &value, std::istream &in)
		{
			sdsl::read_member(value, in);
		}
	};
}}

#endif
