/*
 * Pointer Coordinates plug-in for Stellarium
 *
 * Copyright (C) 2014 Alexander Wolf
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

#include "PointerCoordinates.hpp"
#include "PointerCoordinatesWindow.hpp"
#include "ui_pointerCoordinatesWindow.h"

#include "StelApp.hpp"
#include "StelLocaleMgr.hpp"
#include "StelModule.hpp"
#include "StelModuleMgr.hpp"

PointerCoordinatesWindow::PointerCoordinatesWindow()
{
	ui = new Ui_pointerCoordinatesWindowForm();
}

PointerCoordinatesWindow::~PointerCoordinatesWindow()
{
	delete ui;
}

void PointerCoordinatesWindow::retranslate()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);
		updateAboutText();
		populateCoordinatesPlacesList();
	}
}

void PointerCoordinatesWindow::createDialogContent()
{
	coord = GETSTELMODULE(PointerCoordinates);
	ui->setupUi(dialog);

	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));

	populateValues();

	connect(ui->checkBoxEnableAtStartup, SIGNAL(clicked(bool)), coord, SLOT(setFlagEnableAtStartup(bool)));
	connect(ui->spinBoxFontSize, SIGNAL(valueChanged(int)), coord, SLOT(setFontSize(int)));
	connect(ui->checkBoxShowButton, SIGNAL(clicked(bool)), coord, SLOT(setFlagShowCoordinatesButton(bool)));

	// Place of the string with coordinates
	populateCoordinatesPlacesList();
	int idx = ui->placeComboBox->findData(coord->getCurrentCoordinatesPlaceKey(), Qt::UserRole, Qt::MatchCaseSensitive);
	if (idx==-1)
	{
		// Use TopRight as default
		idx = ui->placeComboBox->findData(QVariant("TopRight"), Qt::UserRole, Qt::MatchCaseSensitive);
	}
	ui->placeComboBox->setCurrentIndex(idx);
	connect(ui->placeComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(setCoordinatesPlace(int)));

	connect(ui->pushButtonSave, SIGNAL(clicked()), this, SLOT(saveCoordinatesSettings()));
	connect(ui->pushButtonReset, SIGNAL(clicked()), this, SLOT(resetCoordinatesSettings()));

	updateAboutText();
}

void PointerCoordinatesWindow::populateValues()
{
	ui->checkBoxEnableAtStartup->setChecked(coord->getFlagEnableAtStartup());
	ui->spinBoxFontSize->setValue(coord->getFontSize());
	ui->checkBoxShowButton->setChecked(coord->getFlagShowCoordinatesButton());
}

void PointerCoordinatesWindow::updateAboutText()
{
	ui->labelTitle->setText(q_("Pointer Coordinates plug-in"));
	QString version = QString(q_("Version %1")).arg(POINTERCOORDINATES_PLUGIN_VERSION);
	ui->labelVersion->setText(version);
}

void PointerCoordinatesWindow::saveCoordinatesSettings()
{
	coord->saveConfiguration();
}

void PointerCoordinatesWindow::resetCoordinatesSettings()
{
	coord->restoreDefaultConfiguration();
	populateValues();
}

void PointerCoordinatesWindow::populateCoordinatesPlacesList()
{
	Q_ASSERT(ui->placeComboBox);

	QComboBox* places = ui->placeComboBox;

	//Save the current selection to be restored later
	places->blockSignals(true);
	int index = places->currentIndex();
	QVariant selectedPlaceId = places->itemData(index);
	places->clear();
	//For each algorithm, display the localized name and store the key as user
	//data. Unfortunately, there's no other way to do this than with a cycle.
	places->addItem(q_("The top center of the screen"), "TopCenter");
	places->addItem(q_("In center of the top right half of the screen"), "TopRight");
	places->addItem(q_("The right bottom corner of the screen"), "RightBottomCorner");

	//Restore the selection
	index = places->findData(selectedPlaceId, Qt::UserRole, Qt::MatchCaseSensitive);
	places->setCurrentIndex(index);
	places->blockSignals(false);
}

void PointerCoordinatesWindow::setCoordinatesPlace(int placeID)
{
	QString currentPlaceID = ui->placeComboBox->itemData(placeID).toString();
	coord->setCurrentCoordinatesPlaceKey(currentPlaceID);
}
