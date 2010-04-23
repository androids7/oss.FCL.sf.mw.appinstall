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


#ifndef __SDP_ATTRIBUTE_NOTIFIER_H__
#define __SDP_ATTRIBUTE_NOTIFIER_H__

class CSdpAttrValue;


/*! 
  @class MSdpAttributeNotifier
  
  @discussion An instance of MSdpAttributeNotifier is used to read selected SDP Attribute
  elements from an attribute value.
  */
class MSdpAttributeNotifier
    {
public:
/*!
  @function FoundElementL
  
  @discussion Read the data element
  @param aKey a key that identifies the element
  @param aValue the data element
  */
    virtual void FoundElementL(TInt aKey, CSdpAttrValue& aValue) = 0;

    };


#endif // __SDP_ATTRIBUTE_NOTIFIER_H__