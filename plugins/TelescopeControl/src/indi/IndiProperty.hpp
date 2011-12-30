/*
 * Qt-based INDI wire protocol client
 * 
 * Copyright (C) 2010-2011 Bogdan Marinov
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _INDI_PROPERTY_HPP_
#define _INDI_PROPERTY_HPP_

#include <QDateTime>
#include <QHash>
#include <QString>
#include <QXmlStreamReader>

#include "IndiTypes.hpp"
#include "IndiElement.hpp"

//! Base class for data structures for the attributes of a vector tag
class TagAttributes
{
public:
	TagAttributes (const QXmlStreamReader& xmlStream);
	
	//! Attempts to read the \b timestamp attribute of the current element.
	//! \returns a UTC datetime if the timestamp can be parsed, otherwise
	//! an invalid QDateTime object.
	static QDateTime readTimestampAttribute(const QXmlStreamAttributes& attributes);
	
	bool areValid;
	
	QString device;
	QString name;
	QString timeoutString;
	QDateTime timestamp;
	QString message;
	
	//INDI XML attributes
	static const char* VERSION;
	static const char* DEVICE;
	static const char* NAME;
	static const char* LABEL;
	static const char* GROUP;
	static const char* STATE;
	static const char* PERMISSION;
	static const char* TIMEOUT;
	static const char* TIMESTAMP;
	static const char* MESSAGE;
	static const char* FORMAT;
	static const char* MINIMUM;
	static const char* MAXIMUM;
	static const char* STEP;
	static const char* RULE;
	static const char* SIZE;
	
protected:
	QXmlStreamAttributes attributes;
};

//! Self-populating data structure for the attributes of a defintion tag.
//! This class should be used in its pure form only in LightProperty only.
//! For everything else, use StandardPropertyDefinitionAttributes and
//! SwitchPropertyDefinitionAttributes.
class BasicDefTagAttributes : public TagAttributes
{
public:
	BasicDefTagAttributes (const QXmlStreamReader& xmlReader);
	
	QString label;
	QString group;
	State state;
};

//! Self-populating data structure for the attributes of a defintion tag.
class StandardDefTagAttributes :
        public BasicDefTagAttributes
{
public:
	StandardDefTagAttributes (const QXmlStreamReader& xmlReader);
	
	Permission permission;
};

//! Self-populating data structure for the attributes of a defintion tag.
class DefSwitchTagAttributes :
        public StandardDefTagAttributes
{
public:
	DefSwitchTagAttributes (const QXmlStreamReader& xmlReader);
	
	SwitchRule rule;
};

class SetTagAttributes : public TagAttributes
{
public:
	SetTagAttributes (const QXmlStreamReader& xmlReader);
	
	bool stateChanged;
	State state;
};

//! Base class of all property classes.
//TODO: Base class or template?
class Property
{
public:
	enum PropertyType {
		TextProperty,
		NumberProperty,
		SwitchProperty,
		LightProperty,
		BlobProperty
	};

	Property(const QString& propertyName,
	         State propertyState,
	         Permission accessPermission,
	         const QString& propertyLabel = QString(),
	         const QString& propertyGroup = QString(),
	         const QDateTime& firstTimestamp = QDateTime());
	Property(const BasicDefTagAttributes& attributes);
	virtual ~Property();
	PropertyType getType() const;
	QString getName();
	QString getLabel();
	QString getGroup();
	bool isReadable();
	bool isWritable();
	Permission getPermission() const;
	void setState(State newState);
	State getCurrentState() const;
	QDateTime getTimestamp() const;
	qint64 getTimestampInMilliseconds() const;
	virtual int elementCount() const = 0;
	virtual QStringList getElementNames() const = 0;

protected:
	//! Sets the timestamp value. If necessary, reinterprets the data as UTC.
	//! If the argument is not a valid QDateTime, uses the current moment.
	void setTimestamp(const QDateTime& timestamp);
	//! Property type
	PropertyType type;

	//! Name used to identify the property internally.
	QString name;
	//! Human-readable label used to represent the property in the GUI.
	//! If not specified, the value of #name is used.
	QString label;
	//! Group name.
	QString group;
	//! Permission limiting client-side actions on this property.
	Permission permission;
	//! Current state of the property
	State state;
	//! Time in seconds before the next expected change in value.
	double timeout;
	//! Time of the last change (UTC)
	QDateTime timestamp;
};

//! A vector/array of string values (TextElement-s).
class TextProperty : public Property
{
public:
	TextProperty(const QString& propertyName,
	             State propertyState,
	             Permission accessPermission,
	             const QString& propertyLabel = QString(),
	             const QString& propertyGroup = QString(),
	             const QDateTime& timestamp = QDateTime());
	TextProperty(const StandardDefTagAttributes& attributes);
	~TextProperty();

	void addElement(TextElement* element);
	TextElement* getElement(const QString& name);

	int elementCount() const;
	QStringList getElementNames() const;

private:
	QHash<QString,TextElement*> elements;
};

//! A vector/array of numeric values (NumberElement-s).
class NumberProperty : public Property
{
public:
	//! \todo Do I need a device name field?
	//! \todo More stuff may need to be added to the constructor's arguments.
	NumberProperty(const QString& propertyName,
	               State propertyState,
	               Permission accessPermission,
	               const QString& propertyLabel = QString(),
	               const QString& propertyGroup = QString(),
	               const QDateTime& timestamp = QDateTime());
	NumberProperty(const StandardDefTagAttributes& attributes);
	virtual ~NumberProperty();

	void addElement(NumberElement* element);
	NumberElement* getElement(const QString& name);
	void update(const QHash<QString,QString>& newValues,
	            const QDateTime& timestamp);
	void update(const QHash<QString,QString>& newValues,
	            const QDateTime& timestamp,
	            State newState);
	int elementCount() const;
	QStringList getElementNames() const;

private:
	QHash<QString,NumberElement*> elements;
};

//! A vector/array of switches (boolean SwitchElement-s)
class SwitchProperty : public Property
{
public:
	SwitchProperty(const QString& propertyName,
	               State propertyState,
	               Permission accessPermission,
	               SwitchRule switchRule,
	               const QString& propertyLabel = QString(),
	               const QString& propertyGroup = QString(),
	               const QDateTime& timestamp = QDateTime());
	SwitchProperty(const DefSwitchTagAttributes& attributes);
	virtual ~SwitchProperty();

	SwitchRule getSwitchRule() const;

	void addElement(SwitchElement* element);
	//! Ignores the SwitchRule - everything is supposed to be checked on the
	//! device side.
	void update(const QHash<QString,QString>& newValues,
	            const QDateTime& timestamp);
	void update(const QHash<QString,QString>& newValues,
	            const QDateTime& timestamp,
	            State newState);
	SwitchElement* getElement(const QString& name);
	int elementCount() const;
	QStringList getElementNames() const;

private:
	SwitchRule rule;
	QHash<QString,SwitchElement*> elements;
};

//! A vector/array of lights (LightElement-s).
class LightProperty : public Property
{
public:
	//! Lights are always read-only
	LightProperty(const QString& propertyName,
	              State propertyState,
	              const QString& propertyLabel = QString(),
	              const QString& propertyGroup = QString(),
	              const QDateTime& timestamp = QDateTime());
	LightProperty(const BasicDefTagAttributes& attributes);
	~LightProperty();

	void addElement(LightElement* element);
	LightElement* getElement(const QString& name);

	int elementCount() const;
	QStringList getElementNames() const;

private:
	QHash<QString,LightElement*> elements;
};

//! A vector/array of BLOBs (BlobElement-s).
class BlobProperty : public Property
{
public:
	BlobProperty(const QString& propertyName,
	             State propertyState,
	             Permission accessPermission,
	             const QString& propertyLabel = QString(),
	             const QString& propertyGroup = QString(),
	             const QDateTime& timestamp = QDateTime());
	BlobProperty(const StandardDefTagAttributes& attributes);
	~BlobProperty();

	void addElement(BlobElement* element);
	BlobElement* getElement(const QString& name);

	void update(const QDateTime& timestamp);
	void update(const QDateTime& timestamp, State newState);


	int elementCount() const;
	QStringList getElementNames() const;

private:
	QHash<QString,BlobElement*> elements;
};

#endif//_INDI_PROPERTY_HPP_
