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

#include <asm_lsw/matrix.hh>
#include <bandit/bandit.h>

using namespace bandit;


template <uint8_t t_bits>
void matrix_tests()
{
	typedef asm_lsw::matrix <t_bits> matrix_type;
	
	it("can create an empty matrix", [](){
		matrix_type matrix;
	});
	
	it("can create a 5 x 6 matrix", [](){
		matrix_type matrix(5, 6, 1);
		
		AssertThat(matrix.rows(), Equals(5));
		AssertThat(matrix.columns(), Equals(6));
	});
	
	it("can access values", [](){
		matrix_type matrix(4, 5, 0);
		
		auto rows(matrix.rows()), cols(matrix.columns());
		for (decltype(rows) i(0); i < rows; ++i)
		{
			for (decltype(cols) j(0); j < cols; ++j)
					AssertThat(matrix(i, j), Equals(0));
		}

		matrix(3, 4) = 1;
		AssertThat(matrix(3, 4), Equals(1));
		
		for (decltype(rows) i(0); i < rows; ++i)
		{
			for (decltype(cols) j(0); j < cols; ++j)
			{
				if (! (3 == i && 4 == j))
					AssertThat(matrix(i, j), Equals(0));
			}
		}
	});
}


go_bandit([](){
	describe("matrix <1>:", [](){
		matrix_tests <1>();
	});

	describe("matrix <8>:", [](){
		matrix_tests <8>();
	});
});
