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


#include <asm_lsw/x_fast_tries.hh>
#include <asm_lsw/y_fast_tries.hh>
#include <bandit/bandit.h>
#include <boost/range/irange.hpp>

using namespace bandit;

go_bandit([](){

	describe("x-fast trie:", [](){
		it("can insert", [&](){
			asm_lsw::x_fast_trie <uint8_t> trie;
			trie.insert('a');
		});

		it("can find inserted keys", [&](){
			asm_lsw::x_fast_trie <uint8_t> trie;

			trie.insert('a');

			AssertThat(trie.contains('a'), Equals(true));
			AssertThat(trie.contains('b'), Equals(false));
		});

		it("can find inserted keys", [&](){
			asm_lsw::x_fast_trie <uint8_t> trie;

			trie.insert('a');
			trie.insert('b');

			AssertThat(trie.contains('a'), Equals(true));
			AssertThat(trie.contains('b'), Equals(true));
		});

		it("can erase", [&](){
			asm_lsw::x_fast_trie <uint8_t> trie;

			trie.insert('a');
			trie.insert('b');
			trie.insert('k');
			trie.insert('j');

			AssertThat(trie.contains('a'), Equals(true));
			AssertThat(trie.contains('b'), Equals(true));
			AssertThat(trie.contains('k'), Equals(true));
			AssertThat(trie.contains('j'), Equals(true));

			trie.erase('j');

			AssertThat(trie.contains('a'), Equals(true));
			AssertThat(trie.contains('b'), Equals(true));
			AssertThat(trie.contains('k'), Equals(true));
			AssertThat(trie.contains('j'), Equals(false));
		});

		it("can find successors", [&](){
			asm_lsw::x_fast_trie <uint8_t> trie;
			trie.insert('a');
			trie.insert('b');
			trie.insert('k');

			decltype(trie)::const_leaf_iterator succ;
			AssertThat(trie.find_successor('i', succ), Equals(true));
			auto s(succ->first);
			AssertThat('k', Equals(s));
		});

		it("can find successors", [&](){
			asm_lsw::x_fast_trie <uint8_t> trie;
			trie.insert('a');
			trie.insert('b');
			trie.insert('k');
			trie.insert('j');

			decltype(trie)::const_leaf_iterator succ;

			{
				AssertThat(trie.find_successor('i', succ), Equals(true));
				auto s(succ->first);
				AssertThat(s, Equals('j'));
			}

			trie.erase('j');

			{
				AssertThat(trie.find_successor('i', succ), Equals(true));
				auto s(succ->first);
				AssertThat(s, Equals('k'));
			}
		});

		it("can find successors", [&](){
			asm_lsw::x_fast_trie <uint8_t> trie;

			trie.insert(128);
			trie.insert(129);
			trie.insert(130);
			trie.insert(131);
			trie.insert(132);
			trie.insert(133);

			for (auto const i : boost::irange(128, 134, 1))
			{
				decltype(trie)::const_leaf_iterator succ;
				AssertThat(trie.find_successor(1, succ), Equals(true));
				auto s(succ->first);
				AssertThat(s, Equals(i));
				trie.erase(i);
			}

			AssertThat(trie.size(), Equals(0));
		});
	});
});
