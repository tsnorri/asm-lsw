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


#include <asm_lsw/pool_allocator.hh>
#include <bandit/bandit.h>
#include <vector>

using namespace bandit;

template <typename t_element>
void common_tests()
{
	it("can allocate memory for std::vector", [](){
		std::size_t const size(10);
		asm_lsw::pool_allocator <t_element> allocator(size);
		std::vector <t_element, decltype(allocator)> vec(size, 5, allocator);
		vec[6] = 7;
		
		std::size_t i(0);
		for (auto const k : vec)
		{
			if (6 == i)
			{
				AssertThat(k, Equals(7));
			}
			else
			{
				AssertThat(k, Equals(5));
			}
			
			++i;
		}
	});
	
	it("throws an exception when it cannot allocate", [](){
		std::size_t const size(10);
		asm_lsw::pool_allocator <t_element> allocator(size);
		AssertThrows(std::bad_alloc, (std::vector <t_element, decltype(allocator)>(1 + size, 5, allocator)));
	});
}


go_bandit([](){
	describe("pool_allocator<uint8_t>:", [](){
		common_tests <uint8_t>();
	});

	describe("pool_allocator<uint64_t>:", [](){
		common_tests <uint64_t>();
	});
});
