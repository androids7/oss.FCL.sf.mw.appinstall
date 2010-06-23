/*
* Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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
* Description:  Installation parameters class.
*
*/

#include "sisxsifplugininstallparams.h" // CSisxSifPluginInstallParams
#include <f32file.h>                    // EDriveC

using namespace Usif;


// ---------------------------------------------------------------------------
// CSisxSifPluginInstallParams::NewL()
// ---------------------------------------------------------------------------
//
CSisxSifPluginInstallParams* CSisxSifPluginInstallParams::NewL(
        const COpaqueNamedParams& aParams )
    {
    CSisxSifPluginInstallParams* self = new ( ELeave ) CSisxSifPluginInstallParams;
    CleanupStack::PushL( self );
    self->ConstructL( aParams );
    CleanupStack::Pop( self );
    return self;
    }

// ---------------------------------------------------------------------------
// CSisxSifPluginInstallParams::NewL()
// ---------------------------------------------------------------------------
//
CSisxSifPluginInstallParams* CSisxSifPluginInstallParams::NewL(
        const CSisxSifPluginInstallParams& aParams )
    {
    CSisxSifPluginInstallParams* self = new ( ELeave ) CSisxSifPluginInstallParams;
    CleanupStack::PushL( self );
    self->ConstructL( aParams );
    CleanupStack::Pop( self );
    return self;
    }

// ---------------------------------------------------------------------------
// CSisxSifPluginInstallParams::~CSisxSifPluginInstallParams()
// ---------------------------------------------------------------------------
//
CSisxSifPluginInstallParams::~CSisxSifPluginInstallParams()
    {
    delete iOCSPUrl;
    }

// ---------------------------------------------------------------------------
// CSisxSifPluginInstallParams::IsSilentMode()
// ---------------------------------------------------------------------------
//
TBool CSisxSifPluginInstallParams::IsSilentMode() const
    {
    return iUseSilentMode;
    }

// ---------------------------------------------------------------------------
// CSisxSifPluginInstallParams::IsInstallInactive()
// ---------------------------------------------------------------------------
//
TBool CSisxSifPluginInstallParams::IsInstallInactive() const
    {
    return iIsInstallInactive;
    }

// ---------------------------------------------------------------------------
// CSisxSifPluginInstallParams::Drive()
// ---------------------------------------------------------------------------
//
TUint CSisxSifPluginInstallParams::Drive() const
    {
    return iDrive;
    }

// ---------------------------------------------------------------------------
// CSisxSifPluginInstallParams::Language()
// ---------------------------------------------------------------------------
//
TLanguage CSisxSifPluginInstallParams::Language() const
    {
    return iLanguage;
    }

// ---------------------------------------------------------------------------
// CSisxSifPluginInstallParams::OCSPUrl()
// ---------------------------------------------------------------------------
//
const TDesC& CSisxSifPluginInstallParams::OCSPUrl() const
    {
    if( iOCSPUrl )
        {
        return *iOCSPUrl;
        }
    return KNullDesC;
    }

// ---------------------------------------------------------------------------
// CSisxSifPluginInstallParams::PerformOCSP()
// ---------------------------------------------------------------------------
//
TSifPolicy CSisxSifPluginInstallParams::PerformOCSP() const
    {
    return iPerformOCSP;
    }

// ---------------------------------------------------------------------------
// CSisxSifPluginInstallParams::IgnoreOCSPWarnings()
// ---------------------------------------------------------------------------
//
TSifPolicy CSisxSifPluginInstallParams::IgnoreOCSPWarnings() const
    {
    return iIgnoreOCSPWarnings;
    }

// ---------------------------------------------------------------------------
// CSisxSifPluginInstallParams::AllowUpgrade()
// ---------------------------------------------------------------------------
//
TSifPolicy CSisxSifPluginInstallParams::AllowUpgrade() const
    {
    return iAllowUpgrade;
    }

// ---------------------------------------------------------------------------
// CSisxSifPluginInstallParams::InstallOptionalItems()
// ---------------------------------------------------------------------------
//
TSifPolicy CSisxSifPluginInstallParams::InstallOptionalItems() const
    {
    return iInstallOptionalItems;
    }

// ---------------------------------------------------------------------------
// CSisxSifPluginInstallParams::AllowUntrusted()
// ---------------------------------------------------------------------------
//
TSifPolicy CSisxSifPluginInstallParams::AllowUntrusted() const
    {
    return iAllowUntrusted;
    }

// ---------------------------------------------------------------------------
// CSisxSifPluginInstallParams::GrantCapabilities()
// ---------------------------------------------------------------------------
//
TSifPolicy CSisxSifPluginInstallParams::GrantCapabilities() const
    {
    return iGrantCapabilities;
    }

// ---------------------------------------------------------------------------
// CSisxSifPluginInstallParams::AllowAppShutdown()
// ---------------------------------------------------------------------------
//
TSifPolicy CSisxSifPluginInstallParams::AllowAppShutdown() const
    {
    return iAllowAppShutdown;
    }

// ---------------------------------------------------------------------------
// CSisxSifPluginInstallParams::AllowAppBreakDependency()
// ---------------------------------------------------------------------------
//
TSifPolicy CSisxSifPluginInstallParams::AllowAppBreakDependency() const
    {
    return iAllowAppBreakDependency;
    }

// ---------------------------------------------------------------------------
// CSisxSifPluginInstallParams::AllowOverwrite()
// ---------------------------------------------------------------------------
//
TSifPolicy CSisxSifPluginInstallParams::AllowOverwrite() const
    {
    return iAllowOverwrite;
    }

// ---------------------------------------------------------------------------
// CSisxSifPluginInstallParams::PackageInfo()
// ---------------------------------------------------------------------------
//
TSifPolicy CSisxSifPluginInstallParams::PackageInfo() const
    {
    return iPackageInfo;
    }

// ---------------------------------------------------------------------------
// CSisxSifPluginInstallParams::AllowIncompatible()
// ---------------------------------------------------------------------------
//
TSifPolicy CSisxSifPluginInstallParams::AllowIncompatible() const
    {
    return iAllowIncompatible;
    }

// ---------------------------------------------------------------------------
// CSisxSifPluginInstallParams::CSisxSifPluginInstallParams()
// ---------------------------------------------------------------------------
//
CSisxSifPluginInstallParams::CSisxSifPluginInstallParams() : iAllowUntrusted( ENotAllowed )
    {
    }

// ---------------------------------------------------------------------------
// CSisxSifPluginInstallParams::ConstructL()
// ---------------------------------------------------------------------------
//
void CSisxSifPluginInstallParams::ConstructL( const COpaqueNamedParams& aParams )
    {
    iUseSilentMode = GetIntParam( aParams, KSifInParam_InstallSilently, EFalse );
    iIsInstallInactive = GetIntParam( aParams, KSifInParam_InstallInactive, EFalse );
    // TODO: change drive and language params as arrays when available
    iDrive = static_cast<TUint>( GetIntParam( aParams, KSifInParam_Drive, EDriveC ) );
    iLanguage = static_cast<TLanguage>( GetIntParam( aParams, KSifInParam_Languages,
            ELangNone ) );
    GetStringParamL( aParams, KSifInParam_OCSPUrl, iOCSPUrl );
    GetPolicyParam( aParams, KSifInParam_PerformOCSP, iPerformOCSP, EAllowed );
    GetPolicyParam( aParams, KSifInParam_IgnoreOCSPWarnings, iIgnoreOCSPWarnings, EAllowed );
    GetPolicyParam( aParams, KSifInParam_AllowUpgrade, iAllowUpgrade, EAllowed );
    GetPolicyParam( aParams, KSifInParam_InstallOptionalItems, iInstallOptionalItems, EAllowed );
    GetPolicyParam( aParams, KSifInParam_AllowUntrusted, iAllowUntrusted, ENotAllowed );
    GetPolicyParam( aParams, KSifInParam_GrantCapabilities, iGrantCapabilities, EAllowed );
    GetPolicyParam( aParams, KSifInParam_AllowAppShutdown, iAllowAppShutdown, EAllowed );
    GetPolicyParam( aParams, KSifInParam_AllowAppBreakDependency, iAllowAppBreakDependency,
            EAllowed );
    GetPolicyParam( aParams, KSifInParam_AllowOverwrite, iAllowOverwrite, EAllowed );
    GetPolicyParam( aParams, KSifInParam_PackageInfo, iPackageInfo, EAllowed );
    GetPolicyParam( aParams, KSifInParam_AllowIncompatible, iAllowIncompatible, EAllowed );
    }

// ---------------------------------------------------------------------------
// CSisxSifPluginInstallParams::ConstructL()
// ---------------------------------------------------------------------------
//
void CSisxSifPluginInstallParams::ConstructL( const CSisxSifPluginInstallParams& aParams )
    {
	iUseSilentMode = aParams.iUseSilentMode;
	iIsInstallInactive = aParams.iIsInstallInactive;
    iDrive = aParams.iDrive;
    iLanguage = aParams.iLanguage;
    if( aParams.iOCSPUrl )
        {
        iOCSPUrl = aParams.iOCSPUrl->AllocL();
        }
    iPerformOCSP = aParams.iPerformOCSP;
    iIgnoreOCSPWarnings = aParams.iIgnoreOCSPWarnings;
    iAllowUpgrade = aParams.iAllowUpgrade;
    iInstallOptionalItems = aParams.iInstallOptionalItems;
    iAllowUntrusted = aParams.iAllowUntrusted;
    iGrantCapabilities = aParams.iGrantCapabilities;
    iAllowAppShutdown = aParams.iAllowAppShutdown;
    iAllowAppBreakDependency = aParams.iAllowAppBreakDependency;
    iAllowOverwrite = aParams.iAllowOverwrite;
    iPackageInfo = aParams.iPackageInfo;
    iAllowIncompatible = aParams.iAllowIncompatible;
    }

// ---------------------------------------------------------------------------
// CSisxSifPluginInstallParams::GetIntParam()
// ---------------------------------------------------------------------------
//
TInt CSisxSifPluginInstallParams::GetIntParam( const COpaqueNamedParams& aParams,
        const TDesC& aParamName, TInt aDefaultValue )
    {
    TInt value = aDefaultValue;
    TBool paramFound = EFalse;
    TRAPD( err, paramFound = aParams.GetIntByNameL( aParamName, value ) );
    if( !err && paramFound )
        {
        return value;
        }
    return aDefaultValue;
    }

// ---------------------------------------------------------------------------
// CSisxSifPluginInstallParams::GetPolicyParam()
// ---------------------------------------------------------------------------
//
void CSisxSifPluginInstallParams::GetPolicyParam( const COpaqueNamedParams& aParams,
        const TDesC& aParamName, TSifPolicy& aPolicy, const TSifPolicy& aDefault )
    {
    TInt value = aDefault;
    TBool paramFound = EFalse;
    TRAPD( err, paramFound = aParams.GetIntByNameL( aParamName, value ) );
    if( !err && paramFound )
        {
        aPolicy = static_cast<TSifPolicy>( value );
        }
    }

// ---------------------------------------------------------------------------
// CSisxSifPluginInstallParams::GetStringParamL()
// ---------------------------------------------------------------------------
//
void CSisxSifPluginInstallParams::GetStringParamL( const COpaqueNamedParams& aParams,
        const TDesC& aParamName, HBufC*& aBuf )
    {
    TPtr value( NULL, 0 );
    TRAPD( err, value = aParams.StringByNameL( aParamName ) );
    if( !err && value.Length() )
        {
        if( aBuf )
            {
            delete aBuf;
            aBuf = NULL;
            }
        aBuf = value.AllocL();
        }
    }

