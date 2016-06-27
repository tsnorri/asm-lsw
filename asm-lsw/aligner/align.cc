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


#include <asm_lsw/dispatch_fn.hh>
#include <asm_lsw/fasta_reader.hh>
#include <asm_lsw/util.hh>
#include <asm_lsw/vector_source.hh>
#include <boost/iostreams/device/file_descriptor.hpp>
#include <dispatch/dispatch.h>
#include <thread>
#include "aligner.hh"

namespace ios = boost::iostreams;


class align_context
{
public:
	virtual ~align_context() {}
	virtual void load_and_align(char const *source_fname_c, char const *cst_fname_c) = 0;
};


template <bool t_report_all>
class align_context_tpl : public align_context
{
protected:
	typedef ios::stream <ios::file_descriptor_source> source_stream_type;

protected:
	cst_type									m_cst{};
	kn_matcher_type								m_matcher{};
	asm_lsw::fasta_reader <align_context_tpl>	m_reader{};
	std::mutex									m_cout_mutex{};
	dispatch_queue_t							m_loading_queue;
	dispatch_queue_t							m_aligning_queue;
	asm_lsw::vector_source						m_vs;
	unsigned short								m_k;
	reporting_style								m_reporting_style;
	
protected:
	static int open_file(char const *fname)
	{
		int fd(open(fname, O_RDONLY | O_SHLOCK));
		if (-1 == fd)
			handle_error();
		return fd;
	}

	void cleanup() { delete this; }

public:
	// hardware_concurrency could be a good hint for the number or required buffers.
	align_context_tpl(
		dispatch_queue_t loading_queue,
		dispatch_queue_t aligning_queue,
		unsigned short const k,
		reporting_style const rs,
		bool single_thread
	):
		m_loading_queue(loading_queue),
		m_aligning_queue(aligning_queue),
		m_vs(single_thread ? 1 : std::thread::hardware_concurrency(), true),
		m_k(k),
		m_reporting_style(rs)
	{
		dispatch_retain(m_loading_queue);
		dispatch_retain(m_aligning_queue);
	}
	
	~align_context_tpl()
	{
		dispatch_release(m_loading_queue);
		dispatch_release(m_aligning_queue);
	}
	
	virtual void load_and_align(char const *source_fname_c, char const *cst_fname_c) override
	{
		assert(cst_fname_c);
		
		std::string cst_fname(cst_fname_c);
		std::string source_fname;
		if (source_fname_c)
			source_fname = std::string(source_fname_c);
		
		auto load_ds_fn = [this, cst_fname = std::move(cst_fname)](){
			int const fd(open_file(cst_fname.c_str()));
			source_stream_type ds_stream(fd, ios::close_handle);
			
			// Load the CST.
			std::cerr << "Loading the CST…" << std::endl;
			m_cst.load(ds_stream);
			
			// Load the other data structures.
			std::cerr << "Loading other data structures…" << std::endl;
			kn_matcher_type tmp_matcher(m_cst, false);
			tmp_matcher.load(ds_stream);
			m_matcher = std::move(tmp_matcher);
			
			std::cerr << "Loading complete." << std::endl;
			loading_complete();
		};
		
		auto read_sequences_fn = [this, source_fname_c, source_fname = std::move(source_fname)](){
			if (nullptr == source_fname_c)
				m_reader.read_from_stream(std::cin, m_vs, *this);
			else
			{
				int const fd(open_file(source_fname.c_str()));
				source_stream_type source_stream(fd, ios::close_handle);
				m_reader.read_from_stream(source_stream, m_vs, *this);
			}
		};
		
		asm_lsw::dispatch_async_fn(m_loading_queue, std::move(read_sequences_fn));
		
		// Prevent aligning blocks from being executed before CST has been read.
		asm_lsw::dispatch_barrier_async_fn(m_aligning_queue, std::move(load_ds_fn));
	}
	
	// Reader callbacks.
	void handle_sequence(
		std::string const &identifier,
		std::unique_ptr <std::vector <char>> &seq,
		asm_lsw::vector_source &vs
	)
	{
		assert(&vs == &m_vs);
		auto *seq_ptr(seq.release());
		
		auto find_approximate_fn = [this, identifier, seq_ptr](){
			std::unique_ptr <std::vector <char>> seq(seq_ptr);
			
			kn_matcher_type::csa_ranges ranges;
			m_matcher.find_approximate <t_report_all>(*seq, m_k, ranges);
			m_vs.put_vector(seq);
			
			asm_lsw::util::post_process_ranges(ranges);
			
			std::stringstream output;
			output << "Sequence identifier: " << identifier << "\n";
			
			switch (m_reporting_style)
			{
				case reporting_style::csa_ranges:
				{
					output << "Ranges:" << '\n';
					for (auto const &k : ranges)
						output << "\t(" << +k.first << ", " << +k.second << ")\n";
					
					break;
				}
					
				case reporting_style::text_positions:
				{
					auto const &isa(m_cst.csa.isa);
					
					output << "Text positions:" << '\n';
					for (auto const &k : ranges)
					{
						for (cst_type::csa_type::size_type i(k.first); i <= k.second; ++i)
							output << '\t' << +isa[i] << '\n';
					}
					
					break;
				}
					
				default:
					assert(0);
					break;
			}
			
			std::lock_guard <std::mutex> guard(m_cout_mutex);
			std::cout << output.str();
			std::cout.flush();
		};
		
		//std::cerr << "Dispatching align block" << std::endl;
		asm_lsw::dispatch_async_fn(m_aligning_queue, find_approximate_fn);
	}
	
	void finish()
	{
		//std::cerr << "Finish called" << std::endl;
		
		// All sequence handling blocks have been executed since m_loading_queue
		// is (supposed to be) serial.
		// Make sure that all aligning blocks have been executed before calling exit.
		auto exit_fn = [this](){
			//std::cerr << "Calling cleanup" << std::endl;
			cleanup();
			//std::cerr << "Calling exit" << std::endl;
			exit(EXIT_SUCCESS);
		};
		
		//std::cerr << "Dispatching finish block" << std::endl;
		asm_lsw::dispatch_barrier_async_fn(m_aligning_queue, std::move(exit_fn));
	}
};


void align(
	char const *source_fname,
	char const *cst_fname,
	short const k,
	reporting_style const rs,
	bool const report_all,
	bool const single_thread
)
{
	// dispatch_main calls pthread_exit, so the supporting data structures need to be
	// stored somewhere else than the stack.
	assert(0 <= k);
	assert(cst_fname);
	
	dispatch_queue_t loading_queue(dispatch_get_main_queue());
	dispatch_queue_t aligning_queue(loading_queue);
	if (!single_thread)
		aligning_queue = dispatch_queue_create("fi.iki.tsnorri.asm_lsw_aligning_queue", DISPATCH_QUEUE_CONCURRENT);
	
	// align_context(_tpl) is deallocated by itself by calling cleanup() (in finish()).
	align_context *ctx(nullptr);
	
	if (report_all)
		ctx = new align_context_tpl <true>(loading_queue, aligning_queue, k, rs, single_thread);
	else
		ctx = new align_context_tpl <false>(loading_queue, aligning_queue, k, rs, single_thread);
	
	if (!single_thread)
		dispatch_release(aligning_queue);
	
	ctx->load_and_align(source_fname, cst_fname);
	
	// Calls pthread_exit.
	dispatch_main();
}
