/*
* Copyright (c) 2006 Nokia Corporation and/or its subsidiary(-ies).
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


#ifndef C_NCDCONFIGURATIONPROTOCOLRESPONSEPARSER_H
#define C_NCDCONFIGURATIONPROTOCOLRESPONSEPARSER_H

#include "ncdsubparser.h"


class CNcdConfigurationProtocolResponseParser : public CNcdSubParser
    {
public:
    static CNcdConfigurationProtocolResponseParser* NewL( MNcdParserObserverBundle& aObservers, 
                                                          MNcdSubParserObserver& aSubParserObserver,
                                                          TInt aDepth,
                                                          const Xml::RTagInfo& aElement, 
                                                          const Xml::RAttributeArray& aAttributes );
    virtual ~CNcdConfigurationProtocolResponseParser();

private:
    CNcdConfigurationProtocolResponseParser( MNcdParserObserverBundle& aObservers,   
                                             MNcdSubParserObserver& aSubParserObserver,
                                             TInt aDepth );

    void ConstructL( const Xml::RTagInfo& aElement, 
                     const Xml::RAttributeArray& aAttributes );


protected:
    virtual void OnStartElementL( const Xml::RTagInfo& aElement, const Xml::RAttributeArray& aAttributes, 
                                  TInt aErrorCode) ;

    virtual void OnEndElementL(const Xml::RTagInfo& aElement, TInt aErrorCode) ;

    virtual void SubParserFinishedL( const TDesC8& aTag,
                                     TInt aErrorCode );

private:
    
    };

#endif
