/*
 *	rrrfff 2016/08/07
 */
#pragma once

typedef HANDLE ZIP_HANDLE;

//-------------------------------------------------------------------------

struct ZIP_IO
{
	ZIP_HANDLE (*IO_Create)(const String &filepath); // INVALID_HANDLE_VALUE
	void (*IO_Write)(ZIP_HANDLE zh, const void *buffer, intptr_t count);
	void (*IO_Flush)(ZIP_HANDLE zh); // Last call
	HANDLE (*IO_NativeHandle)(ZIP_HANDLE zh);
};
