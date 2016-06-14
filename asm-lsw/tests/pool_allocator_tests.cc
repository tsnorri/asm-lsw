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
void typed_tests()
{
	it("can allocate memory with different alignments", [](){
		asm_lsw::pool_allocator <uint16_t> allocator_1((std::size_t) 8);
		allocator_1.allocate(1);

		asm_lsw::pool_allocator <uint64_t> allocator_2(allocator_1);
		allocator_2.allocate(1);
		
		AssertThat(allocator_1.bytes_left(), Equals(0));
		AssertThat(allocator_1.bytes_left(), Equals(0));
	});

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


void common_tests()
{
	it("doesn't affect destructor calls", [](){
		
		struct test
		{
			int32_t *c{nullptr};
			
			~test()
			{
				if (c)
					++*c;
			}
		};
		
		int32_t c{0};
		asm_lsw::pool_allocator <test> allocator((std::size_t) 1);
		
		{
			test t;
			std::vector <test, decltype(allocator)> vec(1, t, allocator);
			vec[0].c = &c;
		}
		
		AssertThat(c, Equals(1));
	});
}


go_bandit([](){
	describe("pool_allocator<uint8_t>:", [](){
		typed_tests <uint8_t>();
	});

	describe("pool_allocator<uint64_t>:", [](){
		typed_tests <uint64_t>();
	});
	
	describe("pool_allocator", [](){
		common_tests();
	});
});
