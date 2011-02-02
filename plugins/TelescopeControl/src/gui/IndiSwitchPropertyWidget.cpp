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

#include "IndiSwitchPropertyWidget.hpp"

#include <QCheckBox>
#include <QPushButton>
#include <QRadioButton>

IndiSwitchPropertyWidget::IndiSwitchPropertyWidget(SwitchProperty* property,
                                                   const QString& title,
                                                   QWidget* parent)
	: IndiPropertyWidget(title, parent)
{
	Q_ASSERT(property);

	setGroup(property->getGroup());
	switchRule = property->getSwitchRule();

	mainLayout = new QHBoxLayout();
	mainLayout->setContentsMargins(0, 0, 0, 0);
	mainLayout->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);

	//State
	stateWidget = new IndiStateWidget(property->getCurrentState());
	mainLayout->addWidget(stateWidget);

	buttonsLayout = new QVBoxLayout();
	buttonsLayout->setContentsMargins(0, 0, 0, 0);
	buttonsLayout->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);
	mainLayout->addLayout(buttonsLayout);

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
			connect(button, SIGNAL(clicked()),
			        this, SLOT(setNewPropertyValue()));
		}
		else
		{
			button->setDisabled(true);
		}
		buttonsLayout->addWidget(button);
	}

	this->setLayout(mainLayout);
}

IndiSwitchPropertyWidget::~IndiSwitchPropertyWidget()
{
	//
}

void IndiSwitchPropertyWidget::updateProperty(Property *property)
{
	Q_UNUSED(property);
}

void IndiSwitchPropertyWidget::setNewPropertyValue()
{
	//
}
