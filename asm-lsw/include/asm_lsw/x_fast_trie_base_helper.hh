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


#ifndef ASM_LSW_X_FAST_TRIE_BASE_HELPER_HH
#define ASM_LSW_X_FAST_TRIE_BASE_HELPER_HH

#include <array>
#include <asm_lsw/map_adaptor_helper.hh>
#include <asm_lsw/util.hh>
#include <cassert>


namespace asm_lsw { namespace detail {

	template <
		typename t_key,
		typename t_value,
		template <typename, typename, bool, typename> class t_map_adaptor_trait,
		bool t_enable_serialize,
		typename t_lss_find_fn
	>
	struct x_fast_trie_base_spec
	{
		typedef t_key key_type;		// Trie key type.
		typedef t_value value_type;	// Trie value type.

		// A class that has a member type “type” which is the actual adaptor.
		// Other parameteres are supposed to have been fixed earlier.
		template <
			typename t_map_key,
			typename t_map_value,
			bool t_default_enable_serialize = t_enable_serialize,
			typename t_map_access_key = map_adaptor_access_key <t_map_key>
		>
		using map_adaptor_trait = t_map_adaptor_trait <t_map_key, t_map_value, t_default_enable_serialize, t_map_access_key>;
		
		typedef t_lss_find_fn lss_find_fn;
	};


	// FIXME: used by the base class.
	template <typename t_key>
	class x_fast_trie_edge
	{
	protected:
		typedef t_key key_type;

	protected:
		key_type m_key;			// Key of the child node in LSS.
		
	public:
		x_fast_trie_edge(): x_fast_trie_edge(0) {};
		
		x_fast_trie_edge(key_type key):
			m_key(key)
		{
		}

		key_type key() const { return m_key; }
		key_type level_key(std::size_t level) const { return m_key >> level; }
	};
	
	
	template <typename t_key>
	class x_fast_trie_node : protected std::array <x_fast_trie_edge <t_key>, 2>
	{
	public:
		typedef std::size_t size_type; // Needed for has_serialize.
		
	protected:
		typedef x_fast_trie_edge <t_key> edge_type;
		typedef std::array <edge_type, 2> base_class;
		
	public:
		using base_class::operator[];

		x_fast_trie_node(): x_fast_trie_node(0, 0) {}

		x_fast_trie_node(edge_type e1, edge_type e2)
		{
			(*this)[0] = e1;
			(*this)[1] = e2;
		}
		
		bool const edge_is_descendant_ptr(uint8_t const level, uint8_t const branch) const
		{
			// If the level-th bit from the left is not equal to branch, the edge
			// contains a descendant pointer.
			assert(0 == branch || 1 == branch);
			auto const edge(operator[](branch));
			bool const retval(branch != (0x1 & edge.level_key(level)));
			return retval;
		}

		class access_key
		{
		protected:
			uint8_t m_level;

		public:
			typedef x_fast_trie_node <t_key> key_type;
			typedef t_key accessed_type;
			
			access_key(): m_level(0) {}
			access_key(uint8_t const level): m_level(level) {}
			
			uint8_t level() const { return m_level; }

			accessed_type operator()(key_type const &node) const
			{
				return node[0].level_key(m_level);
			}
			
			size_type serialize(std::ostream& out, sdsl::structure_tree_node *v = nullptr, std::string name = "") const
			{
				auto *child(sdsl::structure_tree::add_child(v, name, sdsl::util::class_name(*this)));
				std::size_t written_bytes(0);

				written_bytes += sdsl::write_member(m_level, out, child, "level");
		
				sdsl::structure_tree::add_size(child, written_bytes);
				return written_bytes;
			}
			
			void load(std::istream& in)
			{
				sdsl::read_member(m_level, in);
			}
		};
	};
	
	
	// FIXME: used by the base class.
	template <typename t_key, typename t_value>
	struct x_fast_trie_leaf_link
	{
		// FIXME: use iterators instead for prev and next?
		t_key prev;
		t_key next;
		t_value value;
		
		// FIXME: movability for val?
		x_fast_trie_leaf_link(): prev(0), next(0) {}
		x_fast_trie_leaf_link(t_key p, t_key n, t_value val): prev(p), next(n), value(val) {}

		template <typename t_other_key, typename t_other_value>
		explicit x_fast_trie_leaf_link(x_fast_trie_leaf_link <t_other_key, t_other_value> const &other):
			prev(other.prev),
			next(other.next),
			value(other.value)
		{
			static_assert(std::is_same <
				util::remove_c_t <util::remove_ref_t <t_value>>,
				util::remove_c_t <util::remove_ref_t <t_other_value>>
			>::value, "");
			assert(other.prev <= std::numeric_limits <t_key>::max());
			assert(other.next <= std::numeric_limits <t_key>::max());
		}
		
		bool operator==(x_fast_trie_leaf_link const &other) const
		{
			return prev == other.prev && next == other.next && value == other.value;
		}
		
		template <typename Fn>
		std::size_t serialize(
			std::ostream &out,
			Fn value_callback,
			sdsl::structure_tree_node *v = nullptr,
			std::string name = ""
		) const
		{
			auto *child(sdsl::structure_tree::add_child(v, name, sdsl::util::class_name(*this)));
			std::size_t written_bytes(0);

			written_bytes += sdsl::write_member(prev, out, child, "prev");
			written_bytes += sdsl::write_member(next, out, child, "next");
			written_bytes += value_callback(value, out, child);
	
			sdsl::structure_tree::add_size(child, written_bytes);
			return written_bytes;
		}
		
		template <typename Fn>
		void load(std::istream &in, Fn value_callback)
		{
			sdsl::read_member(prev, in);
			sdsl::read_member(next, in);
			value_callback(value, in);
		}
	};
	
	
	// Specialization for a set type collection.
	template <typename t_key>
	struct x_fast_trie_leaf_link <t_key, void>
	{
		typedef std::size_t size_type; // Needed for has_serialize.
		
		// FIXME: use iterators instead for prev and next?
		t_key prev;
		t_key next;
		
		x_fast_trie_leaf_link(): x_fast_trie_leaf_link(0, 0) {}
		x_fast_trie_leaf_link(t_key p, t_key n): prev(p), next(n) {}

		template <typename t_other_key>
		explicit x_fast_trie_leaf_link(x_fast_trie_leaf_link <t_other_key, void> const &other):
			prev(other.prev),
			next(other.prev)
		{
			assert(other.prev <= std::numeric_limits <t_key>::max());
			assert(other.next <= std::numeric_limits <t_key>::max());
		}
		
		bool operator==(x_fast_trie_leaf_link const &other) const
		{
			return prev == other.prev && next == other.next;
		}
		
		size_type serialize(std::ostream &out, sdsl::structure_tree_node *v = nullptr, std::string name = "") const
		{
			auto *child(sdsl::structure_tree::add_child(v, name, sdsl::util::class_name(*this)));
			std::size_t written_bytes(0);

			written_bytes += sdsl::write_member(prev, out, child, "prev");
			written_bytes += sdsl::write_member(next, out, child, "next");
	
			sdsl::structure_tree::add_size(child, written_bytes);
			return written_bytes;
		}
		
		void load(std::istream& in)
		{
			sdsl::read_member(prev, in);
			sdsl::read_member(next, in);
		}
	};
	
	
	template <typename t_key, typename t_value>
	struct x_fast_trie_trait
	{
		typedef t_key key_type;
		typedef t_value value_type;
		
		template <typename t_iterator>
		key_type key(t_iterator it) const
		{
			return it->first;
		}

		template <typename t_iterator>
		value_type const &value(t_iterator it) const
		{
			return it->second.value;
		}

		enum { is_map_type = 1 };
	};
	
	
	template <typename t_key>
	struct x_fast_trie_trait <t_key, void>
	{
		typedef t_key key_type;
		typedef t_key value_type;

		template <typename t_iterator>
		key_type key(t_iterator it)
		{
			return it->first;
		}

		template <typename t_iterator>
		value_type const &value(t_iterator it)
		{
			return it->first;
		}
		
		enum { is_map_type = 0 };
		
		static_assert(
			sdsl::has_serialize <x_fast_trie_leaf_link <t_key, void>>::value,
			"Leaf link needs to have serialize."
		);
			
		static_assert(
			sdsl::has_load <x_fast_trie_leaf_link <t_key, void>>::value,
			"Leaf link needs to have load."
		);
	};
	
	
	template <typename t_mapped_type, bool t_dummy = true>
	struct x_fast_trie_cmp
	{
		template <typename T, typename U>
		bool contains(T const &trie_1, U const &trie_2)
		{
			typename T::const_iterator it;
			for (auto const &kv : trie_2)
			{
				if (!trie_1.find(kv.first, it))
					return false;
				
				if (! (kv.second.value == it->second.value))
					return false;
			}
			
			return true;
		}
	};
	
	
	template <bool t_dummy>
	struct x_fast_trie_cmp <void, t_dummy>
	{
		template <typename T, typename U>
		bool contains(T const &trie_1, U const &trie_2)
		{
			typename T::const_iterator it;
			for (auto const &kv : trie_2)
			{
				if (!trie_1.find(kv.first, it))
					return false;
			}
			
			return true;
		}
	};
}}

#endif
