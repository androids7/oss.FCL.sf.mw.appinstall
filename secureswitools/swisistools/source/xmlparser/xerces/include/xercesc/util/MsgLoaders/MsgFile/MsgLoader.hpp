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
 * $Id: MsgLoader.hpp 568078 2007-08-21 11:43:25Z amassari $
 */

#if !defined(ICONVMSGLOADER_HPP)
#define ICONVMSGLOADER_HPP

#include <xercesc/util/XercesDefs.hpp>
#include <xercesc/util/XMLMsgLoader.hpp>

#include <nl_types.h>

XERCES_CPP_NAMESPACE_BEGIN

//
//  This is a simple in Iconv RC message loader implementation.
//
class XMLUTIL_EXPORT MsgCatalogLoader : public XMLMsgLoader
{
public :
    // -----------------------------------------------------------------------
    //  Public Constructors and Destructor
    // -----------------------------------------------------------------------
    MsgCatalogLoader(const XMLCh* const msgDomain);
    ~MsgCatalogLoader();


    // -----------------------------------------------------------------------
    //  Implementation of the virtual message loader API
    // -----------------------------------------------------------------------
    virtual bool loadMsg
    (
        const   XMLMsgLoader::XMLMsgId  msgToLoad
        ,       XMLCh* const            toFill
        , const unsigned int            maxChars
    );

    virtual bool loadMsg
    (
        const   XMLMsgLoader::XMLMsgId  msgToLoad
        ,       XMLCh* const            toFill
        , const unsigned int            maxChars
        , const XMLCh* const            repText1
        , const XMLCh* const            repText2 = 0
        , const XMLCh* const            repText3 = 0
        , const XMLCh* const            repText4 = 0
        , MemoryManager* const          manager  = XMLPlatformUtils::fgMemoryManager
    );

    virtual bool loadMsg
    (
        const   XMLMsgLoader::XMLMsgId  msgToLoad
        ,       XMLCh* const            toFill
        , const unsigned int            maxChars
        , const char* const             repText1
        , const char* const             repText2 = 0
        , const char* const             repText3 = 0
        , const char* const             repText4 = 0
        , MemoryManager * const         manager  = XMLPlatformUtils::fgMemoryManager
    );


private :
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    MsgCatalogLoader();
    MsgCatalogLoader(const MsgCatalogLoader&);
    MsgCatalogLoader& operator=(const MsgCatalogLoader&);


    // -----------------------------------------------------------------------
    //  Private data members
    //
    //  fModHandle
    //      This is our DLL module handle that we need in order to load
    //      resource messages. This is set during construction.
    //
    //  fMsgDomain
    //      This is the name of the error domain that this loader is for.
    // -----------------------------------------------------------------------
    nl_catd 	fCatalogHandle;
    XMLCh*      fMsgDomain;
	
};

XERCES_CPP_NAMESPACE_END

#endif