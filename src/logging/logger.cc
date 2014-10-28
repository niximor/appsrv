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
 */

#include <gcm/logging/field.h>
#include <gcm/logging/handler.h>
#include <gcm/logging/logger.h>

using namespace gcm::logging;

field::Field::~Field() {}
Handler::~Handler() {}

Logger Logger::root;

Message::~Message() {
	if (!moved) {
		logger.write(*this);
	}
}

void field::Name::format(Msg &msg, std::ostream &stream) {
	stream << msg.get_logger().get_name();
}
