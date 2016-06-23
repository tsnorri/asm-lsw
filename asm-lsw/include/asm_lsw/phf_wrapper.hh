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
#include <sdsl/int_vector.hpp>


namespace asm_lsw {

	// Wrapper for struct phf since it needs a custom destructor.
	class phf_wrapper
	{
	public:
		typedef std::size_t size_type; // Needed for has_serialize.
		
	protected:
		struct phf m_phf{};
		
	public:
		phf_wrapper() {}

		~phf_wrapper() { if (is_valid()) PHF::destroy(&m_phf); }
		
		phf_wrapper(phf_wrapper const &);
		phf_wrapper &operator=(phf_wrapper const &) &;
		
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
		
		void load(std::istream &in);
		size_type serialize(std::ostream &out, sdsl::structure_tree_node *v = nullptr, std::string name = "") const;
		
	protected:
		template <typename T>
		void load_g(std::istream &in);
		
		template <typename T>
		size_type serialize_g(std::ostream &out, sdsl::structure_tree_node *v = nullptr, std::string name = "") const;

		template <typename T>
		void copy_g(phf_wrapper const &other);
	};
	
	
	template <typename T>
	void phf_wrapper::copy_g(phf_wrapper const &other)
	{
		// Allocate g with malloc and copy the values.
		assert(m_phf.r);
		m_phf.g = reinterpret_cast <uint32_t *>(malloc(m_phf.r * sizeof(T)));
		std::copy_n(reinterpret_cast <T const *>(other.m_phf.g), m_phf.r, reinterpret_cast <T *>(m_phf.g));
	}
	
	
	template <typename T>
	void phf_wrapper::load_g(std::istream &in)
	{
		sdsl::int_vector <0> g_vec;
		g_vec.load(in);
		
		// Allocate g with malloc and copy the values.
		m_phf.g = reinterpret_cast <uint32_t *>(malloc(m_phf.r * sizeof(T)));
		assert(m_phf.r);
		assert(m_phf.r == g_vec.size());
		std::copy_n(g_vec.cbegin(), m_phf.r, reinterpret_cast <T *>(m_phf.g));
	}
	
	
	template <typename T>
	std::size_t phf_wrapper::serialize_g(std::ostream &out, sdsl::structure_tree_node *child, std::string name) const
	{
		// There are m_phf.r items in m_phf.g. Copy them to an int_vector and serialize.
		assert(m_phf.r);
		std::size_t const size(sizeof(T));
		sdsl::int_vector <0> g_vec(m_phf.r, 0, 8 * size);
		std::copy_n(reinterpret_cast <T const *>(m_phf.g), m_phf.r, g_vec.begin());
		
		return g_vec.serialize(out, child, "g");
	}
}

#endif
