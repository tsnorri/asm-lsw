/*
 Copyright (c) 2015-2016 Tuukka Norri
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


#ifndef ASM_LSW_EXCEPTION_TPL_HH
#define ASM_LSW_EXCEPTION_TPL_HH

#include <cassert>
#include <cstdint>
#include <string>
#include <stdexcept>


namespace asm_lsw {

	class exception : std::exception
	{
	protected:
		std::string m_what;

	public:
		explicit exception(char const *what):
			exception(std::string(what))
		{
		}

		explicit exception(std::string const &what):
			m_what(what)
		{
		}

		virtual char const *what() const noexcept override
		{
			return m_what.c_str();
		}		
	};


	class exception_base
	{
	protected:
		char const *m_file;
		uint32_t m_line;
		uint32_t m_error;

	public:
		explicit exception_base(char const *file, uint32_t line, uint32_t error):
			m_file(file),
			m_line(line),
			m_error(error)
		{
		}

		char const *file() const noexcept { return m_file; }
		uint32_t line() const noexcept { return m_line; }
		uint32_t error() const noexcept { return m_error; }

		template <typename t_enum>
		t_enum error() const noexcept { return static_cast <t_enum>(m_error); }
	};


	template <typename t_base_class>
	class exception_tpl : public t_base_class, public exception_base
	{
	public:
		explicit exception_tpl(char const *what, uint32_t const error, char const *file, uint32_t const line):
			t_base_class(what),
			exception_base(file, line, error)
		{
		}

		explicit exception_tpl(std::string const &what, uint32_t const error, char const *file, uint32_t const line):
			t_base_class(what),
			exception_base(file, line, error)
		{
		}
	};


	template <typename t_base>
	exception_tpl <t_base> make_custom_exception(char const *file, uint32_t const line, uint32_t const error, char const *what)
	{
		return exception_tpl <t_base>(what, error, file, line);
	}


#	if (defined(ASM_LSW_EXCEPTIONS) && 1 == ASM_LSW_EXCEPTIONS)
#		define asm_lsw_assert_m(expr, exc, error_code, message) ((expr) ?  \
			static_cast <void>(0) : \
			throw asm_lsw::make_custom_exception <exc>(__FILE__, __LINE__, static_cast <typename std::underlying_type <decltype(error_code)>::type>(error_code), message))
#	else
#		define asm_lsw_assert_m(expr, exc, error_code, message) (assert(expr))
#	endif
#	define asm_lsw_assert(expr, exc, error_code) (asm_lsw_assert_m(expr, exc, error_code, #error_code))


	typedef exception_tpl <std::invalid_argument> invalid_argument;
}

#endif
