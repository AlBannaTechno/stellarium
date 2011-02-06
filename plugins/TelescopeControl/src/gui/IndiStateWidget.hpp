/*
 * Qt-based INDI wire protocol client
 * 
 * Copyright (C) 2011 Bogdan Marinov
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

#ifndef _INDI_STATE_WIDGET_HPP_
#define _INDI_STATE_WIDGET_HPP_

#include <QLabel>
#include <QHBoxLayout>
#include <QObject>
#include <QWidget>

#include "IndiTypes.hpp"

//! A widget to display one of the INDI states (Idle, Busy, OK, Alert).
//! Used as a representation of a Light in IndiLightPropertyWidget and
//! as a state indicator in all other child classes of IndiPropertyWidget.
//! \todo Add a graphical component, like in KStars. Static QPixMaps?
class IndiStateWidget : public QWidget
{
	Q_OBJECT

public:
	IndiStateWidget(State intialState, QWidget* parent = 0);

	void setState(State newState);

private:
	State state;
	QHBoxLayout* layout;
	QLabel* label;
};

#endif//_INDI_STATE_WIDGET_HPP_

