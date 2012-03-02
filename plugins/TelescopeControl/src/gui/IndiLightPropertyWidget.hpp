/*
 * Device Control plug-in for Stellarium
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

#ifndef _INDI_LIGHT_PROPERTY_WIDGET_HPP_
#define _INDI_LIGHT_PROPERTY_WIDGET_HPP_

#include <QObject>
#include <QGridLayout>

#include "IndiPropertyWidget.hpp"

//! Widget representing a Light property in the control panel window.
//! A Light property has an indicator displaying its current state
//! (IndiStateWidget) and a single array of similar indicators,
//! one for each Light.
//! \todo Write a proper description when not sleepy.
//! \author Bogdan Marinov
class IndiLightPropertyWidget : public IndiPropertyWidget
{
	Q_OBJECT

public:
	IndiLightPropertyWidget(const LightPropertyP& property,
	                        const QString& title,
	                        QWidget* parent = 0);
	~IndiLightPropertyWidget();

	//Slot implementation:
	void updateProperty(const PropertyP& property);

private:
	QGridLayout* gridLayout;
	QHash<QString,IndiStateWidget*> lightsWidgets;
};

#endif//_INDI_LIGHT_PROPERTY_WIDGET_HPP_
