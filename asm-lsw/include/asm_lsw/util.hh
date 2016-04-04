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

#include <limits>
#include <type_traits>
 

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
}}

#endif
