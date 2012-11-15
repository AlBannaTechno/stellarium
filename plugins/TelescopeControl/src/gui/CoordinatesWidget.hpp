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
#ifndef COORDINATESWIDGET_HPP
#define COORDINATESWIDGET_HPP

#include <QGroupBox>
#include <QPushButton>

class AngleSpinBox;
class QButtonGroup;
class QGridLayout;
class QHBoxLayout;
class QLabel;
class QRadioButton;
class QVBoxLayout;

//! A titled box allowing a pair of coordinates to be set.
//! Some of the members are public to allow them to be connected to slots
//! in the widgets that use this control.
//! @todo Decide whether the "slew to selected" and "slew to center" buttons
//! are a good idea.
class CoordinatesWidget : public QGroupBox
{
	Q_OBJECT
public:
	CoordinatesWidget(QWidget* parent = 0);
	
	AngleSpinBox* raSpinBox;
	AngleSpinBox* decSpinBox;
	QPushButton* slewCoordsButton;
	QPushButton* currentButton;
	QPushButton* slewSelectedButton;
	QPushButton* slewCenterButton;
		
public slots:
	void retranslate();

private slots:
	void setFormat(int buttonId);
	
private:
	QLabel* raLabel;
	QLabel* decLabel;
	
	QButtonGroup* formatButtons;
	QRadioButton* hmsButton;
	QRadioButton* dmsButton;
	QRadioButton* decimalButton;
};

#endif // COORDINATESWIDGET_HPP
