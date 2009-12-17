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
 * $Id: CharToken.hpp 568078 2007-08-21 11:43:25Z amassari $
 */

#if !defined(CHARTOKEN_HPP)
#define CHARTOKEN_HPP

// ---------------------------------------------------------------------------
//  Includes
// ---------------------------------------------------------------------------
#include <xercesc/util/regx/Token.hpp>

XERCES_CPP_NAMESPACE_BEGIN

class XMLUTIL_EXPORT CharToken : public Token {
public:
	// -----------------------------------------------------------------------
    //  Public Constructors and Destructor
    // -----------------------------------------------------------------------
	CharToken(const unsigned short tokType, const XMLInt32 ch
        , MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);
	~CharToken();

	// -----------------------------------------------------------------------
    //  Getter methods
    // -----------------------------------------------------------------------
	XMLInt32 getChar() const;

	// -----------------------------------------------------------------------
    //  Match methods
    // -----------------------------------------------------------------------
	bool match(const XMLInt32 ch);

private:
	// -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    CharToken(const CharToken&);
    CharToken& operator=(const CharToken&);

	// -----------------------------------------------------------------------
    //  Private data members
	// -----------------------------------------------------------------------
	XMLInt32 fCharData;
};


// ---------------------------------------------------------------------------
//  CharToken: getter methods
// ---------------------------------------------------------------------------
inline XMLInt32 CharToken::getChar() const {

	return fCharData;
}


// ---------------------------------------------------------------------------
//  CharToken: getter methods
// ---------------------------------------------------------------------------
inline bool CharToken::match(const XMLInt32 ch){

	return ch == fCharData;
}

XERCES_CPP_NAMESPACE_END

#endif

/**
  * End of file CharToken.hpp
  */