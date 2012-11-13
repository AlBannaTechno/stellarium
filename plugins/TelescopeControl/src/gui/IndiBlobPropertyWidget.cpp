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
#include <QDebug>
#include <QDir>
#include <QLineEdit>

IndiBlobPropertyWidget::IndiBlobPropertyWidget(const BlobPropertyP& property,
                                               const QString& title,
                                               QWidget* parent)
	: IndiPropertyWidget(property, title, parent),
	property(property),
    lineEditFilePath(0)
{
	Q_ASSERT(property);
	
	// TODO: One path display per BLOB element, in case of a vector of BLOBs.
	lineEditFilePath = new QLineEdit();
	lineEditFilePath->setReadOnly(true);
	mainLayout->addWidget(lineEditFilePath);
}

void IndiBlobPropertyWidget::updateFromProperty()
{
	if (property.isNull())
		return;
	
	//State
	State newState = property->getCurrentState();
	stateWidget->setState(newState);
	
	//This is rather poorly thought-out. Though I doubt that it will often
	//encounter vectors of multiple BLOBs.
	//TODO: Remember format/fileaname extension so you don't have to
	//generate them every time.
	
	// The timestamp should not contain characters forbidden for file names!
	QString timestamp = property->getTimestamp().toString("yyyy-MM-ddThhmmss");
	QStringList elementNames = property->getElementNames();
	foreach (const QString& elementName, elementNames)
	{
		BlobElement* element = property->getElement(elementName);
		if (element->getSize() == 0)
			continue;
		
		// TODO: Better code for saving BLOBs (split to different places, etc).
		// TODO: Button for opening directory.
		// TODO: Button for opening FITS viewer. (QDesktopServices::openUrl()?)
		
		// Ensure we have a writable directory.
		// (Tricky on Windows because of permissions.) 
		QString dirPath = QDesktopServices::storageLocation(QDesktopServices::PicturesLocation);
		QDir dir(dirPath);
		if (!dir.exists())
		{
			qWarning() << "Unable to save BLOB:"
			           << "no access to home directory";
			return;
		}
		dirPath += "/Stellarium BLOBs/";
		if (!dir.mkpath(dirPath))
		{
			qWarning() << "Unable to save BLOB:"
			           << "failed to create"
			           << QDir::toNativeSeparators(dirPath);
			return;
		}
		dir.setPath(dirPath);
		// File name
		QString name = element->getName(); //TODO: Remove illegal filename chars
		QString format =  element->getFormat(); //TODO: Chop .z
		QString filename = QString("%1_%2%3").arg(name, timestamp, format);
		QString path = dir.absoluteFilePath(filename);
		QFile blobFile(path);
		if (blobFile.open(QFile::WriteOnly))
		{
			if (blobFile.write(element->getValue()) > 0)
			{
				qWarning() << "BLOB saved:" << path;
				lineEditFilePath->setText(path);
			}
			else
				qWarning() << "Unable to save BLOB: write failed:"
				           << path;
			blobFile.close();
		}
		else
		{
			qWarning() << "Unable to save BLOB: failed to open"
			           << path << "for writing.";
		}
	}
}
