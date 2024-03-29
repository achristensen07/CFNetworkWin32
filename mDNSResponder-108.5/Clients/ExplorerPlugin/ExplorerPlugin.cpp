/*
 * Copyright (c) 2003-2004 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this
 * file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 * 
 * @APPLE_LICENSE_HEADER_END@

    Change History (most recent first):
    
$Log: ExplorerPlugin.cpp,v $
Revision 1.8  2005/06/30 18:01:54  shersche
<rdar://problem/4130635> Cause IE to rebuild cache so we don't have to reboot following an install.

Revision 1.7  2005/02/23 02:00:45  shersche
<rdar://problem/4014479> Delete all the registry entries when component is unregistered

Revision 1.6  2005/01/25 17:56:45  shersche
<rdar://problem/3911084> Load resource DLLs, get icons and bitmaps from resource DLLs
Bug #: 3911084

Revision 1.5  2004/09/15 10:33:54  shersche
<rdar://problem/3721611> Install XP toolbar button (8 bit mask) if running on XP platform, otherwise install 1 bit mask toolbar button
Bug #: 3721611

Revision 1.4  2004/07/13 21:24:21  rpantos
Fix for <rdar://problem/3701120>.

Revision 1.3  2004/06/26 14:12:07  shersche
Register the toolbar button

Revision 1.2  2004/06/24 20:09:39  shersche
Change text
Submitted by: herscher

Revision 1.1  2004/06/18 04:34:59  rpantos
Move to Clients from mDNSWindows

Revision 1.1  2004/01/30 03:01:56  bradley
Explorer Plugin to browse for DNS-SD advertised Web and FTP servers from within Internet Explorer.

*/

#include	"StdAfx.h"

// The following 2 includes have to be in this order and INITGUID must be defined here, before including the file
// that specifies the GUID(s), and nowhere else. The reason for this is that initguid.h doesn't provide separate 
// define and declare macros for GUIDs so you have to #define INITGUID in the single file where you want to define 
// your GUID then in all the other files that just need the GUID declared, INITGUID must not be defined.

#define	INITGUID
#include	<initguid.h>
#include	"ExplorerPlugin.h"

#include	<comcat.h>
#include	<Shlwapi.h>

#include	"CommonServices.h"
#include	"DebugServices.h"

#include	"ClassFactory.h"
#include	"Resource.h"

#include	"loclibrary.h"

// MFC Debugging

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#if 0
#pragma mark == Prototypes ==
#endif

//===========================================================================================================================
//	Prototypes
//===========================================================================================================================

// DLL Exports

extern "C" BOOL WINAPI	DllMain( HINSTANCE inInstance, DWORD inReason, LPVOID inReserved );

// MFC Support

DEBUG_LOCAL OSStatus	MFCDLLProcessAttach( HINSTANCE inInstance );
DEBUG_LOCAL void		MFCDLLProcessDetach( HINSTANCE inInstance );
DEBUG_LOCAL void		MFCDLLThreadDetach( HINSTANCE inInstance );

// Utilities

DEBUG_LOCAL OSStatus	RegisterServer( HINSTANCE inInstance, CLSID inCLSID, LPCTSTR inName );
DEBUG_LOCAL OSStatus	RegisterCOMCategory( CLSID inCLSID, CATID inCategoryID, BOOL inRegister );
DEBUG_LOCAL OSStatus	UnregisterServer( CLSID inCLSID );
DEBUG_LOCAL OSStatus	MyRegDeleteKey( HKEY hKeyRoot, LPTSTR lpSubKey );

// Stash away pointers to our resource DLLs

static HINSTANCE g_nonLocalizedResources	= NULL;
static CString	 g_nonLocalizedResourcesName;
static HINSTANCE g_localizedResources		= NULL;

HINSTANCE
GetNonLocalizedResources()
{
	return g_nonLocalizedResources;
}

HINSTANCE
GetLocalizedResources()
{
	return g_localizedResources;
}

// This is the class GUID for an undocumented hook into IE that will allow us to register
// and have IE notice our new ExplorerBar without rebooting.
// {8C7461EF-2B13-11d2-BE35-3078302C2030}

DEFINE_GUID(CLSID_CompCatCacheDaemon, 
0x8C7461EF, 0x2b13, 0x11d2, 0xbe, 0x35, 0x30, 0x78, 0x30, 0x2c, 0x20, 0x30);


#if 0
#pragma mark == Globals ==
#endif

//===========================================================================================================================
//	Globals
//===========================================================================================================================

HINSTANCE		gInstance		= NULL;
int				gDLLRefCount	= 0;
CWinApp			gApp;

#if 0
#pragma mark -
#pragma mark == DLL Exports ==
#endif

//===========================================================================================================================
//	DllMain
//===========================================================================================================================

BOOL WINAPI	DllMain( HINSTANCE inInstance, DWORD inReason, LPVOID inReserved )
{
	BOOL			ok;
	OSStatus		err;
	
	DEBUG_UNUSED( inReserved );
	
	ok = TRUE;
	switch( inReason )
	{
		case DLL_PROCESS_ATTACH:
			gInstance = inInstance;
			debug_initialize( kDebugOutputTypeWindowsEventLog, "DNSServices Bar", inInstance );
			debug_set_property( kDebugPropertyTagPrintLevel, kDebugLevelTrace );
			dlog( kDebugLevelTrace, "\nDllMain: process attach\n" );
			
			err = MFCDLLProcessAttach( inInstance );
			ok = ( err == kNoErr );
			require_noerr( err, exit );
			break;
		
		case DLL_PROCESS_DETACH:
			dlog( kDebugLevelTrace, "DllMain: process detach\n" );
			MFCDLLProcessDetach( inInstance );
			break;
		
		case DLL_THREAD_ATTACH:
			dlog( kDebugLevelTrace, "DllMain: thread attach\n" );
			break;
		
		case DLL_THREAD_DETACH:
			dlog( kDebugLevelTrace, "DllMain: thread detach\n" );
			MFCDLLThreadDetach( inInstance );
			break;
		
		default:
			dlog( kDebugLevelTrace, "DllMain: unknown reason code (%d)\n",inReason );
			break;
	}
	
exit:
	return( ok );
}

//===========================================================================================================================
//	DllCanUnloadNow
//===========================================================================================================================

STDAPI	DllCanUnloadNow( void )
{
	dlog( kDebugLevelTrace, "DllCanUnloadNow (refCount=%d)\n", gDLLRefCount );
	
	return( gDLLRefCount == 0 );
}

//===========================================================================================================================
//	DllGetClassObject
//===========================================================================================================================

STDAPI	DllGetClassObject( REFCLSID inCLSID, REFIID inIID, LPVOID *outResult )
{
	HRESULT				err;
	BOOL				ok;
	ClassFactory *		factory;
	
	dlog( kDebugLevelTrace, "DllGetClassObject\n" );
	
	*outResult = NULL;
	
	// Check if the class ID is supported.
	
	ok = IsEqualCLSID( inCLSID, CLSID_ExplorerBar );
	require_action_quiet( ok, exit, err = CLASS_E_CLASSNOTAVAILABLE );
	
	// Create the ClassFactory object.
	
	factory = NULL;
	try
	{
		factory = new ClassFactory( inCLSID );
	}
	catch( ... )
	{
		// Do not let exception escape.
	}
	require_action( factory, exit, err = E_OUTOFMEMORY );
	
	// Query for the specified interface. Release the factory since QueryInterface retains it.
	
	err = factory->QueryInterface( inIID, outResult );
	factory->Release();
	
exit:
	return( err );
}

//===========================================================================================================================
//	DllRegisterServer
//===========================================================================================================================

STDAPI	DllRegisterServer( void )
{
	IRunnableTask * pTask = NULL;
	HRESULT			err;
	BOOL			ok;
	CString			s;
	
	dlog( kDebugLevelTrace, "DllRegisterServer\n" );
	
	ok = s.LoadString( IDS_NAME );
	require_action( ok, exit, err = E_UNEXPECTED );
	
	err = RegisterServer( gInstance, CLSID_ExplorerBar, s );
	require_noerr( err, exit );
	
	err = RegisterCOMCategory( CLSID_ExplorerBar, CATID_InfoBand, TRUE );
	require_noerr( err, exit );

	// <rdar://problem/4130635> Clear IE cache so it will rebuild the cache when it runs next.  This
	// will allow us to install and not reboot

	err = CoCreateInstance(CLSID_CompCatCacheDaemon, NULL, CLSCTX_INPROC, IID_IRunnableTask, (void**) &pTask);
	require_noerr( err, exit );

	pTask->Run();
	pTask->Release();

exit:
	return( err );
}

//===========================================================================================================================
//	DllUnregisterServer
//===========================================================================================================================

STDAPI	DllUnregisterServer( void )
{
	HRESULT		err;
	
	dlog( kDebugLevelTrace, "DllUnregisterServer\n" );
	
	err = RegisterCOMCategory( CLSID_ExplorerBar, CATID_InfoBand, FALSE );
	require_noerr( err, exit );

	err = UnregisterServer( CLSID_ExplorerBar );
	require_noerr( err, exit );
	
exit:
	return( err );
}

#if 0
#pragma mark -
#pragma mark == MFC Support ==
#endif

//===========================================================================================================================
//	MFCDLLProcessAttach
//===========================================================================================================================

DEBUG_LOCAL OSStatus	MFCDLLProcessAttach( HINSTANCE inInstance )
{
	wchar_t					resource[MAX_PATH];
	OSStatus				err;
	_AFX_THREAD_STATE *		threadState;
	AFX_MODULE_STATE *		previousModuleState;
	BOOL					ok;
	int						res;
	CWinApp *				app;
	
	app = NULL;
	
	// Simulate what is done in dllmodul.cpp.
	
	threadState = AfxGetThreadState();
	check( threadState );
	previousModuleState = threadState->m_pPrevModuleState;
	
	ok = AfxWinInit( inInstance, NULL, TEXT( "" ), 0 );
	require_action( ok, exit, err = kUnknownErr );
	
	app = AfxGetApp();
	require_action( ok, exit, err = kNotInitializedErr );
	
	// Before we load the resources, let's load the error string

	// errorMessage.LoadString( IDS_REINSTALL );
	// errorCaption.LoadString( IDS_REINSTALL_CAPTION );

	// Load Resources

	res = PathForResource( inInstance, L"ExplorerPluginResources.dll", resource, MAX_PATH );

	err = translate_errno( res != 0, kUnknownErr, kUnknownErr );
	require_noerr( err, exit );

	g_nonLocalizedResources = LoadLibrary( resource );
	translate_errno( g_nonLocalizedResources, GetLastError(), kUnknownErr );
	require_noerr( err, exit );

	g_nonLocalizedResourcesName = resource;

	res = PathForResource( inInstance, L"ExplorerPluginLocalized.dll", resource, MAX_PATH );
	err = translate_errno( res != 0, kUnknownErr, kUnknownErr );
	require_noerr( err, exit );

	g_localizedResources = LoadLibrary( resource );
	translate_errno( g_localizedResources, GetLastError(), kUnknownErr );
	require_noerr( err, exit );

	AfxSetResourceHandle( g_localizedResources );

	ok = app->InitInstance();
	require_action( ok, exit, err = kUnknownErr );
	
	threadState->m_pPrevModuleState = previousModuleState;
	threadState = NULL;
	AfxInitLocalData( inInstance );
	err = kNoErr;
	
exit:
	if( err )
	{
		if( app )
		{
			app->ExitInstance();
		}
		AfxWinTerm();
	}
	if( threadState )
	{
		threadState->m_pPrevModuleState = previousModuleState;
	}
	return( err );
}

//===========================================================================================================================
//	MFCDLLProcessDetach
//===========================================================================================================================

DEBUG_LOCAL void	MFCDLLProcessDetach( HINSTANCE inInstance )
{
	CWinApp *		app;

	// Simulate what is done in dllmodul.cpp.
	
	app = AfxGetApp();
	if( app )
	{
		app->ExitInstance();
	}

#if( DEBUG )
	if( AfxGetModuleThreadState()->m_nTempMapLock != 0 )
	{
		dlog( kDebugLevelWarning, "Warning: Temp map lock count non-zero (%ld).\n", AfxGetModuleThreadState()->m_nTempMapLock );
	}
#endif
	
	AfxLockTempMaps();
	AfxUnlockTempMaps( -1 );

	// Terminate the library before destructors are called.
	
	AfxWinTerm();
	AfxTermLocalData( inInstance, TRUE );
}

//===========================================================================================================================
//	MFCDLLFinalize
//===========================================================================================================================

DEBUG_LOCAL void	MFCDLLThreadDetach( HINSTANCE inInstance )
{
	// Simulate what is done in dllmodul.cpp.
	
#if( DEBUG )
	if( AfxGetModuleThreadState()->m_nTempMapLock != 0 )
	{
		dlog( kDebugLevelWarning, "Warning: Temp map lock count non-zero (%ld).\n", AfxGetModuleThreadState()->m_nTempMapLock );
	}
#endif
	
	AfxLockTempMaps();
	AfxUnlockTempMaps( -1 );
	AfxTermThread( inInstance );
}

#if 0
#pragma mark -
#pragma mark == Utilities ==
#endif

//===========================================================================================================================
//	RegisterServer
//===========================================================================================================================

DEBUG_LOCAL OSStatus	RegisterServer( HINSTANCE inInstance, CLSID inCLSID, LPCTSTR inName )
{
	typedef struct	RegistryBuilder		RegistryBuilder;
	struct	RegistryBuilder
	{
		HKEY		rootKey;
		LPCTSTR		subKey;
		LPCTSTR		valueName;
		LPCTSTR		data;
	};
	
	OSStatus			err;
	LPWSTR				clsidWideString;
	TCHAR				clsidString[ 64 ];
	DWORD				nChars;
	size_t				n;
	size_t				i;
	HKEY				key;
	TCHAR				keyName[ MAX_PATH ];
	TCHAR				moduleName[ MAX_PATH ] = TEXT( "" );
	TCHAR				data[ MAX_PATH ];
	RegistryBuilder		entries[] = 
	{
		{ HKEY_CLASSES_ROOT,	TEXT( "CLSID\\%s" ),					NULL,						inName },
		{ HKEY_CLASSES_ROOT,	TEXT( "CLSID\\%s\\InprocServer32" ),	NULL,						moduleName },
		{ HKEY_CLASSES_ROOT,	TEXT( "CLSID\\%s\\InprocServer32" ),  	TEXT( "ThreadingModel" ),	TEXT( "Apartment" ) }
	};
	DWORD				size;
	OSVERSIONINFO		versionInfo;
	
	// Convert the CLSID to a string based on the encoding of this code (ANSI or Unicode).
	
	err = StringFromIID( inCLSID, &clsidWideString );
	require_noerr( err, exit );
	require_action( clsidWideString, exit, err = kNoMemoryErr );
	
	#ifdef UNICODE
		lstrcpyn( clsidString, clsidWideString, sizeof_array( clsidString ) );
		CoTaskMemFree( clsidWideString );
	#else
		nChars = WideCharToMultiByte( CP_ACP, 0, clsidWideString, -1, clsidString, sizeof_array( clsidString ), NULL, NULL );
		err = translate_errno( nChars > 0, (OSStatus) GetLastError(), kUnknownErr );
		CoTaskMemFree( clsidWideString );
		require_noerr( err, exit );
	#endif
	
	// Register the CLSID entries.
	
	nChars = GetModuleFileName( inInstance, moduleName, sizeof_array( moduleName ) );
	err = translate_errno( nChars > 0, (OSStatus) GetLastError(), kUnknownErr );
	require_noerr( err, exit );

	n = sizeof_array( entries );
	for( i = 0; i < n; ++i )
	{
		wsprintf( keyName, entries[ i ].subKey, clsidString );		
		err = RegCreateKeyEx( entries[ i ].rootKey, keyName, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &key, NULL );
		require_noerr( err, exit );
		
		size = (DWORD)( ( lstrlen( entries[ i ].data ) + 1 ) * sizeof( TCHAR ) );
		err = RegSetValueEx( key, entries[ i ].valueName, 0, REG_SZ, (LPBYTE) entries[ i ].data, size );
		RegCloseKey( key );
		require_noerr( err, exit );
	}
	
	// If running on NT, register the extension as approved.
	
	versionInfo.dwOSVersionInfoSize = sizeof( versionInfo );
	GetVersionEx( &versionInfo );
	if( versionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT )
	{
		lstrcpyn( keyName, TEXT( "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Approved" ), sizeof_array( keyName ) );
		err = RegCreateKeyEx( HKEY_LOCAL_MACHINE, keyName, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &key, NULL );
		require_noerr( err, exit );
		
		lstrcpyn( data, inName, sizeof_array( data ) );
		size = (DWORD)( ( lstrlen( data ) + 1 ) * sizeof( TCHAR ) );
		err = RegSetValueEx( key, clsidString, 0, REG_SZ, (LPBYTE) data, size );
		RegCloseKey( key );
	}

	// register toolbar button
	lstrcpyn( keyName, TEXT( "SOFTWARE\\Microsoft\\Internet Explorer\\Extensions\\{7F9DB11C-E358-4ca6-A83D-ACC663939424}"), sizeof_array( keyName ) );
	err = RegCreateKeyEx( HKEY_LOCAL_MACHINE, keyName, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &key, NULL );
	require_noerr( err, exit );

	lstrcpyn( data, L"Yes", sizeof_array( data ) );
	size = (DWORD)( ( lstrlen( data ) + 1 ) * sizeof( TCHAR ) );
	RegSetValueEx( key, L"Default Visible", 0, REG_SZ, (LPBYTE) data, size );

	lstrcpyn( data, inName, sizeof_array( data ) );
	size = (DWORD)( ( lstrlen( data ) + 1 ) * sizeof( TCHAR ) );
	RegSetValueEx( key, L"ButtonText", 0, REG_SZ, (LPBYTE) data, size );
	
	lstrcpyn( data, L"{E0DD6CAB-2D10-11D2-8F1A-0000F87ABD16}", sizeof_array( data ) );
	size = (DWORD)( ( lstrlen( data ) + 1 ) * sizeof( TCHAR ) );
	RegSetValueEx( key, L"CLSID", 0, REG_SZ, (LPBYTE) data, size );

	lstrcpyn( data, clsidString, sizeof_array( data ) );
	size = (DWORD)( ( lstrlen( data ) + 1 ) * sizeof( TCHAR ) );
	RegSetValueEx( key, L"BandCLSID", 0, REG_SZ, (LPBYTE) data, size );

	// check if we're running XP or later
	if ( ( versionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT ) &&
		 ( versionInfo.dwMajorVersion == 5 ) &&
	     ( versionInfo.dwMinorVersion >= 1 ) )
	{
		wsprintf( data, L"%s,%d", (LPCTSTR) g_nonLocalizedResourcesName, IDI_BUTTON_XP );
		size = (DWORD)( ( lstrlen( data ) + 1 ) * sizeof( TCHAR ) );
		RegSetValueEx( key, L"Icon", 0, REG_SZ, (LPBYTE) data, size);

		wsprintf( data, L"%s,%d", (LPCTSTR) g_nonLocalizedResourcesName, IDI_BUTTON_XP );
		size = (DWORD)( ( lstrlen( data ) + 1 ) * sizeof( TCHAR ) );
		RegSetValueEx( key, L"HotIcon", 0, REG_SZ, (LPBYTE) data, size);
	}
	else
	{
		wsprintf( data, L"%s,%d", (LPCTSTR) g_nonLocalizedResourcesName, IDI_BUTTON_2K );
		size = (DWORD)( ( lstrlen( data ) + 1 ) * sizeof( TCHAR ) );
		RegSetValueEx( key, L"Icon", 0, REG_SZ, (LPBYTE) data, size);

		wsprintf( data, L"%s,%d", (LPCTSTR) g_nonLocalizedResourcesName, IDI_BUTTON_2K );
		size = (DWORD)( ( lstrlen( data ) + 1 ) * sizeof( TCHAR ) );
		RegSetValueEx( key, L"HotIcon", 0, REG_SZ, (LPBYTE) data, size);
	}

	RegCloseKey( key );
	
exit:
	return( err );
}

//===========================================================================================================================
//	RegisterCOMCategory
//===========================================================================================================================

DEBUG_LOCAL OSStatus	RegisterCOMCategory( CLSID inCLSID, CATID inCategoryID, BOOL inRegister )
{
	HRESULT				err;
	ICatRegister *		cat;

	err = CoInitialize( NULL );
	require( SUCCEEDED( err ), exit );
	
	err = CoCreateInstance( CLSID_StdComponentCategoriesMgr, NULL, CLSCTX_INPROC_SERVER, IID_ICatRegister, (LPVOID *) &cat );
	check( SUCCEEDED( err ) );
	if( SUCCEEDED( err ) )
	{
		if( inRegister )
		{
			err = cat->RegisterClassImplCategories( inCLSID, 1, &inCategoryID );
			check_noerr( err );
		}
		else
		{
			err = cat->UnRegisterClassImplCategories( inCLSID, 1, &inCategoryID );
			check_noerr( err );
		}
		cat->Release();
	}
	CoUninitialize();

exit:
	return( err );
}


//===========================================================================================================================
//	UnregisterServer
//===========================================================================================================================

DEBUG_LOCAL OSStatus	UnregisterServer( CLSID inCLSID )
{
	OSStatus			err = 0;
	LPWSTR				clsidWideString;
	TCHAR				clsidString[ 64 ];
	HKEY				key;
	TCHAR				keyName[ MAX_PATH * 2 ];
	OSVERSIONINFO		versionInfo;

	// Convert the CLSID to a string based on the encoding of this code (ANSI or Unicode).
	
	err = StringFromIID( inCLSID, &clsidWideString );
	require_noerr( err, exit );
	require_action( clsidWideString, exit, err = kNoMemoryErr );
	
	#ifdef UNICODE
		lstrcpyn( clsidString, clsidWideString, sizeof_array( clsidString ) );
		CoTaskMemFree( clsidWideString );
	#else
		nChars = WideCharToMultiByte( CP_ACP, 0, clsidWideString, -1, clsidString, sizeof_array( clsidString ), NULL, NULL );
		err = translate_errno( nChars > 0, (OSStatus) GetLastError(), kUnknownErr );
		CoTaskMemFree( clsidWideString );
		require_noerr( err, exit );
	#endif

	wsprintf( keyName, L"CLSID\\%s", clsidString );
	MyRegDeleteKey( HKEY_CLASSES_ROOT, keyName );
	
	// If running on NT, de-register the extension as approved.
	
	versionInfo.dwOSVersionInfoSize = sizeof( versionInfo );
	GetVersionEx( &versionInfo );
	if( versionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT )
	{
		lstrcpyn( keyName, TEXT( "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Approved" ), sizeof_array( keyName ) );
		err = RegCreateKeyEx( HKEY_LOCAL_MACHINE, keyName, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &key, NULL );
		require_noerr( err, exit );

		RegDeleteValue( key, clsidString );

		err = RegCloseKey( key );
		require_noerr( err, exit );
	}

	// de-register toolbar button

	lstrcpyn( keyName, TEXT( "SOFTWARE\\Microsoft\\Internet Explorer\\Extensions\\{7F9DB11C-E358-4ca6-A83D-ACC663939424}"), sizeof_array( keyName ) );
	MyRegDeleteKey( HKEY_LOCAL_MACHINE, keyName );
	
exit:
	return( err );
}



//===========================================================================================================================
//	MyRegDeleteKey
//===========================================================================================================================

DEBUG_LOCAL OSStatus MyRegDeleteKey( HKEY hKeyRoot, LPTSTR lpSubKey )
{
    LPTSTR lpEnd;
    OSStatus err;
    DWORD dwSize;
    TCHAR szName[MAX_PATH];
    HKEY hKey;
    FILETIME ftWrite;

    // First, see if we can delete the key without having to recurse.

    err = RegDeleteKey( hKeyRoot, lpSubKey );

    if ( !err )
	{
		goto exit;
	}

    err = RegOpenKeyEx( hKeyRoot, lpSubKey, 0, KEY_READ, &hKey );
	require_noerr( err, exit );

    // Check for an ending slash and add one if it is missing.

    lpEnd = lpSubKey + lstrlen(lpSubKey);

    if ( *( lpEnd - 1 ) != TEXT( '\\' ) ) 
    {
        *lpEnd =  TEXT('\\');
        lpEnd++;
        *lpEnd =  TEXT('\0');
    }

    // Enumerate the keys

    dwSize = MAX_PATH;
    err = RegEnumKeyEx(hKey, 0, szName, &dwSize, NULL, NULL, NULL, &ftWrite);

    if ( !err ) 
    {
        do
		{
            lstrcpy (lpEnd, szName);

            if ( !MyRegDeleteKey( hKeyRoot, lpSubKey ) )
			{
                break;
            }

            dwSize = MAX_PATH;

            err = RegEnumKeyEx( hKey, 0, szName, &dwSize, NULL, NULL, NULL, &ftWrite );

        }
		while ( !err );
    }

    lpEnd--;
    *lpEnd = TEXT('\0');

    RegCloseKey( hKey );

    // Try again to delete the key.

    err = RegDeleteKey(hKeyRoot, lpSubKey);
	require_noerr( err, exit );

exit:

	return err;
}
