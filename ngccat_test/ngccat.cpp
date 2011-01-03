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

//private:
//	friend struct DrawNebulaFuncObject;
	
	//! @enum NebulaType Nebula types
	enum NebulaType
	{
		NebGx,     //!< Galaxy
		NebOpenC,  //!< Open star cluster
		NebGlobC,  //!< Globular star cluster, usually in the Milky Way Galaxy
		NebN,      //!< Bright emission or reflection nebula [deprecated]
		NebPNe,    //!< Planetary nebula
		NebDn,     //!< ???
		NebIg,     //!< ???
		NebCn,     //!< Cluster associated with nebulosity [deprecated]
		NebUnknown, //!< Unknown type
		// NEW types added for W. Steinicke's catalogue
		NebGNe,		//!< Galactic nebula
		NebEmis,	//!< Emission nebula
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
#if 0
// Stellarium properties
	static StelTextureSP texCircle;   // The symbolic circle texture
	static float hintsBrightness;

	static Vec3f labelColor, circleColor;
	static float circleScale;       // Define the scaling of the hints circle
#endif	
};

typedef class Nebula*	NebulaP;

#define M_PI		3.1415926535
#define q_			QString

int main()
{
	QString catNGC = QString("/home/thomas/.stellarium/nebulae/default/ngc2000steinitz.dat");
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

	int idx = 5466;
	// Look up object
	for(QList<NebulaP>::const_iterator it = NGClist.begin();
		it != NGClist.end();
		++it)
	{
		if( (*it)->NGC_nb == idx)
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
	
	
	data = QString("Messier number #%1").arg(M_nb);
	qDebug() << data;
	data = QString("IC number #%1").arg(IC_nb);
	qDebug() << data;

	qDebug() << "Name: " << englishName << ", " << nameI18;
	qDebug() << "Constellation: " << constellationAbbr;
	
	data = QString("Size (arcmin, XxY): %1x%2").arg(sizeX).arg(sizeY);
	qDebug() << data;
	data = QString("PA (deg): %1").arg(PAdeg);
	qDebug() << data;
	data = QString("Surface brightness (mag/arcmin2): %1").arg(SBrightness);
	qDebug() << data;

	qDebug() << "Type: " << getTypeString();
	qDebug() << "Hubble type: " << hubbleType;
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
			wsType = q_("Galactic Nebula");
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
		case NebEmis:
			wsType = q_("Emission nebula");
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
	nb = record.mid(1,4).toInt();

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
	int NGCType = record.mid(8,1).toInt();
	
	// this is a huge performance drag if called every frame, so cache here
	switch (NGCType)
	{
		case 1: nType = NebGx; break;		// galaxy
		case 2: nType = NebGNe; break;		// galactic nebula (NEW)
		case 3: nType = NebPNe; break;		// planetary nebula
		case 4: nType = NebOpenC; break;	// open cluster
		case 5: nType = NebGlobC; break;	// globular cluster
		case 6: nType = NebEmis; break;		// emission nebula (NEW)
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
	// Dreyer object?
	if (record.mid(7,1) == QString("*"))
		bDreyerObject = true;
	else
		bDreyerObject = false;
	
	// Identification (object type) -- see above
	
	// Precision flags
	
	// Constellation
	constellationAbbr = record.mid(11,3);
	
	// right ascension
	const float raHrs = record.mid(15, 2).toFloat();
	const float raMins = record.mid(18, 2).toFloat();
	const float raSecs = record.mid(20, 3).toFloat();
	float ra = DMS(raHrs, raMins, raSecs).toDecimal();
	
	// declination
	const float decDegs = record.mid(24, 2).toFloat();
	const float decMins = record.mid(27, 2).toFloat();
	const float decSecs = record.mid(29, 2).toFloat();
	const QString decSgn = record.mid(23,1);
	float dec = DMS(decDegs, decMins, decSecs).toDecimal();
	if ( decSgn == QString("-")) dec *= -1.;
	
	// debug
	if (idx==5139)
	{
		qDebug("data: %s", record.toLatin1().data() );
		debugNGC(ra);
	}

	ra *= M_PI/12.;     // Convert from hours to rad
	dec *= M_PI/180.;    // Convert from deg to rad

	// Calc the Cartesian coord with RA and DE
	//StelUtils::spheToRect(ra, dec, XYZ);	

	// Read the blue (photographic) and visual magnitude
	mag = magB = record.mid(33, 4).toFloat();
	magV = record.mid(38, 4).toFloat();

	// surface brightness
	SBrightness = record.mid(42,4).toFloat();
	
	// Calc the angular size in radian
	// TODO: this should be independant of tex_angular_size
	// TODO: now have major and minor axes and PA available for galaxies
	// TODO: also have surface brightness (rendering option?)
	
	sizeX = record.mid(46, 4).toFloat();
	sizeY = record.mid(51, 3).toFloat();
	PAdeg = record.mid(54, 3).toFloat();
	float size;		// size (arcmin)
	if (sizeY > 0)
		size = 0.5 * (sizeX + sizeY);
	else
		size = sizeX;

	angularSize = size/60;
	if (angularSize<=0)
		angularSize=0.01;

	// Hubble classification
	hubbleType = record.mid(57,6);
	
	// TODO: PGC/GCGC/ other designations

	// TODO: where is this used?
	//pointRegion = SphericalRegionP(new SphericalPoint(getJ2000EquatorialPos(NULL)));
}

void Nebula::debugNGC(float ra)
{
	qDebug("RA: %f\n", ra);
	qDebug("For omegaCen, should be: 13hr 26m 47 == %f\n",
		13 +26.0/60 +47.0/3600);
}
