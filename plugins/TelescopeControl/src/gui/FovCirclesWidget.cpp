/*
 * Stellarium Device Control plug-in
 * Copyright (C) 2012  Bogdan Marinov
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street,
 * Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "FovCirclesWidget.hpp"
#include "TelescopeClient.hpp"
#include "StelTranslator.hpp"

#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRegExpValidator>

FovCirclesWidget::FovCirclesWidget(TelescopeClient* devicePtr,
                                   QWidget *parent) :
    QGroupBox(parent),
    circleListLineEdit(0),
    circleListValidator(0)
{
	Q_ASSERT(devicePtr);
	device = devicePtr;
	
	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setAlignment(Qt::AlignLeft|Qt::AlignTop);
	//setLayout(layout); // Unnecessary?
	
	label = new QLabel(this); 
	layout->addWidget(label);
	
	QRegExp regExp("^\\s*(\\d{1,2}(\\.\\d{1,2})?(\\s*,\\s*|\\s*$))*\\s*$");
	circleListValidator = new QRegExpValidator (regExp, this);
	circleListLineEdit = new QLineEdit(this);
	circleListLineEdit->setValidator(circleListValidator);
	layout->addWidget(circleListLineEdit);
	
	connect(circleListLineEdit, SIGNAL(editingFinished()),
	        this, SLOT(readLineEdit()));
	
	QHBoxLayout* buttonLayout = new QHBoxLayout();
	buttonLayout->setContentsMargins(0, 0, 0, 0);
	telradButton = new QPushButton(this);
	clearButton = new QPushButton(this);
	buttonLayout->addWidget(telradButton);
	buttonLayout->addWidget(clearButton);
	layout->addLayout(buttonLayout);
	
	connect(clearButton, SIGNAL(clicked()),
	        this, SLOT(clearFovCircleSizes()));
	connect(telradButton, SIGNAL(clicked()),
	        this, SLOT(addTelradCircles()));
	
	retranslate();
	
	// Initialize FOV values
	loadFovCircleSizes();
}

void FovCirclesWidget::loadFovCircleSizes()
{
	// Load the list from the client object
	QList<double> fovValues = device->getFovCircles();
	QStringList fovStrings;
	foreach (double fov, fovValues)
	{
		fovStrings.append(QString::number(fov, 'g', 2));
	}
	fovCircleSizes = fovStrings.toSet();
	
	// Display the list in the line edit
	displayFovCircleSizes();
}

void FovCirclesWidget::displayFovCircleSizes()
{
	QStringList tempList = fovCircleSizes.toList();
	qSort(tempList);
	circleListLineEdit->setText(tempList.join(", "));
}

void FovCirclesWidget::applyFovCircleSizes()
{
	device->resetFovCircles();
	foreach (const QString& fovString, fovCircleSizes)
	{
		bool ok = false;
		double fov = fovString.toDouble(&ok);
		if (ok && fov > 0.009)
			device->addFovCircle(fov);
	}
	emit fovCirclesChanged();
}

void FovCirclesWidget::readLineEdit()
{
	//qDebug() << "FovCirclesWidget::applyFovCircleSizes()";
	QString string = circleListLineEdit->text().trimmed();
	QRegExp separator("\\s*,\\s*");
	QStringList tempList = string.split(separator, QString::SkipEmptyParts);
	QSet<QString> tempSet = tempList.toSet();
	if (tempSet != fovCircleSizes)
	{
		fovCircleSizes = tempSet;
		//qDebug() << fovCircleSizes;
		
		applyFovCircleSizes();
	}
}

void FovCirclesWidget::clearFovCircleSizes()
{
	//qDebug() << "FovCirclesWidget::clearFovCircleSizes()";
	device->resetFovCircles();
	circleListLineEdit->clear();
	fovCircleSizes.clear();
	emit fovCirclesChanged();
}

void FovCirclesWidget::addTelradCircles()
{
	fovCircleSizes.insert("0.5");
	fovCircleSizes.insert("2.0");
	fovCircleSizes.insert("4.0");
	applyFovCircleSizes();
	loadFovCircleSizes();
	displayFovCircleSizes();
}

void FovCirclesWidget::retranslate()
{
	setTitle(q_("Field of view circles"));
	
	label->setText(q_("Circle size(s) in angular degrees:"));
	circleListLineEdit->setToolTip(q_("Up to ten decimal values in degrees of arc, separated with commas"));
	telradButton->setText(q_("Telrad"));
	clearButton->setText(q_("Clear"));
}
