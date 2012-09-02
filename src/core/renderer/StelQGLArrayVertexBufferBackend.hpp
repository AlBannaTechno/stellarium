/*
 * Stellarium
 * Copyright (C) 2012 Ferdinand Majerech
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
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#ifndef _STELQGLARRAYVERTEXBUFFERBACKEND_HPP_
#define _STELQGLARRAYVERTEXBUFFERBACKEND_HPP_

#include <QVector>

#include "VecMath.hpp"
#include "StelProjectorType.hpp"
#include "StelVertexBuffer.hpp"
#include "StelVertexBufferBackend.hpp"
#include "StelVertexAttribute.hpp"


//! Base class for QGL-using vertex array based vertex buffer backends.
//!
//! @note This is an internal class of the Renderer subsystem and should not be used elsewhere.
class StelQGLArrayVertexBufferBackend : public StelVertexBufferBackend
{
protected:
	//! Buffer containing values of a single vertex attribute.
	//!
	//! Must be downcasted to AttributeArray to access the stored data.
	struct AnyAttributeArray
	{
		//! Required to ensure that derived classes get properly deallocated.
		virtual ~AnyAttributeArray(){};

		//! Get a read only pointer to the data stored in buffer.
		virtual const void* constData() = 0;
	};

	//! Concrete attribute buffer storing attributes of a particular type.
	//!
	//! Handles the GL logic required to provide the buffer to the GPU.
	template<class A> struct AttributeArray : public AnyAttributeArray
	{
		//! Construct an AttributeArray.
		//!
		//! @param interpretation How should GL interpret this attribute? (e.g. color, normal, etc).
		AttributeArray(const AttributeInterpretation interpretation)
			:interpretation(interpretation)
		{
			// Just enough for a triangle pair to be created without reallocation.
			data.reserve(6);
		}

		virtual const void* constData()
		{
			return static_cast<const void*>(data.constData());
		}

		//! Stores the attribute data (in GL terms, a vertex array).
		QVector<A> data;

		//! Specifies how should the attribute be interpreted (color, normal, etc.).
		const AttributeInterpretation interpretation;
	};

public:
	virtual ~StelQGLArrayVertexBufferBackend();

	virtual void addVertex(const void* const vertexInPtr);

	virtual void getVertex(const int index, void* const vertexOutPtr) const;

	virtual void setVertex(const int index, const void* const vertexInPtr)
	{
		setVertexNonVirtual(index, vertexInPtr);
	}

	virtual void lock()
	{
		locked = true;
	}

	virtual void unlock()
	{
		locked = false;
	}

	virtual void clear()
	{
		vertexCount = 0;
	}

	//! Use a StelProjector to project vertex positions.
	//!
	//! Can be called only immediately before drawing. The projected vertex 
	//! positions are only used for one draw call - another one might use a
	//! different/modified projector or index buffer.
	//!
	//! @param projector   Projector to project the vertices.
	//! @param indexBuffer Index buffer specifying which vertices to project.
	//!                    If NULL, all vertices are projected.
	void projectVertices(StelProjector* projector, 
	                     class StelQGLIndexBuffer* indexBuffer);

	//! Get the number of vertices in the buffer.
	int length() const
	{
		return vertexCount;
	}

	//! Return graphics primitive type formed by the vertices of the buffer,
	PrimitiveType getPrimitiveType() const
	{
		return primitiveType;
	}

protected:
	//! Is the vertex buffer locked (i.e. ready to draw?).
	bool locked;

	//! Graphics primitive type formed by the vertices of this buffer.
	PrimitiveType primitiveType;

	//! Number of used vertices in the buffer.
	int vertexCount;

	//! Number of vertices we have allocated space for.
	int vertexCapacity;

	//! Buffers storing vertex attributes.
	QVector<AnyAttributeArray*> buffers;

	//! Are we using vertex positions projected by a StelProjector?
	//!
	//! (Instead of just letting OpenGL handle the projection)
	//!
	//! This is set to true by projectVertices() and back to false by 
	//! the draw call immediately after. The projected positions can only be used 
	//! for one draw call, as for another one the StelProjector might be different/modified.
	bool usingProjectedPositions;

	//! Projected vertex positions to draw when we're projecting vertices with a StelProjector.
	//!
	//! This replaces the buffer with Position interpretation during drawing when 
	//! usingProjectedPositions is true. The positions are projected by projectVertices().
	QVector<Vec3f> projectedPositions;

	//! Construct a StelQGLArrayVertexBufferBackend.
	//!
	//! Initializes vertex attribute buffers.
	//!
	//! @param type Graphics primitive type stored in the buffer.
	//! @param attributes Specifications of vertex attributes that will be stored in the buffer.
	StelQGLArrayVertexBufferBackend(const PrimitiveType type,
	                                const QVector<StelVertexAttribute>& attributes);

private:

	//! SetVertex implementation, non-virtual so it can be inlined in addVertex.
	//! 
	//! @see setVertex
	void setVertexNonVirtual(const int index, const void* const vertexInPtr)
	{
		// Points to the current attribute (e.g. color, normal, vertex) within the vertex.
		const unsigned char* attribPtr = static_cast<const unsigned char*>(vertexInPtr);
		for(int attrib = 0; attrib < attributes.count; ++attrib)
		{
			//Set each attribute in its buffer.
			switch(attributes.attributes[attrib].type)
			{
				case AttributeType_Vec2f:
					getAttribute<Vec2f>(attrib, index) = *reinterpret_cast<const Vec2f*>(attribPtr);
					break;
				case AttributeType_Vec3f:
					getAttribute<Vec3f>(attrib, index) = *reinterpret_cast<const Vec3f*>(attribPtr);
					break;
				case AttributeType_Vec4f:
					getAttribute<Vec4f>(attrib, index) = *reinterpret_cast<const Vec4f*>(attribPtr);
					break;
				default:
					Q_ASSERT(false);
			}

			// This always works, because the C standard requires that 
			// sizeof(unsigned char) == 1  (that 1 might mean e.g. 16 bits instead of 8
			// on some platforms, but both the size of attribute and of unsigned char is 
			// measured in the same unit, so it's not a problem.
			attribPtr += attributes.sizes[attrib];
		}
	}

	//! Add an attribute to specified attribute buffer.
	//!
	//! @param attributeIndex Specifies which attribute (e.g. normal, texcoord, vertex) we're
	//!                       adding.
	//! @param attributePtr   Raw pointer to attribute data. Data format must match
	//!                       the attribute at specified index.
	template<class A>
	void addAttribute(const int attributeIndex, const void* attributePtr)
	{
		const A* attrib = reinterpret_cast<const A*>(attributePtr);
		getBuffer<A>(attributeIndex).data.append(*attrib);
	}

	//! Access specified attribute of a vertex.
	//!
	//! @tparam A             Attribute type. Must match the type of attribute at
	//!                       attributeIndex.
	//! @param attributeIndex Specifies which attribute (e.g. normal, texcoord, vertex) we're
	//!                       accessing.
	//! @param vertexIndex    Specifies which vertex we're accessing.
	//! @return               Non-const reference to the attribute.
	template<class A>
	A& getAttribute(const int attributeIndex, const int vertexIndex) 
	{
		return getBuffer<A>(attributeIndex).data[vertexIndex];
	}

	//! Const version of getAttribute.
	//!
	//! @see getAttribute
	template<class A>
	const A& getAttributeConst(const int attributeIndex, const int vertexIndex)  const
	{
		return getBufferConst<A>(attributeIndex).data[vertexIndex];
	}

	//! Access buffer of the specified vertex attribute.
	//!
	//! @tparam A    Attribute type. Must match the type of attribute at specified index.
	//! @param index Attribute index. Specifies which attribute (e.g. normal, texcoord, vertex)
	//!              we're working with.
	//! @return      Non-const reference to the attribute buffer.
	template<class A>
	AttributeArray<A>& getBuffer(const int attributeIndex)
	{
		return *static_cast<AttributeArray<A>*>(buffers[attributeIndex]);
	}

	//! Const version of getBuffer.
	//!
	//! @see getBuffer
	template<class A>
	const AttributeArray<A>& getBufferConst(const int attributeIndex) const
	{
		return *static_cast<const AttributeArray<A>*>(buffers[attributeIndex]);
	}

	//! Get index of attribute with specified interpretation in the buffers vector.
	//!
	//! (No two attributes can have the same interpretation)
	//!
	//! Returns -1 if not found.
	int getAttributeIndex(const AttributeInterpretation interpretation) const;
};

#endif // _STELQGLARRAYVERTEXBUFFERBACKEND_HPP_
