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

#include <gcm/parser/rules.h>

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
	void parse(I begin, I end) {
		namespace r = gcm::parser::rule;
		r::rule<I> str = ('"' & *(r::all<I>() - '"') & '"') >> std::bind(&Config::got_string<I>, this, std::placeholders::_1, std::placeholders::_2);

		if (str(begin, end)) {
			std::cout << "Success" << std::endl;
		} else {
			std::cout << "Failed." << std::endl;
		}
	}
};
