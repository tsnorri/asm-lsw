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


#ifndef ASM_LSW_MAP_ADAPTOR_PHF_HELPER_HH
#define ASM_LSW_MAP_ADAPTOR_PHF_HELPER_HH

#include <utility>


namespace asm_lsw { namespace detail {

	template <typename t_spec>
	class map_adaptor_phf_base;
	
	
	// Choose either write_member or serialize. The latter probably isn't needed much.
	template <typename t_value, bool t_has_serialize = sdsl::has_serialize <t_value>::value>
	struct map_adaptor_serialize_value_fn {};

	template <typename t_value>
	struct map_adaptor_serialize_value_fn <t_value, true>
	{
		std::size_t operator()(t_value const &value, std::ostream &out, sdsl::structure_tree_node *node)
		{
			return value.serialize(out, node, "");
		}
	};
	
	template <typename t_value>
	struct map_adaptor_serialize_value_fn <t_value, false>
	{
		std::size_t operator()(t_value const &value, std::ostream &out, sdsl::structure_tree_node *node)
		{
			return sdsl::write_member_nn(value, out);
		}
	};
	
	
	// Choose either read_member or load. The latter probably isn't needed much.
	template <typename t_value, bool t_has_load = sdsl::has_load <t_value>::value>
	struct map_adaptor_load_value_fn {};

	template <typename t_value>
	struct map_adaptor_load_value_fn <t_value, true>
	{
		void operator()(t_value &value, std::istream &in)
		{
			value.load(in);
		}
	};
	
	template <typename t_value>
	struct map_adaptor_load_value_fn <t_value, false>
	{
		void operator()(t_value &value, std::istream &in)
		{
			sdsl::read_member(value, in);
		}
	};


	// Specialize when t_value = void.
	template <typename t_spec, bool t_value_is_void = std::is_void <typename t_spec::value_type>::value>
	struct map_adaptor_phf_trait
	{
	};

	template <typename t_spec>
	struct map_adaptor_phf_trait <t_spec, true>
	{
		typedef typename t_spec::access_key_fn_type::key_type				key_type;
		typedef typename t_spec::value_type									value_type;	// FIXME: mapped_type?
		typedef key_type													kv_type;	// FIXME: value_type?
		typedef key_type const												const_kv_type;	// FIXME: value_type?
		
		enum { is_map_type = 0 };

		template <typename t_kv>
		static key_type key(t_kv &kv)
		{
			return kv;
		}

		template <typename t_kv>
		static kv_type kv(t_kv &kv)
		{
			return kv;
		}
		
		template <typename t_map, typename t_other>
		static bool contains(t_map const &map, t_other const &other)
		{
			auto const cend(map.cend());
			for (auto const &k : other)
			{
				auto const it(map.find(k));
				if (cend == it)
					return false;
			}
			
			return true;
		}
		
		template <typename t_vector, typename t_used_indices>
		static std::size_t serialize(
			t_vector const &vector,
			t_used_indices const &used_indices,
			std::ostream &out,
			sdsl::structure_tree_node *v
		)
		{
			std::size_t written_bytes(0);
			
			sdsl::structure_tree_node* node(sdsl::structure_tree::add_child(v, "keys", ""));
			for (typename t_used_indices::size_type i(0), count(used_indices.size() - 1); i < count; ++i)
			{
				if (used_indices[i])
					written_bytes += sdsl::write_member_nn(vector[i], out);
			}
			
			sdsl::structure_tree::add_size(node, written_bytes);
			
			return written_bytes;
		}
		
		template <typename t_vector, typename t_used_indices, typename Fn>
		static std::size_t serialize_keys(
			t_vector const &vector,
			t_used_indices const &used_indices,
			Fn value_callback,
			std::ostream &out,
			sdsl::structure_tree_node *v
		)
		{
			return serialize(vector, used_indices, out, v);
		}
		
		template <typename t_vector, typename t_used_indices>
		static void load(
			t_vector &vector,
			t_used_indices const &used_indices,
			std::istream &in
		)
		{
			for (typename t_used_indices::size_type i(0), count(used_indices.size() - 1); i < count; ++i)
			{
				assert(i < used_indices.size());
				if (used_indices[i])
				{
					assert(i < vector.size());
					sdsl::read_member(vector[i], in);
				}
			}
		}
		
		template <typename t_vector, typename t_used_indices, typename Fn>
		static void load_keys(
			t_vector &vector,
			t_used_indices const &used_indices,
			Fn value_callback,
			std::istream &in
		)
		{
			load(vector, used_indices, in);
		}
	};

	template <typename t_spec>
	struct map_adaptor_phf_trait <t_spec, false>
	{
		typedef typename t_spec::access_key_fn_type::key_type				key_type;
		typedef typename t_spec::value_type									value_type;	// FIXME: mapped_type?
		typedef std::pair <key_type, value_type>							kv_type;	// FIXME: value_type?
		typedef std::pair <key_type const, value_type>						const_kv_type;	// FIXME: value_type?

		enum { is_map_type = 1 };

		template <typename t_kv>
		static key_type key(t_kv &kv)
		{
			return kv.first;
		}

		template <typename t_kv>
		static kv_type kv(t_kv &kv)
		{
			return std::make_pair(kv.first, std::move(kv.second));
		}
		
		template <typename t_map, typename t_other>
		static bool contains(t_map const &map, t_other const &other)
		{
			auto const cend(map.cend());
			for (auto const &kv : other)
			{
				auto const it(map.find(kv.first));
				if (cend == it)
					return false;
				
				if (! (kv.second == it->second))
					return false;
			}
			
			return true;
		}
		
		// Only serialize the keys and call value_callback on values.
		template <typename t_vector, typename t_used_indices, typename Fn>
		static std::size_t serialize_keys(
			t_vector const &vector,
			t_used_indices const &used_indices,
			Fn value_callback,
			std::ostream &out,
			sdsl::structure_tree_node *v
		)
		{
			std::size_t size_keys(0);
			std::size_t size_values(0);
			
			sdsl::structure_tree_node *node_keys(sdsl::structure_tree::add_child(v, "keys", ""));
			sdsl::structure_tree_node *node_values(sdsl::structure_tree::add_child(v, "values", ""));
			
			for (typename t_used_indices::size_type i(0), count(used_indices.size() - 1); i < count; ++i)
			{
				if (used_indices[i])
					size_keys += sdsl::write_member_nn(vector[i].first, out);
			}
			
			for (typename t_used_indices::size_type i(0), count(used_indices.size() - 1); i < count; ++i)
			{
				if (used_indices[i])
					size_values += value_callback(vector[i].second, out, node_values);
			}
			
			sdsl::structure_tree::add_size(node_keys, size_keys);
			sdsl::structure_tree::add_size(node_values, size_values);
			
			return size_keys + size_values;
		}
		
		// Serialize also the values.
		template <typename t_vector, typename t_used_indices>
		static std::size_t serialize(
			t_vector const &vector,
			t_used_indices const &used_indices,
			std::ostream &out,
			sdsl::structure_tree_node *v
		)
		{
			map_adaptor_serialize_value_fn <value_type> serialize_value;
			auto cb = [&](value_type const &value, std::ostream &out, sdsl::structure_tree_node *node) {
				return serialize_value(value, out, node);
			};
			
			return serialize_keys(vector, used_indices, cb, out, v);
		}
		
		template <typename t_vector, typename t_used_indices, typename Fn>
		static void load_keys(
			t_vector &vector,
			t_used_indices const &used_indices,
			Fn value_callback,
			std::istream &in
		)
		{
			for (typename t_used_indices::size_type i(0), count(used_indices.size() - 1); i < count; ++i)
			{
				assert(i < used_indices.size());
				if (used_indices[i])
				{
					assert(i < vector.size());
					sdsl::read_member(vector[i].first, in);
				}
			}
			
			for (typename t_used_indices::size_type i(0), count(used_indices.size() - 1); i < count; ++i)
			{
				assert(i < used_indices.size());
				if (used_indices[i])
				{
					assert(i < vector.size());
					value_callback(vector[i].second, in);
				}
			}
		}
		
		template <typename t_vector, typename t_used_indices>
		static void load(
			t_vector &vector,
			t_used_indices const &used_indices,
			std::istream &in
		)
		{
			map_adaptor_load_value_fn <value_type> load_value;
			auto cb = [&](value_type &value, std::istream &in) {
				return load_value(value, in);
			};
			
			return load_keys(vector, used_indices, cb, in);
		}
	};
	
	
	template <typename t_spec>
	struct map_adaptor_phf_access_value
	{
		typedef typename map_adaptor_phf_trait <t_spec>::kv_type kv_type;
		
		template <typename t_kv>
		kv_type operator()(t_kv &kv)
		{
			return map_adaptor_phf_trait <t_spec>::kv(kv);
		}
	};
}}

#endif
