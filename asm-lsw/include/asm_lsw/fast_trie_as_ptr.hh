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


#ifndef ASM_LSW_FAST_TRIE_AS_PTR_HH
#define ASM_LSW_FAST_TRIE_AS_PTR_HH

#include <memory>
#include <sdsl/io.hpp>


namespace asm_lsw {

	// A serializable pointer for autosizing tries.
	template <typename t_trie>
	class fast_trie_as_ptr
	{
	public:
		typedef t_trie							trie_type;
		typedef std::unique_ptr <trie_type>		pointer_type;
		typedef typename trie_type::size_type	size_type;
		
	protected:
		pointer_type m_ptr;
			
	public:
		fast_trie_as_ptr(typename pointer_type::pointer ptr = nullptr):
			m_ptr(ptr)
		{
		}
		
		inline typename pointer_type::pointer get() const { return m_ptr.get(); }
		inline typename pointer_type::pointer operator->() const { return m_ptr.operator->(); }
		inline void reset(typename pointer_type::pointer ptr) { m_ptr.reset(ptr); }
		
		size_type serialize(std::ostream &out, sdsl::structure_tree_node *v = nullptr, std::string name = "") const
		{
			auto *child(sdsl::structure_tree::add_child(v, name, sdsl::util::class_name(*this)));
			size_type written_bytes(0);
			
			bool is_nullptr(nullptr == m_ptr.get());
			written_bytes += sdsl::write_member(is_nullptr, out, child, "is_nullptr");
			
			if (!is_nullptr)
				written_bytes += m_ptr->serialize(out, v, name);
			
			sdsl::structure_tree::add_size(child, written_bytes);
			return written_bytes;
		}
		
		void load(std::istream &in)
		{
			bool is_nullptr(false);
			sdsl::read_member(is_nullptr, in);
			if (!is_nullptr)
				m_ptr.reset(trie_type::load(in));
		}
	};
}

#endif
