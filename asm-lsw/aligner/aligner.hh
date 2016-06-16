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


#ifndef ASM_LSW_ALIGNER_ALIGNER_HH
#define ASM_LSW_ALIGNER_ALIGNER_HH

#include <asm_lsw/kn_matcher.hh>
#include <sdsl/lcp_support_sada.hpp>
#include <sdsl/csa_rao.hpp>
#include <sdsl/cst_sada.hpp>


typedef sdsl::cst_sada <sdsl::csa_rao <sdsl::csa_rao_spec <0, 0>>, sdsl::lcp_support_sada <>> cst_type;
typedef asm_lsw::kn_matcher <cst_type> kn_matcher_type;


extern "C" void align(char const *source_fname, char const *cst_fname, short const k, bool const single_thread);
extern "C" void create_index(std::istream &source_stream);
extern "C" void handle_error();

#endif
