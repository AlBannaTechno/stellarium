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

#ifndef _INDI_TEXT_PROPERTY_WIDGET_HPP_
#define _INDI_TEXT_PROPERTY_WIDGET_HPP_

#include <QObject>
#include <QPushButton>
#include <QVBoxLayout>

#include "IndiPropertyWidget.hpp"

//! Widget representing a Text property in the control panel window.
//! A Text property has an indicator displaying its current state
//! (IndiStateWidget) and a number of other controls depending on permissions.
//! \todo Write a proper description when not sleepy.
//! \author Bogdan Marinov
class IndiTextPropertyWidget : public IndiPropertyWidget
{
	Q_OBJECT

public:
	IndiTextPropertyWidget(TextProperty* property,
	                       const QString& title,
	                       QWidget* parent = 0);
	~IndiTextPropertyWidget();

	//Slot implementation:
	void updateProperty(Property* property);

public slots:
	//! Called by #setButton.
	//! Reads the current contents of the user-editable fields and emits
	//! newPropertyValue().
	void setNewPropertyValue();

private:
	//! Button for setting a new value for the property.
	//! Unused if the property is read-only.
	QPushButton* setButton;
	//! Layout for the "device values" column.
	QVBoxLayout* deviceColumnLayout;
	//! Layout for the "user values" column.
	QVBoxLayout* userColumnLayout;
};

#endif//_INDI_TEXT_PROPERTY_WIDGET_HPP_
