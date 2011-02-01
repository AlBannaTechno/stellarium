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

#include "IndiTextPropertyWidget.hpp"
#include "IndiNumberPropertyWidget.hpp"
#include "IndiSwitchPropertyWidget.hpp"
#include "IndiLightPropertyWidget.hpp"
#include "IndiBlobPropertyWidget.hpp"

IndiDeviceWidget::IndiDeviceWidget(QWidget* parent)
	: QWidget(parent)
{
	groupsTabWidget = new QTabWidget();
	groupsTabWidget->setSizePolicy(QSizePolicy::Expanding,
	                               QSizePolicy::Expanding);

	QVBoxLayout* layout = new QVBoxLayout();
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);
	layout->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
	layout->addWidget(groupsTabWidget);
	this->setLayout(layout);
}

void IndiDeviceWidget::defineProperty(Property* property)
{
	QString name = property->getName();
	QString label = property->getLabel();
	//TODO: Handle duplicate names.

	//TODO: Exception handling?
	//TODO: Isn't the label thing redundant?
	IndiPropertyWidget* propertyWidget;
	Property::PropertyType type = property->getType();
	switch (type)
	{
		case Property::TextProperty:
		{
			TextProperty* tp = dynamic_cast<TextProperty*>(property);
			if (tp)
			{
				propertyWidget = new IndiTextPropertyWidget(tp, label);
				break;
			}
			else
				return;
		}
		case Property::NumberProperty:
		{
			NumberProperty* np = dynamic_cast<NumberProperty*>(property);
			if (np)
			{
				propertyWidget = new IndiNumberPropertyWidget(np, label);
				break;
			}
			else
				return;
		}
		case Property::SwitchProperty:
		{
			SwitchProperty* sp = dynamic_cast<SwitchProperty*>(property);
			if (sp)
			{
				propertyWidget = new IndiSwitchPropertyWidget(sp, label);
				break;
			}
			else
				return;
		}
		case Property::LightProperty:
		{
			LightProperty* lp = dynamic_cast<LightProperty*>(property);
			if (lp)
			{
				propertyWidget = new IndiLightPropertyWidget(lp, label);
				break;
			}
			else
				return;
		}
		case Property::BlobProperty:
		{
			BlobProperty* bp = dynamic_cast<BlobProperty*>(property);
			if (bp)
			{
				propertyWidget = new IndiBlobPropertyWidget(bp, label);
				break;
			}
			else
				return;
		}
		default:
			return;
	}
	//TODO: Connect signals/slots
	propertyWidgets.insert(name, propertyWidget);

	//TODO: Connect signals/slots

	QString group = property->getGroup();
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
		IndiPropertyWidget* widget = propertyWidgets.take(propertyName);
		QString group = widget->getGroup();
		if (groupWidgets.contains(group))
		{
			IndiGroupWidget* groupWidget = groupWidgets[group];
			groupWidget->removePropertyWidget(widget);
			if (groupWidget->propertyWidgetsCount() == 0)
			{
				int index = groupsTabWidget->indexOf(groupWidget);
				groupsTabWidget->removeTab(index);
				delete groupWidget;
			}
		}
		//TODO: Disconnect connected signals
		delete widget;
	}
}
