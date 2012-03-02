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
#include <QDesktopServices>
#include <QFile>

IndiBlobPropertyWidget::IndiBlobPropertyWidget(const BlobPropertyP& property,
                                               const QString& title,
                                               QWidget* parent)
	: IndiPropertyWidget(property, title, parent)
{
	Q_ASSERT(!property.isNull());

	//TODO
}

void IndiBlobPropertyWidget::updateProperty(const PropertyP& property)
{
	BlobPropertyP blobProperty = qSharedPointerDynamicCast<BlobProperty>(property);
	if (blobProperty.isNull())
		return;
	
	//State
	State newState = blobProperty->getCurrentState();
	stateWidget->setState(newState);
	
	//This is rather poorly thought-out. Though I doubt that it will often
	//encounter vectors of multiple BLOBs.
	//TODO: Remember format/fileaname extension so you don't have to
	//generate them every time.
	QString timestamp = blobProperty->getTimestamp().toString(Qt::ISODate);
	QStringList elementNames = blobProperty->getElementNames();
	foreach (const QString& elementName, elementNames)
	{
		BlobElement* element = blobProperty->getElement(elementName);
		if (element->getSize() == 0)
			continue;
		//TODO: Temporary
		QString desktopPath = QDesktopServices::storageLocation(QDesktopServices::DesktopLocation);
		QString name = element->getName();
		QString format =  element->getFormat();
		QString filename = QString("%1/blob_%2.%3%4")
		                   .arg(desktopPath, name, timestamp, format);
		QFile blobFile(filename);
		if (blobFile.open(QFile::WriteOnly))
		{
			blobFile.write(element->getValue());
			blobFile.close();
		}
	}
}
