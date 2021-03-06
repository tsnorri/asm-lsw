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

#ifndef ASM_LSW_TRIE_TESTS_HH
#define ASM_LSW_TRIE_TESTS_HH

#include <asm_lsw/x_fast_tries.hh>
#include <asm_lsw/y_fast_tries.hh>
#include <bandit/bandit.h>
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/stream.hpp>
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

	template <typename t_istream, typename t_ref_trie>
	void load_and_compare(t_istream &istream, t_ref_trie const &ref_trie)
	{
		trie_type ct;
		ct.load(istream);
		AssertThat(ct == ref_trie, Equals(true));
	}
};


template <typename t_trie, typename t_compact_trie>
struct compact_trie_adaptor
{
	typedef t_compact_trie trie_type;
	typedef t_compact_trie return_type;
	t_compact_trie operator()(t_trie &trie) const { return t_compact_trie(trie); }
	
	template <typename t_istream, typename t_ref_trie>
	void load_and_compare(t_istream &istream, t_ref_trie const &ref_trie)
	{
		trie_type ct;
		ct.load(istream);
		AssertThat(ct == ref_trie, Equals(true));
	}
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
	
	template <typename t_istream, typename t_ref_trie>
	void load_and_compare(t_istream &istream, t_ref_trie const &ref_trie)
	{
		std::unique_ptr <trie_type> ct(trie_type::load(istream));
		auto &ref(*ct);
		AssertThat(ref == ref_trie, Equals(true));
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

		AssertThat(trie.size(), Equals(4));

		AssertThat(trie.contains('a'), Equals(true));
		AssertThat(trie.contains('b'), Equals(true));
		AssertThat(trie.contains('k'), Equals(true));
		AssertThat(trie.contains('j'), Equals(true));

		trie.erase('j');

		AssertThat(trie.size(), Equals(3));

		AssertThat(trie.contains('a'), Equals(true));
		AssertThat(trie.contains('b'), Equals(true));
		AssertThat(trie.contains('k'), Equals(true));
		AssertThat(trie.contains('j'), Equals(false));
		
		// Check predecessors and successors
		{
			typename t_trie::const_iterator it;
			
			AssertThat(trie.find_successor('a', it, true), Equals(true));
			AssertThat(trie.iterator_key(it), Equals('a'));
			AssertThat(trie.find_successor('b', it, true), Equals(true));
			AssertThat(trie.iterator_key(it), Equals('b'));
			AssertThat(trie.find_successor('k', it, true), Equals(true));
			AssertThat(trie.iterator_key(it), Equals('k'));
			
			AssertThat(trie.find_predecessor('j', it, true), Equals(true));
			AssertThat(trie.iterator_key(it), Equals('b'));
			AssertThat(trie.find_successor('j', it, true), Equals(true));
			AssertThat(trie.iterator_key(it), Equals('k'));
			AssertThat(trie.find_successor('b', it, false), Equals(true));
			AssertThat(trie.iterator_key(it), Equals('k'));
			AssertThat(trie.find_predecessor('k', it, false), Equals(true));
			AssertThat(trie.iterator_key(it), Equals('b'));
		}
	});
	
	it("can insert (2)", [](){
		t_trie trie;
		std::vector <uint8_t> vec{
			0x58, 0x50, 0x18, 0x10, 0x0
		};
		
		for (auto const k : vec)
			trie.insert(k);
		
		typename t_trie::const_iterator it;
		AssertThat(trie.find_predecessor(0x20, it, false), Equals(true));
		auto const pred(trie.iterator_key(it));
		trie.insert(0x20);
	});
	
	it("can insert (3)", [](){
		t_trie trie;
		std::vector <uint8_t> vec{
			0x0, 0xd, 0x15, 0x67, 0x5d, 0x55, 0x1d, 0x20
		};
		for (auto const k : vec)
			trie.insert(k);
	});
	
	it("can insert (4)", [](){
		t_trie trie;
		std::vector <uint8_t> vec{
			0x0, 0x10, 0x18, 0x50, 0x58
		};
		for (auto const k : vec)
			trie.insert(k);
		
		typename t_trie::const_iterator it;
		AssertThat(trie.find_predecessor(0x20, it, false), Equals(true));
		auto const pred(trie.iterator_key(it));
		trie.insert(0x20);
	});
	
	it("can insert (5)", [](){
		t_trie trie;
		std::vector <uint8_t> vec{
			0x0, 0x10, 0x58, 0x50, 0x18
		};
		for (auto const k : vec)
			trie.insert(k);
		
		typename t_trie::const_iterator it;
		AssertThat(trie.find_predecessor(0x20, it, false), Equals(true));
		auto const pred(trie.iterator_key(it));
		trie.insert(0x20);
	});
	
	it("can insert (6)", [](){
		t_trie trie;
		std::vector <uint8_t> vec{
			88, 89, 90, 91, 92, 93, 94, 95, 80, 81, 82, 83, 84, 85, 86, 87,
			24, 25, 26, 27, 28, 29, 30, 31,
			16, 17, 18, 19, 20, 21, 22, 23,
			0, 1, 2, 3, 4, 5, 6, 7
		};
		
		for (auto const k : vec)
			trie.insert(k);
		
		typename t_trie::const_iterator it;
		AssertThat(trie.find_predecessor(0x20, it, false), Equals(true));
		auto const pred(trie.iterator_key(it));
		trie.insert(0x20);
	});
	
	it("can insert (7)", [](){
		t_trie trie;
		std::vector <uint8_t> vec{
			97, 98, 99, 100, 10, 102, 103, 104, 105, 106, 107, 108, 109, 110,
			88, 89, 90, 91, 92, 93, 94, 95, 80, 81, 82, 83, 84, 85, 86, 87,
			24, 25, 26, 27, 28, 29, 30, 31,
			16, 17, 18, 19, 20, 21, 22, 23,
			8, 9, 10, 11, 12, 13, 14, 15,
			0, 1, 2, 3, 4, 5, 6, 7,
			32
		};
		
		for (auto const k : vec)
			trie.insert(k);
	});
	
	it("can insert and erase", [](){
		t_trie trie;
		
		trie.insert(0x58);
		trie.erase(0x58);
		trie.insert(0x58);
		trie.insert(0x50);
		trie.erase(0x50);
		trie.insert(0x50);
		trie.insert(0x18);
		trie.erase(0x18);
		trie.insert(0x18);
		trie.insert(0x10);
		trie.erase(0x10);
		trie.insert(0x10);
		trie.insert(0x0);
		
		typename t_trie::const_iterator it;
		AssertThat(trie.find_predecessor(0x20, it, false), Equals(true));
		auto const pred(trie.iterator_key(it));
		trie.insert(0x20);
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
		AssertThat(ct.size(), Equals(1));
		AssertThat(ct.contains('a'), Equals(true));
		AssertThat(ct.contains('b'), Equals(false));
	});

	it("can find inserted keys (2)", [&](){
		t_trie trie;

		trie.insert('a');
		trie.insert('b');

		typename t_adaptor::return_type ct((adaptor(trie)));
		AssertThat(ct.size(), Equals(2));
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
		AssertThat(ct.size(), Equals(3));

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
			t_trie tmp_trie(trie); // Copy since the compact trie takes ownership of the values.
			typename t_adaptor::return_type ct((adaptor(tmp_trie)));
			typename t_adaptor::trie_type::const_iterator succ;
			AssertThat(ct.size(), Equals(4));
			AssertThat(ct.find_successor('i', succ), Equals(true));
			auto s(ct.iterator_key(succ));
			AssertThat(s, Equals('j'));
		}

		trie.erase('j');
		AssertThat(trie.size(), Equals(3));

		{
			typename t_adaptor::return_type ct((adaptor(trie)));
			typename t_adaptor::trie_type::const_iterator succ;
			AssertThat(ct.size(), Equals(3));
			AssertThat(ct.find_successor('i', succ), Equals(true));
			auto s(ct.iterator_key(succ));
			AssertThat(s, Equals('k'));
		}
	});
	
	it("can find successors (lesser key)", [&](){
		// Test with a key lesser than any of the stored values.
		
		t_trie trie;
		for (auto const i : boost::irange(16, 127, 1))
			trie.insert(i);
		
		t_trie tmp_trie(trie); // Copy since the compact trie takes ownership of the values.
		typename t_adaptor::return_type ct((adaptor(tmp_trie)));

		typename t_adaptor::trie_type::const_iterator it;

		AssertThat(ct.find_successor(1, it, true), Equals(true));
		AssertThat(ct.iterator_key(it), Equals(16));
		
		AssertThat(ct.find_successor(1, it, false), Equals(true));
		AssertThat(ct.iterator_key(it), Equals(16));
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

		AssertThat(trie.size(), Equals(6));

		for (auto const i : boost::irange(128, 134, 1))
		{
			t_trie tmp_trie(trie); // Copy since the compact trie takes ownership of the values.
			typename t_adaptor::return_type ct((adaptor(tmp_trie)));
			typename t_adaptor::trie_type::const_iterator succ;
			AssertThat(ct.size(), Equals(trie.size()));
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

		AssertThat(trie.size(), Equals(test_values.size()));
		AssertThat(ct.size(), Equals(trie.size()));

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

		AssertThat(ct.size(), Equals(2));

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


template <typename t_trie, typename t_adaptor>
void compact_set_type_tests()
{
	typedef typename t_trie::key_type trie_key_type;
	it("can serialize", [](){
		t_adaptor adaptor;
		std::vector <trie_key_type> const test_values{1, 15, 27, 33, 92, 120, 148, 163, 199, 201, 214, 227, 228, 229, 230, 243, 249, 255};
		
		t_trie trie;
		for (auto const val : test_values)
			trie.insert(val);
		AssertThat(trie == trie, Equals(true));
		t_trie trie_copy(trie);
		AssertThat(trie_copy == trie, Equals(true));
		
		typename t_adaptor::return_type ct((adaptor(trie_copy)));
		AssertThat(ct == trie, Equals(true));
		
		std::size_t const buffer_size(8192);
		char buffer[buffer_size];

		boost::iostreams::basic_array <char> array(buffer, buffer_size);
		boost::iostreams::stream <boost::iostreams::basic_array <char>> iostream(array);
		
		auto const size(ct.size());
		auto const serialized_size(ct.serialize(iostream));
		assert(serialized_size <= buffer_size);
		AssertThat(ct.size(), Equals(size));
		
		iostream.seekp(0);
		adaptor.load_and_compare(iostream, ct);
	});
}


template <typename t_trie, typename t_as_trie>
void common_as_type_tests()
{
	typedef t_trie trie_type;
	typedef t_as_trie ct_type;

	it("it possible to construct one from an empty trie", [](){
		trie_type trie;
		std::unique_ptr <ct_type> ct(ct_type::construct(trie));
		AssertThat(ct->key_size(), Equals(1));
		AssertThat(ct->size(), Equals(0));
	});
	
	it("chooses the correct key size", [](){
		trie_type trie;
		trie.insert(0x10001);
		trie.insert(0x10002);
		trie.insert(0x10003);
		trie.insert(0x10004);
		
		std::unique_ptr <ct_type> ct(ct_type::construct(trie));
		AssertThat(ct->size(), Equals(4));

		AssertThat(trie.key_size(), Is().GreaterThan(1));
		AssertThat(ct->key_size(), Equals(1));
		AssertThat(ct->min_key(), Equals(0x10001));
		AssertThat(ct->max_key(), Equals(0x10004));
	});
	
	it("handles values outside the allowed range", [](){
		trie_type trie;
		trie.insert(0x10000);
		trie.insert(0x10001);
		trie.insert(0x10002);
		trie.insert(0x10003);
		trie.insert(0x10004);
		
		std::unique_ptr <ct_type> ct(ct_type::construct(trie));
		AssertThat(ct->size(), Equals(5));

		AssertThat(trie.key_size(), Is().GreaterThan(1));
		AssertThat(ct->key_size(), Equals(1));
		AssertThat(ct->offset(), Equals(0x10000));
		
		typename ct_type::const_iterator it;
		
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
void y_fast_set_nc_tests()
{
	it("retains key limit after moving", [&](){
		t_trie t1(5);
		t_trie t2(6);

		t1 = std::move(t2);
		AssertThat(t1.key_limit(), Equals(6));
	});

	it("enforces key limit", [&](){
		t_trie trie(5);

		AssertThrows(asm_lsw::invalid_argument, trie.insert(6));
		AssertThat(LastException <asm_lsw::invalid_argument>().error <typename t_trie::error>(), Equals(t_trie::error::out_of_range));
	});
	
	it("has small subtrees", [&](){
		t_trie trie;
		y_fast_set_test_subtree_size(trie, 20);
		y_fast_set_test_subtree_size(trie, 40);
		y_fast_set_test_subtree_size(trie, 255);
	});
	
	it("can insert and erase", [](){
		t_trie trie;
		std::vector <uint8_t> vec{
			0x58, 0x50, 0x18, 0x10, 0x0
		};
		
		for (auto const k : vec)
			trie.insert(k);
		
		typename t_trie::const_iterator it;
		
		AssertThat(trie.find_successor(0x0, it, false), Equals(true));
		AssertThat(trie.iterator_key(it), Equals(0x10));
		
		trie.erase(0x0);
		
		typename t_trie::const_subtree_map_iterator it2;
		
		AssertThat(trie.find_subtree_exact(0x10, it2), Equals(true));
		AssertThat(trie.find_successor(0x5, it, false), Equals(true));
		AssertThat(trie.iterator_key(it), Equals(0x10));
	});
}


template <typename t_trie, typename t_adaptor>
void y_fast_set_tests()
{
	it("can find predecessors (subtree minimum)", [&]() {
		// Test with a leftmost value of a subtree.
		
		t_trie trie;
		t_adaptor adaptor;
		
		for (auto const i : boost::irange(1, 127, 1))
			trie.insert(i);
		
		t_trie tmp_trie(trie); // Copy since the compact trie takes ownership of the values.
		typename t_adaptor::return_type ct((adaptor(tmp_trie)));
		
		// Find a subtree key.
		auto subtree_key(ct.min_key());
		for (uint8_t i(0); i < 2; ++i)
			AssertThat(ct.find_next_subtree_key(subtree_key), Equals(true));
		typename t_adaptor::trie_type::const_iterator min_it;
		AssertThat(ct.find_subtree_min(subtree_key, min_it), Equals(true));
		
		// Check that find_predecessor works for a leftmost value of a subtree.
		typename t_adaptor::trie_type::const_iterator it;
		AssertThat(ct.find_predecessor(subtree_key, it, false), Equals(true));
		AssertThat(*it, Equals(subtree_key - 1));
	});

	it("can find successors (rightmost subtree minimum)", [&](){
		// Test with a key slightly greater than the rightmost subtree minimum (i.e. representative).
		
		t_trie trie;
		t_adaptor adaptor;

		for (auto const i : boost::irange(1, 127, 1))
			trie.insert(i);
		
		t_trie tmp_trie(trie); // Copy since the compact trie takes ownership of the values.
		typename t_adaptor::return_type ct((adaptor(tmp_trie)));
		
		// Find the rightmost subtree.
		auto key(ct.min_key());
		while (ct.find_next_subtree_key(key)) {}
		
		// Find a successor twice in order to pass the slightly greater key as a parameter.
		typename t_adaptor::trie_type::const_iterator it;
		
		AssertThat(ct.find_successor(key, it, false), Equals(true));
		AssertThat(*it, Equals(1 + key));
		key = *it;
		
		AssertThat(ct.find_successor(key, it, false), Equals(true));
		AssertThat(*it, Equals(1 + key));
		key = *it;
	});
	
	it("can find successors (subtree maximum)", [&]() {
		// Test with a rightmost value of a subtree.
		
		t_trie trie;
		t_adaptor adaptor;
		
		for (auto const i : boost::irange(1, 127, 1))
			trie.insert(i);
		
		t_trie tmp_trie(trie); // Copy since the compact trie takes ownership of the values.
		typename t_adaptor::return_type ct((adaptor(tmp_trie)));
		
		// Get a rightmost value of a subtree.
		auto subtree_key(ct.min_key());
		for (uint8_t i(0); i < 1; ++i)
			AssertThat(ct.find_next_subtree_key(subtree_key), Equals(true));
		typename t_adaptor::trie_type::const_iterator max_it;
		AssertThat(ct.find_subtree_max(subtree_key, max_it), Equals(true));
		auto const rightmost(*max_it);
		
		// Check that find_successor works for a rightmost value of a subtree.
		typename t_adaptor::trie_type::const_iterator it;
		AssertThat(ct.find_successor(rightmost, it, false), Equals(true));
		AssertThat(*it, Equals(1 + rightmost));
	});
}

#endif
