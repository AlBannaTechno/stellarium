/*
 * Copyright (C) 2010 Pep Pujols
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef PrintSky_H
#define PrintSky_H

#include "StelModule.hpp"
#include <QPrinter>
#include <QPrintPreviewWidget>
#include <QPrintPreviewDialog>
#include "StelStyle.hpp"
#include "PrintSkyDialog.hpp"

//class PrintSky
class PrintSky : public StelModule
{
	Q_OBJECT
public:
	 PrintSky();
	virtual ~PrintSky();


	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	virtual void init();
	virtual bool configureGui(bool show=true);
	virtual void update(double deltaTime);
	virtual void draw(StelCore* core);
	virtual double getCallOrder(StelModuleActionName actionName) const;
	virtual void handleKeys(class QKeyEvent* event);
	virtual void handleMouseClicks(class QMouseEvent* event);
	virtual bool handleMouseMoves(int x, int y, Qt::MouseButtons b);
	virtual void setStelStyle(const QString& style);

public slots:

	//! Show dialog printing enabling preview and print buttons
	void initPrintingSky();

private slots:

private:
	//Printing options
	bool useInvertColors, scaleToFit, printData, printSSEphemerides;
	QString orientation;

	PrintSkyDialog *printskyDialog;
};

#include "fixx11h.h"
#include <QObject>
#include "StelPluginInterface.hpp"

//! This class is used by Qt to manage a plug-in interface
class PrintSkyStelPluginInterface : public QObject, public StelPluginInterface
{
	Q_OBJECT
	Q_INTERFACES(StelPluginInterface)
public:
	virtual StelModule* getStelModule() const;
	virtual StelPluginInfo getPluginInfo() const;
 };

#endif // PrintSky_H
