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
REM testcaseA001
if not exist .\cdrive mkdir .\cdrive > NUL
if not exist .\romdrive mkdir .\romdrive > NUL

call interpretsis -z .\romdrive -c .\cdrive  -s /epoc32/winscw/c/tswi/tinterpretsisinteg/sisfiles/testfile1.sis -w info -l /epoc32/winscw/c/interpretsis_test_harness_db.txt > NUL
if not %errorlevel%==0 GOTO LAST
if not exist .\cdrive\private\89334471\import\testresult1.dat GOTO LAST > NUL
ECHO Passed
GOTO END

:LAST
ECHO ***ERRORCODE*** %errorlevel% FAIL>>/epoc32/winscw/c/interpretsis_test_harness_db.txt

:END
rmdir /S /Q .\cdrive > NUL
rmdir /S /Q .\romdrive > NUL
