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

	// Compact Y-fast tries (AS)
	describe("compact AS trie:", [](){
		common_as_type_tests <asm_lsw::y_fast_trie <uint32_t>, asm_lsw::y_fast_trie_compact_as <uint32_t>>();

		it("can be constructed with a vector", [](){
			typedef asm_lsw::y_fast_trie_compact_as <uint32_t> trie_type;
			std::vector <uint32_t> vec{5, 18, 22, 35, 108};

			std::unique_ptr <trie_type> trie_ptr(trie_type::construct(vec, 5, 108));
			AssertThat(trie_ptr->key_size(), Equals(1));
			AssertThat(trie_ptr->size(), Equals(vec.size()));

			for (auto const k : vec)
				AssertThat(trie_ptr->contains(k), Equals(true));
		});

		it("can be constructed with a map", [](){
			typedef asm_lsw::y_fast_trie_compact_as <uint32_t, uint32_t> trie_type;
			std::map <uint32_t, uint32_t> map{{5, 8}, {18, 21}, {22, 3}, {35, 7}, {108, 99}};

			std::unique_ptr <trie_type> trie_ptr(trie_type::construct(map, 5, 108));
			AssertThat(trie_ptr->key_size(), Equals(1));
			AssertThat(trie_ptr->size(), Equals(map.size()));

			for (auto const &kv : map)
			{
				trie_type::const_iterator it;
				bool st(trie_ptr->find(kv.first, it));
				AssertThat(st, Equals(true));
				AssertThat(it->first, Equals(kv.first));
				AssertThat(it->second, Equals(kv.second));
			}
		});
	});
	
	describe("compact Y-fast trie <uint8_t> (AS):", [](){
		typedef asm_lsw::y_fast_trie <uint8_t> trie_type;
		typedef asm_lsw::y_fast_trie_compact_as <uint8_t> ct_type;
		common_any_type_tests <trie_type, compact_as_trie_adaptor <trie_type, ct_type>>();
		common_set_type_tests <trie_type, compact_as_trie_adaptor <trie_type, ct_type>>();
	});

	describe("compact Y-fast trie <uint32_t> (AS):", [](){
		typedef asm_lsw::y_fast_trie <uint32_t> trie_type;
		typedef asm_lsw::y_fast_trie_compact_as <uint32_t> ct_type;
		common_any_type_tests <trie_type, compact_as_trie_adaptor <trie_type, ct_type>>();
		common_set_type_tests <trie_type, compact_as_trie_adaptor <trie_type, ct_type>>();
	});

	describe("compact Y-fast trie <uint8_t, uint8_t> (AS):", [](){
		typedef asm_lsw::y_fast_trie <uint8_t, uint8_t> trie_type;
		typedef asm_lsw::y_fast_trie_compact_as <uint8_t, uint8_t> ct_type;
		common_any_type_tests <trie_type, compact_as_trie_adaptor <trie_type, ct_type>>();
		common_map_type_tests <trie_type, compact_as_trie_adaptor <trie_type, ct_type>>();
	});

	describe("compact Y-fast trie <uint32_t, uint32_t> (AS):", [](){
		typedef asm_lsw::y_fast_trie <uint32_t, uint32_t> trie_type;
		typedef asm_lsw::y_fast_trie_compact_as <uint32_t, uint32_t> ct_type;
		common_any_type_tests <trie_type, compact_as_trie_adaptor <trie_type, ct_type>>();
		common_map_type_tests <trie_type, compact_as_trie_adaptor <trie_type, ct_type>>();
	});
});
