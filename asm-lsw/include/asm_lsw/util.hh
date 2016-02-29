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

#include <type_traits>

namespace asm_lsw {
	// Preserve C++11 compatibility.
	template <typename T>
	using remove_c_t = typename std::remove_const <T>::type;

	template <typename T>
	using remove_ref_t = typename std::remove_reference <T>::type;

	template <typename T>
	using remove_c_ref_t = remove_c_t <remove_ref_t <T>>;
}

#endif
