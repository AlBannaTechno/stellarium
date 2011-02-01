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

#include "IndiNumberPropertyWidget.hpp"

#include <stdexcept>

IndiNumberPropertyWidget::IndiNumberPropertyWidget(NumberProperty* property,
                                                   const QString& title,
                                                   QWidget* parent)
	: IndiPropertyWidget(title, parent),
	setButton(0),
	gridLayout(0)
{
	Q_ASSERT(property);

	setGroup(property->getGroup());

	mainLayout = new QHBoxLayout();
	mainLayout->setContentsMargins(0, 0, 0, 0);
	mainLayout->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);

	//State
	stateWidget = new IndiStateWidget(property->getCurrentState());
	mainLayout->addWidget(stateWidget);

	gridLayout = new QGridLayout();
	gridLayout->setContentsMargins(0, 0, 0, 0);
	QStringList elementNames = property->getElementNames();
	int row = 0;
	foreach (const QString& elementName, elementNames)
	{
		NumberElement* element = property->getElement(elementName);
		int column = 0;

		//Labels are immutable and die with the widget
		QLabel* label = new QLabel(element->getLabel());
		gridLayout->addWidget(label, row, column, 1, 1);

		if (property->isReadable())
		{
			column++;
			QLineEdit* lineEdit = new QLineEdit();
			lineEdit->setReadOnly(true);
			lineEdit->setAlignment(Qt::AlignRight);
			lineEdit->setText(element->getFormattedValue());
			displayWidgets.insert(elementName, lineEdit);
			gridLayout->addWidget(lineEdit, row, column, 1, 1);
		}

		if (property->isWritable())
		{
			column++;
			QLineEdit* lineEdit = new QLineEdit();
			//TODO: Some kind of validation?
			lineEdit->setAlignment(Qt::AlignRight);
			lineEdit->setText(element->getFormattedValue());//Only for init!
			displayWidgets.insert(elementName, lineEdit);
			gridLayout->addWidget(lineEdit, row, column, 1, 1);
		}

		row++;
	}
	mainLayout->addLayout(gridLayout);

	if (property->isWritable())
	{
		setButton = new QPushButton("Set");
		setButton->setSizePolicy(QSizePolicy::Expanding,
		                         QSizePolicy::Preferred);
		connect(setButton, SIGNAL(clicked()),
		        this, SLOT(setNewPropertyValue()));
		mainLayout->addWidget(setButton);
	}

	this->setLayout(mainLayout);
}

IndiNumberPropertyWidget::~IndiNumberPropertyWidget()
{
	//TODO
}

void IndiNumberPropertyWidget::updateProperty(Property *property)
{
	Q_UNUSED(property);
	//TODO
}

void IndiNumberPropertyWidget::setNewPropertyValue()
{
	//TODO
}
