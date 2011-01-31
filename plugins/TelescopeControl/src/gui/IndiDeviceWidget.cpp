/*
 * Stellarium Device Control plug-in
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

#include "IndiDeviceWidget.hpp"

#include <QTabWidget>
#include <QVBoxLayout>

IndiDeviceWidget::IndiDeviceWidget(QWidget* parent)
	: QWidget(parent)
{
	groupsTabWidget = new QTabWidget();
	groupsTabWidget->setSizePolicy(QSizePolicy::Expanding,
	                               QSizePolicy::Expanding);

	QVBoxLayout* layout = new QVBoxLayout();
	layout->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
	layout->addWidget(groupsTabWidget);
	this->setLayout(layout);
}

void IndiDeviceWidget::defineProperty(Property* property)
{
	QString name = property->getName();
	//TODO: Handle duplicate names.

	//TODO: Create a property widget from the necessary type
	IndiPropertyWidget* propertyWidget;
	//TODO: Connect signals/slots

	//TODO: Handle group name
	QString group;
	if (group.isEmpty())
		group = "Main";//TODO: Default name.
	if (!groupWidgets.contains(group))
	{
		IndiGroupWidget* widget = new IndiGroupWidget();
		groupsTabWidget->insertTab(0, widget, group);
		groupWidgets.insert(group, widget);
	}
	groupWidgets[group]->addPropertyWidget(propertyWidget);
}

void IndiDeviceWidget::updateProperty(Property* property)
{
	QString name = property->getName();

	if (propertyWidgets.contains(name))
	{
		propertyWidgets[name]->updateProperty(property);
	}
}

void IndiDeviceWidget::removeProperty(const QString& propertyName)
{
	if (propertyWidgets.contains(propertyName))
	{
		//TODO: Handle removing from the group widget
		IndiPropertyWidget* propertyWidget = propertyWidgets.take(propertyName);
		//TODO: Disconnect connected signals
		delete propertyWidget;
	}
}
