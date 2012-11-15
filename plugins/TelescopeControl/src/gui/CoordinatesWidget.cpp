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

#include <QButtonGroup>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QRadioButton>
#include <QVBoxLayout>

#include "CoordinatesWidget.hpp"

#include "AngleSpinBox.hpp"
#include "StelTranslator.hpp"

CoordinatesWidget::CoordinatesWidget(QWidget *parent) :
    QGroupBox(parent)
{
	// Main widget layout
	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	
	// Coordinate controls group
	raLabel = new QLabel();
	decLabel = new QLabel();
	raSpinBox = new AngleSpinBox();
	decSpinBox = new AngleSpinBox();
	raLabel->setBuddy(raSpinBox);
	decLabel->setBuddy(decSpinBox);
	currentButton = new QPushButton();
	currentButton->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
	QGridLayout* coordLayout = new QGridLayout();
	layout->addLayout(coordLayout);
	coordLayout->setContentsMargins(0, 0, 0, 0);
	coordLayout->addWidget(raLabel, 0, 0, 1, 1);
	coordLayout->addWidget(raSpinBox, 0, 1, 1, 1);
	coordLayout->addWidget(decLabel, 1, 0, 1, 1);
	coordLayout->addWidget(decSpinBox, 1, 1, 1, 1);
	coordLayout->addWidget(currentButton, 0, 2, 2, 2);
	coordLayout->setColumnStretch(1, 2);
	
	// Format buttons group
	hmsButton = new QRadioButton();
	dmsButton = new QRadioButton();
	decimalButton = new QRadioButton();
	formatButtons = new QButtonGroup(this);
	formatButtons->addButton(hmsButton, 0);
	formatButtons->addButton(dmsButton, 1);
	formatButtons->addButton(decimalButton, 2);
	connect(formatButtons, SIGNAL(buttonClicked(int)),
	        this, SLOT(setFormat(int)));
	QHBoxLayout* formatLayout = new QHBoxLayout();
	formatLayout->addWidget(hmsButton);
	formatLayout->addWidget(dmsButton);
	formatLayout->addWidget(decimalButton);
	layout->addLayout(formatLayout);
	
	// Large "slew" button group
	slewButton = new QPushButton();
	slewButton->setMinimumSize(QSize(50, 48));
	buttonLayout = new QGridLayout();
	buttonLayout->addWidget(slewButton, 0, 0);
	layout->addLayout(buttonLayout);
	
	// Init control strings
	retranslate();
	
	// Coordinatess are in HMS by default
	hmsButton->setChecked(true);
	setFormat(0);
}

void CoordinatesWidget::retranslate()
{
	// TODO: Change title to something like "coordinates"
	// TODO: Decide whether to use mnemonics
	setTitle(q_("Slew telescope to coordinates"));
	
	raLabel->setText(q_("&Right Ascension (J2000):"));
	decLabel->setText(q_("De&clination (J2000):"));
	
	hmsButton->setText(q_("&HMS"));
	hmsButton->setToolTip(q_("Hours-minutes-seconds format"));
	dmsButton->setText(q_("&DMS"));
	dmsButton->setToolTip(q_("Degrees-minutes-seconds format"));
	decimalButton->setText(q_("D&ecimal"));
	decimalButton->setToolTip(q_("Decimal degrees"));
	
	slewButton->setText(q_("&Slew"));
	currentButton->setText(q_("&Current"));
	currentButton->setToolTip(q_("Use the telescope's current coordinates"));
}

void CoordinatesWidget::setFormat(int buttonId)
{
	switch(buttonId)
	{
	case 1:
		raSpinBox->setDisplayFormat(AngleSpinBox::DMSLetters);
		decSpinBox->setDisplayFormat(AngleSpinBox::DMSLetters);
		break;
	case 2:
		raSpinBox->setDisplayFormat(AngleSpinBox::DecimalDeg);
		decSpinBox->setDisplayFormat(AngleSpinBox::DecimalDeg);
		break;
	case 0:
	default:
		raSpinBox->setDisplayFormat(AngleSpinBox::HMSLetters);
		decSpinBox->setDisplayFormat(AngleSpinBox::DMSLetters);
	}
}
