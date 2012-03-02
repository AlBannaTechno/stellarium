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

#ifndef _INDI_PROPERTY_WIDGET_HPP_
#define _INDI_PROPERTY_WIDGET_HPP_

#include <QGroupBox>
#include <QObject>
#include <QVariant>
#include <QHBoxLayout>
#include <QWidget>

#include "IndiProperty.hpp"
#include "IndiStateWidget.hpp"

//! Abstract base class for property widgets in the control panel window.
//! Each property vector is represented as a QGroupBox.
class IndiPropertyWidget : public QGroupBox
{
	Q_OBJECT
public:
	IndiPropertyWidget(const PropertyP& property,
	                   const QString& title,
	                   QWidget* parent = 0)
	    : QGroupBox(title, parent)
	{
		Q_ASSERT(property);
		propertyName = property->getName();
		QString newGroupName = property->getGroup();
		if (newGroupName.isEmpty())
			groupName = "Main";//TODO: Default name.
		else
			groupName = newGroupName;
		
		mainLayout = new QHBoxLayout();
		mainLayout->setContentsMargins(0, 0, 0, 0);
		mainLayout->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);
		
		stateWidget = new IndiStateWidget(property->getCurrentState());
		mainLayout->addWidget(stateWidget);
		
		setLayout(mainLayout);
	}
	//virtual ~IndiPropertyWidget() = 0;

	QString getGroup() const {return groupName;}

public slots:
	//What calls this?
	virtual void updateProperty(const PropertyP& property) = 0;

signals:
	void newPropertyValue(const QString& property, const QVariantHash& elements);

protected:
	QString propertyName;
	QString groupName;
	QHBoxLayout* mainLayout;
	IndiStateWidget* stateWidget;
};

#endif//_INDI_PROPERTY_WIDGET_HPP_
