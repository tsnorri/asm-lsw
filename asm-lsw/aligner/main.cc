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


#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/stream.hpp>
#include <cassert>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sys/errno.h>
#include "aligner.hh"
#include "cmdline.h"


namespace chrono = std::chrono;
namespace ios = boost::iostreams;


static bool s_in_align_mode{false};
static chrono::milliseconds s_start_timestamp{};
static chrono::milliseconds s_loading_complete_timestamp{};


chrono::milliseconds timestamp_ms_now()
{
	return chrono::duration_cast <chrono::milliseconds>(chrono::system_clock::now().time_since_epoch());
}


void handle_error()
{
	char const *errmsg(strerror(errno));
	std::cerr << "Unable to create temporary file: " << errmsg << std::endl;
	exit(EXIT_FAILURE);
}


void loading_complete()
{
	s_loading_complete_timestamp = timestamp_ms_now();
}


void handle_atexit()
{
	chrono::milliseconds end_timestamp(timestamp_ms_now());
	if (s_in_align_mode)
	{
		std::cerr
			<< "Milliseconds elapsed: "
			<< (end_timestamp - s_start_timestamp).count()
			<< " (loading: " << (s_loading_complete_timestamp - s_start_timestamp).count()
			<< " aligning and post-processing: " << (end_timestamp - s_loading_complete_timestamp).count()
			<< ")" << std::endl;
	}
	else
	{
		std::cerr << "Milliseconds elapsed: " << (end_timestamp - s_start_timestamp).count() << std::endl;
	}
}


int main(int argc, char **argv)
{
	s_start_timestamp = timestamp_ms_now();
	std::atexit(handle_atexit);
	
	std::cerr << "Warning: --mismatches has not been implemented." << std::endl;
	
	gengetopt_args_info args_info;
	if (0 != cmdline_parser(argc, argv, &args_info))
		exit(EXIT_FAILURE);
	
	if (args_info.create_index_given)
	{
		if (args_info.source_file_given)
		{
			int fd(open(args_info.source_file_arg, O_RDONLY | O_SHLOCK));
			if (-1 == fd)
				handle_error();
			ios::stream <ios::file_descriptor_source> source_stream(fd, ios::close_handle);
			create_index(source_stream);
		}
		else
		{
			create_index(std::cin);
		}
	}
	else if (args_info.align_given)
	{
		s_in_align_mode = true;
		align(
			args_info.source_file_given ? args_info.source_file_arg : nullptr,
			args_info.index_file_arg,
			args_info.error_count_arg,
			(args_info.report_csa_ranges_given ? reporting_style::csa_ranges : reporting_style::text_positions),
			args_info.report_all_given,
			args_info.no_mt_given
		);
	}
	else
	{
		std::cerr << "Error: No mode given." << std::endl;
		exit(EXIT_FAILURE);
	}
	
	return EXIT_SUCCESS;
}
