@rem
@rem Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
@rem All rights reserved.
@rem This component and the accompanying materials are made available
@rem under the terms of the License "Eclipse Public License v1.0"
@rem which accompanies this distribution, and is available
@rem at the URL "http://www.eclipse.org/legal/epl-v10.html".
@rem
@rem Initial Contributors:
@rem Nokia Corporation - initial contribution.
@rem
@rem Contributors:
@rem
@rem Description:
@rem

:: This batch file creates sis file/files
@ECHO OFF
REM testcaseM001
call interpretsis -s \epoc32\winscw\c\tswi\tinterpretsisinteg\sisfiles\interpretsism001.sis -t .\romdrive\system\install\ -c .\cdrive -r \epoc32\winscw\c\tswi\tinterpretsisinteg\testdatam001\rombld.log,\epoc32\winscw\c\tswi\tinterpretsisinteg\testdatam001\rombld.rofs.log -w info -l /epoc32/winscw/c/interpretsis_test_harness.txt
IF NOT %errorlevel%==0 GOTO LAST
ECHO ***ERRORCODE*** %errorlevel% PASS>>/epoc32/winscw/c/interpretsis_test_harness.txt
GOTO END
:LAST
ECHO ***ERRORCODE*** %errorlevel% FAIL>>/epoc32/winscw/c/interpretsis_test_harness.txt
:END
