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

#include <iostream>
#include <fstream>
#include <vector>

#include "constellation.h"
#include "constellation_mgr.h"
#include "stel_utility.h"

// constructor which loads all data from appropriate files
ConstellationMgr::ConstellationMgr(HipStarMgr *_hip_stars) : 
	asterFont(NULL),
	hipStarMgr(_hip_stars),
	selected(NULL)
{
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

	if (asterFont) delete asterFont;
	asterFont = NULL;

	vector<vector<Vec3f> *>::iterator iter1;
	for (iter1 = allBoundarySegments.begin(); iter1 != allBoundarySegments.end(); ++iter1)
	{
		delete (*iter1);
	}
	allBoundarySegments.clear();
}

void ConstellationMgr::setFont(float font_size, const string& ttfFileName)
{
	if (asterFont) delete asterFont;
	asterFont = new s_font(font_size, ttfFileName);
	assert(asterFont);
}

// Load line and art data from files
void ConstellationMgr::loadLinesAndArt(const string &fileName, const string &artfileName, const string &boundaryfileName, LoadingBar& lb)
{
	// Store state
	bool flagArt = getFlagArt();
	bool flagLines = getFlagLines();
	bool flagNames = getFlagNames();
	
	std::ifstream inf(fileName.c_str());

	if (!inf.is_open())
	{
		printf("Can't open constellation data file %s\n", fileName.c_str());
		assert(0);
	}

	// delete existing data, if any
	vector < Constellation * >::iterator iter;
	for (iter = asterisms.begin(); iter != asterisms.end(); ++iter)
	{
		delete(*iter);
	}
	asterisms.clear();

	Constellation *cons = NULL;

	string record;
	int line=0;
	while(!std::getline(inf, record).eof())
	{
		line++;
		cons = new Constellation;
		assert(cons);
		if(cons->read(record, hipStarMgr))
		{
			asterisms.push_back(cons);
		} else
		{ 
			cerr << "ERROR on line " << line << "of " << fileName.c_str() << endl;
			delete cons;
		}
	}
	inf.close();

	FILE *fic = fopen(artfileName.c_str(), "r");
	if (!fic)
	{
		cerr << "Can't open " << artfileName.c_str() << endl;
		return; // no art, but still loaded constellation data
	}

	// Read the constellation art file with the following format :
	// ShortName texture_file x1 y1 hp1 x2 y2 hp2
	// Where :
	// shortname is the international short name (i.e "Lep" for Lepus)
	// texture_file is the graphic file of the art texture
	// x1 y1 are the x and y texture coordinates in pixels of the star of hipparcos number hp1
	// x2 y2 are the x and y texture coordinates in pixels of the star of hipparcos number hp2
	// The coordinate are taken with (0,0) at the top left corner of the image file
	char shortname[20];
	char texfile[255];
	unsigned int x1, y1, x2, y2, x3, y3, hp1, hp2, hp3;
	int texSize;
	
	char tmpstr[2000];
	int total = 0;

	// determine total number to be loaded for percent complete display
	while (fgets(tmpstr, 2000, fic)) {++total;}
	rewind(fic);

	int current = 0;

	while (!feof(fic))
	{
		if (fscanf(fic, "%s %s %u %u %u %u %u %u %u %u %u\n", shortname, texfile, &x1, &y1, &hp1, &x2, &y2, &hp2, &x3, &y3, &hp3) != 11)
		{
			if (feof(fic))
			{
				// Empty constellation file
				fclose(fic);
				return;		// no art is OK
			}
			cerr << "Error while loading art for constellation " <<  shortname << endl;;
			assert(0);
		}

		// Draw loading bar
        wostringstream os;
		os << _("Loading Constellation Art: ") << current+1 << L"/" << total;
		lb.SetMessage(os.str());
		lb.Draw((float)(current+1)/total);
		
		cons = NULL;
		cons = findFromAbbreviation(shortname);
		if (!cons)
		{
			cerr << "ERROR : Can't find constellation called : " << shortname << endl;
		}
		else
		{
			cons->art_tex = new s_texture(texfile);
			texSize = cons->art_tex->getSize();

			Vec3f s1 = hipStarMgr->searchHP(hp1)->get_j2000_pos();
			Vec3f s2 = hipStarMgr->searchHP(hp2)->get_j2000_pos();
			Vec3f s3 = hipStarMgr->searchHP(hp3)->get_j2000_pos();

			// To transform from texture coordinate to 2d coordinate we need to find X with XA = B
			// A formed of 4 points in texture coordinate, B formed with 4 points in 3d coordinate
			// We need 3 stars and the 4th point is deduced from the other to get an normal base
			// X = B inv(A)
			Vec3f s4 = s1 + (s2 - s1) ^ (s3 - s1);
			Mat4f B(s1[0], s1[1], s1[2], 1, s2[0], s2[1], s2[2], 1, s3[0], s3[1], s3[2], 1, s4[0], s4[1], s4[2], 1);
			Mat4f A(x1, texSize - y1, 0.f, 1.f, x2, texSize - y2, 0.f, 1.f, x3, texSize - y3, 0.f, 1.f, x1, texSize - y1, texSize, 1.f);
			Mat4f X = B * A.inverse();

			cons->art_vertex[0] = Vec3f(X * Vec3f(0, 0, 0));
			cons->art_vertex[1] = Vec3f(X * Vec3f(texSize / 2, 0, 0));
			cons->art_vertex[2] = Vec3f(X * Vec3f(texSize / 2, texSize / 2, 0));
			cons->art_vertex[3] = Vec3f(X * Vec3f(0, texSize / 2, 0));
			cons->art_vertex[4] = Vec3f(X * Vec3f(texSize / 2 + texSize / 2, 0, 0));
			cons->art_vertex[5] = Vec3f(X * Vec3f(texSize / 2 + texSize / 2, texSize / 2, 0));
			cons->art_vertex[6] = Vec3f(X * Vec3f(texSize / 2 + texSize / 2, texSize / 2 + texSize / 2, 0));
			cons->art_vertex[7] = Vec3f(X * Vec3f(texSize / 2 + 0, texSize / 2 + texSize / 2, 0));
			cons->art_vertex[8] = Vec3f(X * Vec3f(0, texSize / 2 + texSize / 2, 0));

			current++;
		}
	}
	fclose(fic);
	
	loadBoundaries(boundaryfileName);
	
	// Restore state
	setFlagArt(flagArt);
	setFlagLines(flagLines);
	setFlagNames(flagNames);	
}

void ConstellationMgr::draw(Projector * prj, Navigator * nav) const
{
	prj->set_orthographic_projection();
	draw_lines(prj);
	draw_names(prj);
	draw_art(prj, nav);
	drawBoundaries(prj);
	prj->reset_perspective_projection();
}

// Draw constellations art textures
void ConstellationMgr::draw_art(Projector * prj, Navigator * nav) const
{
	glBlendFunc(GL_ONE, GL_ONE);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glEnable(GL_CULL_FACE);

	vector < Constellation * >::const_iterator iter;
	for (iter = asterisms.begin(); iter != asterisms.end(); ++iter)
	{
		(*iter)->draw_art_optim(prj, nav);
	}

	glDisable(GL_CULL_FACE);
}

// Draw constellations lines
void ConstellationMgr::draw_lines(Projector * prj) const
{
	glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);

	vector < Constellation * >::const_iterator iter;
	for (iter = asterisms.begin(); iter != asterisms.end(); ++iter)
	{
		(*iter)->draw_optim(prj);
	}
}

// Draw the names of all the constellations
void ConstellationMgr::draw_names(Projector * prj) const
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
		if (prj->project_j2000_check((*iter)->XYZname, (*iter)->XYname))
			(*iter)->draw_name(asterFont, prj);
	}
}

Constellation *ConstellationMgr::is_star_in(const HipStar * s) const
{
	vector < Constellation * >::const_iterator iter;
	for (iter = asterisms.begin(); iter != asterisms.end(); ++iter)
	{
		// Check if the star is in one of the constellation
		if ((*iter)->is_star_in(s))
			return (*iter);
	}
	return NULL;
}

Constellation *ConstellationMgr::findFromAbbreviation(const string & abbreviation) const
{
	// search in uppercase only
	string tname = abbreviation;
	transform(tname.begin(), tname.end(), tname.begin(),::toupper);
	
	vector < Constellation * >::const_iterator iter;
	for (iter = asterisms.begin(); iter != asterisms.end(); ++iter)
	{
		// Check if the star is in one of the constellation
		if (string((*iter)->abbreviation) == tname)
			return (*iter);
	}
	return NULL;
}


/** 
 * @brief Read constellation names from the given file
 * @param namesFile Name of the file containing the constellation names in english
 */
void ConstellationMgr::loadNames(const string& namesFile)
{
	// Constellation not loaded yet
	if (asterisms.empty()) return;
	
	// clear previous names
	vector < Constellation * >::const_iterator iter;
	for (iter = asterisms.begin(); iter != asterisms.end(); ++iter)
	{
		(*iter)->englishName.clear();
	}

	// read in translated common names from file
	ifstream commonNameFile(namesFile.c_str());
	if (!commonNameFile.is_open())
	{
		cout << "Can't open file" << namesFile << endl;
		return;
	}
	
	// find matching constellation and update name
	string record;
	string tmpShortName;
	Constellation *aster;
	while (!std::getline(commonNameFile, record).eof())
	{
		istringstream in(record); 
		in >> tmpShortName;
		aster = findFromAbbreviation(tmpShortName);
		if (aster != NULL)
		{
			// Read the names in english
			aster->englishName = record.substr(tmpShortName.length()+1,record.length()).c_str();
		}
	}
	commonNameFile.close();

}

/** 
 * @brief Update i18 names from english names according to current locale
 * The translation is done using gettext with translated strings defined in translations.h
 */
void ConstellationMgr::translateNames(Translator& trans)
{
	vector < Constellation * >::const_iterator iter;
	for (iter = asterisms.begin(); iter != asterisms.end(); ++iter)
	{
		(*iter)->nameI18 = trans.translate((*iter)->englishName.c_str());
		//cout << (*iter)->englishName.c_str() << " -> " << StelUtility::wstringToString((*iter)->nameI18) << endl;
	}
}

vector <wstring> ConstellationMgr::getNames(void) 
{
     vector<wstring> names;
  	 vector < Constellation * >::const_iterator iter;
     wstring name;

	for (iter = asterisms.begin(); iter != asterisms.end(); ++iter)
	{
		name = (*iter)->getNameI18();
		transform(name.begin(), name.end(), name.begin(), ::tolower);
		transform(name.begin(), name.begin()+1, name.begin(), ::toupper);
        names.push_back(name);
     }
     return names;
}

vector<string> ConstellationMgr::getShortNames(void)
{
    vector<string> names;
	vector < Constellation * >::const_iterator iter;

	for (iter = asterisms.begin(); iter != asterisms.end(); ++iter)
        names.push_back((*iter)->getShortName());
    return names;
}

string ConstellationMgr::getShortNameByNameI18(wstring _name) {

	vector < Constellation * >::const_iterator iter;

	for (iter = asterisms.begin(); iter != asterisms.end(); ++iter)
	{
		if( _name==(*iter)->getNameI18()) return (*iter)->getShortName();
	}
	return "";

}


// update faders
void ConstellationMgr::update(int delta_time)
{
	vector < Constellation * >::const_iterator iter;
	for (iter = asterisms.begin(); iter != asterisms.end(); ++iter)
	{
		(*iter)->update(delta_time);
    }
}


void ConstellationMgr::setArtIntensity(float _max)
{
	vector < Constellation * >::const_iterator iter;
	for (iter = asterisms.begin(); iter != asterisms.end(); ++iter)
		(*iter)->art_fader.set_max_value(_max);
}

void ConstellationMgr::setArtFadeDuration(float duration)
{
	vector < Constellation * >::const_iterator iter;
	for (iter = asterisms.begin(); iter != asterisms.end(); ++iter)
		(*iter)->art_fader.set_duration((int) (duration * 1000.f));
}

void ConstellationMgr::setFlagLines(bool b)
{
	if (selected && isolateSelected)
	{
		selected->setFlagLines(b);
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
	if (selected && isolateSelected)
	{
		selected->setFlagBoundaries(b);
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
	if (selected && isolateSelected)
	{
		selected->setFlagArt(b);
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
	if (selected && isolateSelected)
	{
		selected->setFlagName(b);
	}
	else
	{
		vector < Constellation * >::const_iterator iter;
		for (iter = asterisms.begin(); iter != asterisms.end(); ++iter)
			(*iter)->setFlagName(b);
	}
}

void ConstellationMgr::setSelectedConst(Constellation * c)
{
	// update states for other constellations to fade them out
	if (c != NULL)
	{
		Constellation* cc;
		// Propagate old parameters new newly selected constellation
		if (selected) cc=selected;
		else cc=*(asterisms.begin());
		c->setFlagLines(cc->getFlagLines());
		c->setFlagName(cc->getFlagName());
		c->setFlagArt(cc->getFlagArt());
		c->setFlagBoundaries(cc->getFlagBoundaries());
		selected = c;
				
		if (isolateSelected)
		{
		    vector < Constellation * >::const_iterator iter;
		    for (iter = asterisms.begin(); iter != asterisms.end(); ++iter)
		    {
                if ((*iter) != selected)
	    	    {
	    	        (*iter)->setFlagLines(false);
		            (*iter)->setFlagName(false);
		            (*iter)->setFlagArt(false);
		            (*iter)->setFlagBoundaries(false);
		        }
             }
             Constellation::singleSelected = true;
        }
		else
		{
			vector < Constellation * >::const_iterator iter;
			for (iter = asterisms.begin(); iter != asterisms.end(); ++iter)
			{
					(*iter)->setFlagLines(c->getFlagLines());
					(*iter)->setFlagName(c->getFlagName());
					(*iter)->setFlagArt(c->getFlagArt());
		            (*iter)->setFlagBoundaries(c->getFlagBoundaries());
			}
             Constellation::singleSelected = false;
		}
	}
	else
	{
		if (selected==NULL) return;
		vector < Constellation * >::const_iterator iter;
		for (iter = asterisms.begin(); iter != asterisms.end(); ++iter)
		{
			if ((*iter) != selected)
			{
				(*iter)->setFlagLines(selected->getFlagLines());
				(*iter)->setFlagName(selected->getFlagName());
				(*iter)->setFlagArt(selected->getFlagArt());
				(*iter)->setFlagBoundaries(selected->getFlagBoundaries());
			}
		}
		selected = NULL;
	}
}

// Load from file 
bool ConstellationMgr::loadBoundaries(const string& boundaryFile)
{
	Constellation *cons = NULL;
	unsigned int i, j;
	
	vector<vector<Vec3f> *>::iterator iter;
	for (iter = allBoundarySegments.begin(); iter != allBoundarySegments.end(); ++iter)
	{
		delete (*iter);
	}
	allBoundarySegments.clear();

    cout << "Loading Constellation boundary data...";
	// Modified boundary file by Torsten Bronger with permission
	// http://pp3.sourceforge.net
	
    ifstream dataFile;
	dataFile.open(boundaryFile.c_str());
    if (!dataFile.is_open())
    {
        cerr << "Boundary file " << boundaryFile << " not found" << endl;
        return false;
    }

	float DE, RA;
	float oDE, oRA;
	Vec3f XYZ;
	unsigned num, numc;
	vector<Vec3f> *points = NULL;
	string consname;
	i = 0;
	while (!dataFile.eof())	
	{
		points = new vector<Vec3f>;

		num = 0;
		dataFile >> num;
		if(num == 0) continue;  // empty line

		for (j=0;j<num;j++)
		{
			dataFile >> RA >> DE;

			oRA =RA;
			oDE= DE;

			RA*=M_PI/12.;     // Convert from hours to rad
			DE*=M_PI/180.;    // Convert from deg to rad

			// Calc the Cartesian coord with RA and DE
	    	sphe_to_rect(RA,DE,XYZ);
			points->push_back(XYZ);
		}
		
		// this list is for the de-allocation
		allBoundarySegments.push_back(points);

		dataFile >> numc;  
		// there are 2 constellations per boundary
		
		for (j=0;j<numc;j++)
		{
			dataFile >> consname;
			// not used?
			if (consname == "SER1" || consname == "SER2") consname = "SER";
			
			cons = findFromAbbreviation(consname);
				if (!cons) cout << "ERROR : Can't find constellation called : " << consname << endl;
			else cons->isolatedBoundarySegments.push_back(points);
		}

		if (cons) cons->sharedBoundarySegments.push_back(points);
		i++;

	}
    dataFile.close();
	cout << "(" << i << " segments loaded)" << endl;
    delete points;
    
    return true;
}

// Draw constellations lines
void ConstellationMgr::drawBoundaries(Projector * prj) const
{
	glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);

	glLineStipple(2, 0x3333);
	glEnable(GL_LINE_STIPPLE);

	vector < Constellation * >::const_iterator iter;
	for (iter = asterisms.begin(); iter != asterisms.end(); ++iter)
	{
		(*iter)->draw_boundary_optim(prj);
	}
	glDisable(GL_LINE_STIPPLE);
}
