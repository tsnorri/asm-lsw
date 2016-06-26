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


#include <asm_lsw/phf_wrapper.hh>


using namespace asm_lsw;


phf_wrapper::phf_wrapper(phf_wrapper const &other)
{
	m_phf = other.m_phf;
	
	if (!other.is_valid())
		return;
	
	// The items in m_phf.g are either uint8_t, uint16_t or uint32_t. Compare to PHF::compact.
	if (m_phf.d_max <= 255)
		copy_g <uint8_t>(other);
	else if (m_phf.d_max <= 65535)
		copy_g <uint16_t>(other);
	else
		copy_g <uint32_t>(other);
}


phf_wrapper &phf_wrapper::operator=(phf_wrapper const &other) &
{
	if (&other != this)
	{
		if (is_valid())
			PHF::destroy(&m_phf);
	
		phf_wrapper tmp(other); // Copy.
		*this = std::move(tmp);
	}
	return *this;
}


std::size_t phf_wrapper::serialize(std::ostream &out, sdsl::structure_tree_node *v, std::string name) const
{
	// Use SDSL's IO facility to serialize the representation.
	
	auto *child(sdsl::structure_tree::add_child(v, name, sdsl::util::class_name(*this)));
	std::size_t written_bytes(0);
	
	written_bytes += sdsl::write_member(m_phf.nodiv, out, child, "nodiv");
	written_bytes += sdsl::write_member(m_phf.seed, out, child, "seed");
	written_bytes += sdsl::write_member(m_phf.r, out, child, "r");
	written_bytes += sdsl::write_member(m_phf.m, out, child, "m");
	written_bytes += sdsl::write_member(m_phf.d_max, out, child, "d_max");
	written_bytes += sdsl::write_member(m_phf.g_op, out, child, "g_op");
	
	// Don't serialize g_jmp.
	
	// Also don't serialize m_phf.g if it is empty.
	if (m_phf.r)
	{
		// The items in m_phf.g are either uint8_t, uint16_t or uint32_t. Compare to PHF::compact.
		if (m_phf.d_max <= 255)
			written_bytes += serialize_g <uint8_t>(out, child, name);
		else if (m_phf.d_max <= 65535)
			written_bytes += serialize_g <uint16_t>(out, child, name);
		else
			written_bytes += serialize_g <uint32_t>(out, child, name);
	}
	
	sdsl::structure_tree::add_size(child, written_bytes);
	return written_bytes;
}


void phf_wrapper::load(std::istream &in)
{
	// Only allow loading into a default-initialized object.
	assert(!is_valid());
	
	sdsl::read_member(m_phf.nodiv, in);
	sdsl::read_member(m_phf.seed, in);
	sdsl::read_member(m_phf.r, in);
	sdsl::read_member(m_phf.m, in);
	sdsl::read_member(m_phf.d_max, in);
	sdsl::read_member(m_phf.g_op, in);
	
	if (m_phf.r)
	{
		if (m_phf.d_max <= 255)
			load_g <uint8_t>(in);
		else if (m_phf.d_max <= 65535)
			load_g <uint16_t>(in);
		else
			load_g <uint32_t>(in);
	}
}
