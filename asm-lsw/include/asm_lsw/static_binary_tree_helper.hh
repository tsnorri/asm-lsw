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

#ifndef ASM_LSW_STATIC_BINARY_TREE_HELPER_HH
#define ASM_LSW_STATIC_BINARY_TREE_HELPER_HH

#include <asm_lsw/util.hh>
#include <sdsl/int_vector.hpp>
#include <utility>
#include <vector>


namespace asm_lsw { namespace detail {

	template <typename t_key, typename t_mapped, bool t_enable_serialize>
	class static_binary_tree_helper
	{
	public:
		typedef t_key														key_type;
		typedef t_mapped													mapped_type;
		typedef std::pair <t_key, t_mapped>									value_type;
		typedef std::vector <value_type>									value_vector_type;
		typedef typename value_vector_type::size_type						size_type;
		
		typedef typename value_vector_type::reference						reference;
		typedef typename value_vector_type::const_reference					const_reference;
		
		// constness is required for making the pairs eligible for equality comparison.
		// References are required since the trie classes have accessors for iterator keys
		// and values but Boost's implementation returns a proxy object in case the iterator
		// doesn't return a reference. As a consequence, the returned reference to
		// it->second would become invalid at the time of returning from the accessor.
		// Keys are required to be unsigned integers anyway, so the said accessors copy them.
		typedef std::pair <t_key const, t_mapped const &>					iterator_val;
		typedef iterator_val												iterator_ref;
		
		enum { is_map_type = 1 };
		
	protected:
		value_vector_type	m_values;
		
	public:
		static_binary_tree_helper() = default;

		static_binary_tree_helper(size_type const size):
			m_values(size)
		{
		}
		
		size_type const size() const { return m_values.size(); }
		
		key_type const &key(size_type idx) const { return m_values[idx].first; }
		mapped_type const &mapped(size_type idx) const { return m_values[idx].second; }

		reference value(size_type idx) { return m_values[idx]; }
		const_reference value(size_type idx) const { return m_values[idx]; }
		
		iterator_ref const dereference(size_type idx) const
		{
			auto &kv(value(idx));
			return iterator_ref(kv.first, kv.second);
		}
		
		template <typename t_vector>
		void sort(t_vector &vec) { std::sort(vec.begin(), vec.end(), [](auto &left, auto &right){ return left.first < right.first; }); }
		
		template <typename Fn>
		auto serialize_keys(
			std::ostream &out,
			Fn value_callback,				// std::size_t(t_value const &, std::ostream &, sdsl::structure_tree_node *)
			sdsl::structure_tree_node *v,
			std::string name
		) const -> size_type
		{
			auto *child(sdsl::structure_tree::add_child(v, name, sdsl::util::class_name(*this)));
			size_type written_bytes(0);
		
			sdsl::write_member_nn(m_values.size(), out);
			for (auto const &kv : m_values)
			{
				written_bytes += sdsl::write_member_nn(kv.first, out);
				value_callback(kv.second, out, child);
			}
		
			sdsl::structure_tree::add_size(child, written_bytes);
			return written_bytes;
		}
		
		template <bool t_dummy = true>
		auto serialize(
			std::ostream &out,
			sdsl::structure_tree_node *v,
			std::string name
		) const -> typename std::enable_if <t_enable_serialize && t_dummy, size_type>::type
		{
			asm_lsw::util::serialize_value_fn <mapped_type> serialize_value;
			return serialize_keys(out, serialize_value, v, name);
		}
		
		template <typename Fn>
		auto load_keys(
			std::istream &in,
			Fn value_callback				// void(t_value &, std::istream &)
		)
		{
			size_type count(0);
			sdsl::read_member(count, in);
			
			m_values.resize(count);
			for (size_type i(0); i < count; ++i)
			{
				auto &kv(m_values[i]);
				sdsl::read_member(kv.first, in);
				value_callback(kv.second, in);
			}
		}
		
		template <bool t_dummy = true>
		auto load(std::istream& in) -> typename std::enable_if <t_enable_serialize && t_dummy, void>::type
		{
			asm_lsw::util::load_value_fn <mapped_type> load_value;
			load_keys(in, load_value);
		}
		
		template <typename t_tree>
		void print(t_tree const &tree) const
		{
			std::cerr << "FIXME: implement me." << std::endl;
		}
	};
	
	
	template <typename t_key, bool t_enable_serialize>
	class static_binary_tree_helper <t_key, void, t_enable_serialize>
	{
	public:
		typedef t_key														key_type;
		typedef t_key														mapped_type;
		typedef t_key														value_type;
		typedef sdsl::int_vector <std::numeric_limits <t_key>::digits>		key_vector_type;
		typedef typename key_vector_type::size_type							size_type;
		
		typedef typename key_vector_type::reference							reference;
		typedef typename key_vector_type::const_reference					const_reference;
		
		typedef value_type													iterator_val;
		typedef const_reference												iterator_ref;
		
		enum { is_map_type = 0 };
		
	protected:
		key_vector_type		m_keys;
	
	public:
		static_binary_tree_helper() = default;
		
		static_binary_tree_helper(size_type const size):
			m_keys(size, 0)
		{
		}
		
		size_type const size() const { return m_keys.size(); }
		
		key_type const key(size_type idx) const { return m_keys[idx]; }
		mapped_type const mapped(size_type idx) const { return key(idx); }
		const_reference const value(size_type idx) const { return key(idx); }
		
		reference value(size_type idx) { return m_keys[idx]; }
		
		iterator_val dereference(size_type idx) const
		{
			auto const kv(value(idx));
			iterator_val retval(kv);
			return retval;
		}
		
		template <typename t_vector>
		void sort(t_vector &vec) { std::sort(vec.begin(), vec.end()); }
		
		size_type serialize(std::ostream& out, sdsl::structure_tree_node *v = nullptr, std::string name = "") const
		{
			auto *child(sdsl::structure_tree::add_child(v, name, sdsl::util::class_name(*this)));
			size_type written_bytes(0);
		
			written_bytes += m_keys.serialize(out, child, "keys");
		
			sdsl::structure_tree::add_size(child, written_bytes);
			return written_bytes;
		}
		
		void load(std::istream& in)
		{
			m_keys.load(in);
		}
		
		template <typename t_tree>
		void print(t_tree const &tree) const
		{
			size_type const count(tree.m_used_indices.size());

			std::cerr << "Keys:   ";
			auto it(m_keys.cbegin());
			for (size_type i(0); i < count; ++i)
			{
				if (tree.m_used_indices[i])
				{
					std::cerr << " " << std::hex << std::setw(2) << std::setfill('0') << +(*it);
					++it;
				}
				else
				{
					std::cerr << "   ";
				}
			}
			std::cerr << std::endl;
		}
	};
}}

#endif
