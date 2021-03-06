// freeWRLAx.cpp : Implementation of CfreeWRLAxApp and DLL registration.

#include "stdafx.h"
#include "freeWRLAx.h"
#include "cathelp.h" 

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CfreeWRLAxApp NEAR theApp;
// {DCD27C9F-770B-49D8-A2A8-A8D3029D7339}
//IMPLEMENT_OLECREATE(<<class>>, <<external_name>>, 
//0xdcd27c9f, 0x770b, 0x49d8, 0xa2, 0xa8, 0xa8, 0xd3, 0x2, 0x9d, 0x73, 0x39);
//// {DCD27C9F-770B-49D8-A2A8-A8D3029D7339}
//static const GUID <<name>> = 
//{ 0xdcd27c9f, 0x770b, 0x49d8, { 0xa2, 0xa8, 0xa8, 0xd3, 0x2, 0x9d, 0x73, 0x39 } };

const GUID CDECL BASED_CODE _tlid =
	{ 0xdcd27c9f, 0x770b, 0x49d8, { 0xa2, 0xa8, 0xa8, 0xd3, 0x2, 0x9d, 0x73, 0x39 } }; //version 3.0

	//{ 0x7e8586c1, 0x3869, 0x453d, { 0x82, 0x30, 0x62, 0x68, 0x7, 0x87, 0x6f, 0x82 } }; //version 2.0
	//{ 0x656FC6F3, 0x9CF2, 0x4674, { 0xB3, 0x32, 0xB2, 0x55, 0x8, 0x52, 0xE6, 0x44 } }; //version 1.22.12_pre2

const WORD _wVerMajor = 3; //2;
const WORD _wVerMinor = 0;

      const CATID CATID_SafeForScripting     =
      {0x7dd95801,0x9882,0x11cf,{0x9f,0xa9,0x00,0xaa,0x00,0x6c,0x42,0xc4}};
      const CATID CATID_SafeForInitializing  =
      {0x7dd95802,0x9882,0x11cf,{0x9f,0xa9,0x00,0xaa,0x00,0x6c,0x42,0xc4}}; 


// CfreeWRLAxApp::InitInstance - DLL initialization

BOOL CfreeWRLAxApp::InitInstance()
{
	BOOL bInit = COleControlModule::InitInstance();

	if (bInit)
	{
		// TODO: Add your own module initialization code here.
	}

	return bInit;
}



// CfreeWRLAxApp::ExitInstance - DLL termination

int CfreeWRLAxApp::ExitInstance()
{
	// TODO: Add your own module termination code here.

	return COleControlModule::ExitInstance();
}



// DllRegisterServer - Adds entries to the system registry
//#include <olectl.h>  //included indirectly, but has the dllregisterserver error codes
STDAPI DllRegisterServer(void)
{
    HKEY        hkey = NULL;
    HKEY        hkey1 = NULL;
    BOOL        fErr = TRUE;
    char        szSubKey[513];

    // file extension for new mime type
    const char* pszMTExt0 = ".wrl";
    const char* pszMTExt1 = ".wrl";
    const char* pszMTExt2 = ".x3dv";
    const char* pszMTExt3 = ".x3d";
    const char* pszMTExt4 = ".x3z";
    const char* pszMTExt6 = ".x3d";

    // text for new mime content type
    const char* pszMTContent0 = "x-world/x-vrml";
    const char* pszMTContent1 = "model/vrml";
    const char* pszMTContent2 = "model/x3d+vrml";
    const char* pszMTContent3 = "model/x3d+xml";
    const char* pszMTContent4 = "model/x3d+zip";
    const char* pszMTContent6 = "model/x3d";

    // text for mimetype subkey
    const char* pszMTSubKey0 = "MIME\\DataBase\\Content Type\\x-world/x-vrml";
    const char* pszMTSubKey1 = "MIME\\DataBase\\Content Type\\model/vrml";
    const char* pszMTSubKey2 = "MIME\\DataBase\\Content Type\\model/x3d+vrml";
    const char* pszMTSubKey3 = "MIME\\DataBase\\Content Type\\model/x3d+xml";
    const char* pszMTSubKey4 = "MIME\\DataBase\\Content Type\\model/x3d+zip";
    const char* pszMTSubKey6 = "MIME\\DataBase\\Content Type\\model/x3d";

    // extension named value
    const char* pszMTExtVal = "Extension";
    // clsid
	const char* pszMTCLSID = "{28C209BE-D3AE-493A-82B6-CD36A2E6D9A7}"; //version 3.0
	//"{4E814FCE-B546-4d91-8EA8-358264E5D423}"; //version 2.0
	// "{582C9301-A2C8-45FC-831B-654DE7F3AF11}"; //version 1.22.12_pre2
    const GUID CDECL BASED_CODE _ctlid =
	{ 0x28c209be, 0xd3ae, 0x493a, { 0x82, 0xb6, 0xcd, 0x36, 0xa2, 0xe6, 0xd9, 0xa7 } }; //version 3.0
	// { 0x4e814fce, 0xb546, 0x4d91, { 0x8e, 0xa8, 0x35, 0x82, 0x64, 0xe5, 0xd4, 0x23 } }; //version 2.0
	// { 0x582c9301, 0xa2c8, 0x45fc, {0x83, 0x1b, 0x65, 0x4d, 0xe7, 0xf3, 0xaf, 0x11}}; //version 1.22.12_pre2

    // clsid named value name
    const char* pszMTCLSIDVal = "CLSID";
    // content type named value name
    const char* pszMTContentVal = "Content Type";
    // EnableFullPage key
    const char* pszMTFullPage = "EnableFullPage";

	AFX_MANAGE_STATE(_afxModuleAddrThis);
    //MessageBox(0, "Trying to register freeWRLAx_ocx player for web3d mime types", "Registration Error", MB_OK);

	if (!COleObjectFactoryEx::UpdateRegistryAll(TRUE)){
		MessageBox(0, "Cannot update registry", "Registration Error 1", MB_OK);
		return ResultFromScode(SELFREG_E_CLASS);
	}

	if (!AfxOleRegisterTypeLib(AfxGetInstanceHandle(), _tlid)){
		MessageBox(0, "Cannot register typelib", "Registration Error 2", MB_OK);
		return ResultFromScode(SELFREG_E_TYPELIB);
	}



    do 
    {
	//HKCR/MIME/DataBase/Content Type/x-world/x-vrml
        // create new mime type key for our new mimetype.  Only necessary for new mime types
        if ( ERROR_SUCCESS != RegCreateKey(HKEY_CLASSES_ROOT, pszMTSubKey0, &hkey) )
            break;

        // add extension value to that mime type key to associate .xxx files with the 
        //  mime type
        if ( ERROR_SUCCESS != RegSetValueEx(hkey, pszMTExtVal, 0, REG_SZ, 
            (const BYTE *)pszMTExt0, (DWORD)strlen(pszMTExt0)) )
            break;

        // Add class id to associate this object with the mime type
        if ( ERROR_SUCCESS != RegSetValueEx(hkey, pszMTCLSIDVal, 0, REG_SZ,
            (const BYTE *)pszMTCLSID, (DWORD)strlen(pszMTCLSID)) )
            break;

        RegCloseKey(hkey);

	//HKCR/MIME/DataBase/Content Type/model/vrml
        // create new mime type key for our new mimetype.  Only necessary for new mime types
        if ( ERROR_SUCCESS != RegCreateKey(HKEY_CLASSES_ROOT, pszMTSubKey1, &hkey) )
            break;

        // add extension value to that mime type key to associate .xxx files with the 
        //  mime type
        if ( ERROR_SUCCESS != RegSetValueEx(hkey, pszMTExtVal, 0, REG_SZ, 
            (const BYTE *)pszMTExt1, (DWORD)strlen(pszMTExt1)) )
            break;

        // Add class id to associate this object with the mime type
        if ( ERROR_SUCCESS != RegSetValueEx(hkey, pszMTCLSIDVal, 0, REG_SZ,
            (const BYTE *)pszMTCLSID, (DWORD)strlen(pszMTCLSID)) )
            break;

        RegCloseKey(hkey);
	//HKCR/MIME/DataBase/Content Type/model/x3d+vrml
        // create new mime type key for our new mimetype.  Only necessary for new mime types
        if ( ERROR_SUCCESS != RegCreateKey(HKEY_CLASSES_ROOT, pszMTSubKey2, &hkey) )
            break;

        // add extension value to that mime type key to associate .xxx files with the 
        //  mime type
        if ( ERROR_SUCCESS != RegSetValueEx(hkey, pszMTExtVal, 0, REG_SZ, 
            (const BYTE *)pszMTExt2, (DWORD)strlen(pszMTExt2)) )
            break;

        // Add class id to associate this object with the mime type
        if ( ERROR_SUCCESS != RegSetValueEx(hkey, pszMTCLSIDVal, 0, REG_SZ,
            (const BYTE *)pszMTCLSID, (DWORD)strlen(pszMTCLSID)) )
            break;

        RegCloseKey(hkey);
	//HKCR/MIME/DataBase/Content Type/model/x3d+xml
        // create new mime type key for our new mimetype.  Only necessary for new mime types
        if ( ERROR_SUCCESS != RegCreateKey(HKEY_CLASSES_ROOT, pszMTSubKey3, &hkey) )
            break;

        // add extension value to that mime type key to associate .xxx files with the 
        //  mime type
        if ( ERROR_SUCCESS != RegSetValueEx(hkey, pszMTExtVal, 0, REG_SZ, 
            (const BYTE *)pszMTExt3, (DWORD)strlen(pszMTExt3)) )
            break;

        // Add class id to associate this object with the mime type
        if ( ERROR_SUCCESS != RegSetValueEx(hkey, pszMTCLSIDVal, 0, REG_SZ,
            (const BYTE *)pszMTCLSID, (DWORD)strlen(pszMTCLSID)) )
            break;

        RegCloseKey(hkey);

	//HKCR/MIME/DataBase/Content Type/model/x3d+zip .x3z
        // create new mime type key for our new mimetype.  Only necessary for new mime types
        if ( ERROR_SUCCESS != RegCreateKey(HKEY_CLASSES_ROOT, pszMTSubKey4, &hkey) )
            break;

        // add extension value to that mime type key to associate .xxx files with the 
        //  mime type
        if ( ERROR_SUCCESS != RegSetValueEx(hkey, pszMTExtVal, 0, REG_SZ, 
            (const BYTE *)pszMTExt4, (DWORD)strlen(pszMTExt4)) )
            break;

        // Add class id to associate this object with the mime type
        if ( ERROR_SUCCESS != RegSetValueEx(hkey, pszMTCLSIDVal, 0, REG_SZ,
            (const BYTE *)pszMTCLSID, (DWORD)strlen(pszMTCLSID)) )
            break;

        RegCloseKey(hkey);


	//HKCR/MIME/DataBase/Content Type/model/x3d
        // create new mime type key for our new mimetype.  Only necessary for new mime types
        if ( ERROR_SUCCESS != RegCreateKey(HKEY_CLASSES_ROOT, pszMTSubKey6, &hkey) )
            break;

        // add extension value to that mime type key to associate .xxx files with the 
        //  mime type
        if ( ERROR_SUCCESS != RegSetValueEx(hkey, pszMTExtVal, 0, REG_SZ, 
            (const BYTE *)pszMTExt6, (DWORD)strlen(pszMTExt6)) )
            break;

        // Add class id to associate this object with the mime type
        if ( ERROR_SUCCESS != RegSetValueEx(hkey, pszMTCLSIDVal, 0, REG_SZ,
            (const BYTE *)pszMTCLSID, (DWORD)strlen(pszMTCLSID)) )
            break;

        RegCloseKey(hkey);


//For object/embed IE uses the type="content type" field of the tag to get the mime type directly.
//For href links (which show FullPage), IE uses the .xxx extension to get the Content Type, 
//and uses that mime type. So for the hrefs we need to register our prefered mime type with the .xxx 
	if(TRUE)
	{
	//HKCR/.wrl
        // Register .xxx as a file extension this is only necessary for new file extensions, addimg
        // a new player for .avi files for instance would not require this
        if ( ERROR_SUCCESS != RegCreateKey(HKEY_CLASSES_ROOT, pszMTExt1, &hkey) )
            break;

        // Add content type to associate this extension with the content type.  This is required
        // and is used when the mime type is unknown and IE looks up associations by extension
        if ( ERROR_SUCCESS != RegSetValueEx(hkey, pszMTContentVal, 0, REG_SZ,
            (const BYTE *)pszMTContent1, (DWORD)strlen(pszMTContent1)) )
            break;

        RegCloseKey(hkey);
	//HKCR/.x3d
        // Register .xxx as a file extension this is only necessary for new file extensions, addimg
        // a new player for .avi files for instance would not require this
        if ( ERROR_SUCCESS != RegCreateKey(HKEY_CLASSES_ROOT, pszMTExt2, &hkey) )
            break;

        // Add content type to associate this extension with the content type.  This is required
        // and is used when the mime type is unknown and IE looks up associations by extension
        if ( ERROR_SUCCESS != RegSetValueEx(hkey, pszMTContentVal, 0, REG_SZ,
            (const BYTE *)pszMTContent2, (DWORD)strlen(pszMTContent2)) )
            break;

        RegCloseKey(hkey);
	//HKCR/.wrl
        // Register .xxx as a file extension this is only necessary for new file extensions, addimg
        // a new player for .avi files for instance would not require this
        if ( ERROR_SUCCESS != RegCreateKey(HKEY_CLASSES_ROOT, pszMTExt3, &hkey) )
            break;

        // Add content type to associate this extension with the content type.  This is required
        // and is used when the mime type is unknown and IE looks up associations by extension
        if ( ERROR_SUCCESS != RegSetValueEx(hkey, pszMTContentVal, 0, REG_SZ,
            (const BYTE *)pszMTContent3, (DWORD)strlen(pszMTContent3)) )
            break;

        RegCloseKey(hkey);
	//HKCR/.x3z
        // Register .xxx as a file extension this is only necessary for new file extensions, addimg
        // a new player for .avi files for instance would not require this
        if ( ERROR_SUCCESS != RegCreateKey(HKEY_CLASSES_ROOT, pszMTExt4, &hkey) )
            break;

        // Add content type to associate this extension with the content type.  This is required
        // and is used when the mime type is unknown and IE looks up associations by extension
        if ( ERROR_SUCCESS != RegSetValueEx(hkey, pszMTContentVal, 0, REG_SZ,
            (const BYTE *)pszMTContent4, (DWORD)strlen(pszMTContent4)) )
            break;

        RegCloseKey(hkey);
	}
//<<<<<<<<<<<<<<<<<
//I can start here for my first tests
        // Open the key under the control's clsid HKEY_CLASSES_ROOT\CLSID\<CLSID>
        wsprintf(szSubKey, "%s\\%s", pszMTCLSIDVal, pszMTCLSID);
        if ( ERROR_SUCCESS != RegOpenKey(HKEY_CLASSES_ROOT, szSubKey, &hkey) )
            break;

        // Create the EnableFullPage and extension key under this so that we can display files
        // with the extension full frame in the browser
        wsprintf(szSubKey, "%s\\%s", pszMTFullPage, pszMTExt1);
        if ( ERROR_SUCCESS != RegCreateKey(hkey, szSubKey, &hkey1) )
            break;
        RegCloseKey(hkey1);
        wsprintf(szSubKey, "%s\\%s", pszMTFullPage, pszMTExt2);
        if ( ERROR_SUCCESS != RegCreateKey(hkey, szSubKey, &hkey1) )
            break;
        RegCloseKey(hkey1);
        wsprintf(szSubKey, "%s\\%s", pszMTFullPage, pszMTExt3);
        if ( ERROR_SUCCESS != RegCreateKey(hkey, szSubKey, &hkey1) )
            break;
        RegCloseKey(hkey1);
        wsprintf(szSubKey, "%s\\%s", pszMTFullPage, pszMTExt4);
        if ( ERROR_SUCCESS != RegCreateKey(hkey, szSubKey, &hkey1) )
            break;
        RegCloseKey(hkey1);


        // Register with Internet Explorer, as per:
        // http://msdn.microsoft.com/en-us/library/bb250471.aspx  ActiveX opt-In / signing / registering 
		// "To put your control on the pre-approved list, you need to write the CLSID of the control to the following registry location.
		//  HKEY_LOCAL_MACHINE/SOFTWARE/Microsoft/Windows/CurrentVersion/Ext/PreApproved"
		if ( ERROR_SUCCESS != RegCreateKey(HKEY_LOCAL_MACHINE,
			"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Ext\\PreApproved\\{28C209BE-D3AE-493A-82B6-CD36A2E6D9A7}",&hkey) ) //version 3.0
			//"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Ext\\PreApproved\\{4E814FCE-B546-4D91-8EA8-358264E5D423}",&hkey) ) //version 2.0
			//"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Ext\\PreApproved\\{582C9301-A2C8-45FC-831B-654DE7F3AF11}",&hkey) ) //version 1.22.12_pre2
            break;

         RegCloseKey(hkey);


		 //from http://www.codeproject.com/KB/COM/CompleteActiveX.aspx vs2005
          if (FAILED( CreateComponentCategory(
                  CATID_SafeForScripting,
                  L"Controls that are safely scriptable") ))
                return ResultFromScode(SELFREG_E_CLASS);

          if (FAILED( CreateComponentCategory(
                  CATID_SafeForInitializing,
                  L"Controls safely initializable from persistent data") ))
                return ResultFromScode(SELFREG_E_CLASS);

          if (FAILED( RegisterCLSIDInCategory(
                  _ctlid, CATID_SafeForScripting) ))
                return ResultFromScode(SELFREG_E_CLASS);

          if (FAILED( RegisterCLSIDInCategory(
                  _ctlid, CATID_SafeForInitializing) ))
                return ResultFromScode(SELFREG_E_CLASS);




        fErr = FALSE;
    } while (FALSE);

    if ( hkey )
        RegCloseKey(hkey);

    if ( hkey1 )
        RegCloseKey(hkey1);

    if ( fErr )
        MessageBox(0, "Cannot register player for mime type", "Registration Error 3", MB_OK);


	return NOERROR;
}



// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
	AFX_MANAGE_STATE(_afxModuleAddrThis);

	if (!AfxOleUnregisterTypeLib(_tlid, _wVerMajor, _wVerMinor))
		return ResultFromScode(SELFREG_E_TYPELIB);

	if (!COleObjectFactoryEx::UpdateRegistryAll(FALSE))
		return ResultFromScode(SELFREG_E_CLASS);

	return NOERROR;
}
//STDAPI DllCanUnloadNow(void)
//{
//	return NOERROR;
//}
//STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
//{
//	return NOERROR;
//}