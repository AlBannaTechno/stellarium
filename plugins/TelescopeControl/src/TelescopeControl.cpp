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

#include "TelescopeControl.hpp"
#include "TelescopeClient.hpp"
#include "TelescopeClientDummy.hpp"
#include "TelescopeClientIndi.hpp"
#include "TelescopeClientTcp.hpp"
#include "TelescopeClientDirectLx200.hpp"
#include "TelescopeClientDirectNexStar.hpp"
#ifdef Q_OS_WIN32
#include "TelescopeClientAscom.hpp"
#endif
#include "TelescopeControlConfigurationWindow.hpp"
#include "SlewWindow.hpp"
#include "LogFile.hpp"

#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelFileMgr.hpp"
#include "StelGui.hpp"
#include "StelGuiItems.hpp"
#include "StelIniParser.hpp"
#include "StelLocaleMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelMovementMgr.hpp"
#include "StelObject.hpp"
#include "StelObjectMgr.hpp"
#include "StelPainter.hpp"
#include "StelProjector.hpp"
#include "StelStyle.hpp"
#include "StelTextureMgr.hpp"

#include <QAction>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QMapIterator>
#include <QSettings>
#include <QString>
#include <QStringList>
#ifdef Q_OS_WIN32
#include <QAxObject>
#endif

#include <QXmlStreamReader>

#include <QDebug>

////////////////////////////////////////////////////////////////////////////////
//
StelModule* TelescopeControlStelPluginInterface::getStelModule() const
{
	return new TelescopeControl();
}

StelPluginInfo TelescopeControlStelPluginInterface::getPluginInfo() const
{
	// Allow to load the resources when used as a static plugin
	Q_INIT_RESOURCE(TelescopeControl);

	StelPluginInfo info;
	info.id = "TelescopeControl";
	info.displayedName = q_("Telescope Control");
	info.authors = "Bogdan Marinov, Johannes Gajdosik";
	info.contact = "http://stellarium.org";
	info.description = q_("This plug-in allows Stellarium to send \"slew\" commands to a telescope on a computerized mount (a \"GoTo telescope\"). It can also simulate a moving telescope reticle surrounded by field-of-view circles without connecting to any real telescope.");
	return info;
}

Q_EXPORT_PLUGIN2(TelescopeControl, TelescopeControlStelPluginInterface)


////////////////////////////////////////////////////////////////////////////////
// Constructor and destructor
TelescopeControl::TelescopeControl()
{
	setObjectName("TelescopeControl");

	interfaceTypeNames.append("virtual");
	interfaceTypeNames.append("Stellarium");
	interfaceTypeNames.append("INDI");
	//TODO: Ifdef?
	interfaceTypeNames.append("ASCOM");

	configurationWindow = NULL;
	slewWindow = NULL;

#ifdef Q_OS_WIN32
	ascomPlatformIsInstalled = false;
#endif
}

TelescopeControl::~TelescopeControl()
{

}


////////////////////////////////////////////////////////////////////////////////
// Methods inherited from the StelModule class
// init(), update(), draw(), setStelStyle(), getCallOrder()
void TelescopeControl::init()
{
	//TODO: I think I've overdone the try/catch...
	try
	{
		//Main configuration
		loadConfiguration();
		//Make sure that such a section is created, if it doesn't exist
		saveConfiguration();
		
		//Make sure that the module directory exists
		QString moduleDirectoryPath = StelFileMgr::getUserDir() + "/modules/TelescopeControl";
		if(!StelFileMgr::exists(moduleDirectoryPath))
			StelFileMgr::mkDir(moduleDirectoryPath);

#ifdef Q_OS_WIN32
	//This should be done before loading the device models and before
	//initializing the windows, as they rely on canUseAscom()
	ascomPlatformIsInstalled = checkIfAscomIsInstalled();
#endif
		
		//Load the device models
		loadIndiDeviceModels();
		loadDeviceModels();
		if(deviceModels.isEmpty())
		{
			qWarning() << "TelescopeControl: No device model descriptions have been loaded. Stellarium will not be able to control a telescope on its own, but it is still possible to do it through an external application or to connect to a remote host.";
		}
		
		//Unload Stellarium's internal telescope control module
		//(not necessary since revision 6308; remains as an example)
		//StelApp::getInstance().getModuleMgr().unloadModule("TelescopeMgr", false);
		/*If the alsoDelete parameter is set to true, Stellarium crashes with a
		  segmentation fault when an object is selected. TODO: Find out why.
		  unloadModule() didn't work prior to revision 5058: the module unloaded
		  normally, but Stellarium crashed later with a segmentation fault,
		  because LandscapeMgr::getCallOrder() depended on the module's
		  existence to return a value.*/
		
		//Load and start all telescope clients
		loadConnections();
		
		//Load OpenGL textures
		StelTextureMgr& textureMgr = StelApp::getInstance().getTextureManager();
		reticleTexture = textureMgr.createTexture(":/telescopeControl/telescope_reticle.png");
		selectionTexture = textureMgr.createTexture("textures/pointeur2.png");
		
		StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
		
		//Create telescope key bindings
		 /* QAction-s with these key bindings existed in Stellarium prior to
			revision 6311. Any future backports should account for that. */
		QString group = N_("Telescope Control");
		for (int i = MIN_SLOT_NUMBER; i <= MAX_SLOT_NUMBER; i++)
		{
			// "Slew to object" commands
			QString name = QString("actionMove_Telescope_To_Selection_%1").arg(i);
			QString description = q_("Move telescope #%1 to selected object").arg(i);
			QString shortcut = QString("Ctrl+%1").arg(i);
			QAction* action = gui->addGuiActions(name,
			                                     description,
			                                     shortcut,
			                                     group, false, false);
			connect(action, SIGNAL(triggered()),
			        &gotoSelectedShortcutMapper, SLOT(map()));
			gotoSelectedShortcutMapper.setMapping(action, i);

			// "Slew to the center of the screen" commands
			name = QString("actionSlew_Telescope_To_Direction_%1").arg(i);
			description = q_("Move telescope #%1 to the point currently in the center of the screen").arg(i);
			shortcut = QString("Alt+%1").arg(i);
			action = gui->addGuiActions(name,
			                            description,
			                            shortcut,
			                            group, false, false);
			connect(gui->getGuiActions(name), SIGNAL(triggered()),
			        &gotoDirectionShortcutMapper, SLOT(map()));
			gotoDirectionShortcutMapper.setMapping(action, i);
		}
		connect(&gotoSelectedShortcutMapper, SIGNAL(mapped(int)),
		        this, SLOT(slewTelescopeToSelectedObject(int)));
		connect(&gotoDirectionShortcutMapper, SIGNAL(mapped(int)),
		        this, SLOT(slewTelescopeToViewDirection(int)));
	
		//Create and initialize dialog windows
		configurationWindow = new TelescopeControlConfigurationWindow();
		slewWindow = new SlewWindow();
		
		//TODO: Think of a better keyboard shortcut
		QAction* slewWindowAction = gui->addGuiActions(
			"actionShow_Slew_Window",
			N_("Move a telescope to a given set of coordinates"),
			"Ctrl+0", group, true, false);
		connect(slewWindowAction, SIGNAL(toggled(bool)),
		        slewWindow, SLOT(setVisible(bool)));
		connect(slewWindow, SIGNAL(visibleChanged(bool)),
		        slewWindowAction, SLOT(setChecked(bool)));
		
		//Create toolbar button
		pixmapHover =   new QPixmap(":/graphicGui/glow32x32.png");
		pixmapOnIcon =  new QPixmap(":/telescopeControl/button_Slew_Dialog_on.png");
		pixmapOffIcon = new QPixmap(":/telescopeControl/button_Slew_Dialog_off.png");
		toolbarButton = new StelButton(NULL,
		                               *pixmapOnIcon,
		                               *pixmapOffIcon,
		                               *pixmapHover,
		                               slewWindowAction);
		gui->getButtonBar()->addButton(toolbarButton, "065-pluginsGroup");
	}
	catch (std::runtime_error &e)
	{
		qWarning() << "TelescopeControl::init() error: " << e.what();
		return;
	}
	
	GETSTELMODULE(StelObjectMgr)->registerStelObjectMgr(this);
	
	//Initialize style, as it is not called at startup:
	//(necessary to initialize the reticle/label/circle colors)
	setStelStyle(StelApp::getInstance().getCurrentStelStyle());
}

void TelescopeControl::deinit()
{
	//Close the interface
	if (configurationWindow != NULL)
	{
		delete configurationWindow;
	}
	if (slewWindow != NULL)
	{
		delete slewWindow;
	}

	//Destroy all clients first in order to avoid displaying a TCP error
	removeAllConnections();

	//TODO: Decide if it should be saved on change
	//Save the configuration on exit
	saveConfiguration();
}

void TelescopeControl::update(double deltaTime)
{
	labelFader.update((int)(deltaTime*1000));
	reticleFader.update((int)(deltaTime*1000));
	circleFader.update((int)(deltaTime*1000));
	// communicate with the telescopes:
	communicate();
}

void TelescopeControl::draw(StelCore* core)
{
	StelNavigator* nav = core->getNavigator();
	const StelProjectorP prj = core->getProjection(StelCore::FrameJ2000);
	StelPainter sPainter(prj);
	sPainter.setFont(labelFont);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	reticleTexture->bind();
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Normal transparency mode
	foreach (const TelescopeClientP& telescope, telescopes)
	{
		if (telescope->isConnected() && telescope->hasKnownPosition())
		{
			Vec3d XY;
			if (prj->projectCheck(telescope->getJ2000EquatorialPos(nav), XY))
			{
				//Telescope circles appear synchronously with markers
				if (circleFader.getInterstate() >= 0)
				{
					glColor4f(circleColor[0], circleColor[1], circleColor[2], circleFader.getInterstate());
					glDisable(GL_TEXTURE_2D);
					foreach (double circle, telescope->getOculars())
					{
						sPainter.drawCircle(XY[0], XY[1], 0.5 * prj->getPixelPerRadAtCenter() * (M_PI/180) * (circle));
					}
					glEnable(GL_TEXTURE_2D);
				}
				if (reticleFader.getInterstate() >= 0)
				{
					glColor4f(reticleColor[0], reticleColor[1], reticleColor[2], reticleFader.getInterstate());
					sPainter.drawSprite2dMode(XY[0],XY[1],15.f);
				}
				if (labelFader.getInterstate() >= 0)
				{
					glColor4f(labelColor[0], labelColor[1], labelColor[2], labelFader.getInterstate());
					//TODO: Different position of the label if circles are shown?
					//TODO: Remove magic number (text spacing)
					sPainter.drawText(XY[0], XY[1], telescope->getNameI18n(), 0, 6 + 10, -4, false);
					//Same position as the other objects: doesn't work, telescope label overlaps object label
					//sPainter.drawText(XY[0], XY[1], scope->getNameI18n(), 0, 10, 10, false);
					reticleTexture->bind();
				}
			}
		}
	}

	if(GETSTELMODULE(StelObjectMgr)->getFlagSelectedObjectPointer())
		drawPointer(prj, nav, sPainter);
}

void TelescopeControl::setStelStyle(const QString& section)
{
	if (section == "night_color")
	{
		setLabelColor(labelNightColor);
		setReticleColor(reticleNightColor);
		setCircleColor(circleNightColor);
	}
	else
	{
		setLabelColor(labelNormalColor);
		setReticleColor(reticleNormalColor);
		setCircleColor(circleNormalColor);
	}

	configurationWindow->updateStyle();
}

double TelescopeControl::getCallOrder(StelModuleActionName actionName) const
{
	//TODO: Remove magic number (call order offset)
	if (actionName == StelModule::ActionDraw)
		return StelApp::getInstance().getModuleMgr().getModule("MeteorMgr")->getCallOrder(actionName) + 2.;
	return 0.;
}


////////////////////////////////////////////////////////////////////////////////
// Methods inherited from the StelObjectModule class
//
QList<StelObjectP> TelescopeControl::searchAround(const Vec3d& vv, double limitFov, const StelCore* core) const
{
	QList<StelObjectP> result;
	if (!getFlagTelescopeReticles())
		return result;
	Vec3d v(vv);
	v.normalize();
	double cosLimFov = cos(limitFov * M_PI/180.);
	foreach (const TelescopeClientP& telescope, telescopes)
	{
		if (telescope->getJ2000EquatorialPos(core->getNavigator()).dot(v) >= cosLimFov)
		{
			result.append(qSharedPointerCast<StelObject>(telescope));
		}
	}
	return result;
}

StelObjectP TelescopeControl::searchByNameI18n(const QString &nameI18n) const
{
	foreach (const TelescopeClientP& telescope, telescopes)
	{
		if (telescope->getNameI18n() == nameI18n)
			return qSharedPointerCast<StelObject>(telescope);
	}
	return 0;
}

StelObjectP TelescopeControl::searchByName(const QString &name) const
{
	foreach (const TelescopeClientP& telescope, telescopes)
	{
		if (telescope->getEnglishName() == name)
		return qSharedPointerCast<StelObject>(telescope);
	}
	return 0;
}

QStringList TelescopeControl::listMatchingObjectsI18n(const QString& objPrefix, int maxNbItem) const
{
	QStringList result;
	if (maxNbItem==0) return result;

	QString objw = objPrefix.toUpper();
	foreach (const TelescopeClientP& telescope, telescopes)
	{
		QString constw = telescope->getNameI18n().mid(0, objw.size()).toUpper();
		if (constw==objw)
		{
			result << telescope->getNameI18n();
		}
	}
	result.sort();
	if (result.size()>maxNbItem)
	{
		result.erase(result.begin() + maxNbItem, result.end());
	}
	return result;
}

bool TelescopeControl::configureGui(bool show)
{
	if(show)
	{
		configurationWindow->setVisible(true);
	}

	return true;
}


////////////////////////////////////////////////////////////////////////////////
// Misc methods (from TelescopeMgr; TODO: Better categorization)
void TelescopeControl::setFontSize(int fontSize)
{
	 labelFont.setPixelSize(fontSize);
}

void TelescopeControl::slewTelescopeToSelectedObject(int number)
{
	// Find out the coordinates of the target
	StelObjectMgr* omgr = GETSTELMODULE(StelObjectMgr);
	if (omgr->getSelectedObject().isEmpty())
		return;

	StelObjectP selectObject = omgr->getSelectedObject().at(0);
	if (!selectObject)  // should never happen
		return;

	const StelNavigator* nav = StelApp::getInstance().getCore()->getNavigator();
	Vec3d objectPosition = selectObject->getJ2000EquatorialPos(nav);

	telescopeGoto(idFromShortcutNumber.value(number), objectPosition);
}

void TelescopeControl::slewTelescopeToViewDirection(int number)
{
	// Find out the coordinates of the target
	Vec3d centerPosition = GETSTELMODULE(StelMovementMgr)->getViewDirectionJ2000();

	telescopeGoto(idFromShortcutNumber.value(number), centerPosition);
}

void TelescopeControl::drawPointer(const StelProjectorP& prj, const StelNavigator * nav, StelPainter& sPainter)
{
	const QList<StelObjectP> newSelected = GETSTELMODULE(StelObjectMgr)->getSelectedObject("Telescope");
	if (!newSelected.empty())
	{
		const StelObjectP obj = newSelected[0];
		Vec3d pos = obj->getJ2000EquatorialPos(nav);
		Vec3d screenpos;
		// Compute 2D pos and return if outside screen
		if (!prj->project(pos, screenpos))
			return;

		const Vec3f& c(obj->getInfoColor());
		sPainter.setColor(c[0], c[1], c[2]);
		selectionTexture->bind();
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Normal transparency mode
		sPainter.drawSprite2dMode(screenpos[0], screenpos[1], 25., StelApp::getInstance().getTotalRunTime() * 40.);
	}
}

void TelescopeControl::telescopeGoto(const QString& id, const Vec3d &j2000Pos)
{
	if(telescopes.contains(id))
		telescopes.value(id)->telescopeGoto(j2000Pos);
}

void TelescopeControl::communicate()
{
	if (!connections.empty())
	{
		QHashIterator<QString, TelescopeClientP> i(connections);
		while (i.hasNext())
		{
			i.next();
			setCurrentLog(i.key());//If there's no log, it will be ignored
			if(i.value()->prepareCommunication())
			{
				i.value()->performCommunication();
			}
		}
	}
}

QString TelescopeControl::getPluginDirectoryPath() const
{
	QString name = "modules/TelescopeControl";
	//TODO: Add the Qt flag mechanism to StelFileMgr
	StelFileMgr::Flags flags = StelFileMgr::Flags(StelFileMgr::Directory |
	                                              StelFileMgr::Writable);
	try
	{
		QString path = StelFileMgr::findFile(name, flags);
		return path;
	}
	catch (std::runtime_error& e)
	{
		qWarning() << "Error finding" << name << ":" << e.what();
		return QString();
	}
}

QString TelescopeControl::getConnectionsFilePath() const
{
	QString directoryPath = getPluginDirectoryPath();
	if (directoryPath.isEmpty())
		return directoryPath;

	QString path = directoryPath.append("/connections.json");
	return path;
}


////////////////////////////////////////////////////////////////////////////////
// Methods for managing telescope client objects
//

void TelescopeControl::unselectTelescopes()
{
	StelObjectMgr* objectMgr = GETSTELMODULE(StelObjectMgr);
	const QList<StelObjectP> list = objectMgr->getSelectedObject("Telescope");
	if(!list.isEmpty())
	{
		objectMgr->unSelect();
	}
}

void TelescopeControl::removeAllConnections()
{
	unselectTelescopes();

	//TODO: I really hope that this won't cause a memory leak...
	//foreach (TelescopeClient* telescope, telescopeClients)
	//	delete telescope;
	telescopes.clear();
	connections.clear();
	connectionsProperties.clear();
}

QStringList TelescopeControl::listAllConnectionNames() const
{
	return connectionsProperties.keys();
}

bool TelescopeControl::doesClientExist(const QString& id)
{
	return (connections.contains(id));
}

bool TelescopeControl::isClientConnected(const QString& id)
{
	if(connections.contains(id))
		return connections.value(id)->isConnected();
	else
		return false;
}


////////////////////////////////////////////////////////////////////////////////
// Methods for reading from and writing to the configuration file

void TelescopeControl::loadConfiguration()
{
	QSettings* settings = StelApp::getInstance().getSettings();
	Q_ASSERT(settings);

	settings->beginGroup("TelescopeControl");

	//Load display flags
	setFlagTelescopeReticles(settings->value("flag_telescope_reticles", true).toBool());
	setFlagTelescopeLabels(settings->value("flag_telescope_labels", true).toBool());
	setFlagTelescopeCircles(settings->value("flag_telescope_circles", true).toBool());

	//Load font size
	#ifdef Q_OS_WIN32
	setFontSize(settings->value("telescope_labels_font_size", 13).toInt()); //Windows Qt bug workaround
	#else
	setFontSize(settings->value("telescope_labels_font_size", 12).toInt());
	#endif

	//Load colours
	reticleNormalColor = StelUtils::strToVec3f(settings->value("color_telescope_reticles", "0.6,0.4,0").toString());
	reticleNightColor = StelUtils::strToVec3f(settings->value("night_color_telescope_reticles", "0.5,0,0").toString());
	labelNormalColor = StelUtils::strToVec3f(settings->value("color_telescope_labels", "0.6,0.4,0").toString());
	labelNightColor = StelUtils::strToVec3f(settings->value("night_color_telescope_labels", "0.5,0,0").toString());
	circleNormalColor = StelUtils::strToVec3f(settings->value("color_telescope_circles", "0.6,0.4,0").toString());
	circleNightColor = StelUtils::strToVec3f(settings->value("night_color_telescope_circles", "0.5,0,0").toString());

	//Load logging flag
	useTelescopeServerLogs = settings->value("flag_enable_telescope_logs", false).toBool();

	settings->endGroup();
}

void TelescopeControl::saveConfiguration()
{
	QSettings* settings = StelApp::getInstance().getSettings();
	Q_ASSERT(settings);

	settings->beginGroup("TelescopeControl");

	//Save display flags
	settings->setValue("flag_telescope_reticles", getFlagTelescopeReticles());
	settings->setValue("flag_telescope_labels", getFlagTelescopeLabels());
	settings->setValue("flag_telescope_circles", getFlagTelescopeCircles());

	//Save colours
	settings->setValue("color_telescope_reticles", QString("%1,%2,%3").arg(reticleNormalColor[0], 0, 'f', 2).arg(reticleNormalColor[1], 0, 'f', 2).arg(reticleNormalColor[2], 0, 'f', 2));
	settings->setValue("night_color_telescope_reticles", QString("%1,%2,%3").arg(reticleNightColor[0], 0, 'f', 2).arg(reticleNightColor[1], 0, 'f', 2).arg(reticleNightColor[2], 0, 'f', 2));
	settings->setValue("color_telescope_labels", QString("%1,%2,%3").arg(labelNormalColor[0], 0, 'f', 2).arg(labelNormalColor[1], 0, 'f', 2).arg(labelNormalColor[2], 0, 'f', 2));
	settings->setValue("night_color_telescope_labels", QString("%1,%2,%3").arg(labelNightColor[0], 0, 'f', 2).arg(labelNightColor[1], 0, 'f', 2).arg(labelNightColor[2], 0, 'f', 2));
	settings->setValue("color_telescope_circles", QString("%1,%2,%3").arg(circleNormalColor[0], 0, 'f', 2).arg(circleNormalColor[1], 0, 'f', 2).arg(circleNormalColor[2], 0, 'f', 2));
	settings->setValue("night_color_telescope_circles", QString("%1,%2,%3").arg(circleNightColor[0], 0, 'f', 2).arg(circleNightColor[1], 0, 'f', 2).arg(circleNightColor[2], 0, 'f', 2));

	//If telescope server executables flag and directory are
	//specified, remove them
	settings->remove("flag_use_server_executables");
	settings->remove("server_executables_path");

	//Save logging flag
	settings->setValue("flag_enable_telescope_logs", useTelescopeServerLogs);

	settings->endGroup();
}

void TelescopeControl::saveConnections()
{
	try
	{
		//Open/create the JSON file
		QString telescopesJsonPath = getConnectionsFilePath();
		QFile telescopesJsonFile(telescopesJsonPath);
		if(!telescopesJsonFile.open(QFile::WriteOnly|QFile::Text))
		{
			qWarning() << "TelescopeControl: Telescopes can not be saved. A file can not be open for writing:" << telescopesJsonPath;
			return;
		}

		//Add the version:
		connectionsProperties.insert("version", QString(TELESCOPE_CONTROL_VERSION));

		//Convert the tree to JSON
		StelJsonParser::write(connectionsProperties, &telescopesJsonFile);
		telescopesJsonFile.flush();//Is this necessary?
		telescopesJsonFile.close();
	}
	catch(std::runtime_error &e)
	{
		qWarning() << "TelescopeControl: Error saving telescopes: " << e.what();
		return;
	}
}

void TelescopeControl::loadConnections()
{
	//TODO: Determine what exactly needed to be in a try/catch
	try
	{
		QString connectionsFilePath = getConnectionsFilePath();

		if(!QFileInfo(connectionsFilePath).exists())
		{
			//This is a normal occurence - no need to log a message.
			return;
		}

		QFile connectionsJsonFile(connectionsFilePath);
		QVariantMap map;
		if(!connectionsJsonFile.open(QFile::ReadOnly))
		{
			qWarning() << "TelescopeControl: No connections loaded."
			           << "Unable to open for reading" << connectionsFilePath;
			return;
		}
		else
		{
			map = StelJsonParser::parse(&connectionsJsonFile).toMap();
			connectionsJsonFile.close();
		}
		if(map.isEmpty())
		{
			return;
		}

		QString version = map.value("version", "0.0.0").toString();
		if(version != QString(TELESCOPE_CONTROL_VERSION))
		{
			qWarning() << "TelescopeControl:"
				<< "The existing version of connections.json"
				<< "is not compatible with the current version of the plug-in.";

			QString newName = connectionsFilePath
				+ ".backup."
				+ QDateTime::currentDateTime().toString("yyyy-MM-dd-hh-mm-ss");
			if(connectionsJsonFile.rename(newName))
			{
				qWarning() << "The file has been backed up as" << newName;
				return;
			}
			else
			{
				qWarning() << "The file cannot be replaced.";
				return;
			}
		}
		map.remove("version");//Otherwise it will try to read it as a connection

		//Make sure that there are no clients yet
		removeAllConnections();

		//Read telescopes, if any
		QMapIterator<QString, QVariant> node(map);
		while(node.hasNext())
		{
			node.next();
			QString key = node.key();

			QVariantMap connectionProperties = node.value().toMap();
			if (!addConnection(connectionProperties))
				continue;

			//TODO: Warning! Possible error source - the key is not necessary the name
			connectionProperties = connectionsProperties.value(key).toMap();

			//Connect at startup
			bool connectAtStartup = connectionProperties.value("connectsAtStartup", false).toBool();
			bool isRemote = connectionProperties.value("isRemoteConnection", false).toBool();

			//Initialize a telescope client for this slot
			if (connectAtStartup)
			{
				if (!isRemote)
				{
					addLogForClient(key);
					setCurrentLog(key);
				}
				if (!startClient(key, connectionProperties))
				{
					qDebug() << "TelescopeControl: Unable to create a connection:" << key;
					//Unnecessary due to if-else construction;
					//also, causes bug #608533
					//continue;
				}
			}
		}

		int count = connectionsProperties.count();
		if (count > 0)
		{
			qDebug() << "TelescopeControl: Loaded successfully"
			         << count << "connections.";
		}
	}
	catch(std::runtime_error &e)
	{
		qWarning() << "TelescopeControl: Error loading connections: "
		           << e.what();
	}
}

bool TelescopeControl::addConnection(const QVariantMap& properties)
{
	//Name
	QString name = properties.value("name").toString();
	if (name.isEmpty())
	{
		qDebug() << "TelescopeControl: Unable to add connection: "
		         << "No name specified.";
		return false;
	}
	//TODO: Escape the symbols?
	if (name.contains('\\') || name.contains('"'))
	{
		qDebug() << "TelescopeControl: Unable to add connection: "
		         << "The name contains invalid characters (\\ or \"):"
		         << name;
		return false;
	}
	if (connections.contains(name))
	{
		qDebug() << "TelescopeControl: Unable to add connection: "
		         << "This name is already in use:"
		         << name;
		return false;
	}

	//Interface type
	QString interfaceType = properties.value("interface").toString();
	if (!interfaceTypeNames.contains(interfaceType))
	{
		qDebug() << "TelescopeControl: Unable to add connection: "
		         << "Invalid interface type:" << interfaceType;
		return false;
	}

	QVariantMap newProperties;
	newProperties.insert("name", name);
	newProperties.insert("interface", interfaceType);

	bool isRemote = properties.value("isRemoteConnection", false).toBool();
	QString host = properties.value("host", "localhost").toString();
	uint tcpPort = properties.value("tcpPort").toUInt();

	QString driver = properties.value("driverId").toString();
	QString deviceModel = properties.value("deviceModel").toString();

	if (interfaceType == "Stellarium")
	{
		if (!isRemote)
		{
			if (driver.isEmpty() ||
			    !EMBEDDED_TELESCOPE_SERVERS.contains(driver))
			{
				qDebug() << "TelescopeControl: Unable to add connection: "
				            "No Stellarium driver specified for" << name;
				return false;
			}

			QString serialPort = properties.value("serialPort").toString();
			//TODO: More validation! Especially on Windows!
			if(serialPort.isEmpty() || !serialPort.startsWith(SERIAL_PORT_PREFIX))
			{
				qDebug() << "TelescopeControl: Unable to add connection: "
				         << "No valid serial port specified for" << name;
				return false;
			}

			//Add the stuff to the new node
			if (deviceModel.isEmpty() || !deviceModels.contains(deviceModel))
			{
				//TODO: Autodetect the device model here?
				//Or leave it to TelescopePropertiesWindow?
				newProperties.insert("driverId", driver);
			}
			else
			{
				newProperties.insert("driverId", deviceModels.value(deviceModel).driver);
				newProperties.insert("deviceModel", deviceModel);
			}
			newProperties.insert("serialPort", serialPort);
		}
	}
	else if (interfaceType == "INDI")
	{
		if (!isRemote)
		{
			//TODO: Better check
			if (driver.isEmpty() ||
				!indiDeviceModels.values().contains(driver))
			{
				qDebug() << "TelescopeControl: Unable to add connection: "
				         << "No INDI driver specified for" << name;
				return false;
			}

			//Add the stuff to the new node
			if (deviceModel.isEmpty() || !indiDeviceModels.contains(deviceModel))
			{
				newProperties.insert("driverId", driver);
			}
			else
			{
				newProperties.insert("driverId", indiDeviceModels.value(deviceModel));
				newProperties.insert("deviceModel", deviceModel);
			}
		}
	}
#ifdef Q_OS_WIN32
	else if (interfaceType == "ASCOM")
	{
		if (driver.isEmpty())
			return false;
		newProperties.insert("driverId", driver);
	}
#endif
	//TODO: Add INDI here

	if (isRemote)
	{
		if (host.isEmpty())
		{
			qDebug() << "TelescopeControl:  Unable to add connection: "
			         << "No host name for" << name;
			return false;
		}
		if (!isValidTcpPort(tcpPort))
			tcpPort = getFreeTcpPort();

		newProperties.insert("host", host);
		newProperties.insert("tcpPort", tcpPort);
	}
	newProperties.insert("isRemoteConnection", isRemote);

	if (interfaceType != "virtual")
	{
		QString equinox = properties.value("equinox", "J2000").toString();
		if (equinox != "J2000" && equinox != "JNow")
		{
			//TODO: Assume J2000 if the name is invalid?
			qDebug() << "TelescopeControl: Unable to add connection: "
			         << "Invalid equinox value for" << name;
			return false;
		}
		int delay = properties.value("delay").toInt();
		if (!isValidDelay(delay))
			delay = DEFAULT_DELAY;

		newProperties.insert("equinox", equinox);
		newProperties.insert("delay", delay);
	}

	bool connectAtStartup = properties.value("connectsAtStartup", false).toBool();
	newProperties.insert("connectsAtStartup", connectAtStartup);

	QVariantList fovCircles = properties.value("fovCircles").toList();
	//TODO: Check if MAX_CIRCLE_COUNT matters
	if (!fovCircles.isEmpty())
	{
		QVariantList newFovCircles;
		bool ok;
		for (int i=0; i < fovCircles.count(); i++)
		{
			double fov = fovCircles.at(i).toDouble(&ok);
			if (ok)
				newFovCircles.append(fov);
		}
		if (!newFovCircles.isEmpty())
			newProperties.insert("fovCircles", newFovCircles);
	}

	int shortcutNumber = properties.value("shortcutNumber").toInt();
	if (shortcutNumber > 0 && shortcutNumber < 10)
	{
		newProperties.insert("shortcutNumber", shortcutNumber);
		if (!idFromShortcutNumber.contains(shortcutNumber))
			idFromShortcutNumber.insert(shortcutNumber, name);
	}
	if (isValidTcpPort(tcpPort))
		usedTcpPorts.append(tcpPort);

	connectionsProperties.insert(name, newProperties);

	return true;
}

const QVariantMap TelescopeControl::getConnection(const QString& id) const
{
	//Validation
	if(id.isEmpty())
		return QVariantMap();

	return connectionsProperties.value(id).toMap();
}

bool TelescopeControl::removeConnection(const QString& id)
{
	//Validation
	if(id.isEmpty())
		return false;

	uint tcpPort = getConnection(id).value("tcpPort").toUInt();
	if (tcpPort > 0)
		usedTcpPorts.removeOne(tcpPort);

	idFromShortcutNumber.remove(idFromShortcutNumber.key(id));
	return (connectionsProperties.remove(id) == 1);
}

bool TelescopeControl::startConnection(const QString& id)
{
	//Validation
	if(id.isEmpty())
		return false;

	//Read the connection properties
	const QVariantMap properties = getConnection(id);
	if(properties.isEmpty())
	{
		//TODO: Add debug
		return false;
	}

	bool isRemote = properties.value("isRemoteConnection").toBool();

	//If it's not a remote connection, attach a log file
	if(!isRemote)
	{
		addLogForClient(id);
		setCurrentLog(id);
	}
	if (startClient(id, properties))
	{
		emit clientConnected(id);
		return true;
	}
	else if (!isRemote)
	{
		removeLogForClient(id);
	}

	return false;
}

bool TelescopeControl::stopConnection(const QString& id)
{
	//Validation
	if(id.isEmpty())
		return false;

	return stopClient(id);
}

bool TelescopeControl::stopAllConnections()
{
	bool allStoppedSuccessfully = true;

	if (!connections.empty())
	{
		QHashIterator<QString, TelescopeClientP> i(connections);
		while (i.hasNext())
		{
			i.next();
			allStoppedSuccessfully = stopConnection(i.key()) &&
			                         allStoppedSuccessfully;
		}
	}

	return allStoppedSuccessfully;
}

bool TelescopeControl::isValidTcpPort(uint port)
{
	//Check if the port number is in IANA's allowed range
	return (port > 1023 && port <= 65535);
}

uint TelescopeControl::getFreeTcpPort()
{
	uint slot;
	for (slot = 10001; slot < 10010; slot++)
	{
		if (!usedTcpPorts.contains(slot))
			return slot;
	}
	for (slot = 49152; slot <= 65535; slot++)
	{
		if (!usedTcpPorts.contains(slot))
			return slot;
	}
	return 10001;
}

bool TelescopeControl::isValidDelay(int delay)
{
	return (delay > 0 && delay <= MICROSECONDS_FROM_SECONDS(10));
}

bool TelescopeControl::startClient(const QString& id,
                                   const QVariantMap& properties)
{
	//Validation
	if(id.isEmpty())
		return false;

	//TODO: Check if it's not already running
	//TODO: Should it return, or should it remove the old one and proceed?
	if(connections.contains(id))
	{
		qDebug() << "A client already exists with that ID:" << id;
		return false;
	}

	//TODO: Refactor to differentiate between connections and telescopes.
	TelescopeClient* newTelescope = createClient(properties);
	if (newTelescope)
	{
		TelescopeClientP newTelescopeP(newTelescope);
		connections.insert(id, newTelescopeP);
		//TODO: Temporary:
		telescopes.insert(id, newTelescopeP);
		return true;
	}

	return false;
}

TelescopeClient* TelescopeControl::createClient(const QVariantMap &properties)
{
	TelescopeClient * newTelescope = 0;

	if (properties.isEmpty())
		return newTelescope;

	//At least two values should exist: name and connection type
	QString name = properties.value("name").toString();
	if (name.isEmpty())
		return newTelescope;

	QString interfaceType = properties.value("interface").toString();
	if (interfaceType.isEmpty())
		return newTelescope;

	bool isRemote = properties.value("isRemoteConnection", false).toBool();

	qDebug() << "Attempting to create a telescope client:" << properties;

	int delay = properties.value("delay", DEFAULT_DELAY).toInt();
	QString equinoxString = properties.value("equinox", "J2000").toString();
	Equinox equinox = (equinoxString == "JNow") ? EquinoxJNow : EquinoxJ2000;

	QString parameters;
	if (interfaceType == "virtual")
	{
		newTelescope = new TelescopeClientDummy(name, "");
	}
	else if (interfaceType == "Stellarium")
	{
		if (isRemote)
		{
			QString host = properties.value("host", "localhost").toString();
			int port = properties.value("tcpPort").toInt();
			parameters = QString("%1:%2:%3").arg(host).arg(port).arg(delay);
			newTelescope = new TelescopeClientTcp(name, parameters, equinox);
		}
		else
		{
			QString driver = properties.value("driverId").toString();
			if (driver.isEmpty() || !EMBEDDED_TELESCOPE_SERVERS.contains(driver))
				return newTelescope;
			QString serialPort = properties.value("serialPort").toString();

			//TODO: Change driver names when the whole mechanism depends on it.
			if (driver == "Lx200")
			{
				parameters = QString("%1:%2").arg(serialPort).arg(delay);
				newTelescope = new TelescopeClientDirectLx200(name, parameters, equinox);
			}
			else if (driver == "NexStar")
			{
				parameters = QString("%1:%2").arg(serialPort).arg(delay);
				newTelescope = new TelescopeClientDirectNexStar(name, parameters, equinox);
			}
		}
	}
	else if (interfaceType == "INDI")
	{
		if (isRemote)
		{
			QString host = properties.value("host", "localhost").toString();
			int port = properties.value("tcpPort").toInt();
			parameters = QString("%1:%2:%3").arg(host).arg(port).arg(delay);
			newTelescope = TelescopeClientIndi::telescopeClient(name, host, port, equinox);
		}
		else
		{
			QString driver = properties.value("driverId").toString();
			//TODO: Fix the file check!
			if (driver.isEmpty()
				|| !QFile::exists("/usr/bin/" + driver)
				|| !QFileInfo("/usr/bin/" + driver).isExecutable())
				return newTelescope;
			parameters = QString("%1:%2").arg(driver).arg(delay);
			newTelescope = TelescopeClientIndi::telescopeClient(name, driver, equinox);
		}
	}
#ifdef Q_OS_WIN32
	else if (interfaceType == "ASCOM")
	{
		QString ascomDriverObjectId = properties.value("driverId").toString();
		if (ascomDriverObjectId.isEmpty())
			return newTelescope;

		//parameters = QString("%1:%2").arg(ascomDriverObjectId).arg(delay);
		parameters = QString("%1").arg(ascomDriverObjectId);
		newTelescope = new TelescopeClientAscom(name, parameters, equinox);
	}
#endif
	else
	{
		qWarning() << "TelescopeControl: unable to create a client of type"
				<< interfaceType;
		qDebug() << "Additional parameters are:";
		foreach (QString key, properties.keys())
		{
			qDebug() << key << "=" << properties.value(key);
		}
	}

	if (newTelescope && !newTelescope->isInitialized())
	{
		qDebug() << "TelescopeClient::create(): Unable to create a telescope client.";
		delete newTelescope;
		newTelescope = 0;
	}

	//Read and add FOV circles
	QVariantList circleList = properties.value("fovCircles").toList();
	if(!circleList.isEmpty() && circleList.size() <= MAX_CIRCLE_COUNT)
	{
		for(int i = 0; i < circleList.size(); i++)
			newTelescope->addOcular(circleList.value(i, -1.0).toDouble());
	}

	return newTelescope;
}

bool TelescopeControl::stopClient(const QString& id)
{
	//Validation
	//If it doesn't exist, it is stopped :)
	if(id.isEmpty() ||
	   !connections.contains(id))
		return false;

	//If a telescope is selected, deselect it first.
	//(Otherwise trying to delete a selected telescope client causes Stellarium to crash.)
	unselectTelescopes();
	connections.remove(id);
	telescopes.remove(id);

	//This is not needed by every client
	removeLogForClient(id);

	emit clientDisconnected(id);
	return true;
}

void TelescopeControl::loadDeviceModels()
{
	//qDebug() << "TelescopeControl: Loading device model descriptions...";
	
	//Make sure that the device models file exists
	bool useDefaultList = false;
	QString deviceModelsJsonPath = getPluginDirectoryPath() + "/device_models.json";
	if(!QFileInfo(deviceModelsJsonPath).exists())
	{
		if(!restoreDeviceModelsListTo(deviceModelsJsonPath))
		{
			qWarning() << "TelescopeControl: Unable to find" << deviceModelsJsonPath;
			useDefaultList = true;
		}
	}
	else
	{
		QFile deviceModelsJsonFile(deviceModelsJsonPath);
		if(!deviceModelsJsonFile.open(QFile::ReadOnly))
		{
			qWarning() << "TelescopeControl: Can't open for reading" << deviceModelsJsonPath;
			useDefaultList = true;
		}
		else
		{
			//Check the version and move the old file if necessary
			QVariantMap deviceModelsJsonMap;
			deviceModelsJsonMap = StelJsonParser::parse(&deviceModelsJsonFile).toMap();
			QString version = deviceModelsJsonMap.value("version", "0.0.0").toString();
			if(version < QString(TELESCOPE_CONTROL_VERSION))
			{
				deviceModelsJsonFile.close();
				QString newName = deviceModelsJsonPath + ".backup." + QDateTime::currentDateTime().toString("yyyy-MM-dd-hh-mm-ss");
				if(deviceModelsJsonFile.rename(newName))
				{
					qWarning() << "TelescopeControl: The existing version of device_models.json is obsolete. Backing it up as" << newName;
					if(!restoreDeviceModelsListTo(deviceModelsJsonPath))
					{
						useDefaultList = true;
					}
				}
				else
				{
					qWarning() << "TelescopeControl: The existing version of device_models.json is obsolete. Unable to rename.";
					useDefaultList = true;
				}
			}
			else
				deviceModelsJsonFile.close();
		}
	}

	if (useDefaultList)
	{
		qWarning() << "TelescopeControl: Using embedded device models list.";
		deviceModelsJsonPath = ":/telescopeControl/device_models.json";
	}

	//Open the file and parse the device list
	QVariantList deviceModelsList;
	QFile deviceModelsJsonFile(deviceModelsJsonPath);
	if(deviceModelsJsonFile.open(QFile::ReadOnly))
	{
		deviceModelsList = (StelJsonParser::parse(&deviceModelsJsonFile).toMap()).value("list").toList();
		deviceModelsJsonFile.close();
	}
	else
	{
		return;
	}

	//Clear the list of device models - it may not be empty.
	deviceModels.clear();

	//Cicle the list of telescope deifinitions
	for(int i = 0; i < deviceModelsList.size(); i++)
	{
		QVariantMap model = deviceModelsList.at(i).toMap();
		if(model.isEmpty())
			continue;

		//Model name
		QString name = model.value("name").toString();
		if(name.isEmpty())
		{
			//TODO: Add warning
			continue;
		}

		if(deviceModels.contains(name))
		{
			qWarning() << "TelescopeControl: Skipping device model: Duplicate name:" << name;
			continue;
		}

		//Telescope server
		QString server = model.value("server").toString();
		if(server.isEmpty())
		{
			qWarning() << "TelescopeControl: Skipping device model: No server specified for" << name;
			continue;
		}

		if(!EMBEDDED_TELESCOPE_SERVERS.contains(server))
		{
			qWarning() << "TelescopeControl: Skipping device model: No server" << server << "found for" << name;
			continue;
		}
		//else: everything is OK, using embedded server
		
		//Description and default connection delay
		QString description = model.value("description", "No description is available.").toString();
		int delay = model.value("default_delay", DEFAULT_DELAY).toInt();
		
		//Add this to the main list
		DeviceModel newDeviceModel = {name, description, server, delay};
		deviceModels.insert(name, newDeviceModel);
		//qDebug() << "TelescopeControl: Adding device model:" << name << description << server << delay;
	}
}

void TelescopeControl::loadIndiDeviceModels()
{
	indiDeviceModels = IndiClient::loadDeviceDescriptions();
}

const QHash<QString, DeviceModel>& TelescopeControl::getDeviceModels()
{
	return deviceModels;
}

const QHash<QString, QString>& TelescopeControl::getIndiDeviceModels()
{
	return indiDeviceModels;
}

QStringList TelescopeControl::listConnectedTelescopeNames()
{
	QStringList connectedTelescopes;
	if (!telescopes.isEmpty())
		connectedTelescopes = telescopes.keys();
	return connectedTelescopes;
}

QList<int> TelescopeControl::listUsedShortcutNumbers() const
{
	return idFromShortcutNumber.keys();
}

bool TelescopeControl::restoreDeviceModelsListTo(QString deviceModelsListPath)
{
	QFile defaultFile(":/telescopeControl/device_models.json");
	if (!defaultFile.copy(deviceModelsListPath))
	{
		qWarning() << "TelescopeControl: Unable to copy the default device models list to" << deviceModelsListPath;
		return false;
	}
	QFile newCopy(deviceModelsListPath);
	newCopy.setPermissions(newCopy.permissions() | QFile::WriteOwner);

	qDebug() << "TelescopeControl: The default device models list has been copied to" << deviceModelsListPath;
	return true;
}

void TelescopeControl::addLogForClient(const QString& id)
{
	if(!telescopeServerLogFiles.contains(id)) // || !telescopeServerLogFiles.value(slot)->isOpen()
	{
		//If logging is off, use an empty stream to avoid segmentation fault
		if(!useTelescopeServerLogs)
		{
			QFile* emptyFile = new QFile();
			telescopeServerLogFiles.insert(id, emptyFile);
			telescopeServerLogStreams.insert(id, new QTextStream(emptyFile));
			return;
		}

		QString filePath = StelFileMgr::getUserDir() + "/deviceLog_" + id + ".txt";
		QFile* logFile = new QFile(filePath);
		if (!logFile->open(QFile::WriteOnly|QFile::Text|QFile::Truncate|QFile::Unbuffered))
		{
			qWarning() << "TelescopeControl: Unable to create a log file for"
					   << id << ":" << filePath;
			telescopeServerLogFiles.insert(id, logFile);
			telescopeServerLogStreams.insert(id, new QTextStream(new QFile()));
		}

		telescopeServerLogFiles.insert(id, logFile);
		QTextStream * logStream = new QTextStream(logFile);
		telescopeServerLogStreams.insert(id, logStream);
	}
}

void TelescopeControl::removeLogForClient(const QString& id)
{
	if(telescopeServerLogFiles.contains(id))
	{
		telescopeServerLogFiles.value(id)->close();
		telescopeServerLogStreams.remove(id);
		telescopeServerLogFiles.remove(id);
	}
}

void TelescopeControl::setCurrentLog(const QString& id)
{
	if(telescopeServerLogStreams.contains(id))
		log_file = telescopeServerLogStreams.value(id);
}

#ifdef Q_OS_WIN32
bool TelescopeControl::checkIfAscomIsInstalled()
{
	//Try to detect the ASCOM platform by trying to use the Helper
	//control. (If it doesn't exist, Stellarium's ASCOM support
	//has no way of selecting ASCOM drivers anyway.)
	QAxObject ascomChooser;
	if (ascomChooser.setControl("DriverHelper.Chooser"))
		return true;
	else
		return false;
	//TODO: I hope that the QAxObject is de-initialized when
	//the flow of control leaves its scope, i.e. this function body.
}
#endif
