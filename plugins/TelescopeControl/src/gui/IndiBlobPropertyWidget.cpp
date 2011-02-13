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

#include "IndiBlobPropertyWidget.hpp"
#include <QFile>

IndiBlobPropertyWidget::IndiBlobPropertyWidget(BlobProperty *property,
                                               const QString& title,
                                               QWidget* parent)
	: IndiPropertyWidget(title, parent)
{
	Q_ASSERT(property);

	propertyName = property->getName();
	setGroup(property->getGroup());

	mainLayout = new QHBoxLayout();
	mainLayout->setContentsMargins(0, 0, 0, 0);
	mainLayout->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);

	//State
	stateWidget = new IndiStateWidget(property->getCurrentState());
	mainLayout->addWidget(stateWidget);

	//TODO

	this->setLayout(mainLayout);
}

void IndiBlobPropertyWidget::updateProperty(Property* property)
{
	BlobProperty* blobProperty = dynamic_cast<BlobProperty*>(property);
	if (blobProperty)
	{
		//State
		State newState = blobProperty->getCurrentState();
		stateWidget->setState(newState);

		//This is rather poorly thought-out. Though I doubt that it will often
		//encounter vectors of multiple BLOBs.
		//TODO: Remember format/fileaname extension so you don't have to
		//generate them every time.
		QDateTime timestamp = blobProperty->getTimestamp();
		QString timestampString = timestamp.toString(Qt::ISODate);
		QStringList elementNames = blobProperty->getElementNames();
		foreach (const QString& elementName, elementNames)
		{
			BlobElement* element = blobProperty->getElement(elementName);
			if (element->getSize() == 0)
				continue;
			//TODO: Temporary
			QString filename = QString("~/Desktop/blob_%1.%2.%3")
				.arg(element->getName(), timestampString, element->getFormat());
			QFile blobFile(filename);
			if (blobFile.open(QFile::WriteOnly))
			{
				blobFile.write(element->getValue());
				blobFile.close();
			}
		}
	}
}
