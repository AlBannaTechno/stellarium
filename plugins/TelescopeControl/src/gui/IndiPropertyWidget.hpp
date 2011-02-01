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

#ifndef _INDI_PROPERTY_WIDGET_HPP_
#define _INDI_PROPERTY_WIDGET_HPP_

#include <QGroupBox>
#include <QObject>
#include <QVariant>
#include <QHBoxLayout>
#include <QWidget>

#include "Indi.hpp"
#include "IndiStateWidget.hpp"

//! Abstract base class for property widgets in the control panel window.
//! Each property vector is represented as a QGroupBox.
class IndiPropertyWidget : public QGroupBox
{
	Q_OBJECT
public:
	IndiPropertyWidget(const QString& title, QWidget* parent = 0) : QGroupBox(title, parent) {;}
	//virtual ~IndiPropertyWidget() = 0;

	QString getGroup() const {return group;}
	void setGroup(const QString& groupName)
	{
		if (groupName.isEmpty())
			group = "Main";//TODO: Default name.
		else
			group = groupName;
	}

public slots:
	//What calls this?
	virtual void updateProperty(Property* property) = 0;

signals:
	void newPropertyValue(const QVariantHash& elements);

protected:
	QString group;
	QHBoxLayout* mainLayout;
	IndiStateWidget* stateWidget;
};

#endif//_INDI_PROPERTY_WIDGET_HPP_
