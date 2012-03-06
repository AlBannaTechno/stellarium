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

#include "IndiTextPropertyWidget.hpp"

IndiTextPropertyWidget::IndiTextPropertyWidget(const TextPropertyP& property,
                                               const QString& title,
                                               QWidget* parent)
	: IndiPropertyWidget(property, title, parent),
	setButton(0),
	gridLayout(0)
{
	Q_ASSERT(property);

	gridLayout = new QGridLayout();
	gridLayout->setContentsMargins(0, 0, 0, 0);
	QStringList elementNames = property->getElementNames();
	int row = 0;
	foreach (const QString& elementName, elementNames)
	{
		TextElement* element = property->getElement(elementName);
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
			lineEdit->setText(element->getValue());
			displayWidgets.insert(elementName, lineEdit);
			gridLayout->addWidget(lineEdit, row, column, 1, 1);
		}

		if (property->isWritable())
		{
			column++;
			QLineEdit* lineEdit = new QLineEdit();
			//TODO: Some kind of validation?
			lineEdit->setAlignment(Qt::AlignRight);
			lineEdit->setText(element->getValue());//Only for init!
			displayWidgets.insert(elementName, lineEdit);
			gridLayout->addWidget(lineEdit, row, column, 1, 1);
		}

		row++;
	}
	mainLayout->addLayout(gridLayout);

	if (property->isWritable())
	{
		setButton = new QPushButton("Set");
		setButton->setSizePolicy(QSizePolicy::Preferred,
		                         QSizePolicy::Preferred);
		connect(setButton, SIGNAL(clicked()),
		        this, SLOT(setNewPropertyValue()));
		mainLayout->addWidget(setButton);
	}
}

IndiTextPropertyWidget::~IndiTextPropertyWidget()
{
	//TODO
}

void IndiTextPropertyWidget::updateProperty(const PropertyP& property)
{
	TextPropertyP textProperty = qSharedPointerDynamicCast<TextProperty>(property);
	if (textProperty.isNull())
		return;
	
	//State
	State newState = textProperty->getCurrentState();
	stateWidget->setState(newState);
	
	if (textProperty->isReadable())
	{
		QStringList elementNames = textProperty->getElementNames();
		foreach (const QString& elementName, elementNames)
		{
			if (displayWidgets.contains(elementName))
			{
				TextElement* element = textProperty->getElement(elementName);
				QString value = element->getValue();
				displayWidgets[elementName]->setText(value);
			}
		}
	}
}

void IndiTextPropertyWidget::setNewPropertyValue()
{
	QVariantHash elements;
	QHashIterator<QString,QLineEdit*> i(inputWidgets);
	while (i.hasNext())
	{
		i.next();
		QString value = i.value()->text();
		elements.insert(i.key(), value);
	}
	emit newPropertyValue(propertyName, elements);
}
