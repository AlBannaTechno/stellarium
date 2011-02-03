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

#ifndef _INDI_BLOG_PROPERTY_WIDGET_HPP_
#define _INDI_BLOG_PROPERTY_WIDGET_HPP_

#include <QObject>

#include "IndiPropertyWidget.hpp"

#include "Indi.hpp"

class IndiBlobPropertyWidget : public IndiPropertyWidget
{
	Q_OBJECT

public:
	IndiBlobPropertyWidget(BlobProperty* property,
	                       const QString& title,
	                       QWidget* parent = 0);

//Slots:
	void updateProperty(Property* property);

//Signals: no need to re-declare them.

private:

};

#endif//_INDI_BLOG_PROPERTY_WIDGET_HPP_
