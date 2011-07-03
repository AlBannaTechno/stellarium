/*
 *	Test class for the NGC catalogue
 */

#include <QTextStream>
#include <QFile>
#include <QString>
#include <QDebug>

// Updated Nebula class based on catalogue of Wolfgang Steinicke
class Nebula
{
public:
	Nebula();
	~Nebula();

	//! Nebula support the following InfoStringGroup flags:
	//! - Name
	//! - CatalogNumber
	//! - Magnitude
	//! - RaDec
	//! - AltAzi
	//! - Distance
	//! - Size
	//! - Extra1 (contains the Nebula type, which might be "Galaxy", "Cluster" or similar)
	//! - PlainText
	//! @param core the Stelore object
	//! @param flags a set of InfoStringGroup items to include in the return value.
	//! @return a QString containing an HMTL encoded description of the Nebula.
#if 0
	virtual QString getInfoString(const StelCore *core, const InfoStringGroup& flags) const;
	virtual QString getType() const {return "Nebula";}
	virtual Vec3d getJ2000EquatorialPos(const StelNavigator*) const {return XYZ;}
	virtual double getCloseViewFov(const StelNavigator* nav = NULL) const;
	virtual float getVMagnitude(const StelNavigator* nav = NULL) const {Q_UNUSED(nav); return mag;}
	virtual float getSelectPriority(const StelNavigator *nav) const;
	virtual Vec3f getInfoColor() const;
	virtual QString getNameI18n() const {return nameI18;}
	virtual QString getEnglishName() const {return englishName;}
	virtual double getAngularSize(const StelCore*) const {return angularSize*0.5;}
	virtual SphericalRegionP getRegion() const {return pointRegion;}

	// Methods specific to Nebula
	void setLabelColor(const Vec3f& v) {labelColor = v;}
	void setCircleColor(const Vec3f& v) {circleColor = v;}
#endif

	//! Get the printable nebula Type.
	//! @return the nebula type code.
	QString getTypeString() const;
	
	QString getTypeDetailString(void) const;

//private:
//	friend struct DrawNebulaFuncObject;
	
	//! @enum NebulaType Nebula types
	enum NebulaType
	{
		NebGx,     //!< Galaxy
		NebPNe,    //!< Planetary nebula
		NebOpenC,  //!< Open star cluster
		NebGlobC,  //!< Globular star cluster, usually in the Milky Way Galaxy
		NebUnknown, //!< Unknown type
		// NEW types added for W. Steinicke's catalogue
		NebGNe,		//!< Galactic nebula/SN remnant
		NebDiffuse,	//!< Diffuse nebula/part of Galaxy, e.g. H-II regions
		NebCopy,	//!< WARNING: repeated object
		NebInNGC,	//!< WARNING: object already exists in NGC catalogue
		NebStar		//!< Nebula is actually a star
	};
#if 0
	//! Translate nebula name using the passed translator
	void translateName(StelTranslator& trans) {nameI18 = trans.qtranslate(englishName);}
#endif
	//void readNGC(QDataStream& in);	//TODO: implement binary file with new catalogue
	bool readNGC(QString& record);
	void readIdentifiers(const QString& record);
	void parseRecord(const QString& record, int idx);
	
	// debug only!
	void debugNGC(float ra);

	void listData() const;
#if 0
	void drawLabel(StelPainter& sPainter, float maxMagLabel);
	void drawHints(StelPainter& sPainter, float maxMagHints);
#endif
	unsigned int M_nb;              // Messier Catalog number
	unsigned int NGC_nb;            // New General Catalog number
	unsigned int IC_nb;             // Index Catalog number
	QString englishName;            // English name
	QString nameI18;                // Nebula name
	float mag;                      // Apparent magnitude
	float angularSize;              // Angular size in degree
	//Vec3d XYZ;                      // Cartesian equatorial position
	//Vec3d XY;                       // Store temporary 2D position
	NebulaType nType;

	// Position J2000.0
	float ra, dec;
	
	// Additional data from W. Steinicke's catalogue
	bool bDreyerObject;			//!< is it in original Dreyer catalogue?
	QString constellationAbbr;	//!< constellation abbrev.
	float magB;					//!< blue magnitude
	float magV;					//!< visual magnitude
	float SBrightness;			//!< surface brightness (mag/arcmin2)
	float sizeX;
	float sizeY;
	float PAdeg;				//!< principal angle (range 0..360 degrees)
	QString hubbleType;			//!< Hubble type for galaxies
	bool bNGCObject;
#if 0
// Stellarium properties
	static StelTextureSP texCircle;   // The symbolic circle texture
	static float hintsBrightness;

	static Vec3f labelColor, circleColor;
	static float circleScale;       // Define the scaling of the hints circle
#endif	
	private:
	float BmV;
	int PGC_nb;
	QString altDesig1;			//!< alternative designation 1
};

typedef class Nebula*	NebulaP;

#define M_PI		3.1415926535
#define q_			QString

int main(int argc, char **argv)
{
	if(argc != 3)
	{
		qDebug() << "Usage: ngccat_test N/I <number>";
		exit(0);
	}
	bool bNGC = (argv[1][0] == 'N') ? true : false;
	int idx = QString(argv[2]).toInt();
	
	QString catNGC = QString("/home/thomas/.stellarium/nebulae/default/ngc2011steinicke.dat");
	QList<NebulaP> NGClist;
	
	QFile in(catNGC);
	if (!in.open(QIODevice::ReadOnly | QIODevice::Text))
		return false;

	int totalRecords=0;
	QString record;
	while (!in.atEnd())
	{
		in.readLine();
		++totalRecords;
	}

	// rewind the file to the start
	in.seek(0);

	int currentLineNumber = 0;	// what input line we are on
	int currentRecordNumber = 0;	// what record number we are on
	int readOk = 0;			// how many records weree rad without problems
	while (!in.atEnd())
	{
		record = QString::fromUtf8(in.readLine());
		++currentLineNumber;

		// skip comments
		if (record.startsWith("//") || record.startsWith("#"))
			continue;
		++currentRecordNumber;

		// Create a new Nebula record
		NebulaP e = NebulaP(new Nebula);
		if (!e->readNGC(record)) // reading error
		{
			//e.clear();
			delete(e);
		}
		else
		{
			// do stuff
			NGClist.append(e);
			++readOk;
		}
	}
	in.close();
	qDebug() << "Loaded" << readOk << "/" << totalRecords << "NGC records";

	//idx = 5466;
	
	// Look up object
	for(QList<NebulaP>::const_iterator it = NGClist.begin();
		it != NGClist.end();
		++it)
	{
		if( ((*it)->NGC_nb == idx) && ((*it)->bNGCObject == bNGC))
		{
			(*it)->listData();
		}
	}
}

/*********************************************************
 Nebula methods
*********************************************************/

Nebula::Nebula() :
		M_nb(0),
		NGC_nb(0),
		IC_nb(0)
{
	nameI18 = "";
	angularSize = -1;
}

Nebula::~Nebula()
{
}

void Nebula::listData() const
{
	QString data;
	
	data = QString("NGC #%1").arg(NGC_nb);
	qDebug() << data;
	data = QString("Magnitude: %1").arg(mag);
	qDebug() << data;
	data = QString("B magnitude: %1").arg(magB);
	qDebug() << data;
	data = QString("V magnitude: %1").arg(magV);
	qDebug() << data;
	data = QString("B-V colour: %1").arg(BmV);
	qDebug() << data;
	
	data = QString("R.A. (J2000.0): %1").arg(ra);
	qDebug() << data;
	data = QString("Declination (J2000.0): %1").arg(dec);
	qDebug() << data;
	
	data = QString("Messier number #%1").arg(M_nb);
	qDebug() << data;
	data = QString("IC number #%1").arg(IC_nb);
	qDebug() << data;
	data = QString("PGC number #%1").arg(PGC_nb);
	qDebug() << data;
	qDebug() << "Alt. designation: " << altDesig1;

	qDebug() << "Name: " << englishName << ", " << nameI18;
	qDebug() << "Constellation: " << constellationAbbr;
	
	data = QString("Size (arcmin, XxY): %1x%2").arg(sizeX).arg(sizeY);
	qDebug() << data;
	data = QString("PA (deg): %1").arg(PAdeg);
	qDebug() << data;
	data = QString("Surface brightness (mag/arcmin2): %1").arg(SBrightness);
	qDebug() << data;

	qDebug() << "Type: " << getTypeString();
	qDebug() << "Sub-type(s): " << getTypeDetailString();
	qDebug() << "Hubble type: " << hubbleType;
}

QString Nebula::getTypeDetailString(void) const
{
	QString wsType;
	
	// Parse the abbreviations in the 'Hubble type' field
	switch(nType)
	{
		case NebGx:
		{
			char* type = hubbleType.toLatin1().data();
			switch(type[0])
			{
				case 'C':
					wsType = q_("Compact");
					break;
				case 'D':
					wsType = q_("Dwarf");
					break;
				case 'E':
					wsType = q_("Elliptical");
					break;
				case 'I':
					wsType = q_("Irregular");
					break;
				case 'P':
					wsType = q_("Peculiar");
					break;
				case 'S':
					wsType = q_("Spiral");
					break;
				default:
					break;
			}
			wsType += ", " + hubbleType;
			break;
		}	
		case NebGlobC:
			// concentration class, otherwise GCL
			if (hubbleType.contains("I") || hubbleType.contains("V") || hubbleType.contains("X"))
			{
				wsType = q_("Concentration class %1").arg(hubbleType);
				break;
			}
			
			// Following objects share abbreviations
		case NebOpenC:
			// Trumpler class, otherwise OCL (sometimes EN too)
			if (hubbleType.contains("I") || hubbleType.contains("V"))
			{
				wsType = q_("Trumpler class %1").arg(hubbleType);
				break;
			}
		case NebPNe:
			// PN, sometimes OCL too
		case NebGNe:
			// EN, RN, sometimes OCL too
			if (hubbleType.contains("EN"))
				wsType += "Emission nebula; ";
			if (hubbleType.contains("DN"))
				wsType += "Dark nebula; ";
			if (hubbleType.contains("RN"))
				wsType += "Reflection nebula; ";
			if (hubbleType.contains("PN"))
				wsType += "Planetary nebula; ";
			if (hubbleType.contains("SNR"))
				wsType += "Supernova remnant; ";

			if (hubbleType.contains("OCL"))
				wsType += "Open cluster; ";
			if (hubbleType.contains("GCL"))
				wsType += "Globular cluster; ";

		// NEW types added for W. Steinicke's catalogue
		case NebDiffuse:
			//!< (6) Diffuse nebula/part of Galaxy, e.g. H-II regions
			// All GxyP in 2010 edition of catalogue
			break;
		case NebCopy:
			//!< (7) WARNING: repeated object
		case NebInNGC:
			//!< (8) WARNING: object already exists in NGC catalogue
			// Check field which stores true NGC identification number
			wsType = "Repeated object, see " + altDesig1;
			break;
		case NebStar:
			//!< (9) Nebula is actually a star
			// * followed by 1,2,Group or Cloud
		case NebUnknown:
			//!< Unknown type
			break;
		default:
			break;
	}
	
	return wsType;
}

QString Nebula::getTypeString(void) const
{
	QString wsType;

	switch(nType)
	{
		case NebGx:
			wsType = q_("Galaxy");
			break;
		case NebGNe:
			wsType = q_("Galactic nebula");
			break;
		case NebPNe:
			wsType = q_("Planetary nebula");
			break;
		case NebOpenC:
			wsType = q_("Open cluster");
			break;
		case NebGlobC:
			wsType = q_("Globular cluster");
			break;
		case NebDiffuse:
			wsType = q_("Diffuse nebula");
			break;
		case NebStar:
			wsType = q_("Star"); // TODO: May look strange to stellarium users?
			break;
		case NebUnknown:
			wsType = q_("Unknown");
			break;
		default:
			wsType = q_("Undocumented type");
			break;
	}
	return wsType;
}

bool Nebula::readNGC(QString& record)
{
	int nb;

	// TODO: No longer purely numeric, e.g. NGC 5144A and NGC 5144B
	// TODO: Number of components also
	nb = record.mid(2,6).toInt();

	// Is it NGC or IC object?
	if (record.mid(0,1) == QString("I"))
	{
		IC_nb = nb;
	}
	else
	{
		NGC_nb = nb;
	}

	readIdentifiers(record);
	parseRecord(record, nb);
	
	return true;
}

void Nebula::readIdentifiers(const QString& record)
{
	int NGCType = record.mid(15, 2).toInt();
	
	// this is a huge performance drag if called every frame, so cache here
	switch (NGCType)
	{
		case 1: nType = NebGx; break;		// galaxy
		case 2: nType = NebGNe; break;		// galactic nebula/SN remnant (NEW)
		case 3: nType = NebPNe; break;		// planetary nebula
		case 4: nType = NebOpenC; break;	// open cluster
		case 5: nType = NebGlobC; break;	// globular cluster
		case 6: nType = NebDiffuse; break;	// part of Galaxy/diffuse emission, e.g. H-II regions (NEW)
		case 7: nType = NebCopy; break;		// !repeated object! (NEW)
		case 8: nType = NebInNGC; break;	// !object in NGC catalogue! (NEW)
		case 9: nType = NebStar; break;		// star (NEW)
		default:  nType = NebUnknown;
	}	
}

//! \brief Degrees (or Hours), Mins, Secs to decimal
struct DMS
{
	float Degrees, Minutes, Secs;
	
	DMS(const float _degs, const float _mins, const float _secs)
		: Degrees(_degs), Minutes(_mins), Secs(_secs)
	{}
	
	float toDecimal()
	{
		return Degrees + Minutes/60.0 + Secs/3600.0;
	}	
};

//! \brief Read the following positional info:
//		o constellation
//		o RA, declination
//		o B,V mag, surface bightness
//		o Semimajor/semiminor axis
//		o PA (degrees)
//		o Type
void Nebula::parseRecord(const QString& record, int idx)
{
	// In NGC catalogue?
	bNGCObject = (record.mid(0, 1) == QString("N")) ? true : false;
	
	// Dreyer object?
	if (record.mid(12, 1) == QString("*"))
		bDreyerObject = true;
	else
		bDreyerObject = false;
	
	// Identification (object type) -- see above
	
	// Precision flags
	
	// Constellation
	constellationAbbr = record.mid(20,3);
	
	// right ascension
	const float raHrs = record.mid(27, 2).toFloat();
	const float raMins = record.mid(31, 2).toFloat();
	const float raSecs = record.mid(34, 4).toFloat();
	ra = DMS(raHrs, raMins, raSecs).toDecimal();
	
	// declination
	const float decDegs = record.mid(42, 2).toFloat();
	const float decMins = record.mid(46, 2).toFloat();
	const float decSecs = record.mid(50, 2).toFloat();
	const QString decSgn = record.mid(39, 1);
	dec = DMS(decDegs, decMins, decSecs).toDecimal();
	if ( decSgn == QString("-")) dec *= -1.;
	
	// debug
	if (idx==5139)
	{
		qDebug("data: %s", record.toLatin1().data() );
		debugNGC(ra);
	}

	//ra *= M_PI/12.;     // Convert from hours to rad
	//dec *= M_PI/180.;    // Convert from deg to rad

	// Calc the Cartesian coord with RA and DE
	//StelUtils::spheToRect(ra, dec, XYZ);	

	// Read the blue (photographic) and visual magnitude
	magB = record.mid(56, 4).toFloat();
	mag = magV = record.mid(64, 4).toFloat();

	// B-V colour
	BmV = record.mid(70, 3).toFloat();
	
	// surface brightness
	SBrightness = record.mid(74,4).toFloat();
	
	// Calc the angular size in radian
	// TODO: this should be independant of tex_angular_size
	// TODO: now have major and minor axes and PA available for galaxies
	// TODO: also have surface brightness (rendering option?)
	
	sizeX = record.mid(81, 5).toFloat();
	sizeY = record.mid(89, 5).toFloat();
	PAdeg = record.mid(95, 3).toFloat();
	float size;		// size (arcmin)
	if (sizeY > 0)
		size = 0.5 * (sizeX + sizeY);
	else
		size = sizeX;

	angularSize = size/60;

	// Hubble classification
	hubbleType = record.mid(98, 9);
	
	// TODO: PGC/GCGC/ other designations
	PGC_nb = record.mid(113, 6).toInt();
	altDesig1 = record.mid(119, 27);
	
	// Messier number
	if (altDesig1.contains("M "))
	{
		M_nb = altDesig1.mid(2, 3).toInt();
	}
	
	// TODO: where is this used?
	//pointRegion = SphericalRegionP(new SphericalPoint(getJ2000EquatorialPos(NULL)));
}

void Nebula::debugNGC(float ra)
{
	qDebug("RA: %f\n", ra);
	qDebug("For omegaCen, should be: 13hr 26m 47 == %f\n",
		13 +26.0/60 +47.0/3600);
}
