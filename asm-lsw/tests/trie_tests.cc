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
#include <boost/iterator/zip_iterator.hpp>
#include <boost/range.hpp>
#include <boost/range/irange.hpp>

using namespace bandit;


template <typename t_trie>
struct ref_trie_adaptor
{
	typedef t_trie trie_type;
	typedef t_trie &return_type;
	t_trie &operator()(t_trie &trie) const { return trie; }
	t_trie const &operator()(t_trie const &trie) const { return trie; }
};


template <typename t_trie, typename t_compact_trie>
struct compact_trie_adaptor
{
	typedef t_compact_trie trie_type;
	typedef t_compact_trie return_type;
	t_compact_trie operator()(t_trie &trie) const { return t_compact_trie(trie); }
};


template <typename t_trie, typename t_compact_trie>
struct compact_as_trie_adaptor
{
	typedef t_compact_trie trie_type;
	typedef t_compact_trie &return_type;

	std::unique_ptr <t_compact_trie> trie_ptr{};
	
	return_type operator()(t_trie &trie)
	{
		trie_ptr.reset(t_compact_trie::construct(trie));
		return *trie_ptr;
	} 
};


template <typename t_trie>
void insert_with_limit(t_trie &trie, typename t_trie::key_type const last, typename t_trie::key_type const max_value)
{
	t_trie tmp_trie(max_value);

	typename t_trie::key_type key(1);
	while (true)
	{
		tmp_trie.insert(key);
		if (key == last)
			break;
		++key;
	}

	trie = std::move(tmp_trie);
}


template <typename t_trie, typename t_adaptor>
void common_any_type_tests()
{
	t_adaptor adaptor;

	it("can initialize", [&](){
		t_trie trie;
		typename t_adaptor::return_type ct((adaptor(trie)));
		AssertThat(trie.size(), Equals(0));
	});
	
	it("works when empty", [&](){
		t_trie trie;
		typename t_adaptor::return_type ct((adaptor(trie)));
		
		AssertThat(ct.size(), Equals(0));
		
		AssertThat(ct.contains(0), Equals(false));
		
		typename t_adaptor::trie_type::const_iterator it;
		AssertThat(ct.find(0, it), Equals(false));
		AssertThat(ct.find_predecessor(0, it, false), Equals(false));
		AssertThat(ct.find_predecessor(0, it, true), Equals(false));
		AssertThat(ct.find_successor(0, it, false), Equals(false));
		AssertThat(ct.find_successor(0, it, true), Equals(false));
		
		// FIXME: add more tests when completing x_fast_trie_compact_as.
	});
}


template <typename t_trie>
void set_type_tests()
{
	it("can insert", [](){
		t_trie trie;
		trie.insert('a');
	});


	it("can erase", [](){
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
}


template <typename t_trie, typename t_adaptor>
void common_set_type_tests()
{
	t_adaptor adaptor;

	it("can find inserted keys (1)", [&](){
		t_trie trie;

		trie.insert('a');

		typename t_adaptor::return_type ct((adaptor(trie)));
		AssertThat(ct.contains('a'), Equals(true));
		AssertThat(ct.contains('b'), Equals(false));
	});

	it("can find inserted keys (2)", [&](){
		t_trie trie;

		trie.insert('a');
		trie.insert('b');

		typename t_adaptor::return_type ct((adaptor(trie)));
		AssertThat(ct.contains('a'), Equals(true));
		AssertThat(ct.contains('b'), Equals(true));
	});

	it("can find successors (1)", [&](){
		t_trie trie;
		trie.insert('a');
		trie.insert('b');
		trie.insert('k');

		typename t_adaptor::return_type ct((adaptor(trie)));
		typename t_adaptor::trie_type::const_iterator succ;
		AssertThat(ct.find_successor('i', succ), Equals(true));
		auto s(ct.iterator_key(succ));
		AssertThat('k', Equals(s));
	});

	it("can find successors (2)", [&](){
		t_trie trie;
		trie.insert('a');
		trie.insert('b');
		trie.insert('k');
		trie.insert('j');

		{
			typename t_adaptor::return_type ct((adaptor(trie)));
			typename t_adaptor::trie_type::const_iterator succ;
			AssertThat(ct.find_successor('i', succ), Equals(true));
			auto s(ct.iterator_key(succ));
			AssertThat(s, Equals('j'));
		}

		trie.erase('j');

		{
			typename t_adaptor::return_type ct((adaptor(trie)));
			typename t_adaptor::trie_type::const_iterator succ;
			AssertThat(ct.find_successor('i', succ), Equals(true));
			auto s(ct.iterator_key(succ));
			AssertThat(s, Equals('k'));
		}
	});

	it("can find successors (range)", [&](){
		t_trie trie;
		t_adaptor adaptor;

		trie.insert(128);
		trie.insert(129);
		trie.insert(130);
		trie.insert(131);
		trie.insert(132);
		trie.insert(133);

		for (auto const i : boost::irange(128, 134, 1))
		{
			typename t_adaptor::return_type ct((adaptor(trie)));
			typename t_adaptor::trie_type::const_iterator succ;
			AssertThat(ct.find_successor(1, succ), Equals(true));
			auto s(ct.iterator_key(succ));
			AssertThat(s, Equals(i));
			trie.erase(i);
		}

		typename t_adaptor::return_type ct((adaptor(trie)));
		AssertThat(ct.size(), Equals(0));
	});

	{
		// Make a trie with test_values and pass items in search_values to various functions.

		typedef typename t_trie::key_type trie_key_type;
		typedef typename t_adaptor::return_type ct_var_type;
		typedef typename t_adaptor::trie_type::key_type ct_key_type;
		typedef typename t_adaptor::trie_type::const_iterator ct_const_iterator;

		std::vector<trie_key_type> const test_values{1, 15, 27, 33, 92, 120, 148, 163, 199, 201, 214, 227, 228, 229, 230, 243, 249, 255};
		std::vector<ct_key_type> const search_values{1, 14, 22, 32, 33, 34, 255};

		t_trie trie;
		for (auto const val : test_values)
			trie.insert(val);
		ct_var_type ct((adaptor(trie)));

		// The test loop. Call to the trie's member function to be tested is packed into cb.
		// Zero in the expected values indicates that the tested function should return false.
		auto fn = [&](
			decltype(search_values) const &expected_values,
			std::function <bool (ct_var_type &, ct_key_type, ct_const_iterator &)> cb
		) -> void {
			auto zbegin(boost::make_zip_iterator(boost::make_tuple(search_values.cbegin(), expected_values.cbegin())));
			auto const zend(boost::make_zip_iterator(boost::make_tuple(search_values.cend(), expected_values.cend())));
			for (auto const tup : boost::make_iterator_range(zbegin, zend))
			{
				trie_key_type test{0};
				ct_key_type expected{0};
				boost::tie(test, expected) = tup;
				ct_const_iterator it;
				auto const res(cb(ct, test, it));
				if (res)
				{
					AssertThat(ct.iterator_key(it), Equals(expected));
				}
				else
				{
					AssertThat(0, Equals(expected));
				}
			}
		};

		it("can find exact matches", [&](){
			decltype(search_values) const expected{1, 0, 0, 0, 33, 0, 255};
			auto cb = [](ct_var_type &ct, ct_key_type search_value, ct_const_iterator &it) -> bool {
				return ct.find(search_value, it);
			};
			fn(expected, cb);
		});

		it("can find predecessors", [&](){
			decltype(search_values) const expected{0, 1, 15, 27, 27, 33, 249};
			auto cb = [](ct_var_type &ct, ct_key_type search_value, ct_const_iterator &it) -> bool {
				return ct.find_predecessor(search_value, it);
			};
			fn(expected, cb);
		});

		it("can find predecessors with allow_equal", [&](){
			decltype(search_values) const expected{1, 1, 15, 27, 33, 33, 255};
			auto cb = [](ct_var_type &ct, ct_key_type search_value, ct_const_iterator &it) -> bool {
				return ct.find_predecessor(search_value, it, true);
			};
			fn(expected, cb);
		});

		it("can find successors", [&](){
			decltype(search_values) const expected{15, 15, 27, 33, 92, 92, 0};
			auto cb = [](ct_var_type &ct, ct_key_type search_value, ct_const_iterator &it) -> bool {
				return ct.find_successor(search_value, it);
			};
			fn(expected, cb);
		});

		it("can find successors with allow_equal", [&](){
			decltype(search_values) const expected{1, 15, 27, 33, 33, 92, 255};
			auto cb = [](ct_var_type &ct, ct_key_type search_value, ct_const_iterator &it) -> bool {
				return ct.find_successor(search_value, it, true);
			};
			fn(expected, cb);
		});
	}
}


template <typename t_trie, typename t_adaptor>
void common_map_type_tests()
{
	it("can find inserted values by keys", [](){
		t_trie trie;
		t_adaptor adaptor;

		trie.insert('a', 'k');
		trie.insert('b', 'l');

		typename t_adaptor::return_type ct((adaptor(trie)));
		typename t_adaptor::trie_type::const_iterator it;

		AssertThat(ct.find('a', it), Equals(true));
		AssertThat(ct.iterator_value(it), Equals('k'));

		AssertThat(ct.find('b', it), Equals(true));
		AssertThat(ct.iterator_value(it), Equals('l'));

		AssertThat(ct.find('c', it), Equals(false));
	});
}


template <typename t_trie, typename t_compact_trie>
void compact_any_type_tests()
{
	it("can initialize with empty container", [](){
		t_trie trie;
		AssertThat(trie.size(), Equals(0));

		t_compact_trie ct(trie);
		AssertThat(ct.size(), Equals(0));
	});
}


void common_as_type_tests()
{
	it("should be possible to construct one from an empty trie", [](){
		typedef asm_lsw::x_fast_trie <uint32_t> trie_type;
		typedef asm_lsw::x_fast_trie_compact_as <uint32_t> ct_type;
		
		trie_type trie;
		std::unique_ptr <ct_type> ct(ct_type::construct(trie));
		AssertThat(ct->key_size(), Equals(1));
		AssertThat(ct->size(), Equals(0));
	});
	
	it("should choose the correct key size", [](){
		typedef asm_lsw::x_fast_trie <uint32_t> trie_type;
		typedef asm_lsw::x_fast_trie_compact_as <uint32_t> ct_type;
		
		trie_type trie;
		trie.insert(0x10001);
		trie.insert(0x10002);
		trie.insert(0x10003);
		trie.insert(0x10004);
		
		std::unique_ptr <ct_type> ct(ct_type::construct(trie));
		AssertThat(ct->key_size(), Equals(1));
		AssertThat(ct->min_key(), Equals(0x10001));
		AssertThat(ct->max_key(), Equals(0x10004));
	});
	
	it("should handle values outside the allowed range", [](){
		typedef asm_lsw::x_fast_trie <uint32_t> trie_type;
		typedef asm_lsw::x_fast_trie_compact_as <uint32_t> ct_type;
		
		trie_type trie;
		trie.insert(0x10000);
		trie.insert(0x10001);
		trie.insert(0x10002);
		trie.insert(0x10003);
		trie.insert(0x10004);
		
		std::unique_ptr <ct_type> ct(ct_type::construct(trie));
		AssertThat(ct->key_size(), Equals(1));
		AssertThat(ct->offset(), Equals(0x10000));
		
		ct_type::const_iterator it;
		
		AssertThat(ct->find_predecessor(0x10, it, false), Equals(false));
		AssertThat(ct->find_successor(0x10, it, false), Equals(true));
		AssertThat(ct->iterator_key(it), Equals(0x10000));
		
		AssertThat(ct->find_successor(0x20000, it, false), Equals(false));
		AssertThat(ct->find_predecessor(0x20000, it, false), Equals(true));
		AssertThat(ct->iterator_key(it), Equals(0x10004));
	});
}


template <typename t_trie, typename t_adaptor>
void x_fast_any_tests()
{
	t_adaptor adaptor;

	it("works when empty", [&](){
		t_trie trie;
		typename t_adaptor::return_type ct((adaptor(trie)));
		
		AssertThat(ct.size(), Equals(0));
		
		{
			typename t_adaptor::trie_type::const_iterator begin(ct.cbegin());
			typename t_adaptor::trie_type::const_iterator end(ct.cend());
			AssertThat(begin, Equals(end));
		}
	});
}


template <typename t_trie>
void y_fast_set_test_subtree_size(t_trie &trie, typename t_trie::size_type const max)
{
	insert_with_limit(trie, max, max);
	auto const log2M(std::log2(max));
	for (auto const &kv : trie.subtree_map_proxy())
	{
		auto const &subtree(kv.second);
		AssertThat(subtree.size(), IsLessThan(2 * log2M));
	}			
}


template <typename t_trie>
void y_fast_set_tests()
{
	it("should retain key limit after moving", [&](){
		t_trie t1(5);
		t_trie t2(6);

		t1 = std::move(t2);
		AssertThat(t1.max_key(), Equals(6));
	});

	it("should enforce key limit", [&](){
		t_trie trie(5);

		AssertThrows(asm_lsw::invalid_argument, trie.insert(6));
		AssertThat(LastException <asm_lsw::invalid_argument>().error <typename t_trie::error>(), Equals(t_trie::error::out_of_range));
	});

	it("should have small subtrees", [&](){
		t_trie trie;
		y_fast_set_test_subtree_size(trie, 20);
		y_fast_set_test_subtree_size(trie, 40);
		y_fast_set_test_subtree_size(trie, 255);
	});
}


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

	// Compact X-fast tries
	describe("compact X-fast trie <uint8_t>:", [](){
		typedef asm_lsw::x_fast_trie <uint8_t> trie_type;
		typedef asm_lsw::x_fast_trie_compact <uint8_t> ct_type;
		common_any_type_tests <trie_type, compact_trie_adaptor <trie_type, ct_type>>();
		x_fast_any_tests <trie_type, ref_trie_adaptor <trie_type>>();
		common_set_type_tests <trie_type, compact_trie_adaptor <trie_type, ct_type>>();
	});

	describe("compact X-fast trie <uint32_t>:", [](){
		typedef asm_lsw::x_fast_trie <uint32_t> trie_type;
		typedef asm_lsw::x_fast_trie_compact <uint32_t> ct_type;
		common_any_type_tests <trie_type, compact_trie_adaptor <trie_type, ct_type>>();
		x_fast_any_tests <trie_type, ref_trie_adaptor <trie_type>>();
		common_set_type_tests <trie_type, compact_trie_adaptor <trie_type, ct_type>>();
	});

	describe("compact X-fast trie <uint8_t, uint8_t>:", [](){
		typedef asm_lsw::x_fast_trie <uint8_t, uint8_t> trie_type;
		typedef asm_lsw::x_fast_trie_compact <uint8_t, uint8_t> ct_type;
		common_any_type_tests <trie_type, compact_trie_adaptor <trie_type, ct_type>>();
		x_fast_any_tests <trie_type, ref_trie_adaptor <trie_type>>();
		common_map_type_tests <trie_type, compact_trie_adaptor <trie_type, ct_type>>();
	});

	describe("compact X-fast trie <uint32_t, uint32_t>:", [](){
		typedef asm_lsw::x_fast_trie <uint32_t, uint32_t> trie_type;
		typedef asm_lsw::x_fast_trie_compact <uint32_t, uint32_t> ct_type;
		common_any_type_tests <trie_type, compact_trie_adaptor <trie_type, ct_type>>();
		x_fast_any_tests <trie_type, ref_trie_adaptor <trie_type>>();
		common_map_type_tests <trie_type, compact_trie_adaptor <trie_type, ct_type>>();
	});

	// Compact X-fast tries (AS)
	describe("compact AS trie:", [](){
		common_as_type_tests();
	});

	describe("compact X-fast trie <uint8_t> (AS):", [](){
		typedef asm_lsw::x_fast_trie <uint8_t> trie_type;
		typedef asm_lsw::x_fast_trie_compact_as <uint8_t> ct_type;
		common_any_type_tests <trie_type, compact_as_trie_adaptor <trie_type, ct_type>>();
		x_fast_any_tests <trie_type, ref_trie_adaptor <trie_type>>();
		common_set_type_tests <trie_type, compact_as_trie_adaptor <trie_type, ct_type>>();
	});

	describe("compact X-fast trie <uint32_t> (AS):", [](){
		typedef asm_lsw::x_fast_trie <uint32_t> trie_type;
		typedef asm_lsw::x_fast_trie_compact_as <uint32_t> ct_type;
		common_any_type_tests <trie_type, compact_as_trie_adaptor <trie_type, ct_type>>();
		x_fast_any_tests <trie_type, ref_trie_adaptor <trie_type>>();
		common_set_type_tests <trie_type, compact_as_trie_adaptor <trie_type, ct_type>>();
	});

	describe("compact X-fast trie <uint8_t, uint8_t> (AS):", [](){
		typedef asm_lsw::x_fast_trie <uint8_t, uint8_t> trie_type;
		typedef asm_lsw::x_fast_trie_compact_as <uint8_t, uint8_t> ct_type;
		common_any_type_tests <trie_type, compact_as_trie_adaptor <trie_type, ct_type>>();
		x_fast_any_tests <trie_type, ref_trie_adaptor <trie_type>>();
		common_map_type_tests <trie_type, compact_as_trie_adaptor <trie_type, ct_type>>();
	});

	describe("compact X-fast trie <uint32_t, uint32_t> (AS):", [](){
		typedef asm_lsw::x_fast_trie <uint32_t, uint32_t> trie_type;
		typedef asm_lsw::x_fast_trie_compact_as <uint32_t, uint32_t> ct_type;
		common_any_type_tests <trie_type, compact_as_trie_adaptor <trie_type, ct_type>>();
		x_fast_any_tests <trie_type, ref_trie_adaptor <trie_type>>();
		common_map_type_tests <trie_type, compact_as_trie_adaptor <trie_type, ct_type>>();
	});

	// Y-fast tries
	describe("Y-fast trie <uint8_t>:", [](){
		typedef asm_lsw::y_fast_trie <uint8_t> trie_type;
		common_any_type_tests <trie_type, ref_trie_adaptor <trie_type>>();
		set_type_tests <trie_type>();
		common_set_type_tests <trie_type, ref_trie_adaptor <trie_type>>();
		y_fast_set_tests <trie_type>();
	});

	describe("Y-fast trie <uint32_t>:", [](){
		typedef asm_lsw::y_fast_trie <uint32_t> trie_type;
		common_any_type_tests <trie_type, ref_trie_adaptor <trie_type>>();
		set_type_tests <trie_type>();
		common_set_type_tests <trie_type, ref_trie_adaptor <trie_type>>();
		y_fast_set_tests <trie_type>();
	});

	describe("Y-fast trie <uint8_t, uint8_t>:", [](){
		typedef asm_lsw::y_fast_trie <uint8_t, uint8_t> trie_type;
		common_any_type_tests <trie_type, ref_trie_adaptor <trie_type>>();
		common_map_type_tests <trie_type, ref_trie_adaptor <trie_type>>();
	});

	describe("Y-fast trie <uint32_t, uint32_t>:", [](){
		typedef asm_lsw::y_fast_trie <uint32_t, uint32_t> trie_type;
		common_any_type_tests <trie_type, ref_trie_adaptor <trie_type>>();
		common_map_type_tests <trie_type, ref_trie_adaptor <trie_type>>();
	});

	// Compact Y-fast tries
	describe("compact Y-fast trie <uint8_t>:", [](){
		typedef asm_lsw::y_fast_trie <uint8_t> trie_type;
		typedef asm_lsw::y_fast_trie_compact <uint8_t> ct_type;
		common_any_type_tests <trie_type, compact_trie_adaptor <trie_type, ct_type>>();
		common_set_type_tests <trie_type, compact_trie_adaptor <trie_type, ct_type>>();
	});

	describe("compact Y-fast trie <uint32_t>:", [](){
		typedef asm_lsw::y_fast_trie <uint32_t> trie_type;
		typedef asm_lsw::y_fast_trie_compact <uint32_t> ct_type;
		common_any_type_tests <trie_type, compact_trie_adaptor <trie_type, ct_type>>();
		common_set_type_tests <trie_type, compact_trie_adaptor <trie_type, ct_type>>();
	});

	describe("compact Y-fast trie <uint8_t, uint8_t>:", [](){
		typedef asm_lsw::y_fast_trie <uint8_t, uint8_t> trie_type;
		typedef asm_lsw::y_fast_trie_compact <uint8_t, uint8_t> ct_type;
		common_any_type_tests <trie_type, compact_trie_adaptor <trie_type, ct_type>>();
		common_map_type_tests <trie_type, compact_trie_adaptor <trie_type, ct_type>>();
	});

	describe("compact Y-fast trie <uint32_t, uint32_t>:", [](){
		typedef asm_lsw::y_fast_trie <uint32_t, uint32_t> trie_type;
		typedef asm_lsw::y_fast_trie_compact <uint32_t, uint32_t> ct_type;
		common_any_type_tests <trie_type, compact_trie_adaptor <trie_type, ct_type>>();
		common_map_type_tests <trie_type, compact_trie_adaptor <trie_type, ct_type>>();
	});
});
