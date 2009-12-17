/*
* Copyright (c) 2002-2004 Nokia Corporation and/or its subsidiary(-ies).
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
* Description:   This file contains the header file of the CSilentLauncher class.
*
*/


#ifndef SILENTLAUNCHER_H
#define SILENTLAUNCHER_H

//  INCLUDES
#include <e32base.h>
#include <f32file.h>
#include <SWInstApi.h>

namespace Swi
{

/**
* Class that handles the launching of installation of pre-installed apps.
* 
* @since 3.0 
*/
class CSilentLauncher : public CBase
    {
    public:  // Constructors and destructor
        
        /**
        * Two-phased constructor.
        */
        static CSilentLauncher* NewL( RFs& aFs );
        
        /**
        * Destructor.
        */
        virtual ~CSilentLauncher();

    public:  // New functions

        /**
        * Perform installation.
        * @since 3.0
        * @param aFile - Name of the file to install
        * @param aStatus - Request status
        */
        void InstallL( const TDesC& aFile, TRequestStatus& aStatus );

        /**
        * Cancel the current installation.
        * @since 3.0
        */
        void Cancel();        
        
    private:

        /**
        * Constructor.
        */  
        CSilentLauncher( RFs& aFs );
         
        /**
        * 2nd phase constructor.
        */
        void ConstructL();

    private: //  Data
            
        SwiUI::RSWInstSilentLauncher iLauncher;    
        SwiUI::TInstallOptions iOptions;
        SwiUI::TInstallOptionsPckg iOptionsPckg;   

        RFs& iFs;        
        TBool iConnected;
    };
}

#endif      // SILENTLAUNCHER_H   
            
// End of File
