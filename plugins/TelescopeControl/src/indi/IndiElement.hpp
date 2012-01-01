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

#ifndef _ELEMENT_HPP_
#define _ELEMENT_HPP_

#include <QByteArray>
#include <QString>
#include <QXmlStreamReader>

#include <limits>
const double DOUBLE_MAX = std::numeric_limits<double>::max();
const double DOUBLE_MIN = std::numeric_limits<double>::min();

#include "IndiTypes.hpp"

//In all element classes, put the required elements first.

class Element
{
public:
	Element(const QString& elementName, const QString& elementLabel = QString());
	//! Creates an element with the given attributes.
	//! If the attributes are missing or invalid, returns an invalid Element.
	Element(const QXmlStreamAttributes& attributes);
	
	virtual void setValue(const QString& stringValue) = 0;

	const QString& getName() const;
	const QString& getLabel() const;
	//! All the required attributes have been provided in the definition.
	//! Can return true even if the element is empty.
	bool isValid() const {return valid;}
	//! Returns false if the element has not been assigned a value.
	bool isEmpty() const {return !used;}
	
	static const char* A_NAME;
	static const char* A_LABEL;
	static const char* A_FORMAT;
	static const char* A_MINIMUM;
	static const char* A_MAXIMUM;
	static const char* A_STEP;
	static const char* A_SIZE;

protected:
	QString name;
	QString label;
	bool valid;
	bool used;
};

//! Sub-property representing a single string.
class TextElement : public Element
{
public:
	TextElement(const QString& elementName,
	            const QString& initialValue,
	            const QString& label = QString());
	//! This constructor does not provide an initial value.
	//! Use setValue() after construction to pass it.
	TextElement(const QXmlStreamAttributes& attributes);

	void setValue(const QString& stringValue);
	QString getValue() const;

private:
	QString value;
};

//! Sub-property representing a single number.
//! \todo Better handling of sexagesimal numbers.
class NumberElement : public Element
{
public:
	NumberElement(const QString& elementName,
	              const double initialValue = 0.0,
	              const QString& format = "%d",
	              const double minimumValue = DOUBLE_MIN,
	              const double maximumValue = DOUBLE_MAX,
	              const double step = 0,
	              const QString& label = QString());

	NumberElement(const QString& elementName,
	              const QString& initialValue,
	              const QString& format,
	              const QString& minimalValue,
	              const QString& maximalValue,
	              const QString& step,
	              const QString& label = QString());
	
	//! This constructor does not provide an initial value.
	//! Use setValue() after construction to pass it.
	//! \todo Add validation of the format string.
	NumberElement(const QXmlStreamAttributes& attributes);
	
	double getValue() const;
	QString getFormattedValue() const;
	void setValue(const QString& stringValue);

	QString getFormatString() const;
	double getMinValue() const;
	double getMaxValue() const;
	double getStep() const;

	//! \todo Add error handling instead of returning 0.
	static double readDoubleFromString(const QString& string);

private:
	// For unit tests
	friend class TestNumberElement;

	bool isSexagesimal;
	double value;
	double maxValue;
	double minValue;
	double step;
	QString formatString;
};

//! Sub-property representing a single switch/button.
//! \todo Is there a point of this existing, or it can be replaced with a hash
//! of booleans?
class SwitchElement : public Element
{
public:
	SwitchElement(const QString& elementName,
				  const QString& initialValue,
				  const QString& label = QString());
	
	//! This constructor does not provide an initial value.
	//! Use setValue() after construction to pass it.
	SwitchElement(const QXmlStreamAttributes& attributes);

	bool isOn();
	void setValue(const QString& stringValue);

private:
	//! State of the switch. True is "on", false is "off". :)
	bool state;
};

//! Sub-property representing a single indicator light.
class LightElement : public Element
{
public:
	LightElement(const QString& elementName,
	             const QString& initialValue,
	             const QString& label = QString());
	
	//! This constructor does not provide an initial value.
	//! Use setValue() after construction to pass it.
	LightElement(const QXmlStreamAttributes& attributes);

	State getValue() const;
	//! \todo Decide how to handle wrong values. (At the moment if the parameter
	//! is not recognised it ignores it).
	void setValue(const QString& stringValue);

private:
	State state;
};

//! Sub-property representing a single BLOB (Binary Large OBject).
//! \todo Think what to make of this - it may be massive.
//! \todo Is there a point of it existing? It is not handled as the rest of the
//! elements - it has no initial value.
class BlobElement : public Element
{
public:
	//! \param intialValue is ignored.
	BlobElement(const QString& elementName,
	            const QString& initialValue,
	            const QString& label = QString());
	
	//! This constructor does not provide an initial value.
	//! (And BLOB shouldn't have any, so forget about setValue().)
	BlobElement(const QXmlStreamAttributes& attributes);
	
	//! Does nothing.
	void setValue(const QString&){;}
	
	//! \todo bool or void?
	bool prepareToReceiveData(const QString& blobSize,
	                          const QString& blobFormat);
	//! Handle another portion of data.
	void receiveData(const QString& dataString);
	//! 
	void finishReceivingData();

	//! Example: Returns the decoded data?
	const QByteArray& getValue() const;

	//! Example
	QString getFormat() const;
	//! Example
	//! \todo Isn't int too small?
	int getSize() const;
	//! Was the BLOB compressed when it was received?
	bool isCompressed() const {return compressed;}

private:
	//! Original size of the current BLOB before encoding (and compression, if any).
	int originalSize;
	QString format;
	bool compressed;
	QByteArray binaryData;
};

#endif//_ELEMENT_HPP_
