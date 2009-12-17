/*
* Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description: 
*
*/
/*
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 * 
 *      http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * $Id: XPathMatcherStack.hpp 568078 2007-08-21 11:43:25Z amassari $
 */

#if !defined(XPATHMATCHERSTACK_HPP)
#define XPATHMATCHERSTACK_HPP


// ---------------------------------------------------------------------------
//  Includes
// ---------------------------------------------------------------------------
#include <xercesc/validators/schema/identity/XPathMatcher.hpp>

XERCES_CPP_NAMESPACE_BEGIN

class VALIDATORS_EXPORT XPathMatcherStack : public XMemory
{
public:
    // -----------------------------------------------------------------------
    //  Constructors/Destructor
    // -----------------------------------------------------------------------
    XPathMatcherStack(MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);
	~XPathMatcherStack();

	// -----------------------------------------------------------------------
    //  Getter methods
    // -----------------------------------------------------------------------
    XPathMatcher* getMatcherAt(const unsigned int index) const;
    unsigned int  getMatcherCount() const;
    unsigned int  size() const;

	// -----------------------------------------------------------------------
    //  Access methods
    // -----------------------------------------------------------------------
    void addMatcher(XPathMatcher* const matcher);

	// -----------------------------------------------------------------------
    //  Stack methods
    // -----------------------------------------------------------------------
    void pushContext();
    void popContext();

	// -----------------------------------------------------------------------
    //  Reset methods
    // -----------------------------------------------------------------------
    void clear();

private:
    // -----------------------------------------------------------------------
    //  Private helper methods
    // -----------------------------------------------------------------------
    void cleanUp();

    // -----------------------------------------------------------------------
    //  Unimplemented contstructors and operators
    // -----------------------------------------------------------------------
    XPathMatcherStack(const XPathMatcherStack& other);
    XPathMatcherStack& operator= (const XPathMatcherStack& other);

    // -----------------------------------------------------------------------
    //  Data members
    // -----------------------------------------------------------------------
    unsigned int                fMatchersCount;
    ValueStackOf<int>*          fContextStack;
    RefVectorOf<XPathMatcher>*  fMatchers;
};

// ---------------------------------------------------------------------------
//  XPathMatcherStack: Getter methods
// ---------------------------------------------------------------------------
inline unsigned int XPathMatcherStack::size() const {

    return fContextStack->size();
}

inline unsigned int XPathMatcherStack::getMatcherCount() const {

    return fMatchersCount;
}

inline XPathMatcher*
XPathMatcherStack::getMatcherAt(const unsigned int index) const {

    return fMatchers->elementAt(index);
}

// ---------------------------------------------------------------------------
//  XPathMatcherStack: Stack methods
// ---------------------------------------------------------------------------
inline void XPathMatcherStack::pushContext() {

    fContextStack->push(fMatchersCount);
}

inline void XPathMatcherStack::popContext() {

    fMatchersCount = fContextStack->pop();
}

// ---------------------------------------------------------------------------
//  XPathMatcherStack: Access methods
// ---------------------------------------------------------------------------
inline void XPathMatcherStack::addMatcher(XPathMatcher* const matcher) {

    if (fMatchersCount == fMatchers->size()) {

        fMatchers->addElement(matcher);
        fMatchersCount++;
    }
    else {
        fMatchers->setElementAt(matcher, fMatchersCount++);
    }
}

XERCES_CPP_NAMESPACE_END

#endif

/**
  * End of file XPathMatcherStack.hpp
  */

