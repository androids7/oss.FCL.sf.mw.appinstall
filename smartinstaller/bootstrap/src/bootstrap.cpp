/*
* Copyright (c) 2009-2010 Nokia Corporation and/or its subsidiary(-ies).
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
*     Main Application class.
*
*
*/


// INCLUDE FILES
#include <eikstart.h>
#include "bootstrapapplication.h"

/**
 * factory function to create the CBootstrapApplication class
 */
LOCAL_C CApaApplication* NewApplication()
	{
	return new CBootstrapApplication;
	}


GLDEF_C TInt E32Main()
	{
	return EikStart::RunApplication( NewApplication );
	}
