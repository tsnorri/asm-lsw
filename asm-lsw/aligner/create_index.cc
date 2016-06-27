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


#include <asm_lsw/fasta_reader.hh>
#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/stream.hpp>
#include <fcntl.h>
#include <iostream>
#include <sdsl/psi_k_support.hpp>
#include <sdsl/csa_rao_builder.hpp>
#include <sdsl/io.hpp>
#include <unistd.h>
#include "aligner.hh"

namespace ios = boost::iostreams;


class create_index_cb
{
protected:
	char const *m_source_fname{};
	std::ostream *m_output_stream{};
	bool m_handled_seq{false};
	
public:
	create_index_cb(char const *source_fname, std::ostream &output_stream):
		m_source_fname(source_fname),
		m_output_stream(&output_stream)
	{
		assert(m_source_fname);
	}
	
	void handle_sequence(
		std::string const &identifier,
		std::unique_ptr <std::vector <char>> &seq,
		asm_lsw::vector_source &vs
	)
	{
		if (m_handled_seq)
			throw std::runtime_error("Read more than one sequence");
		
		// Write the sequence to the specified file.
		std::copy(seq->begin(), seq->end(), std::ostream_iterator <char>(*m_output_stream));
		m_output_stream->flush();
		vs.put_vector(seq);
		
		// Read the sequence from the file.
		std::cerr << "Creating the CST…" << std::endl;
		cst_type cst;
		sdsl::construct(cst, m_source_fname, 1);
		
		// Other data structures.
		std::cerr << "Creating other data structures…" << std::endl;
		kn_matcher_type matcher(cst);
		
		// Serialize.
		std::cerr << "Serializing…" << std::endl;
		sdsl::serialize(cst, std::cout);
		sdsl::serialize(matcher, std::cout);
		
		m_handled_seq = true;
	}
	
	void finish() {}
};


void create_index(std::istream &source_stream)
{
	// SDSL reads the whole string from a file so copy the contents without the newlines
	// into a temporary file, then create the index.
	char temp_fname[]{"/tmp/asm_lsw_aligner_XXXXXX"};
	int const temp_fd(mkstemp(temp_fname));
	if (-1 == temp_fd)
		handle_error();
	
	try
	{
		// Read the sequence from input and create the index in the callback.
		asm_lsw::vector_source vs(1, false);
		asm_lsw::fasta_reader <create_index_cb, 10 * 1024 * 1024> reader;
		ios::stream <ios::file_descriptor_sink> output_stream(temp_fd, ios::close_handle);
		create_index_cb cb(temp_fname, output_stream);
		
		reader.read_from_stream(source_stream, vs, cb);
	}
	catch (...)
	{
		if (-1 == unlink(temp_fname))
			handle_error();
	}
}
