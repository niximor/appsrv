/**
 * Copyright 2014 Michal Kuchta <niximor@gmail.com>
 *
 * This file is part of GCM::AppSrv.
 *
 * GCM::AppSrv is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * GCM::AppSrv is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with GCM::AppSrv. If not, see http://www.gnu.org/licenses/.
 *
 * @author Michal Kuchta <niximor@gmail.com>
 * @date 2014-10-28
 *
 * This code uses AXE parser library.
 * You can download it from http://www.gbresearch.com/axe/
 */

#include <iostream>
#include <fstream>
#include <string>

#include <gcm/parser/parser.h>

class Config {
public:
	Config(const std::string &file) {
		/*std::ifstream f(file, std::ios_base::in);
		std::string contents((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
		parse(contents.begin(), contents.end());*/
		parse(file.begin(), file.end());
	}

private:
	template<typename I>
	void got_string(I begin, I end) {
		std::cout << "Captured string: " << std::string(begin, end) << std::endl;
	}

	template<typename I>
	void debug(const std::string &desc, I begin, I end) {
		std::cout << "Captured " << desc << ": " << std::string(begin, end) << std::endl;
	}

	template<typename I>
	void parse(I begin, I end) {
		using namespace std::placeholders;
		using namespace gcm::parser;

		auto line_comment = (char_rule('#') | "//") & many(any_rule() - '\n');
		auto block_comment = "/*" & many(any_rule() - "*/") & "*/";
		auto comment = line_comment | block_comment;

		auto space = *(comment | gcm::parser::space());
		auto identifier = (alpha() & many(alnum())) >> std::bind(&Config::debug<I>, this, "identifier", _1, _2);

		rule<I> value;
		auto item = (identifier & space & '=' & space & value & space & ';') >> std::bind(&Config::debug<I>, this, "item", _1, _2);

		auto v_string = ('"' & many(any_rule() - '"') & '"') >> std::bind(&Config::debug<I>, this, "v_string", _1, _2);
		auto v_int = (-(char_rule('+') | '-') & +digit()) >> std::bind(&Config::debug<I>, this, "v_int", _1, _2);
		auto v_double = (-(char_rule('+') | '-') & ((+digit() & -('.' & many(digit()))) | (many(digit()) & '.' & +digit()))) >> std::bind(&Config::debug<I>, this, "v_double", _1, _2);
		auto v_array = ('[' & space & many(value & ',' & space) & -(value) & space & ']') >> std::bind(&Config::debug<I>, this, "v_array", _1, _2);
		auto v_struct = ('{' & space & many(item & space) & space & '}') >> std::bind(&Config::debug<I>, this, "v_struct", _1, _2);

		auto v_bool = literal_rule("true") | "false";
		auto v_null = literal_rule("null");

		value = (v_string | v_double | v_int | v_array | v_struct | v_bool | v_null) >> std::bind(&Config::debug<I>, this, "value", _1, _2);

		auto &&parser = space & many(item & space) >> std::bind(&Config::debug<I>, this, "parser", _1, _2);

		if (parser(begin, end) && begin == end) {
			std::cout << "Success" << std::endl;
		} else {
			std::cout << "Failed." << std::endl;
		}
	}
};
