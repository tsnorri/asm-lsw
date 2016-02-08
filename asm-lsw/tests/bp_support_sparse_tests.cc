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


#include <asm_lsw/bp_support_sparse.hh>
#include <bandit/bandit.h>

using namespace bandit;

go_bandit([](){

	describe("bp_support_sparse:", [](){

		asm_lsw::bp_support_sparse <> bps;
		char const *input("  () (( ) ()  ) ");

		before_each([&]() {
			decltype(bps)::from_string(bps, input);
		});

		it("finds closings", [&](){
			AssertThat(bps.find_close(2),	Equals(3));
			AssertThat(bps.find_close(5),	Equals(14));
			AssertThat(bps.find_close(6),	Equals(8));
			AssertThat(bps.find_close(10),	Equals(11));
		});

		it("finds openings", [&](){
			AssertThat(bps.find_open(3),	Equals(2));
			AssertThat(bps.find_open(14),	Equals(5));
			AssertThat(bps.find_open(8),	Equals(6));
			AssertThat(bps.find_open(11),	Equals(10));
		});

		it("checks openings before finding closings", [&](){
			AssertThrows(asm_lsw::invalid_argument, bps.find_close(0));
			AssertThat(LastException <asm_lsw::invalid_argument>().error <decltype(bps)::error>(), Equals(decltype(bps)::error::sparse_index));

			AssertThrows(asm_lsw::invalid_argument, bps.find_close(1));
			AssertThat(LastException <asm_lsw::invalid_argument>().error <decltype(bps)::error>(), Equals(decltype(bps)::error::sparse_index));

			AssertThrows(asm_lsw::invalid_argument, bps.find_close(3));
			AssertThat(LastException <asm_lsw::invalid_argument>().error <decltype(bps)::error>(), Equals(decltype(bps)::error::bad_parenthesis));

			AssertThrows(asm_lsw::invalid_argument, bps.find_close(20));
			AssertThat(LastException <asm_lsw::invalid_argument>().error <decltype(bps)::error>(), Equals(decltype(bps)::error::out_of_range));
		});

		it("checks closings before finding openings", [&](){
			AssertThrows(asm_lsw::invalid_argument, bps.find_open(0));
			AssertThat(LastException <asm_lsw::invalid_argument>().error <decltype(bps)::error>(), Equals(decltype(bps)::error::sparse_index));

			AssertThrows(asm_lsw::invalid_argument, bps.find_open(1));
			AssertThat(LastException <asm_lsw::invalid_argument>().error <decltype(bps)::error>(), Equals(decltype(bps)::error::sparse_index));

			AssertThrows(asm_lsw::invalid_argument, bps.find_open(2));
			AssertThat(LastException <asm_lsw::invalid_argument>().error <decltype(bps)::error>(), Equals(decltype(bps)::error::bad_parenthesis));

			AssertThrows(asm_lsw::invalid_argument, bps.find_open(20));
			AssertThat(LastException <asm_lsw::invalid_argument>().error <decltype(bps)::error>(), Equals(decltype(bps)::error::out_of_range));
		});

		it("has rank support", [&](){
			AssertThat(bps.rank(0), Equals(0));
			AssertThat(bps.rank(2), Equals(1));
			AssertThat(bps.rank(4), Equals(1));
			AssertThat(bps.rank(5), Equals(2));
		});

		it("has select support", [&](){
			AssertThat(bps.select(1), Equals(2));
			AssertThat(bps.select(2), Equals(5));
			AssertThat(bps.select(3), Equals(6));
		});
	});
});
