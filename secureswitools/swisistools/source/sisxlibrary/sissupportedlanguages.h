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

#ifndef __SISSUPPORTEDLANGUAGES_H__
#define __SISSUPPORTEDLANGUAGES_H__

#include "sislanguage.h"
#include "sisarray.h"


class CSISSupportedLanguages : public CStructure <CSISFieldRoot::ESISSupportedLanguages>

	{
public:
	CSISSupportedLanguages ();
	CSISSupportedLanguages (const CSISSupportedLanguages& aInitialiser);

	virtual void MakeNeat ();
	virtual std::string Name () const;

	void AddLanguage (const CSISLanguage::TDialect aLan);
	bool AreLanguagesSpecified () const;
	TUint32 LanguageCount () const;
	CSISLanguage::TLanguage Language (const TUint32 aIndex) const;
	void AddPackageEntry(std::wostream& aStream, bool aVerbose, bool aCompatible) const;

private:
	CSISArray <CSISLanguage, CSISFieldRoot::ESISLanguage> iSupportedLanguage;
	
	};



inline CSISSupportedLanguages::CSISSupportedLanguages ()
	{
	InsertMember (iSupportedLanguage); 
	}


inline CSISSupportedLanguages::CSISSupportedLanguages (const CSISSupportedLanguages& aInitialiser) :
			CStructure <CSISFieldRoot::ESISSupportedLanguages> (aInitialiser),
			iSupportedLanguage (aInitialiser.iSupportedLanguage)
	{
	InsertMember (iSupportedLanguage); 
	}


inline void CSISSupportedLanguages::AddLanguage (const CSISLanguage::TDialect aLan)
	{ 
	iSupportedLanguage.Push (CSISLanguage (aLan));
	}


inline bool CSISSupportedLanguages::AreLanguagesSpecified () const
	{ 
	return iSupportedLanguage.size () != 0;
	}


inline TUint32 CSISSupportedLanguages::LanguageCount () const
	{ 
	return iSupportedLanguage.size ();
	}


inline CSISLanguage::TLanguage CSISSupportedLanguages::Language (const TUint32 aIndex) const
	{
	return iSupportedLanguage [aIndex].Value ();
	}

#endif // __SISSUPPORTEDLANGUAGES_H__

