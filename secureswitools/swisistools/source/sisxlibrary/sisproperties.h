/*
* Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of the License "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description: 
* Note: This file may contain code to generate corrupt files for test purposes.
* Such code is excluded from production builds by use of compiler defines;
* it is recommended that such code should be removed if this code is ever published publicly.
* As specified in SGL.GT0188.251
*
*/


/**
 @file 
 @internalComponent
 @released
*/

#ifndef __SISPROPERTIES_H__
#define __SISPROPERTIES_H__

#include "sisproperty.h"
#include "sisarray.h"

class CSISProperties : public CStructure <CSISFieldRoot::ESISProperties>

	{
public:
	typedef CSISArray <CSISProperty, CSISFieldRoot::ESISProperty> SISPropertyArray;
public:
	CSISProperties ();
	CSISProperties (const CSISProperties& aInitialiser);

	virtual std::string Name () const;

	void AddProperty (const TUint32 aKey, const TUint32 aValue);
	void AddPackageEntry(std::wostream& aStream, bool aVerbose) const;
	inline const SISPropertyArray& Properties() const;

private:
	SISPropertyArray iProperty;
	};




inline CSISProperties::CSISProperties ()
	{
	InsertMember (iProperty); 
	}


inline CSISProperties::CSISProperties (const CSISProperties& aInitialiser) :
		CStructure <CSISFieldRoot::ESISProperties> (aInitialiser),
		iProperty (aInitialiser.iProperty)
	{
	InsertMember (iProperty); 
	}


inline void CSISProperties::AddProperty (const TUint32 aKey, const TUint32 aValue) 
	{ 
	iProperty.Push (CSISProperty (aKey, aValue)); 
	}


inline std::string CSISProperties::Name () const
	{
	return "Properties";
	}

inline const CSISProperties::SISPropertyArray& CSISProperties::Properties() const
	{
	return iProperty;
	}


#endif // __SISPROPERTIES_H__

