/*
* This file is part of the Shiboken Python Bindings Generator project.
*
* Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
*
* Contact: PySide team <contact@pyside.org>
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation; either
* version 2.1 of the License, or (at your option) any later version.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this library; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef BASEWRAPPER_P_H
#define BASEWRAPPER_P_H

#include <Python.h>
#include <list>
#include <map>

struct SbkObject;

namespace Shiboken
{
/**
    * This mapping associates a method and argument of an wrapper object with the wrapper of
    * said argument when it needs the binding to help manage its reference counting.
    */
typedef std::map<std::string, std::list<SbkObject*> > RefCountMap;


/// Linked list of SbkBaseWrapper pointers
typedef std::list<SbkObject*> ChildrenList;

/// Struct used to store information about object parent and children.
struct ParentInfo
{
    /// Default ctor.
    ParentInfo() : parent(0), hasWrapperRef(false) {}
    /// Pointer to parent object.
    SbkObject* parent;
    /// List of object children.
    ChildrenList children;
    /// has internal ref
    bool hasWrapperRef;
};

} // namespace Shiboken

extern "C"
{

/**
 * \internal
 * Private data for SbkBaseWrapper
 */
struct SbkBaseWrapperPrivate
{
    /// Pointer to the C++ class.
    void** cptr;
    /// True when Python is responsible for freeing the used memory.
    unsigned int hasOwnership : 1;
    /// Is true when the C++ class of the wrapped object has a virtual destructor AND was created by Python.
    unsigned int containsCppWrapper : 1;
    /// Marked as false when the object is lost to C++ and the binding can not know if it was deleted or not.
    unsigned int validCppObject : 1;
    /// Information about the object parents and children, can be null.
    Shiboken::ParentInfo* parentInfo;
    /// Manage reference counting of objects that are referred but not owned.
    Shiboken::RefCountMap* referredObjects;
};

} // extern "C"

namespace Shiboken
{
/**
 * Utility function uset to transform PyObject which suppot sequence protocol in a std::list
 **/
std::list<SbkObject*> splitPyObject(PyObject* pyObj);

struct SbkBaseWrapperType;

/**
*   Visitor class used by walkOnClassHierarchy function.
*/
class HierarchyVisitor
{
public:
    HierarchyVisitor() : m_wasFinished(false) {}
    virtual ~HierarchyVisitor() {}
    virtual void visit(SbkBaseWrapperType* node) = 0;
    void finish() { m_wasFinished = true; };
    bool wasFinished() const { return m_wasFinished; }
private:
    bool m_wasFinished;
};

class BaseCountVisitor : public HierarchyVisitor
{
public:
    BaseCountVisitor() : m_count(0) {}

    void visit(SbkBaseWrapperType*)
    {
        m_count++;
    }

    int count() const { return m_count; }
private:
    int m_count;
};

class BaseAccumulatorVisitor : public HierarchyVisitor
{
public:
    BaseAccumulatorVisitor() {}

    void visit(SbkBaseWrapperType* node)
    {
        m_bases.push_back(node);
    }

    std::list<SbkBaseWrapperType*> bases() const { return m_bases; }
private:
    std::list<SbkBaseWrapperType*> m_bases;
};

class GetIndexVisitor : public HierarchyVisitor
{
public:
    GetIndexVisitor(PyTypeObject* desiredType) : m_index(-1), m_desiredType(desiredType) {}
    virtual void visit(SbkBaseWrapperType* node)
    {
        m_index++;
        if (PyType_IsSubtype(reinterpret_cast<PyTypeObject*>(node), m_desiredType))
            finish();
    }
    int index() const { return m_index; }

private:
    int m_index;
    PyTypeObject* m_desiredType;
};

/// \internal Internal function used to walk on classes inheritance trees.
/**
*   Walk on class hierarchy using a DFS algorithm.
*   For each pure Shiboken type found, HiearchyVisitor::visit is called and the algorithm consider
*   all children of this type as visited.
*/
void walkThroughClassHierarchy(PyTypeObject* currentType, HierarchyVisitor* visitor);

inline int getTypeIndexOnHierarchy(PyTypeObject* baseType, PyTypeObject* desiredType)
{
    GetIndexVisitor visitor(desiredType);
    walkThroughClassHierarchy(baseType, &visitor);
    return visitor.index();
}

inline int getNumberOfCppBaseClasses(PyTypeObject* baseType)
{
    BaseCountVisitor visitor;
    walkThroughClassHierarchy(baseType, &visitor);
    return visitor.count();
}

inline std::list<SbkBaseWrapperType*> getCppBaseClasses(PyTypeObject* baseType)
{
    BaseAccumulatorVisitor visitor;
    walkThroughClassHierarchy(baseType, &visitor);
    return visitor.bases();
}

/**
*   Decrements the reference counters of every object referred by self.
*   \param self    the wrapper instance that keeps references to other objects.
*/
void clearReferences(SbkObject* self);

} // namespace Shiboken

#endif