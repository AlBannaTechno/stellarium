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

#ifndef _INDI_GROUP_WIDGET_HPP_
#define _INDI_GROUP_WIDGET_HPP_

#include <QObject>
#include <QVBoxLayout>
#include <QWidget>

#include "IndiPropertyWidget.hpp"

//! Widget representing a named group of INDI properties in the control panel.
//! At the moment, it is a sub-tab in a IndiDeviceWidget.
//! It doesn't handle property widget updates directly - it just adds them.
//! \todo Think of a good mechanism for removing property widgets from groups.
class IndiGroupWidget : public QWidget
{
	Q_OBJECT

public:
	IndiGroupWidget(QWidget* parent = 0);
	~IndiGroupWidget();

	void addPropertyWidget(IndiPropertyWidget* widget);
	void removePropertyWidget(IndiPropertyWidget* widget);

private:
	QVBoxLayout* layout;
};

#endif//_INDI_GROUP_WIDGET_HPP_
