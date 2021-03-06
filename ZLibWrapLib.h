//------------------------------------------------------------------------------
//
//    Copyright (C) Streamlet. All rights reserved.
//
//    File Name:   ZLibWrapLib.h
//    Author:      Streamlet
//    Create Time: 2010-09-14
//    Description: 
//
//    Version history:
//				   2011-11-16 1.0.0.3
//				   2016-08-11 1.0.0.3x by @rrrfff
//
//
//------------------------------------------------------------------------------

#ifndef __ZLIBWRAPLIB_H_C9F256BA_4887_4C1C_A594_17452697B02B_INCLUDED__
#define __ZLIBWRAPLIB_H_C9F256BA_4887_4C1C_A594_17452697B02B_INCLUDED__

#include "ZLibInternal.h"

//------------------------------------------------------------------------------
// Description: Compress files to a ZIP file.
// Parameter: lpszSourceFiles Source files, supporting wildcards.
// Parameter: lpszDestFile The ZIP file path.
// Parameter: bUtf8 If using UTF-8 to encode the file name.
// Return Value: TRUE/FALSE.
//------------------------------------------------------------------------------
BOOL ZipCompress(LPCTSTR lpszSourceFiles, LPCTSTR lpszDestFile, bool bUtf8 = false);

//------------------------------------------------------------------------------
// Description: Extract files from a ZIP file.
// Parameter: lpszSourceFile Source ZIP file.
// Parameter: lpszDestFolder The folder to output files. The parent of the
//                           specified folder MUST exist.
// Return Value: TRUE/FALSE.
//------------------------------------------------------------------------------
BOOL ZipExtract(LPCTSTR lpszSourceFile, LPCTSTR lpszDestFolder, const ZIP_IO &io);

#endif // #ifndef __ZLIBWRAPLIB_H_C9F256BA_4887_4C1C_A594_17452697B02B_INCLUDED__
