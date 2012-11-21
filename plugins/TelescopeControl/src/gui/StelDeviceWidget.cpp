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
#include "StelModuleMgr.hpp"
#include "StelObjectMgr.hpp"
#include "StelStyle.hpp"
#include "StelUtils.hpp"
#include "VecMath.hpp"
#include "TelescopeControl.hpp"
#include "FovCirclesWidget.hpp"
#include "CoordinatesWidget.hpp"

#include "StelDeviceWidget.hpp"
#include "ui_StelDeviceWidget.h"

#include <QDebug>

using namespace TelescopeControlGlobals;

StelDeviceWidget::StelDeviceWidget(TelescopeControl *plugin,
                                   const QString &id,
                                   const TelescopeClientP& telescope,
                                   QWidget *parent) :
    QWidget(parent),
    clientId(id)
{
	Q_ASSERT(plugin);
	deviceManager = plugin;
	
	Q_ASSERT(telescope);
	client = telescope;
	
	ui = new Ui_StelDeviceWidget;
	ui->setupUi(this);
	
	coordsWidget = new CoordinatesWidget(this);
	ui->verticalLayout->addWidget(coordsWidget);
	coordsWidget->abortButton->setHidden(true);
	coordsWidget->syncButton->setHidden(true);
	
	fcWidget = new FovCirclesWidget(telescope.data(), this);
	ui->verticalLayout->addWidget(fcWidget);
	
	// TODO: Check if this is really necessary.
	connect(&StelApp::getInstance(), SIGNAL(languageChanged()),
	        this, SLOT(retranslate()));

	// TODO: Find out if there's a way to reuse this code in
	// CoordinatesWidget or somewhere else.
	connect(coordsWidget->slewCoordsButton, SIGNAL(clicked()),
	        this, SLOT(slewToCoords()));
	connect(coordsWidget->slewCenterButton, SIGNAL(clicked()),
	        this, SLOT(slewToCenter()));
	connect(coordsWidget->slewSelectedButton, SIGNAL(clicked()),
	        this, SLOT(slewToObject()));
	
	// TODO: Make it current telescope coordinates, not current object.
	connect(coordsWidget->currentButton, SIGNAL(clicked()),
	        this, SLOT(getCurrentObjectInfo()));
	
	connect (fcWidget, SIGNAL(fovCirclesChanged()),
	         deviceManager, SLOT(saveConnections()));
}

StelDeviceWidget::~StelDeviceWidget()
{
	delete ui;
	ui = 0;
}

void StelDeviceWidget::retranslate()
{
	if (ui)
		ui->retranslateUi(this);
	if (coordsWidget)
		coordsWidget->retranslate();
	if (fcWidget)
		fcWidget->retranslate();
}

void StelDeviceWidget::slewToCoords()
{
	if (clientId.isEmpty())
		return;
	
	double radiansRA = coordsWidget->raSpinBox->valueRadians();
	double radiansDec = coordsWidget->decSpinBox->valueRadians();

	Vec3d targetPosition;
	StelUtils::spheToRect(radiansRA, radiansDec, targetPosition);

	deviceManager->telescopeGoto(clientId, targetPosition);
}

void StelDeviceWidget::slewToCenter()
{
	deviceManager->slewTelescopeToViewDirection(clientId);
}

void StelDeviceWidget::slewToObject()
{
	deviceManager->slewTelescopeToSelectedObject(clientId);
}

void StelDeviceWidget::getCurrentObjectInfo()
{
	const QList<StelObjectP>& selected = GETSTELMODULE(StelObjectMgr)->getSelectedObject();
	if (!selected.isEmpty())
	{
		double dec_j2000 = 0;
		double ra_j2000 = 0;
		StelUtils::rectToSphe(&ra_j2000,&dec_j2000,selected[0]->getJ2000EquatorialPos(StelApp::getInstance().getCore()));
		coordsWidget->raSpinBox->setRadians(ra_j2000);
		coordsWidget->decSpinBox->setRadians(dec_j2000);
	}
}
