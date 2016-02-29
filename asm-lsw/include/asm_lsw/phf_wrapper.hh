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


#ifndef ASM_LSW_PHF_WRAPPER_HH
#define ASM_LSW_PHF_WRAPPER_HH

#include <cstring>
#include <phf.h>

namespace asm_lsw {

	// Wrapper for struct phf since it needs a custom destructor.
	class phf_wrapper
	{
	protected:
		struct phf m_phf{};
		
	public:
		phf_wrapper() {}
                
		~phf_wrapper() { if (is_valid()) PHF::destroy(&m_phf); }
		
		phf_wrapper(phf_wrapper const &) = delete;
		phf_wrapper &operator=(phf_wrapper const &) & = delete;
		
		phf_wrapper(phf_wrapper &&other):
			m_phf(other.m_phf)
		{
			memset(&other.m_phf, 0, sizeof(struct phf));
		}
		
		phf_wrapper &operator=(phf_wrapper &&other) &
		{
			m_phf = other.m_phf;
			memset(&other.m_phf, 0, sizeof(struct phf));
			return *this;
		}

		// Checked from PHF source that g is freed in PHF::destroy.
		bool is_valid() const { return (nullptr != m_phf.g); }
		
		struct phf &get()				{ return m_phf; }
		struct phf const &get() const	{ return m_phf; }
	};
}

#endif
