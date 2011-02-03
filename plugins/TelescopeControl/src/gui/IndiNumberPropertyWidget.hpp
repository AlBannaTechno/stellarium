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

#ifndef _INDI_NUMBER_PROPERTY_WIDGET_HPP_
#define _INDI_NUMBER_PROPERTY_WIDGET_HPP_

#include <QObject>
#include <QGridLayout>
#include <QLineEdit>
#include <QPushButton>

#include "IndiPropertyWidget.hpp"

//! Widget representing a Number property in the control panel window.
//! It contains an indicator displaying its current state
//! (IndiStateWidget), a column of labels of the sub-properties, and:
//!  - a column of read-only QLineEdit-s if the property can be read;
//!  - a column of QLineEdits and a "set" button if the property can be written.
//! \todo Write a proper description when not sleepy.
//! \author Bogdan Marinov
class IndiNumberPropertyWidget : public IndiPropertyWidget
{
	Q_OBJECT

public:
	IndiNumberPropertyWidget(NumberProperty* property,
	                         const QString& title,
	                         QWidget* parent = 0);
	~IndiNumberPropertyWidget();

	//Slot implementation:
	void updateProperty(Property* property);

private slots:
	//! Called by #setButton.
	//! Reads the current contents of the user-editable fields and emits
	//! newPropertyValue().
	void setNewPropertyValue();

private:
	//! Button for setting a new value for the property.
	//! Unused if the property is read-only.
	QPushButton* setButton;
	//! Layout for the display and input widgets.
	QGridLayout* gridLayout;
	//! Widgets to display the device values (updated by updateProperty()).
	QHash<QString,QLineEdit*> displayWidgets;
	//! Widgets to allow the user to enter new values.
	QHash<QString,QLineEdit*> inputWidgets;
};

#endif//_INDI_NUMBER_PROPERTY_WIDGET_HPP_
