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

#include "IndiSwitchPropertyWidget.hpp"

#include <QCheckBox>
#include <QPushButton>
#include <QRadioButton>

IndiSwitchPropertyWidget::IndiSwitchPropertyWidget(const SwitchPropertyP& property,
                                                   const QString& title,
                                                   QWidget* parent)
	: IndiPropertyWidget(property, title, parent),
	property(property),
	signalMapper(0)
{
	Q_ASSERT(property);
	switchRule = property->getSwitchRule();

	buttonsLayout = new QVBoxLayout();
	buttonsLayout->setContentsMargins(0, 0, 0, 0);
	buttonsLayout->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);
	mainLayout->addLayout(buttonsLayout);

	signalMapper = new QSignalMapper(this);

	QStringList elementNames = property->getElementNames();
	foreach (const QString& elementName, elementNames)
	{
		SwitchElement* element = property->getElement(elementName);
		QString label = element->getLabel();
		QAbstractButton* button;
		if (switchRule == SwitchOnlyOne)
		{
			button = new QRadioButton(label);
		}
		else if (switchRule == SwitchAny)
		{
			button = new QCheckBox(label);
		}
		else //switchRule == SwitchAtMostOne
		{
			button = new QPushButton(label);
			button->setCheckable(true);
			//Note: this requires changes to the stylesheet to display properly
			//checked buttons.
		}

		bool checked = element->isOn();
		button->setChecked(checked);

		if (property->isWritable())
		{
			if (switchRule == SwitchAtMostOne)
			{
				connect(button, SIGNAL(clicked()),
				        signalMapper, SLOT(map()));
				signalMapper->setMapping(button, elementName);
			}
			else
			{
				connect(button, SIGNAL(clicked()),
				        this, SLOT(setNewPropertyValue()));
			}
		}
		else
		{
			button->setDisabled(true);
		}
		buttonsLayout->addWidget(button);
		buttons.insert(elementName, button);
	}

	if (switchRule == SwitchAtMostOne)
	{
		connect(signalMapper, SIGNAL(mapped(QString)),
		        this, SLOT(handleClickedButton(QString)));
	}
}

IndiSwitchPropertyWidget::~IndiSwitchPropertyWidget()
{
	//
}

void IndiSwitchPropertyWidget::updateFromProperty()
{
	if (property.isNull())
		return;
	
	//State
	State newState = property->getCurrentState();
	stateWidget->setState(newState);
	
	QStringList elementNames = property->getElementNames();
	foreach (const QString& elementName, elementNames)
	{
		if (buttons.contains(elementName))
		{
			SwitchElement* element = property->getElement(elementName);
			bool value = element->isOn();
			buttons[elementName]->setChecked(value);;
		}
	}
}

void IndiSwitchPropertyWidget::setNewPropertyValue()
{
	QVariantHash elements;
	QHashIterator<QString,QAbstractButton*> i(buttons);
	while (i.hasNext())
	{
		i.next();
		bool value = i.value()->isChecked();
		elements.insert(i.key(), value);
	}
	emit newPropertyValue(propertyName, elements);
}

void IndiSwitchPropertyWidget::handleClickedButton(const QString& buttonId)
{
	if (buttonId.isEmpty() || !buttons.contains(buttonId))
		return;

	//If the button has been checked, uncheck everything else.
	if (buttons[buttonId]->isChecked())
	{
		QHashIterator<QString,QAbstractButton*> i(buttons);
		while (i.hasNext())
		{
			i.next();
			if (i.key() != buttonId && i.value()->isChecked())
			{
				i.value()->setChecked(false);
			}
		}
	}

	setNewPropertyValue();
}
