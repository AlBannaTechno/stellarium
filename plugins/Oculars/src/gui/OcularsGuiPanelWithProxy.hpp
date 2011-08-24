/*
Oculars plug-in for Stellarium: graphical user interface widget
Copyright (C) 2011  Bogdan Marinov

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef OCULARSGUIPANEL_HPP
#define OCULARSGUIPANEL_HPP

#include <QGraphicsWidget>

class Oculars;
class QGraphicsProxyWidget;
class QLabel;
class QPushButton;
class QWidget;

//! A screen widget similar to InfoPanel. Contains controls and information.
class OcularsGuiPanel : public QGraphicsWidget
{
	Q_OBJECT

public:
	OcularsGuiPanel(Oculars* ocularsPlugin,
	                QGraphicsWidget * parent = 0,
	                Qt::WindowFlags wFlags = 0);
	~OcularsGuiPanel();

public slots:
	//! Show only the controls used with an ocular overlay.
	void showOcularGui();
	//! Show only the controls used with a CCD overlay.
	void showCcdGui();

private slots:
	//! Update the position of the widget within the parent.
	//! Tied to the parent's geometryChanged() signal.
	void updatePosition();

	void openOcularsConfigurationWindow();

	//! Updates the information shown when an ocular overlay is displayed
	void updateOcularInfo();
	//! Updates the information shown when a CCD overlay is displayed
	void updateCcdInfo();
	//! Updates the information that depends on the current telescope.
	//! Called in both updateOcularInfo() and updateCcdInfo().
	void updateTelescopeInfo();

private:
	Oculars* ocularsPlugin;

	//! This is actually SkyGui. Perhaps it should be more specific?
	QGraphicsWidget* parentWidget;

	// For now, this uses regular widgets wrapped in a proxy. In the future
	// it may be implemented purely with classes derived from QGraphicsItem.
	QGraphicsProxyWidget* proxy;
	QWidget* mainWidget;

	//Buttons
	QPushButton* nextTelescopeButton;
	QPushButton* nextCcdButton;
	QPushButton* nextOcularButton;
	QPushButton* prevTelescopeButton;
	QPushButton* prevCcdButton;
	QPushButton* prevOcularButton;
	QPushButton* toggleCrosshairsButton;
	QPushButton* configurationButton;

	QLabel* labelOcularName;
	QLabel* labelOcularFl;
	QLabel* labelOcularAfov;
	QLabel* labelCcdName;
	QLabel* labelCcdDimensions;
	QLabel* labelTelescopeName;
	QLabel* labelMagnification;
	QLabel* labelFov;

	//Sets the visibility of the ocular name label and the associated buttons.
	void setOcularControlsVisible(bool show);
	//Sets the visibility of the ocular labels that are not used for binoculars.
	void setOcularInfoVisible(bool show);
	void setCcdControlsVisible(bool show);
	//Sets the visibility of the other information shown for an ocular overlay.
	void setOtherControlsVisible(bool show);
};

#endif // OCULARSGUIPANEL_HPP
