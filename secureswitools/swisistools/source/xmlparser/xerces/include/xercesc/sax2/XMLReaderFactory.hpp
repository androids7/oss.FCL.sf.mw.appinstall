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
 * $Id: XMLReaderFactory.hpp 568078 2007-08-21 11:43:25Z amassari $
 */

#ifndef XMLREADERFACTORY_HPP
#define XMLREADERFACTORY_HPP

#include <xercesc/sax2/SAX2XMLReader.hpp>
#include <xercesc/sax/SAXException.hpp>

XERCES_CPP_NAMESPACE_BEGIN

class MemoryManager;
class XMLGrammarPool;

/**
  * Creates a SAX2 parser (SAX2XMLReader).
  *
  * <p>Note: The parser object returned by XMLReaderFactory is owned by the
  * calling users, and it's the responsiblity of the users to delete that
  * parser object, once they no longer need it.</p>
  *
  * @see SAX2XMLReader#SAX2XMLReader
  */
class SAX2_EXPORT XMLReaderFactory
{
protected:                // really should be private, but that causes compiler warnings.
	XMLReaderFactory() ;
	~XMLReaderFactory() ;

public:
	static SAX2XMLReader * createXMLReader(  MemoryManager* const  manager = XMLPlatformUtils::fgMemoryManager
                                           , XMLGrammarPool* const gramPool = 0
                                          ) ;
	static SAX2XMLReader * createXMLReader(const XMLCh* className)  ;

private:
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    XMLReaderFactory(const XMLReaderFactory&);
    XMLReaderFactory& operator=(const XMLReaderFactory&);
};

inline SAX2XMLReader * XMLReaderFactory::createXMLReader(const XMLCh *)
{	
	throw SAXNotSupportedException();
	// unimplemented
	return 0;
}

XERCES_CPP_NAMESPACE_END

#endif