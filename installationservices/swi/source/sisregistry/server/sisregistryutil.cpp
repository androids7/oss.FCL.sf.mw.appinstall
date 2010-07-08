/*
* Copyright (c) 2004-2010 Nokia Corporation and/or its subsidiary(-ies).
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
* SisRegistryUtil class - implementation
* A collection of static functions to provide useful functionality to a the sisregistryserver as a whole 	
*
*/


/**
 @file 
 @released
 @internalComponent
*/
#include "sisregistryutil.h"
#include <s32file.h>
#include "log.h"
#include "sisregistryserverconst.h"
#include "dessisdataprovider.h"
#include "sisregistryobject.h"
#include "sisinstallblock.h"
#include "sisregistryfiledescription.h"
#include "sisinfo.h"
#include "filesisdataprovider.h"
#include "sisuid.h"

using namespace Swi;

const TInt KStubSisPUidPos = 0x18;

HBufC* SisRegistryUtil::BuildControllerFileNameLC(TUid aUid, TInt aIndex, TInt aController, TInt aDrive)
	{
	TDriveUnit drive(aDrive);
	TDriveName driveName = drive.Name();
	HBufC *filename = HBufC::NewMaxLC(KMaxFileName);
	TPtr filenamePtr(filename->Des());
	filenamePtr.Format(KControllerFileNameFormat, &driveName, &KRegistryPath, aUid.iUid,
					aIndex, aController);
	return filename;
	}

HBufC* SisRegistryUtil::BuildStubFilesPathLC(TInt aDrive)
	{
	TDriveUnit drive(aDrive);
	TDriveName driveName = drive.Name();
	HBufC *filePath = HBufC::NewMaxLC(KMaxFileName);
	TPtr filePathPtr(filePath->Des());
	filePathPtr.Format(_L("%S%S"), &driveName, &KSwiDaemonPrivateDirectory);
	return filePath;
	}

HBufC* SisRegistryUtil::BuildControllerFileNameLC(TUid aUid, TInt aIndex, TInt aController)
	{
	return BuildControllerFileNameLC(aUid, aIndex, aController, RFs::GetSystemDrive());
	}

HBufC* SisRegistryUtil::BuildUninstallLogFileNameLC(TUid aUid, TInt aIndex, TChar aDriveLetter)
	{
	TDriveUnit systemDrive(RFs::GetSystemDrive());
	TDriveName systemDriveName = systemDrive.Name();
	HBufC *filename = HBufC::NewMaxLC(KMaxFileName);
	TPtr filenamePtr(filename->Des());
	filenamePtr.Format(KUninstallLogPathFormat, (TUint)systemDriveName[0], aUid.iUid, aIndex, (TUint)aDriveLetter);	
	return filename;
	}

TBool SisRegistryUtil::FileExistsL(RFs& aFs, const TDesC& aFullPath)
	{
	TBool found = EFalse;
	TEntry entry;
	TInt err = aFs.Entry(aFullPath, entry);
	if (err == KErrNone)
		{
		return  ETrue;
		}
	else if (err != KErrNotFound && err != KErrPathNotFound)
		{
		User::Leave(err);
		}
	
	return found;
	}

void SisRegistryUtil::EnsureDirExistsL(RFs& aFs, const TDesC& aPath)
	{
	TInt err = aFs.MkDirAll(aPath);
	if (err != KErrNone && err != KErrAlreadyExists)
		{
		User::LeaveIfError(err);
		}
	}

void SisRegistryUtil::CreateFileWithAttributesL(RFs& aFs, const TDesC& aPath, const TUint aAttributesMask/*= 0*/)
    {
    // Creates the folder structure by ignoring the filename at the end.
    EnsureDirExistsL(aFs, aPath);

    RFile file;
    CleanupClosePushL(file);
    User::LeaveIfError(file.Create(aFs, aPath, EFileWrite));
   
    if(aAttributesMask != 0)
        {
        file.SetAtt(aAttributesMask, 0); // Ignoring return value.
        }
    CleanupStack::PopAndDestroy(&file);
    }

TInt SisRegistryUtil::DeleteFile(RFs& aFs, const TDesC& aPath)
    {
    TInt err = aFs.Delete(aPath);
    
    if(KErrNone != err)
        {
        DEBUG_PRINTF2(_L("Failed to delete file %S."), &aPath);
        }
    else
        {
        DEBUG_PRINTF2(_L("Deleted file %S successfully."), &aPath);
        }
    
    return err;
    }

// Processes the ROM stub files. Retuns ETrue if the aUid or aName
// is found in any of the ROM stub files, else returns EFalse.
TBool SisRegistryUtil::RomBasedPackageL(const TUid& aUid)
	{	
	TBool isInRom = EFalse;
	
	RFs fileServer;
	User::LeaveIfError(fileServer.Connect());
	CleanupClosePushL(fileServer);
	
	CDir* stubList = NULL;
	_LIT(KRomStubPath, "z:\\system\\install\\%S");
	_LIT(KRomStubWildCard,"z:\\system\\install\\*.sis");
	
	TEntry stubEntry;
	RBuf stubFileName;
	stubFileName.CreateL(KMaxFileName);
	CleanupClosePushL(stubFileName);
	//Iterates through the stub files in the ROM calling CheckIfMatchingStubFileL() 
	//to search for the presence of the package name ignoring any SIS parsing errors.
	if (fileServer.GetDir(KRomStubWildCard,KEntryAttNormal, ESortByName, stubList)==KErrNone)
		{		
		CleanupStack::PushL(stubList);		
		for (TInt stubCount = 0; stubCount < stubList->Count(); stubCount++)
			{			
			stubEntry = (*stubList)[stubCount];
			stubFileName.Format(KRomStubPath, &(stubEntry.iName));		
			
			
			TRAPD(err, isInRom = SisRegistryUtil::CheckIfMatchingStubPUidL(aUid,  fileServer, stubFileName));
			
			if ((err >= KErrSISNotEnoughSpaceToInstall)  && (err <= KErrSISFieldIdMissing))
  				{
  				//Error reading SIS stub details, skip this one and continue searching the list.
  				DEBUG_PRINTF3(_L("ERROR: Stub file in ROM failed to parse. Filename: %S. Error code: %d."), &stubEntry.iName, err);
  				}
  			else if (err != KErrNone)
  				{
  				User::Leave(err);
  				}
  			else if (isInRom)
  				{
				break;
  				}
			}
		CleanupStack::PopAndDestroy(stubList);
		}			
	CleanupStack::PopAndDestroy(2, &fileServer); // stubFileName
	return isInRom;
	}

 HBufC* SisRegistryUtil::BuildLogFileNameLC()
 	{
 	HBufC *filename = HBufC::NewMaxLC(KMaxFileName);
 	TPtr filenamePtr(filename->Des());
 	TDriveUnit sysDrive(RFs::GetSystemDrive());
 	filenamePtr = sysDrive.Name();
 	filenamePtr.Append(KLogDir);
 	return filename;
 	}

TUid SisRegistryUtil::IdentifyUidFromSisFileL(RFs& aFs, TDesC& aFilename)
	{
	// Returns the package UID read from the symbian header in the specified
	// file.  Note that there is no check to ensure that it is a valid sisx
	// file, but it will leave if the file is too short.
	RFile file;
	User::LeaveIfError(file.Open(aFs, aFilename, EFileRead | EFileStream));
	CleanupClosePushL(file);
	TUid packageUid;
	TUid uid1;
	TUid uid2;
	ReadSymbianHeaderL(file, uid1, uid2, packageUid);
	CleanupStack::PopAndDestroy(&file);
	return packageUid;
	}

void SisRegistryUtil::ReadSymbianHeaderL(RFile& aFile, TUid& aUid1, TUid& aUid2, TUid& aUid3)
	{
	// Read the first 3 32-bit values from the file as UIDs 1 to 3, leave with
	// KErrUnderflow if the file is too short.
	TInt uidLen = sizeof(TInt32);

	TPckg<TInt32> uid1(aUid1.iUid);
	User::LeaveIfError(aFile.Read(uid1, uidLen));
	if (uid1.Length() != uidLen)
		{
		User::Leave(KErrUnderflow);
		}

	TPckg<TInt32> uid2(aUid2.iUid);
	User::LeaveIfError(aFile.Read(uid2, uidLen));
	if (uid1.Length() != uidLen)
		{
		User::Leave(KErrUnderflow);
		}

	TPckg<TInt32> uid3(aUid3.iUid);
	User::LeaveIfError(aFile.Read(uid3, uidLen));
	if (uid1.Length() != uidLen)
		{
		User::Leave(KErrUnderflow);
		}
	}

/**
Reads the pUid of stub sis file from a given position in the file.
and match it with aUid
@param aName The file name being searched for.
@param aUid  The package uid being searched for.
@param aFileServer Handle to the file server to use.
@param aStubFileName Complete path and filename of the stub file being checked.
@return ETrue if the package uid or the pkg name is found, EFalse otherwise.
*/
TBool SisRegistryUtil::CheckIfMatchingStubPUidL(const TUid& aUid, RFs& aFileServer, const TFileName& aStubFileName)
	{
	
	RFile sisFile;
	User::LeaveIfError(sisFile.Open(aFileServer, aStubFileName, EFileRead));
	CleanupClosePushL(sisFile);
	RFileReadStream sisFileReadStream(sisFile, KStubSisPUidPos);
	CleanupClosePushL(sisFileReadStream);
	
	TUid packageUid = TUid::Uid(sisFileReadStream.ReadUint32L());
	
	CleanupStack::PopAndDestroy(2,&sisFile); //readstream, file
	return (aUid == packageUid);
	
	}

