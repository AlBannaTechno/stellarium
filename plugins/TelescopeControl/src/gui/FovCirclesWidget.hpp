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

#ifndef FOVCIRCLESWIDGET_HPP
#define FOVCIRCLESWIDGET_HPP

#include <QGroupBox>
#include <QSet>

class QLineEdit;
class QRegExpValidator;
class TelescopeClient;

//! A box with controls manipulating a telescope device's FOV circles.
//! \author Bogdan Marinov
class FovCirclesWidget : public QGroupBox
{
	Q_OBJECT
public:
	FovCirclesWidget(TelescopeClient* devicePtr, QWidget *parent = 0);
	//! Load the FOV circle sizes from the telescope object.
	void loadFovCircleSizes();
	//! Displays the formatted FOV circle sizes in the line edit widget.
	void displayFovCircleSizes();
	//! Applies the currently stored FOV circle sizes to the telescope object.
	void applyFovCircleSizes();
	
signals:
	void fovCirclesChanged();
	
public slots:
	void readLineEdit();
	void clearFovCircleSizes();
	void addTelradCircles();
	
private:
	QLineEdit* lineEditCircleList;
	QRegExpValidator* circleListValidator;
	
	TelescopeClient* device;
	
	QSet<QString> fovCircleSizes;
};

#endif // FOVCIRCLESWIDGET_HPP
