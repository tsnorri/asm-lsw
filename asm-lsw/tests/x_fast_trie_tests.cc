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

#include "trie_tests.hh"


go_bandit([](){

	// X-fast tries
	describe("X-fast trie <uint8_t>:", [](){
		typedef asm_lsw::x_fast_trie <uint8_t> trie_type;
		common_any_type_tests <trie_type, ref_trie_adaptor <trie_type>>();
		x_fast_any_tests <trie_type, ref_trie_adaptor <trie_type>>();
		set_type_tests <trie_type>();
		common_set_type_tests <trie_type, ref_trie_adaptor <trie_type>>();
	});

	describe("X-fast trie <uint32_t>:", [](){
		typedef asm_lsw::x_fast_trie <uint32_t> trie_type;
		common_any_type_tests <trie_type, ref_trie_adaptor <trie_type>>();
		x_fast_any_tests <trie_type, ref_trie_adaptor <trie_type>>();
		set_type_tests <trie_type>();
		common_set_type_tests <trie_type, ref_trie_adaptor <trie_type>>();
	});

	describe("X-fast trie <uint8_t, uint8_t>:", [](){
		typedef asm_lsw::x_fast_trie <uint8_t, uint8_t> trie_type;
		common_any_type_tests <trie_type, ref_trie_adaptor <trie_type>>();
		x_fast_any_tests <trie_type, ref_trie_adaptor <trie_type>>();
		common_map_type_tests <trie_type, ref_trie_adaptor <trie_type>>();
	});

	describe("X-fast trie <uint32_t, uint32_t>:", [](){
		typedef asm_lsw::x_fast_trie <uint32_t, uint32_t> trie_type;
		common_any_type_tests <trie_type, ref_trie_adaptor <trie_type>>();
		x_fast_any_tests <trie_type, ref_trie_adaptor <trie_type>>();
		common_map_type_tests <trie_type, ref_trie_adaptor <trie_type>>();
	});
});
