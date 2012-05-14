/*
 * Stellarium Telescope Control Plug-in
 * 
 * Copyright (C) 2010-2012 Bogdan Marinov (this file)
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

#include "AngleSpinBox.hpp"
#include "StelApp.hpp"
#include "StelLocaleMgr.hpp"
#include "StelStyle.hpp"
#include "StelUtils.hpp"
#include "VecMath.hpp"
#include "TelescopeControl.hpp"
#include "FovCirclesWidget.hpp"

#include "StelDeviceWidget.hpp"
#include "ui_StelDeviceWidget.h"

#include <QDebug>

using namespace TelescopeControlGlobals;

StelDeviceWidget::StelDeviceWidget(TelescopeControl *plugin,
                                   const QString &id, QWidget *parent) :
    QWidget(parent),
    clientId(id)
{
	Q_ASSERT(plugin);
	deviceManager = plugin;
	
	ui = new Ui_StelDeviceWidget;
	ui->setupUi(this);
	
	// TODO: Check if this is really necessary.
	connect(&StelApp::getInstance(), SIGNAL(languageChanged()),
	        this, SLOT(languageChanged()));
	
	connect(ui->radioButtonHMS, SIGNAL(toggled(bool)),
	        this, SLOT(setFormatHMS(bool)));
	connect(ui->radioButtonDMS, SIGNAL(toggled(bool)),
	        this, SLOT(setFormatDMS(bool)));
	connect(ui->radioButtonDecimal, SIGNAL(toggled(bool)),
	        this, SLOT(setFormatDecimal(bool)));

	connect(ui->pushButtonSlew, SIGNAL(pressed()), this, SLOT(slew()));
	
	//Coordinates are in HMS by default:
	ui->radioButtonHMS->setChecked(true);
	
	TelescopeClientP telescope = deviceManager->getTelescope(id);
	FovCirclesWidget* fcWidget = new FovCirclesWidget(telescope.data(), this);
	ui->verticalLayout->addWidget(fcWidget);
	connect (fcWidget, SIGNAL(fovCirclesChanged()),
	         deviceManager, SLOT(saveConnections()));
}

StelDeviceWidget::~StelDeviceWidget()
{
	delete ui;
	ui = 0;
}

void StelDeviceWidget::languageChanged()
{
	if (ui)
		ui->retranslateUi(this);
}

/*
void StelDeviceWidget::showConfiguration()
{
	//Hack to work around having no direct way to do display the window
	deviceManager->configureGui(true);
}
*/

void StelDeviceWidget::setFormatHMS(bool set)
{
	if (!set) return;
	ui->spinBoxRA->setDisplayFormat(AngleSpinBox::HMSLetters);
	ui->spinBoxDec->setDisplayFormat(AngleSpinBox::DMSLetters);
}

void StelDeviceWidget::setFormatDMS(bool set)
{
	if (!set) return;
	ui->spinBoxRA->setDisplayFormat(AngleSpinBox::DMSLetters);
	ui->spinBoxDec->setDisplayFormat(AngleSpinBox::DMSLetters);
}

void StelDeviceWidget::setFormatDecimal(bool set)
{
	if (!set) return;
	ui->spinBoxRA->setDisplayFormat(AngleSpinBox::DecimalDeg);
	ui->spinBoxDec->setDisplayFormat(AngleSpinBox::DecimalDeg);
}

/*
void StellariumDeviceWidget::updateTelescopeList()
{
	connectedTelescopes.clear();
	ui->comboBoxTelescope->clear();

	QStringList telescopes = deviceManager->listConnectedTelescopeNames();
	if (!telescopes.isEmpty())
	{
		connectedTelescopes = telescopes;
		ui->comboBoxTelescope->addItems(telescopes);
	}
	
	updateTelescopeControls();
}

void StellariumDeviceWidget::updateTelescopeControls()
{
	bool connectedTelescopeAvailable = !connectedTelescopes.isEmpty();
	ui->groupBoxSlew->setVisible(connectedTelescopeAvailable);
	ui->labelNoTelescopes->setVisible(!connectedTelescopeAvailable);
	if (connectedTelescopeAvailable)
		ui->comboBoxTelescope->setCurrentIndex(0);
}

void StellariumDeviceWidget::addTelescope(const QString& name)
{
	if (name.isEmpty())
		return;
	if (connectedTelescopes.contains(name))
		return;

	connectedTelescopes.append(name);
	ui->comboBoxTelescope->addItem(name);

	updateTelescopeControls();
}

void StellariumDeviceWidget::removeTelescope(const QString& name)
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
*/

void StelDeviceWidget::slew()
{
	if (clientId.isEmpty())
		return;
	
	double radiansRA = ui->spinBoxRA->valueRadians();
	double radiansDec = ui->spinBoxDec->valueRadians();

	Vec3d targetPosition;
	StelUtils::spheToRect(radiansRA, radiansDec, targetPosition);

	deviceManager->telescopeGoto(clientId, targetPosition);
}
