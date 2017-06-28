//------------------------------------------------------------------------------
//
//    Copyright (C) Streamlet. All rights reserved.
//
//    File Name:   ZLibWrapLib.cpp
//    Author:      Streamlet
//    Create Time: 2010-09-16
//    Description: 
//
//    Version history:
//				   2011-11-16 1.0.0.3
//				   2016-08-11 1.0.0.3x by @rrrfff
//
//
//
//------------------------------------------------------------------------------

#include "../zlib/zlib.h"
#define _ZLIB_H
#include "MiniZip/zip.h"
#include "MiniZip/unzip.h"
#include <RLib_Import.h>
#include "ZLibWrapLib.h"

#define ZIP_GPBF_LANGUAGE_ENCODING_FLAG 0x800

//-------------------------------------------------------------------------

static BOOL RLIB_VECTORCALL ZipAddFile(zipFile zf, const String &szFileNameInZip, const String &szFilePath, bool bUtf8)
{
    DWORD dwFileAttr = GetFileAttributes(szFilePath);

	if (dwFileAttr == INVALID_FILE_ATTRIBUTES) {
		return FALSE;
	} //if

	ManagedObject<FileStream> file = File::Create(szFilePath, FileMode::OpenExist, FileAccess::Read, 
												  FileAttributes::Normal, FileShare::Read, 
												  (dwFileAttr & FILE_ATTRIBUTE_DIRECTORY) != 0 ? static_cast<FileOptions>(FILE_OPEN_FOR_BACKUP_INTENT) : FileOptions::None);
	if (!file) {
		return FALSE;
	} //if
    
    FILETIME ftUTC, ftLocal;
    GetFileTime(*file, NULL, NULL, &ftUTC);
    FileTimeToLocalFileTime(&ftUTC, &ftLocal);

    WORD wDate, wTime;
    FileTimeToDosDateTime(&ftLocal, &wDate, &wTime);

    zip_fileinfo FileInfo;
    ZeroMemory(&FileInfo, sizeof(FileInfo));

    FileInfo.dosDate      = (static_cast<DWORD>(wDate) << 16) | static_cast<DWORD>(wTime);
    FileInfo.external_fa |= dwFileAttr;

    if (bUtf8) {
		if (zipOpenNewFileInZip4(zf, GlobalizeString(szFileNameInZip).toUtf8(), &FileInfo, NULL, 0, NULL, 0, NULL, Z_DEFLATED, 9,
								 0, -MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY, NULL, 0, 0, ZIP_GPBF_LANGUAGE_ENCODING_FLAG) != ZIP_OK) {
			return FALSE;
		} //if
    } else {
		if (zipOpenNewFileInZip(zf, GlobalizeString(szFileNameInZip).toGBK(), &FileInfo, NULL, 0, NULL, 0, NULL, Z_DEFLATED, 9) != ZIP_OK) {
			return FALSE;
		} //if
	} //if

	if ((dwFileAttr & FILE_ATTRIBUTE_DIRECTORY) == 0) {

		BYTE byBuffer[RLIB_DEFAULT_MAX_BUFFER_SIZE];

		LARGE_INTEGER length = file->GetLength64();
		while (length.QuadPart > 0)
		{
			DWORD dwSizeToRead = length.QuadPart > RLIB_DEFAULT_MAX_BUFFER_SIZE ? RLIB_DEFAULT_MAX_BUFFER_SIZE : static_cast<DWORD>(length.LowPart);
			DWORD dwRead       = static_cast<DWORD>(file->Read(byBuffer, dwSizeToRead));
			if (dwRead <= 0) {
				return FALSE;
			} //if

			if (zipWriteInFileInZip(zf, byBuffer, dwRead) < 0) {
				return FALSE;
			} //if

			length.QuadPart -= dwRead;
		}
	} //if

	zipCloseFileInZip(zf);
    return TRUE;
}

//-------------------------------------------------------------------------

static BOOL RLIB_VECTORCALL ZipAddFiles(zipFile zf, const String &strFileNameInZip, String strFilePath, bool bUtf8)
{
    WIN32_FIND_DATA wfd = {0};

    HANDLE hFind = FindFirstFile(strFilePath, &wfd);
	if (hFind == INVALID_HANDLE_VALUE) {
		return FALSE;
	} //if

	AutoFinalize<HANDLE> __finalizer([](HANDLE hFind) {
		FindClose(hFind);
	}, hFind);

    int nPos = strFilePath.LastIndexOf(_T('\\'));
	if (nPos != -1) {
		strFilePath = strFilePath.Substring(0, nPos + 1);
	} else {
		strFilePath.Empty();
	} //if

    do 
    {
        String strFileName = wfd.cFileName;

		if (strFileName == _T(".") || strFileName == _T("..")) {
			continue;
		} //if

        if ((wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
			if (!ZipAddFile(zf, strFileNameInZip + strFileName + _R("/"), strFilePath + strFileName, bUtf8)) {
				return FALSE;
			} //if

			if (!ZipAddFiles(zf, strFileNameInZip + strFileName + _R("/"), strFilePath + strFileName + _R("\\*"), bUtf8)) {
				return FALSE;
			} //if
        } else {
			if (!ZipAddFile(zf, strFileNameInZip + strFileName, strFilePath + strFileName, bUtf8)) {
				return FALSE;
			} //if
        } //if

    } while (FindNextFile(hFind, &wfd));

    return TRUE;
}

//-------------------------------------------------------------------------

BOOL ZipCompress(LPCTSTR lpszSourceFiles, LPCTSTR lpszDestFile, bool bUtf8 /*= false*/)
{
    zipFile zf = zipOpen64(GlobalizeString(lpszDestFile).toGBK(), 0);

	if (zf == NULL) {
		return FALSE;
	} //if

	BOOL ret = ZipAddFiles(zf, _T(""), lpszSourceFiles, bUtf8);

	zipClose(zf, NULL);

	return ret;
}

//-------------------------------------------------------------------------

static BOOL RLIB_VECTORCALL ZipExtractCurrentFile(unzFile uf, String strDestPath, const ZIP_IO &io)
{
    char szFilePathA[RLIB_MAX_PATH];
    unz_file_info64 FileInfo;

	if (unzGetCurrentFileInfo64(uf, &FileInfo, szFilePathA, sizeof(szFilePathA), NULL, 0, NULL, 0) != UNZ_OK) {
		return FALSE;
	} //if

	if (unzOpenCurrentFile(uf) != UNZ_OK) {
		return FALSE;
	} //if
    
	AutoFinalize<unzFile> __finalizer([](unzFile uf) {
		unzCloseCurrentFile(uf);
	}, uf);

    String strFileName;
	if ((FileInfo.flag & ZIP_GPBF_LANGUAGE_ENCODING_FLAG) != 0) {
		auto lpwstr = GlobalizeString::ConvertToWideChar(szFilePathA, -1, nullptr, UTF8Encoding);
		if (!lpwstr) {
			return FALSE;
		} //if
		strFileName = lpwstr;
		String::Collect(lpwstr);
	} else {
        strFileName = szFilePathA;
    } //if

    intptr_t nLength       = strFileName.Length;
    LPTSTR lpszFileName    = strFileName;
    LPTSTR lpszCurrentFile = lpszFileName;

    for (intptr_t i = 0; i <= nLength; ++i)
    {
		if (lpszFileName[i] == _T('\0')) {
			strDestPath += lpszCurrentFile;
			break;
		} //if

		if (lpszFileName[i] == _T('\\') || lpszFileName[i] == _T('/')) {

			lpszFileName[i] = _T('\0');

			strDestPath += lpszCurrentFile;
			strDestPath += _R("\\");

			Directory::CreateIf(strDestPath);

			lpszCurrentFile = lpszFileName + i + 1;
		} //if
    }

	if (lpszCurrentFile[0] == _T('\0')) {
		return TRUE;
	} //if

//	ManagedObject<FileStream> file = File::Create(strDestPath, FileMode::CreateAlways, FileAccess::Write);
// 	if (!file) {
// 		return FALSE;
// 	} //if
	ZIP_HANDLE zh = io.IO_Create(strDestPath);
	if (zh == INVALID_HANDLE_VALUE) {
		return FALSE;
	} //if

	FILETIME ftLocal, ftUTC;
	DosDateTimeToFileTime(static_cast<WORD>(FileInfo.dosDate >> 16), static_cast<WORD>(FileInfo.dosDate), &ftLocal);
	LocalFileTimeToFileTime(&ftLocal, &ftUTC);
//	SetFileTime(*file, &ftUTC, &ftUTC, &ftUTC);
	SetFileTime(io.IO_NativeHandle(zh), &ftUTC, &ftUTC, &ftUTC);

    BYTE byBuffer[RLIB_DEFAULT_MAX_BUFFER_SIZE];
	int nSize;
    while ((nSize = unzReadCurrentFile(uf, byBuffer, RLIB_DEFAULT_MAX_BUFFER_SIZE)) > 0)
    {
//		file->Write(byBuffer, nSize);
		io.IO_Write(zh, byBuffer, nSize);
    }

	io.IO_Flush(zh);
    
    return nSize == 0;
}

//-------------------------------------------------------------------------

BOOL ZipExtract(LPCTSTR lpszSourceFile, LPCTSTR lpszDestFolder, const ZIP_IO &io)
{
    unzFile uf = unzOpen64(RT2A(lpszSourceFile));

	if (uf == NULL) {
		return FALSE;
	} //if

	AutoFinalize<unzFile> __finalizer([](unzFile uf) {
		unzClose(uf);
	}, uf);

    unz_global_info64 gi;
	if (unzGetGlobalInfo64(uf, &gi) != UNZ_OK) {
		return FALSE;
	} //if

    String strDestFolder = Path::AddBackslash(lpszDestFolder);
	Directory::CreateIf(strDestFolder);
    
    for (int i = 0; i < gi.number_entry; ++i)
    {
		if (!ZipExtractCurrentFile(uf, strDestFolder, io)) {
			return FALSE;
		} //if
        
		if (i < gi.number_entry - 1) {
			if (unzGoToNextFile(uf) != UNZ_OK) {
				return FALSE;
			} //if
		} //if
    }
  
    return TRUE;
}
