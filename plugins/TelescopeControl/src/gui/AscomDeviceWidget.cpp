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

#include <QVBoxLayout>
#include "AngleSpinBox.hpp"
#include "CoordinatesWidget.hpp"
#include "FovCirclesWidget.hpp"
#include "TelescopeControl.hpp"
#include "StelUtils.hpp"

#include "AscomDeviceWidget.hpp"

AscomDeviceWidget::AscomDeviceWidget(TelescopeControl* plugin,
                                     const QString& id,
                                     const TelescopeClientAscomP& telescope,
                                     QWidget* parent) :
    QWidget(parent),
    clientId(id),
    coordsWidget(0),
    fcWidget(0)
{
	Q_ASSERT(plugin);
	deviceManager = plugin;
	
	Q_ASSERT(telescope);
	client = telescope;
	if (!client)
		return;
	
	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);
	
	if (client->canSlew())
	{
		coordsWidget = new CoordinatesWidget(this);
		layout->addWidget(coordsWidget);
		
		connect(coordsWidget->slewCenterButton, SIGNAL(clicked()),
		        this, SLOT(slewToCenter()));
		connect(coordsWidget->slewCoordsButton, SIGNAL(clicked()),
		        this, SLOT(slewToCoords()));
		connect(coordsWidget->slewSelectedButton, SIGNAL(clicked()),
		        this, SLOT(slewToObject()));
		
		// TODO: Stop
		// TODO: Sync
	}
	
	fcWidget = new FovCirclesWidget(telescope.data(), this);
	layout->addWidget(fcWidget);
}

void AscomDeviceWidget::slewToCoords()
{
	if (clientId.isEmpty())
		return;
	
	double radiansRA = coordsWidget->raSpinBox->valueRadians();
	double radiansDec = coordsWidget->decSpinBox->valueRadians();

	Vec3d targetPosition;
	StelUtils::spheToRect(radiansRA, radiansDec, targetPosition);

	client->telescopeGoto(targetPosition);
}

void AscomDeviceWidget::slewToCenter()
{
	deviceManager->slewTelescopeToViewDirection(clientId);
}

void AscomDeviceWidget::slewToObject()
{
	deviceManager->slewTelescopeToSelectedObject(clientId);
}
