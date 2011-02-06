/*
 * Qt-based INDI wire protocol client
 * 
 * Copyright (C) 2010-2011 Bogdan Marinov
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _INDI_HPP_
#define _INDI_HPP_

//! Possible property permissions according to the INDI protocol specification.
//! Lights are assumed to be always read-only, Switches can be read-only or
//! read-write.
enum Permission {
	PermissionReadOnly,
	PermissionWriteOnly,
	PermissionReadWrite
};

//! Possible property states according to the INDI protocol specification.
enum State {
	StateIdle,
	StateOk,
	StateBusy,
	StateAlert
};

//! Rules governing the behaviour of vectors (arrays) of switches 
enum SwitchRule {
	SwitchOnlyOne,
	SwitchAtMostOne,
	SwitchAny
};

#endif//_INDI_HPP_
