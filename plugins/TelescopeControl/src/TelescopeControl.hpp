/*
 * Stellarium Telescope Control Plug-in
 * 
 * Copyright (C) 2006 Johannes Gajdosik
 * Copyright (C) 2009-2011 Bogdan Marinov
 * 
 * This module was originally written by Johannes Gajdosik in 2006
 * as a core module of Stellarium. In 2009 it was significantly extended with
 * GUI features and later split as an external plug-in module by Bogdan Marinov.
 *
 * This class used to be called TelescopeMgr before the split.
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

#ifndef _TELESCOPE_CONTROL_HPP_
#define _TELESCOPE_CONTROL_HPP_

#include "StelFader.hpp"
#include "StelGui.hpp"
#include "StelJsonParser.hpp"
#include "StelObjectModule.hpp"
#include "StelProjectorType.hpp"
#include "StelTextureTypes.hpp"
#include "TelescopeControlGlobals.hpp"
#include "VecMath.hpp"

#include <QFile>
#include <QFont>
#include <QHash>
#include <QMap>
#include <QProcess>
#include <QSettings>
#include <QSignalMapper>
#include <QString>
#include <QStringList>
#include <QTextStream>
#include <QVariant>

class StelNavigator;
class StelObject;
class StelPainter;
class StelProjector;
class TelescopeClient;
class TelescopeControlConfigurationWindow;
class SlewWindow;
class DeviceControlPanel;

using namespace TelescopeControlGlobals;

typedef QSharedPointer<TelescopeClient> TelescopeClientP;

//! This class manages the controlling of one or more telescopes by one
//! instance of the Stellarium program. "Controlling a telescope"
//! means receiving position information from the telescope
//! and sending GOTO commands to the telescope.
//! No esoteric features like motor focus, electric heating and such.
//! The actual controlling of a telescope is left to the implementation
//! of the abstract base class TelescopeClient.
class TelescopeControl : public StelObjectModule
{
	Q_OBJECT

public:
	TelescopeControl();
	virtual ~TelescopeControl();
	
	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	virtual void init();
	virtual void deinit();
	virtual void update(double deltaTime);
	virtual void draw(StelCore * core);
	virtual void setStelStyle(const QString& section);
	virtual double getCallOrder(StelModuleActionName actionName) const;
	
	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelObjectModule class
	virtual QList<StelObjectP> searchAround(const Vec3d& v, double limitFov, const StelCore* core) const;
	virtual StelObjectP searchByNameI18n(const QString& nameI18n) const;
	virtual StelObjectP searchByName(const QString& name) const;
	virtual QStringList listMatchingObjectsI18n(const QString& objPrefix, int maxNbItem=5) const;
	virtual bool configureGui(bool show = true);
	
	///////////////////////////////////////////////////////////////////////////
	// Methods specific to TelescopeControl
	//! Send a J2000-goto-command to the specified telescope
	//! @param id the identifier of the telescope
	//! @param j2000Pos the direction in equatorial J2000 frame
	void telescopeGoto(const QString& id, const Vec3d &j2000Pos);
	
	//! Safe access to the loaded list of telescope models
	const QHash<QString, DeviceModel>& getDeviceModels();
	//! \todo Add description
	const QHash<QString, QString>& getIndiDeviceModels();
	
	//! Loads the module's configuration from the configuration file.
	void loadConfiguration();
	//! Saves the module's configuration to the configuration file.
	void saveConfiguration();
	
	//! Saves to \b connections.json the parameters of the active connections.
	void saveConnections();
	//! Loads \b connections.json and intializes connections from the data.
	//! If there are already any initialized telescope clients, they are removed.
	void loadConnections();
	
	//These are public, but not slots, because they don't use sufficient validation.
	//TODO: Is the above note still valid?
	//! Adds a new device connection with the specified properties.
	//! Connection ID is taken from the "name" property.
	//! Call saveTelescopes() to write the modified configuration to disc.
	//! Call startConnection() to start this telescope.
	//! \todo Add description of the properties format.
	//! \todo Return ID instead of bool?
	bool addConnection(const QVariantMap& properties);
	//! Retrieves a connection description.
	//! \returns empty map if no connection with this identifier exists.
	const QVariantMap getConnection(const QString& id) const;
	//! Removes a connection description.
	//! \todo Should it stop it first?
	bool removeConnection(const QString& id);
	//! Removes all connections.
	//! \todo Make it work properly.
	void removeAllConnections();
	//! Lists the names of all defined connections.
	QStringList listAllConnectionNames() const;
	
	//! Initializes the connection with the given id.
	//! Uses getConnection() to get its description.
	//! Creates a TelescopeClient object and adds it to #connections.
	bool startConnection(const QString& id);
	//! Closes the connection with the given ID.
	//! Destroys the TelescopeClient object.
	bool stopConnection(const QString& id);
	//! Closes all connections without removing them like removeAllConnections().
	//! \todo Find which of the two methods is actually needed/used.
	bool stopAllConnections();
	
	//! Checks if there's an initialized TelescopeClient object.
	bool doesClientExist(const QString& id);
	//! Checks if the TelescopeClient object is connected.
	bool isClientConnected(const QString& id);

	//! Returns a list of the currently connected #telescopes.
	QStringList listConnectedTelescopeNames();

	//!
	QList<int> listUsedShortcutNumbers() const;

	//! Returns a free TCP port in Stellarium's range or higher.
	uint getFreeTcpPort();
	
	bool getFlagUseTelescopeServerLogs () {return useTelescopeServerLogs;}

#ifdef Q_OS_WIN32
	//! \returns true if the ASCOM platform has been detected.
	bool canUseAscom() const {return ascomPlatformIsInstalled;}
#endif

public slots:
	//! Set display flag for telescope reticles
	void setFlagTelescopeReticles(bool b) {reticleFader = b;}
	//! Get display flag for telescope reticles
	bool getFlagTelescopeReticles() const {return (bool)reticleFader;}
	
	//! Set display flag for telescope name labels
	void setFlagTelescopeLabels(bool b) {labelFader = b;}
	//! Get display flag for telescope name labels
	bool getFlagTelescopeLabels() const {return labelFader==true;}

	//! Set display flag for telescope field of view circles
	void setFlagTelescopeCircles(bool b) {circleFader = b;}
	//! Get display flag for telescope field of view circles
	bool getFlagTelescopeCircles() const {return circleFader==true;}
	
	//! Set the telescope reticle color
	void setReticleColor(const Vec3f &c) {reticleColor = c;}
	//! Get the telescope reticle color
	const Vec3f& getReticleColor() const {return reticleColor;}
	
	//! Get the telescope labels color
	const Vec3f& getLabelColor() const {return labelColor;}
	//! Set the telescope labels color
	void setLabelColor(const Vec3f &c) {labelColor = c;}

	//! Set the field of view circles color
	void setCircleColor(const Vec3f &c) {circleColor = c;}
	//! Get the field of view circles color
	const Vec3f& getCircleColor() const {return circleColor;}
	
	//! Define font size to use for telescope names display
	void setFontSize(int fontSize);
	
	//! slews a telescope to the selected object.
	//! For use from the GUI.
	void slewTelescopeToSelectedObject(int number);

	//! slews a telescope to the point of the celestial sphere currently
	//! in the center of the screen.
	//! For use from the GUI.
	void slewTelescopeToViewDirection(int number);
	
	//! Used in the GUI
	void setFlagUseTelescopeServerLogs (bool b) {useTelescopeServerLogs = b;}

signals:
	//! Emitted when a connection has been established.
	void clientConnected(const QString& id);
	//! Emitted when a connection has been closed.
	void clientDisconnected(const QString& id);
	
private:
	//! Draw a nice animated pointer around the object if it's selected
	void drawPointer(const StelProjectorP& prj,
	                 const StelNavigator* nav,
	                 StelPainter& sPainter);

	//! Perform the communication with the telescope servers
	void communicate();

	//! Returns the path to the "modules/TelescopeControl" directory.
	//! Returns an empty string if it doesn't exist.
	QString getPluginDirectoryPath() const;
	//! Returns the path to the "connections.json" file
	QString getConnectionsFilePath() const;

	//! De-selects the currently selected objects if they contain a telescope.
	void unselectTelescopes();
	
	LinearFader labelFader;
	LinearFader reticleFader;
	LinearFader circleFader;
	//! Colour currently used to draw telescope reticles
	Vec3f reticleColor;
	//! Colour currently used to draw telescope text labels
	Vec3f labelColor;
	//! Colour currently used to draw field of view circles
	Vec3f circleColor;
	//! Colour used to draw telescope reticles in normal mode, as set in the configuration file
	Vec3f reticleNormalColor;
	//! Colour used to draw telescope reticles in night mode, as set in the configuration file
	Vec3f reticleNightColor;
	//! Colour used to draw telescope labels in normal mode, as set in the configuration file
	Vec3f labelNormalColor;
	//! Colour used to draw telescope labels in night mode, as set in the configuration file
	Vec3f labelNightColor;
	//! Colour used to draw field of view circles in normal mode, as set in the configuration file
	Vec3f circleNormalColor;
	//! Colour used to draw field of view circles in night mode, as set in the configuration file
	Vec3f circleNightColor;
	
	//! Font used to draw telescope text labels
	QFont labelFont;
	
	//Toolbar button to toggle the Slew window
	QPixmap* pixmapHover;
	QPixmap* pixmapOnIcon;
	QPixmap* pixmapOffIcon;
	StelButton* slewWindowButton;
	StelButton* controlPanelButton;
	
	//! Telescope reticle texture
	StelTextureSP reticleTexture;
	//! Telescope selection marker texture
	StelTextureSP selectionTexture;
	
	//! Stores internally all the information from the connections file.
	QVariantMap connectionsProperties;
	//! All initialized objects representing current connections.
	QHash<QString, TelescopeClientP> connections;
	//! All initialized objects that actually represent a steerable device.
	//! A "steerable device" is any device that can send and/or accept
	//! coordinates. #telescopes should be a subset of #connections.
	QHash<QString, TelescopeClientP> telescopes;


	QStringList telescopeServers;

	QHash<QString, DeviceModel> deviceModels;
	//! \todo Temporary.
	QHash<QString, QString> indiDeviceModels;

	//! Matches shortcut numbers and connection IDs.
	QHash<int, QString> idFromShortcutNumber;
	//!
	QList<uint> usedTcpPorts;
	
	QStringList interfaceTypeNames;
	
	bool useTelescopeServerLogs;
	QHash<QString, QFile*> telescopeServerLogFiles;
	QHash<QString, QTextStream*> telescopeServerLogStreams;
	
	//GUI
	TelescopeControlConfigurationWindow* configurationWindow;
	SlewWindow* slewWindow;
	DeviceControlPanel* controlPanelWindow;
	
	//! Checks if the argument is a TCP port number in IANA's allowed range.
	bool isValidTcpPort(uint port);
	//! Checks if the argument is a valid delay value in microseconds.
	bool isValidDelay(int delay);

	QSignalMapper gotoSelectedShortcutMapper;
	QSignalMapper gotoDirectionShortcutMapper;

	//! A wrapper for TelescopeControl::createClient(). Used internally by
	//! loadTelescopes() and startConnection().
	//! Does not perform any validation on its arguments.
	//! It is separate because the previous implementation separated clients
	//! from servers.
	//! TODO: Do away with this?
	bool startClient(const QString& id, const QVariantMap& properties);

	//! Returns true if the client has been stopped successfully or doesn't exist.
	bool stopClient(const QString& id);

	//! Creates a client object belonging to a subclass of TelescopeClient.
	//! Replaces TelescopeClient::create().
	//! \returns a base class pointer to the client object.
	TelescopeClient* createClient(const QVariantMap& properties);
	

	//! Loads the list of supported telescope models.
	void loadDeviceModels();
	//! If the INDI library is installed, loads the list of available INDI
	//! drivers
	void loadIndiDeviceModels();
	
	//! Copies the default device_models.json to the given destination
	//! \returns true if the file has been copied successfully
	bool restoreDeviceModelsListTo(QString deviceModelsListPath);
	
	void addLogForClient(const QString& id);
	void removeLogForClient(const QString& id);
	void setCurrentLog(const QString& id);

#ifdef Q_OS_WIN32
	bool ascomPlatformIsInstalled;
	bool checkIfAscomIsInstalled();
#endif
};


#include "fixx11h.h"
#include <QObject>
#include "StelPluginInterface.hpp"

//! This class is used by Qt to manage a plug-in interface
class TelescopeControlStelPluginInterface : public QObject, public StelPluginInterface
{
	Q_OBJECT
	Q_INTERFACES(StelPluginInterface)
public:
	virtual StelModule* getStelModule() const;
	virtual StelPluginInfo getPluginInfo() const;
};

#endif /*_TELESCOPE_CONTROL_HPP_*/
