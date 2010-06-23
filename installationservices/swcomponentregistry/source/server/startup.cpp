/*
* Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
* Executable entrypoint and server object factory function. The latter
* is supplied to the SCS boilerplate code which starts up the server.
*
*/


/**
 @file
 @internalComponent
 @released
*/

#include "scrserver.h"

using namespace Usif;

static CScsServer* NewScrServerLC()
/**
	This factory function is called by SCS framework.  It allocates
	the Scr Server object and leaves it on the cleanup stack.

	@return		New initialized instance of CScrServer.
				On return this is on the cleanup stack.
 */
	{
	return Usif::CScrServer::NewLC();
	}


GLDEF_C TInt E32Main()
/**
	Executable entrypoint. Calls the SCS boilerplate code which starts the SCR server
	and does not return until the server shuts down.

	@return	Symbian OS error code where KErrNone indicates success 
			and any other value indicates failure.
 */
	{
	return StartScsServer(NewScrServerLC);
	}
