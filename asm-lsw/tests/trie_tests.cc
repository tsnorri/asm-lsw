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


template <typename t_trie>
void set_type_tests()
{
	it("can insert", [&](){
		t_trie trie;
		trie.insert('a');
	});

	it("can find inserted keys", [&](){
		t_trie trie;

		trie.insert('a');

		AssertThat(trie.contains('a'), Equals(true));
		AssertThat(trie.contains('b'), Equals(false));
	});

	it("can find inserted keys", [&](){
		t_trie trie;

		trie.insert('a');
		trie.insert('b');

		AssertThat(trie.contains('a'), Equals(true));
		AssertThat(trie.contains('b'), Equals(true));
	});

	it("can erase", [&](){
		t_trie trie;

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

	it("can find successors (1)", [&](){
		t_trie trie;
		trie.insert('a');
		trie.insert('b');
		trie.insert('k');

		typename t_trie::const_iterator succ;
		AssertThat(trie.find_successor('i', succ), Equals(true));
		auto s(trie.iterator_key(succ));
		AssertThat('k', Equals(s));
	});

	it("can find successors (2)", [&](){
		t_trie trie;
		trie.insert('a');
		trie.insert('b');
		trie.insert('k');
		trie.insert('j');

		typename t_trie::const_iterator succ;

		{
			AssertThat(trie.find_successor('i', succ), Equals(true));
			auto s(trie.iterator_key(succ));
			AssertThat(s, Equals('j'));
		}

		trie.erase('j');

		{
			AssertThat(trie.find_successor('i', succ), Equals(true));
			auto s(trie.iterator_key(succ));
			AssertThat(s, Equals('k'));
		}
	});

	it("can find successors (range)", [&](){
		t_trie trie;

		trie.insert(128);
		trie.insert(129);
		trie.insert(130);
		trie.insert(131);
		trie.insert(132);
		trie.insert(133);

		for (auto const i : boost::irange(128, 134, 1))
		{
			typename t_trie::const_iterator succ;
			AssertThat(trie.find_successor(1, succ), Equals(true));
			auto s(trie.iterator_key(succ));
			AssertThat(s, Equals(i));
			trie.erase(i);
		}

		AssertThat(trie.size(), Equals(0));
	});
}


template <typename t_trie, typename t_compact_trie>
void compact_set_type_tests()
{
	it("can find inserted keys", [&](){
		t_trie trie;

		trie.insert('a');

		t_compact_trie ct(trie);
		AssertThat(ct.contains('a'), Equals(true));
		AssertThat(ct.contains('b'), Equals(false));
	});

	it("can find inserted keys", [&](){
		t_trie trie;

		trie.insert('a');
		trie.insert('b');

		t_compact_trie ct(trie);
		AssertThat(ct.contains('a'), Equals(true));
		AssertThat(ct.contains('b'), Equals(true));
	});

	it("can find successors (1)", [&](){
		t_trie trie;
		trie.insert('a');
		trie.insert('b');
		trie.insert('k');

		t_compact_trie ct(trie);

		typename t_compact_trie::const_iterator succ;
		AssertThat(ct.find_successor('i', succ), Equals(true));
		auto s(ct.iterator_key(succ));
		AssertThat('k', Equals(s));
	});

	it("can find successors (2)", [&](){
		t_trie trie;
		t_compact_trie ct;
		trie.insert('a');
		trie.insert('b');
		trie.insert('k');
		trie.insert('j');

		{
			t_trie tmp(trie);
			t_compact_trie tmp_ct(tmp);
			ct = std::move(tmp_ct);
		}

		typename t_compact_trie::const_iterator succ;

		{
			AssertThat(ct.find_successor('i', succ), Equals(true));
			auto s(ct.iterator_key(succ));
			AssertThat(s, Equals('j'));
		}

		trie.erase('j');

		{
			t_trie tmp(trie);
			t_compact_trie tmp_ct(tmp);
			ct = std::move(tmp_ct);
		}

		{
			AssertThat(ct.find_successor('i', succ), Equals(true));
			auto s(ct.iterator_key(succ));
			AssertThat(s, Equals('k'));
		}
	});

	it("can find successors (range)", [&](){
		t_trie trie;
		t_compact_trie ct;

		trie.insert(128);
		trie.insert(129);
		trie.insert(130);
		trie.insert(131);
		trie.insert(132);
		trie.insert(133);

		for (auto const i : boost::irange(128, 134, 1))
		{
			{
				t_trie tmp(trie);
				t_compact_trie tmp_ct(tmp);
				ct = std::move(tmp_ct);
			}

			typename t_compact_trie::const_iterator succ;
			AssertThat(ct.find_successor(1, succ), Equals(true));
			auto s(ct.iterator_key(succ));
			AssertThat(s, Equals(i));
			trie.erase(i);
		}

		AssertThat(trie.size(), Equals(0));
	});
}


template <typename t_trie>
void map_type_tests()
{
	it("can find inserted values by keys", [&](){
		t_trie trie;

		trie.insert('a', 'k');
		trie.insert('b', 'l');

		typename t_trie::const_iterator it;

		AssertThat(trie.find('a', it), Equals(true));
		AssertThat(trie.iterator_value(it), Equals('k'));

		AssertThat(trie.find('b', it), Equals(true));
		AssertThat(trie.iterator_value(it), Equals('l'));

		AssertThat(trie.find('c', it), Equals(false));
	});
}


template <typename t_trie, typename t_compact_trie>
void compact_map_type_tests()
{
	it("can find inserted values by keys", [&](){
		t_trie trie;

		trie.insert('a', 'k');
		trie.insert('b', 'l');

		t_compact_trie ct(trie);

		typename t_compact_trie::const_iterator it;

		AssertThat(ct.find('a', it), Equals(true));
		AssertThat(ct.iterator_value(it), Equals('k'));

		AssertThat(ct.find('b', it), Equals(true));
		AssertThat(ct.iterator_value(it), Equals('l'));

		AssertThat(ct.find('c', it), Equals(false));
	});
}


go_bandit([](){

	// X-fast tries

	describe("X-fast trie <uint8_t>:", [](){
		set_type_tests <asm_lsw::x_fast_trie <uint8_t>>();
	});

	describe("X-fast trie <uint32_t>:", [](){
		set_type_tests <asm_lsw::x_fast_trie <uint32_t>>();
	});

	describe("X-fast trie <uint8_t, uint8_t>:", [](){
		map_type_tests <asm_lsw::x_fast_trie <uint8_t, uint8_t>>();
	});

	describe("X-fast trie <uint32_t, uint32_t>:", [](){
		map_type_tests <asm_lsw::x_fast_trie <uint32_t, uint32_t>>();
	});

	// Compact X-fast tries
	describe("compact X-fast trie <uint8_t>:", [](){
		compact_set_type_tests <asm_lsw::x_fast_trie <uint8_t>, asm_lsw::x_fast_trie_compact <uint8_t>>();
	});

	describe("compact X-fast trie <uint32_t>:", [](){
		compact_set_type_tests <asm_lsw::x_fast_trie <uint32_t>, asm_lsw::x_fast_trie_compact <uint32_t>>();
	});

	describe("compact X-fast trie <uint8_t, uint8_t>:", [](){
		compact_map_type_tests <asm_lsw::x_fast_trie <uint8_t, uint8_t>, asm_lsw::x_fast_trie_compact <uint8_t, uint8_t>>();
	});

	describe("compact X-fast trie <uint32_t, uint32_t>:", [](){
		compact_map_type_tests <asm_lsw::x_fast_trie <uint32_t, uint32_t>, asm_lsw::x_fast_trie_compact <uint32_t, uint32_t>>();
	});

	// Y-fast tries

	describe("Y-fast trie <uint8_t>:", [](){
		set_type_tests <asm_lsw::y_fast_trie <uint8_t>>();
	});

	describe("Y-fast trie <uint32_t>:", [](){
		set_type_tests <asm_lsw::y_fast_trie <uint32_t>>();
	});

	describe("Y-fast trie <uint8_t, uint8_t>:", [](){
		map_type_tests <asm_lsw::y_fast_trie <uint8_t, uint8_t>>();
	});

	describe("Y-fast trie <uint32_t, uint32_t>:", [](){
		map_type_tests <asm_lsw::y_fast_trie <uint32_t, uint32_t>>();
	});

	// Compact Y-fast tries

	describe("compact Y-fast trie <uint8_t>:", [](){
		compact_set_type_tests <asm_lsw::y_fast_trie <uint8_t>, asm_lsw::y_fast_trie_compact <uint8_t>>();
	});

	describe("compact Y-fast trie <uint32_t>:", [](){
		compact_set_type_tests <asm_lsw::y_fast_trie <uint32_t>, asm_lsw::y_fast_trie_compact <uint32_t>>();
	});

	describe("compact Y-fast trie <uint8_t, uint8_t>:", [](){
		compact_map_type_tests <asm_lsw::y_fast_trie <uint8_t, uint8_t>, asm_lsw::y_fast_trie_compact <uint8_t, uint8_t>>();
	});

	describe("compact Y-fast trie <uint32_t, uint32_t>:", [](){
		compact_map_type_tests <asm_lsw::y_fast_trie <uint32_t, uint32_t>, asm_lsw::y_fast_trie_compact <uint32_t, uint32_t>>();
	});
});
