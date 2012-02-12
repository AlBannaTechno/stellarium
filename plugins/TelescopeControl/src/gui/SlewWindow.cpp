/*
 * Stellarium Telescope Control Plug-in
 * 
 * Copyright (C) 2010-2011 Bogdan Marinov (this file)
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
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
*/

#include "Dialog.hpp"
#include "AngleSpinBox.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelModuleMgr.hpp"
#include "StelLocaleMgr.hpp"
#include "StelStyle.hpp"
#include "StelUtils.hpp"
#include "VecMath.hpp"
#include "TelescopeControl.hpp"
#include "SlewWindow.hpp"
#include "ui_slewWindow.h"

#include <QDebug>

using namespace TelescopeControlGlobals;


SlewWindow::SlewWindow()
{
	ui = new Ui_widgetSlew;
	
	//TODO: Fix this - it's in the same plugin
	telescopeManager = GETSTELMODULE(TelescopeControl);
}

SlewWindow::~SlewWindow()
{	
	delete ui;
}

void SlewWindow::languageChanged()
{
	if (dialog)
		ui->retranslateUi(dialog);
}

void SlewWindow::createDialogContent()
{
	ui->setupUi(dialog);
	
	//Inherited connect
	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(languageChanged()));
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));

	connect(ui->radioButtonHMS, SIGNAL(toggled(bool)),
	        this, SLOT(setFormatHMS(bool)));
	connect(ui->radioButtonDMS, SIGNAL(toggled(bool)),
	        this, SLOT(setFormatDMS(bool)));
	connect(ui->radioButtonDecimal, SIGNAL(toggled(bool)),
	        this, SLOT(setFormatDecimal(bool)));

	connect(ui->pushButtonSlew, SIGNAL(pressed()), this, SLOT(slew()));
	connect(ui->pushButtonConfigure, SIGNAL(pressed()),
	        this, SLOT(showConfiguration()));

	connect(telescopeManager, SIGNAL(clientConnected(const QString&)),
	        this, SLOT(addTelescope(const QString&)));
	connect(telescopeManager, SIGNAL(clientDisconnected(const QString&)),
	        this, SLOT(removeTelescope(const QString&)));

	//Coordinates are in HMS by default:
	ui->radioButtonHMS->setChecked(true);

	updateTelescopeList();
}

void SlewWindow::showConfiguration()
{
	//Hack to work around having no direct way to do display the window
	telescopeManager->configureGui(true);
}

void SlewWindow::setFormatHMS(bool set)
{
	if (!set) return;
	ui->spinBoxRA->setDisplayFormat(AngleSpinBox::HMSLetters);
	ui->spinBoxDec->setDisplayFormat(AngleSpinBox::DMSLetters);
}

void SlewWindow::setFormatDMS(bool set)
{
	if (!set) return;
	ui->spinBoxRA->setDisplayFormat(AngleSpinBox::DMSLetters);
	ui->spinBoxDec->setDisplayFormat(AngleSpinBox::DMSLetters);
}

void SlewWindow::setFormatDecimal(bool set)
{
	if (!set) return;
	ui->spinBoxRA->setDisplayFormat(AngleSpinBox::DecimalDeg);
	ui->spinBoxDec->setDisplayFormat(AngleSpinBox::DecimalDeg);
}

void SlewWindow::updateTelescopeList()
{
	connectedTelescopes.clear();
	ui->comboBoxTelescope->clear();

	QStringList telescopes = telescopeManager->listConnectedTelescopeNames();
	if (!telescopes.isEmpty())
	{
		connectedTelescopes = telescopes;
		ui->comboBoxTelescope->addItems(telescopes);
	}
	
	updateTelescopeControls();
}

void SlewWindow::updateTelescopeControls()
{
	bool connectedTelescopeAvailable = !connectedTelescopes.isEmpty();
	ui->groupBoxSlew->setVisible(connectedTelescopeAvailable);
	ui->labelNoTelescopes->setVisible(!connectedTelescopeAvailable);
	if (connectedTelescopeAvailable)
		ui->comboBoxTelescope->setCurrentIndex(0);
}

void SlewWindow::addTelescope(const QString& name)
{
	if (name.isEmpty())
		return;
	if (connectedTelescopes.contains(name))
		return;

	connectedTelescopes.append(name);
	ui->comboBoxTelescope->addItem(name);

	updateTelescopeControls();
}

void SlewWindow::removeTelescope(const QString& name)
{
	if (name.isEmpty())
		return;

	connectedTelescopes.removeAll(name);

	int index = ui->comboBoxTelescope->findText(name);
	if (index != -1)
	{
		ui->comboBoxTelescope->removeItem(index);
	}
	else
	{
		//Something very wrong just happened, so:
		updateTelescopeList();
		return;
	}

	updateTelescopeControls();
}

void SlewWindow::slew()
{
	double radiansRA = ui->spinBoxRA->valueRadians();
	double radiansDec = ui->spinBoxDec->valueRadians();
	QString id = ui->comboBoxTelescope->currentText();
	if (id.isEmpty())
		return;

	Vec3d targetPosition;
	StelUtils::spheToRect(radiansRA, radiansDec, targetPosition);

	telescopeManager->telescopeGoto(id, targetPosition);
}
