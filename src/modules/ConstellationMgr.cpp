/*
 * Stellarium
 * Copyright (C) 2002 Fabien Chereau
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

// Class used to manage group of constellation

#include <config.h>

#include <vector>
#include <QDebug>
#include <QFile>
#include <QSettings>
#include <QRegExp>
#include <QString>
#include <QStringList>

#include "ConstellationMgr.hpp"
#include "Constellation.hpp"
#include "StarMgr.hpp"
#include "StelUtils.hpp"
#include "StelApp.hpp"
#include "StelTextureMgr.hpp"
#include "Projector.hpp"
#include "LoadingBar.hpp"
#include "StelObjectMgr.hpp"
#include "StelFontMgr.hpp"
#include "StelLocaleMgr.hpp"
#include "StelSkyCultureMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelFileMgr.hpp"
#include "StelCore.hpp"
#include "SFont.hpp"

// constructor which loads all data from appropriate files
ConstellationMgr::ConstellationMgr(StarMgr *_hip_stars) :
	fontSize(15),
	asterFont(NULL),
	hipStarMgr(_hip_stars),
	flagNames(0),
	flagLines(0),
	flagArt(0),
	flagBoundaries(0)
{
	setObjectName("ConstellationMgr");
	assert(hipStarMgr);
	isolateSelected = false;
}

ConstellationMgr::~ConstellationMgr()
{
	vector<Constellation *>::iterator iter;

	for (iter = asterisms.begin(); iter != asterisms.end(); iter++)
	{
		delete(*iter);
	}

	vector<vector<Vec3f> *>::iterator iter1;
	for (iter1 = allBoundarySegments.begin(); iter1 != allBoundarySegments.end(); ++iter1)
	{
		delete (*iter1);
	}
	allBoundarySegments.clear();
}

void ConstellationMgr::init()
{
	QSettings* conf = StelApp::getInstance().getSettings();
	assert(conf);

	lastLoadedSkyCulture = "dummy";
	updateSkyCulture();
	fontSize = conf->value("viewing/constellation_font_size",16.).toDouble();
	setFlagLines(conf->value("viewing/flag_constellation_drawing").toBool());
	setFlagNames(conf->value("viewing/flag_constellation_name").toBool());
	setFlagBoundaries(conf->value("viewing/flag_constellation_boundaries",false).toBool());
	setArtIntensity(conf->value("viewing/constellation_art_intensity", 0.5).toDouble());
	setArtFadeDuration(conf->value("viewing/constellation_art_fade_duration",2.).toDouble());
	setFlagArt(conf->value("viewing/flag_constellation_art").toBool());
	setFlagIsolateSelected(conf->value("viewing/flag_constellation_isolate_selected",
				conf->value("viewing/flag_constellation_pick", false).toBool() ).toBool());
	StelApp::getInstance().getStelObjectMgr().registerStelObjectMgr(this);
}

/*************************************************************************
 Reimplementation of the getCallOrder method
*************************************************************************/
double ConstellationMgr::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName==StelModule::ACTION_DRAW)
		return StelApp::getInstance().getModuleMgr().getModule("GridLinesMgr")->getCallOrder(actionName)+10;
	return 0;
}

void ConstellationMgr::updateSkyCulture()
{
	QString newSkyCulture = StelApp::getInstance().getSkyCultureMgr().getSkyCultureDir();
	StelFileMgr& fileMan = StelApp::getInstance().getFileMgr();
	
	// Check if the sky culture changed since last load, if not don't load anything
	if (lastLoadedSkyCulture == newSkyCulture)
		return;
	
	// Find constellation art.  If this doesn't exist, warn, but continue using ""
	// the loadLinesAndArt function knows how to handle this (just loads lines).
	QString conArtFile;
	try
	{
		conArtFile = fileMan.findFile("skycultures/"+newSkyCulture+"/constellationsart.fab");
	}
	catch(exception& e)
	{
		qWarning() << "WARNING: no constellationsart.fab file found for sky culture " << newSkyCulture;
	}

	try
	{
		loadLinesAndArt(fileMan.findFile("skycultures/"+newSkyCulture+"/constellationship.fab"), conArtFile, newSkyCulture);
			
		// load constellation names
		loadNames(fileMan.findFile("skycultures/" + newSkyCulture + "/constellation_names.eng.fab"));

		// Translate constellation names for the new sky culture
		updateI18n();
		
		// as constellations have changed, clear out any selection and retest for match!
		selectedObjectChangeCallBack(StelModule::REPLACE_SELECTION);		
	}
	catch(exception& e)
	{
		qWarning() << "ERROR: while loading new constellation data for sky culture " 
			<< newSkyCulture << ", reason: " << e.what() << endl;		
	}
		
	// TODO: do we need to have an else { clearBoundaries(); } ?
	if (newSkyCulture=="western") 
	{
		try
		{
			loadBoundaries(fileMan.findFile("data/constellations_boundaries.dat"));
		}
		catch(exception& e)
		{
			qWarning() << "ERROR loading constellation boundaries file: " << e.what();
		}
	}

	lastLoadedSkyCulture = newSkyCulture;
}

void ConstellationMgr::setColorScheme(const QSettings* conf, const QString& section)
{
	// Load colors from config file
	QString defaultColor = conf->value(section+"/default_color").toString();
	setLinesColor(StelUtils::str_to_vec3f(conf->value(section+"/const_lines_color",    defaultColor).toString()));
	setBoundariesColor(StelUtils::str_to_vec3f(conf->value(section+"/const_boundary_color", "0.8,0.3,0.3").toString()));
	setNamesColor(StelUtils::str_to_vec3f(conf->value(section+"/const_names_color",    defaultColor).toString()));
}

void ConstellationMgr::selectedObjectChangeCallBack(StelModuleSelectAction action)
{
	const std::vector<StelObjectP> newSelected = StelApp::getInstance().getStelObjectMgr().getSelectedObject();
	if (newSelected.empty())
	{
		// Even if do not have anything selected, KEEP constellation selection intact
		// (allows viewing constellations without distraction from star pointer animation)
		// setSelected(NULL);
		return;
	}
	
	const std::vector<StelObjectP> newSelectedConst = StelApp::getInstance().getStelObjectMgr().getSelectedObject("Constellation");
	if (!newSelectedConst.empty())
	{
//		const boost::intrusive_ptr<Constellation> c = boost::dynamic_pointer_cast<Constellation>(newSelectedConst[0]);
//		StelApp::getInstance().getStelObjectMgr().setSelectedObject(c->getBrightestStarInConstellation());

		// If removing this selection
		if(action == StelModule::REMOVE_FROM_SELECTION) {
			unsetSelectedConst((Constellation *)newSelectedConst[0].get());
		} else {
			// Add constellation to selected list (do not select a star, just the constellation)
			setSelectedConst((Constellation *)newSelectedConst[0].get());
		}
	}
	else
	{
		const std::vector<StelObjectP> newSelectedStar = StelApp::getInstance().getStelObjectMgr().getSelectedObject("Star");
		if (!newSelectedStar.empty())
		{
//			if (!added)
//				setSelected(NULL);
			setSelected(newSelectedStar[0].get());
		}
		else
		{
//			if (!added)
				setSelected(NULL);
		}
	}
}

void ConstellationMgr::setLinesColor(const Vec3f& c)
{
	Constellation::lineColor = c;
}

Vec3f ConstellationMgr::getLinesColor() const
{
	return Constellation::lineColor;
}

void ConstellationMgr::setBoundariesColor(const Vec3f& c) 
{
	Constellation::boundaryColor = c;
}

Vec3f ConstellationMgr::getBoundariesColor() const 
{
	return Constellation::boundaryColor;
}

void ConstellationMgr::setNamesColor(const Vec3f& c) 
{
	Constellation::labelColor = c;
}

Vec3f ConstellationMgr::getNamesColor() const 
{
	return Constellation::labelColor;
}

void ConstellationMgr::setFontSize(double newFontSize)
{
	asterFont = &StelApp::getInstance().getFontManager().getStandardFont(StelApp::getInstance().getLocaleMgr().getSkyLanguage(), fontSize);
}

double ConstellationMgr::getFontSize() const
{
	return asterFont->getSize();
}

// Load line and art data from files
void ConstellationMgr::loadLinesAndArt(const QString &fileName, const QString &artfileName, const QString& cultureName)
{
	QFile in(fileName);
	if (!in.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qWarning() << "Can't open constellation data file" << fileName  << "for culture" << cultureName;
		assert(0);
	}

	int totalRecords=0;
	QString record;
	QRegExp commentRx("^(\\s*#.*|\\s*)$");
	while (!in.atEnd()) 
	{
		record = QString::fromUtf8(in.readLine());
		if (!commentRx.exactMatch(record))
			totalRecords++;
	}
	in.seek(0);

	// delete existing data, if any
	vector < Constellation * >::iterator iter;
	for (iter = asterisms.begin(); iter != asterisms.end(); ++iter)
		delete(*iter);

	asterisms.clear();
	selected.clear();
	Constellation *cons = NULL;

	// read the file, adding a record per non-comment line
	int currentLineNumber = 0;	// line in file
	int readOk = 0;			// count of records processed OK
	while (!in.atEnd()) 
	{
		record = QString::fromUtf8(in.readLine());
		currentLineNumber++;
		if (commentRx.exactMatch(record))
			continue;

		cons = new Constellation;
		if(cons->read(record, hipStarMgr))
		{
			asterisms.push_back(cons);
			++readOk;
		}
		else
		{
			qWarning() << "ERROR reading constellation rec at line " << currentLineNumber << "for culture" << cultureName;
			delete cons;
		}
	}
	in.close();
	qDebug() << "Loaded" << readOk << "/" << totalRecords << "constellation records successfully for culture" << cultureName;

	// Set current states
	setFlagArt(flagArt);
	setFlagLines(flagLines);
	setFlagNames(flagNames);	
	setFlagBoundaries(flagBoundaries);	

	QFile fic(artfileName);
	if (!fic.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qWarning() << "Can't open constellation art file" << fileName  << "for culture" << cultureName;
		return; // possible to have no art - just constellations
		assert(0);
	}

	totalRecords=0;
	while (!fic.atEnd()) 
	{
		record = QString::fromUtf8(fic.readLine());
		if (!commentRx.exactMatch(record))
			totalRecords++;
	}
	fic.seek(0);

	// Read the constellation art file with the following format :
	// ShortName texture_file x1 y1 hp1 x2 y2 hp2
	// Where :
	// shortname is the international short name (i.e "Lep" for Lepus)
	// texture_file is the graphic file of the art texture
	// x1 y1 are the x and y texture coordinates in pixels of the star of hipparcos number hp1
	// x2 y2 are the x and y texture coordinates in pixels of the star of hipparcos number hp2
	// The coordinate are taken with (0,0) at the top left corner of the image file
	QString shortname;
	QString texfile;
	unsigned int x1, y1, x2, y2, x3, y3, hp1, hp2, hp3;
	QString tmpstr;

	currentLineNumber = 0;	// line in file
	readOk = 0;		// count of records processed OK

	StelApp::getInstance().getTextureManager().setDefaultParams();
	LoadingBar& lb = *StelApp::getInstance().getLoadingBar();
	while (!fic.atEnd()) 
	{
		++currentLineNumber;
		record = QString::fromUtf8(fic.readLine());
		if (commentRx.exactMatch(record))
			continue;

		// prevent leaving zeros on numbers from being interpretted as octal numbers
		record.replace(" 0", " ");
		QTextStream rStr(&record);
		rStr >> shortname >> texfile >> x1 >> y1 >> hp1 >> x2 >> y2 >> hp2 >> x3 >> y3 >> hp3;
		if (rStr.status()!=QTextStream::Ok)
		{
			qWarning() << "ERROR parsing constellation art record at line" << currentLineNumber << "of art file for culture" << cultureName;
			continue;
		}
		
		// Draw loading bar
		lb.SetMessage(q_("Loading Constellation Art: %1/%2").arg(currentLineNumber).arg(totalRecords));
		lb.Draw((float)(currentLineNumber)/totalRecords);
		
		cons = NULL;
		cons = findFromAbbreviation(shortname);
		if (!cons)
		{
			qWarning() << "ERROR in constellation art file at line" << currentLineNumber << "for culture" << cultureName
			           << "constellation" << shortname << "unknown";
		}
		else
		{
			StelApp::getInstance().getTextureManager().setDefaultParams();
			QString texturePath(texfile);
			try
			{
				texturePath = StelApp::getInstance().getFileMgr().findFile("skycultures/"+cultureName+"/"+texfile);
			}
			catch(exception& e)
			{
				// if the texture isn't found in the skycultures/[culture] directory,
				// try the central textures diectory.
				qWarning() << "WARNING, could not locate texture file " << texfile
				     << " in the skycultures/" << cultureName
				     << " directory...  looking in general textures/ directory...";
				try
				{
					texturePath = StelApp::getInstance().getFileMgr().findFile(QString("textures/")+texfile);
				}
				catch(exception& e2)
				{
					qWarning() << "ERROR: could not find texture, " << texfile << ": " << e2.what();
				}
			}
			
			cons->artTexture = StelApp::getInstance().getTextureManager().createTextureThread(texturePath);
			
			int texSizeX, texSizeY;
			if (cons->artTexture==NULL || !cons->artTexture->getDimensions(texSizeX, texSizeY))
			{
				qWarning() << "Texture dimension not available";
			}

			Vec3f s1 = hipStarMgr->searchHP(hp1)->getObsJ2000Pos(0);
			Vec3f s2 = hipStarMgr->searchHP(hp2)->getObsJ2000Pos(0);
			Vec3f s3 = hipStarMgr->searchHP(hp3)->getObsJ2000Pos(0);

			// To transform from texture coordinate to 2d coordinate we need to find X with XA = B
			// A formed of 4 points in texture coordinate, B formed with 4 points in 3d coordinate
			// We need 3 stars and the 4th point is deduced from the other to get an normal base
			// X = B inv(A)
			Vec3f s4 = s1 + (s2 - s1) ^ (s3 - s1);
			Mat4f B(s1[0], s1[1], s1[2], 1, s2[0], s2[1], s2[2], 1, s3[0], s3[1], s3[2], 1, s4[0], s4[1], s4[2], 1);
			Mat4f A(x1, texSizeY - y1, 0.f, 1.f, x2, texSizeY - y2, 0.f, 1.f, x3, texSizeY - y3, 0.f, 1.f, x1, texSizeY - y1, texSizeX, 1.f);
			Mat4f X = B * A.inverse();

			cons->art_vertex[0] = Vec3f(X * Vec3f(0, 0, 0));
			cons->art_vertex[1] = Vec3f(X * Vec3f(texSizeX / 2, 0, 0));
			cons->art_vertex[2] = Vec3f(X * Vec3f(texSizeX / 2, texSizeY / 2, 0));
			cons->art_vertex[3] = Vec3f(X * Vec3f(0, texSizeY / 2, 0));
			cons->art_vertex[4] = Vec3f(X * Vec3f(texSizeX / 2 + texSizeX / 2, 0, 0));
			cons->art_vertex[5] = Vec3f(X * Vec3f(texSizeX / 2 + texSizeX / 2, texSizeY / 2, 0));
			cons->art_vertex[6] = Vec3f(X * Vec3f(texSizeX / 2 + texSizeX / 2, texSizeY / 2 + texSizeY / 2, 0));
			cons->art_vertex[7] = Vec3f(X * Vec3f(texSizeX / 2 + 0, texSizeY / 2 + texSizeY / 2, 0));
			cons->art_vertex[8] = Vec3f(X * Vec3f(0, texSizeY / 2 + texSizeY / 2, 0));

			++readOk;
		}
	}

	qDebug() << "Loaded" << readOk << "/" << totalRecords << "constellation art records successfully for culture" << cultureName;
	fic.close();
}

double ConstellationMgr::draw(StelCore* core)
{
	Navigator* nav = core->getNavigation();
	Projector* prj = core->getProjection();
	
	prj->setCurrentFrame(Projector::FRAME_J2000);
	drawLines(prj);
	drawNames(prj);
	drawArt(prj, nav);
	drawBoundaries(prj);
	return 0.;
}

// Draw constellations art textures
void ConstellationMgr::drawArt(Projector * prj, const Navigator * nav) const
{
	glBlendFunc(GL_ONE, GL_ONE);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glEnable(GL_CULL_FACE);

	vector < Constellation * >::const_iterator iter;
	for (iter = asterisms.begin(); iter != asterisms.end(); ++iter)
	{
		(*iter)->drawArtOptim(prj, nav);
	}

	glDisable(GL_CULL_FACE);
}

// Draw constellations lines
void ConstellationMgr::drawLines(Projector * prj) const
{
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);

	vector < Constellation * >::const_iterator iter;
	for (iter = asterisms.begin(); iter != asterisms.end(); ++iter)
	{
		(*iter)->drawOptim(prj);
	}
}

// Draw the names of all the constellations
void ConstellationMgr::drawNames(Projector * prj) const
{
	glEnable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	
	glBlendFunc(GL_ONE, GL_ONE);
	// if (draw_mode == DM_NORMAL) glBlendFunc(GL_ONE, GL_ONE);
	// else glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // charting
	
	vector < Constellation * >::const_iterator iter;
	for (iter = asterisms.begin(); iter != asterisms.end(); iter++)
	{
		// Check if in the field of view
		if (prj->projectCheck((*iter)->XYZname, (*iter)->XYname))
			(*iter)->drawName(asterFont, prj);
	}
}

Constellation *ConstellationMgr::isStarIn(const StelObject* s) const
{
	vector < Constellation * >::const_iterator iter;
	for (iter = asterisms.begin(); iter != asterisms.end(); ++iter)
	{
		// Check if the star is in one of the constellation
		if ((*iter)->isStarIn(s)) 
		{
			return (*iter);
		}
	}
	return NULL;
}

Constellation* ConstellationMgr::findFromAbbreviation(const QString& abbreviation) const
{
	// search in uppercase only
	QString tname = abbreviation.toUpper();
	
	vector < Constellation * >::const_iterator iter;
	for (iter = asterisms.begin(); iter != asterisms.end(); ++iter)
	{
		if ((*iter)->abbreviation == tname)
			return (*iter);
	}
	return NULL;
}

// Can't find constellation from a position because it's not well localized
vector<StelObjectP> ConstellationMgr::searchAround(const Vec3d& v, double limitFov, const StelCore* core) const
{
	return vector<StelObjectP>(0);
}

void ConstellationMgr::loadNames(const QString& namesFile)
{
	// Constellation not loaded yet
	if (asterisms.empty()) return;
	
	// clear previous names
	vector < Constellation * >::const_iterator iter;
	for (iter = asterisms.begin(); iter != asterisms.end(); ++iter)
	{
		(*iter)->englishName.clear();
	}

	// Open file
	QFile commonNameFile(namesFile);
	if (!commonNameFile.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qDebug() << "Cannot open file" << namesFile;
		return;
	}

	// Now parse the file
	// lines to ignore which start with a # or are empty
	QRegExp commentRx("^(\\s*#.*|\\s*)$"); 

	// lines which look like records - we use the RE to extract the fields
	// which will be available in recRx.capturedTexts()
	QRegExp recRx("^\\s*(\\w+)\\s+\"(.*)\"\\s+_[(]\"(.*)\"[)]\\n");    

	// Some more variables to use in the parsing
	Constellation *aster;
	QString record, shortName;

	// keep track of how many records we processed.
	int totalRecords=0;
	int readOk=0; 
	int lineNumber=0;
	while (!commonNameFile.atEnd()) 
	{
		record = QString::fromUtf8(commonNameFile.readLine());
		lineNumber++;

		// Skip comments
		if (commentRx.exactMatch(record))
			continue;

		totalRecords++;

		if (!recRx.exactMatch(record))
		{
			qWarning() << "ERROR - cannot parse record at line" << lineNumber << "in constellation names file" << namesFile;
		}
		else
		{
			shortName = recRx.capturedTexts().at(1);
			aster = findFromAbbreviation(shortName);
			// If the constellation exists, set the English name
			if (aster != NULL)
			{
				aster->nativeName = recRx.capturedTexts().at(2);
				aster->englishName = recRx.capturedTexts().at(3);
				readOk++;
			}
			else 
			{
				qWarning() << "WARNING - constellation abbreviation" << shortName << "not found when loading constellation names";
			}
		}
	}
	commonNameFile.close();
	qDebug() << "Loaded" << readOk << "/" << totalRecords << "constellation names";
}

void ConstellationMgr::updateI18n()
{
	Translator trans(PACKAGE_NAME "-skycultures", StelApp::getInstance().getFileMgr().getLocaleDir(), StelApp::getInstance().getLocaleMgr().getSkyTranslator().getTrueLocaleName());
	vector < Constellation * >::const_iterator iter;
	for (iter = asterisms.begin(); iter != asterisms.end(); ++iter)
	{
		(*iter)->nameI18 = trans.qtranslate((*iter)->englishName);
	}
	asterFont = &StelApp::getInstance().getFontManager().getStandardFont(trans.getTrueLocaleName(), fontSize);
}

// update faders
void ConstellationMgr::update(double delta_time)
{
	vector < Constellation * >::const_iterator iter;
	for (iter = asterisms.begin(); iter != asterisms.end(); ++iter)
	{
		(*iter)->update((int)(delta_time*1000));
	}
}


void ConstellationMgr::setArtIntensity(double _max)
{
	artMaxIntensity = _max;
	vector < Constellation * >::const_iterator iter;
	for (iter = asterisms.begin(); iter != asterisms.end(); ++iter)
		(*iter)->art_fader.set_max_value(_max);
}

void ConstellationMgr::setArtFadeDuration(float duration)
{
	artFadeDuration = duration;
	vector < Constellation * >::const_iterator iter;
	for (iter = asterisms.begin(); iter != asterisms.end(); ++iter)
		(*iter)->art_fader.set_duration((int) (duration * 1000.f));
}

void ConstellationMgr::setFlagLines(bool b)
{
	flagLines = b;
	if (selected.begin() != selected.end()  && isolateSelected)
	{
		vector < Constellation * >::const_iterator iter;
		for (iter = selected.begin(); iter != selected.end(); ++iter)
			(*iter)->setFlagLines(b);
	}
	else
	{
		vector < Constellation * >::const_iterator iter;
		for (iter = asterisms.begin(); iter != asterisms.end(); ++iter)
			(*iter)->setFlagLines(b);
	}
}

void ConstellationMgr::setFlagBoundaries(bool b)
{
	flagBoundaries = b;
	if (selected.begin() != selected.end() && isolateSelected)
	{
		vector < Constellation * >::const_iterator iter;
		for (iter = selected.begin(); iter != selected.end(); ++iter)
			(*iter)->setFlagBoundaries(b);
	}
	else
	{
		vector < Constellation * >::const_iterator iter;
		for (iter = asterisms.begin(); iter != asterisms.end(); ++iter)
			(*iter)->setFlagBoundaries(b);
	}
}

void ConstellationMgr::setFlagArt(bool b)
{
	flagArt = b;
	if (selected.begin() != selected.end() && isolateSelected)
	{
		vector < Constellation * >::const_iterator iter;
		for (iter = selected.begin(); iter != selected.end(); ++iter)
			(*iter)->setFlagArt(b);
	}
	else
	{
		vector < Constellation * >::const_iterator iter;
		for (iter = asterisms.begin(); iter != asterisms.end(); ++iter)
			(*iter)->setFlagArt(b);
	}
}


void ConstellationMgr::setFlagNames(bool b)
{
	flagNames = b;
	if (selected.begin() != selected.end() && isolateSelected)
	{
		vector < Constellation * >::const_iterator iter;
		for (iter = selected.begin(); iter != selected.end(); ++iter)
			(*iter)->setFlagName(b);
	}
	else
	{
		vector < Constellation * >::const_iterator iter;
		for (iter = asterisms.begin(); iter != asterisms.end(); ++iter)
			(*iter)->setFlagName(b);
	}
}

StelObject* ConstellationMgr::getSelected(void) const {
	return *selected.begin();  // TODO return all or just remove this method
}

void ConstellationMgr::setSelected(const QString& abbreviation) 
{
	Constellation * c = findFromAbbreviation(abbreviation);
	if(c != NULL) setSelectedConst(c);
}

StelObjectP ConstellationMgr::setSelectedStar(const QString& abbreviation) 
{
	Constellation * c = findFromAbbreviation(abbreviation);
	if(c != NULL) {
		setSelectedConst(c);
		return c->getBrightestStarInConstellation();
	}
	return NULL;
}

void ConstellationMgr::setSelectedConst(Constellation * c)
{
	// update states for other constellations to fade them out
	if (c != NULL)
	{
		selected.push_back(c);

		// Propagate current settings to newly selected constellation
		c->setFlagLines(getFlagLines());
		c->setFlagName(getFlagNames());
		c->setFlagArt(getFlagArt());
		c->setFlagBoundaries(getFlagBoundaries());
	
		if (isolateSelected)
		{
			vector < Constellation * >::const_iterator iter;
			for (iter = asterisms.begin(); iter != asterisms.end(); ++iter)
			{

				bool match = 0;
				vector < Constellation * >::const_iterator s_iter;
				for (s_iter = selected.begin(); s_iter != selected.end(); ++s_iter)
				{
					if( (*iter)==(*s_iter) ) 
					{
						match=true; // this is a selected constellation
						break;  
					}
				}

				if(!match)
				{
					// Not selected constellation
					(*iter)->setFlagLines(false);
					(*iter)->setFlagName(false);
					(*iter)->setFlagArt(false);
					(*iter)->setFlagBoundaries(false);
		        	}
             		}
			Constellation::singleSelected = true;  // For boundaries
        	}
		else
			Constellation::singleSelected = false; // For boundaries
	}
	else
	{
		if (selected.begin() == selected.end()) return;

		// Otherwise apply standard flags to all constellations
		vector < Constellation * >::const_iterator iter;
		for (iter = asterisms.begin(); iter != asterisms.end(); ++iter)
		{
			(*iter)->setFlagLines(getFlagLines());
			(*iter)->setFlagName(getFlagNames());
			(*iter)->setFlagArt(getFlagArt());
			(*iter)->setFlagBoundaries(getFlagBoundaries());
		}

		// And remove all selections
		selected.clear();


	}
}


//! Remove a constellation from the selected constellation list
void ConstellationMgr::unsetSelectedConst(Constellation * c)
{
	if (c != NULL)
	{

		vector < Constellation * >::const_iterator iter;
		int n=0;
		for (iter = selected.begin(); iter != selected.end(); ++iter)
		{
			if( (*iter)->getEnglishName() == c->getEnglishName() ) 
			{
				selected.erase(selected.begin()+n, selected.begin()+n+1);
				iter--;
				n--;
			}
			n++;
		}

		// If no longer any selection, restore all flags on all constellations
		if (selected.begin() == selected.end()) { 

			// Otherwise apply standard flags to all constellations
			for (iter = asterisms.begin(); iter != asterisms.end(); ++iter)
			{
				(*iter)->setFlagLines(getFlagLines());
				(*iter)->setFlagName(getFlagNames());
				(*iter)->setFlagArt(getFlagArt());
				(*iter)->setFlagBoundaries(getFlagBoundaries());
			}

			Constellation::singleSelected = false; // For boundaries

		} else if(isolateSelected) {

			// No longer selected constellation
			c->setFlagLines(false);
			c->setFlagName(false);
			c->setFlagArt(false);
			c->setFlagBoundaries(false);

			Constellation::singleSelected = true;  // For boundaries
		}
	}
}


bool ConstellationMgr::loadBoundaries(const QString& boundaryFile)
{
	Constellation *cons = NULL;
	unsigned int i, j;
	
	// delete existing boundaries if any exist
	vector<vector<Vec3f> *>::iterator iter;
	for (iter = allBoundarySegments.begin(); iter != allBoundarySegments.end(); ++iter)
	{
		delete (*iter);
	}
	allBoundarySegments.clear();

	qDebug() << "Loading constellation boundary data ... ";

	// Modified boundary file by Torsten Bronger with permission
	// http://pp3.sourceforge.net
	QFile dataFile(boundaryFile);
	if (!dataFile.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qWarning() << "Boundary file " << boundaryFile << " not found";
		return false;
	}

	QTextStream istr(&dataFile);
	float DE, RA;
	float oDE, oRA;
	Vec3f XYZ;
	unsigned num, numc;
	vector<Vec3f> *points = NULL;
	QString consname;
	i = 0;
	while (!istr.atEnd())	
	{
		points = new vector<Vec3f>;

		num = 0;
		istr >> num;
		if(num == 0) continue;  // empty line

		for (j=0;j<num;j++)
		{
			istr >> RA >> DE;

			oRA =RA;
			oDE= DE;

			RA*=M_PI/12.;     // Convert from hours to rad
			DE*=M_PI/180.;    // Convert from deg to rad

			// Calc the Cartesian coord with RA and DE
			StelUtils::sphe_to_rect(RA,DE,XYZ);
			points->push_back(XYZ);
		}
		
		// this list is for the de-allocation
		allBoundarySegments.push_back(points);

		istr >> numc;  
		// there are 2 constellations per boundary
		
		for (j=0;j<numc;j++)
		{
			istr >> consname;
			// not used?
			if (consname == "SER1" || consname == "SER2") consname = "SER";
			
			cons = findFromAbbreviation(consname);
			if (!cons)
				qWarning() << "ERROR while processing boundary file - cannot find constellation: " << consname;
			else 
				cons->isolatedBoundarySegments.push_back(points);
		}

		if (cons) cons->sharedBoundarySegments.push_back(points);
		i++;

	}
	dataFile.close();
	qDebug() << "Loaded" << i << "constellation boundary segments";
	delete points;

	return true;
}

void ConstellationMgr::drawBoundaries(Projector * prj) const
{
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);

	glLineStipple(2, 0x3333);
	glEnable(GL_LINE_STIPPLE);

	vector < Constellation * >::const_iterator iter;
	for (iter = asterisms.begin(); iter != asterisms.end(); ++iter)
	{
		(*iter)->drawBoundaryOptim(prj);
	}
	glDisable(GL_LINE_STIPPLE);
}

///unsigned int ConstellationMgr::getFirstSelectedHP(void) {
///  if (selected) return selected->asterism[0]->get_hp_number();
///  return 0;
///}

StelObjectP ConstellationMgr::searchByNameI18n(const QString& nameI18n) const
{
	QString objw = nameI18n.toUpper();
	
	vector <Constellation*>::const_iterator iter;
	for (iter = asterisms.begin(); iter != asterisms.end(); ++iter)
	{
		QString objwcap = (*iter)->nameI18.toUpper();
		if (objwcap==objw) return *iter;
	}
	return NULL;
}

StelObjectP ConstellationMgr::searchByName(const QString& name) const
{
	QString objw = name.toUpper();
	vector <Constellation*>::const_iterator iter;
	for (iter = asterisms.begin(); iter != asterisms.end(); ++iter)
	{
		QString objwcap = (*iter)->englishName.toUpper();
		if (objwcap==objw) return *iter;

		objwcap = (*iter)->abbreviation.toUpper();
		if (objwcap==objw) return *iter;
	}
	return NULL;
}

QStringList ConstellationMgr::listMatchingObjectsI18n(const QString& objPrefix, int maxNbItem) const
{
	QStringList result;
	if (maxNbItem==0) return result;
		
	QString objw = objPrefix.toUpper();
	
	vector < Constellation * >::const_iterator iter;
	for (iter = asterisms.begin(); iter != asterisms.end(); ++iter)
	{
		QString constw = (*iter)->getNameI18n().mid(0, objw.size()).toUpper();
		if (constw==objw)
		{
			result << (*iter)->getNameI18n();
			if (result.size()==maxNbItem)
				return result;
		}
	}
	return result;
}
