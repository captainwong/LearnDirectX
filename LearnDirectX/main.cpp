#include <Windows.h>
#include <iostream>
#include <list>
#include <string>
#include <map>
#include <cstdlib>

#include <dsound.h>
#pragma comment(lib,"Dsound.lib") 

#include <iphlpapi.h>
#pragma comment(lib, "iphlpapi.lib")


typedef std::list<std::wstring> string_list;
typedef std::map<std::wstring, std::wstring> guid_string_map;
auto primary_sound_driver = L"Primary Sound Driver";

BOOL CALLBACK DSEnumProc(LPGUID lpGUID,
						 LPCTSTR lpszDesc,
						 LPCTSTR lpszDrvName,
						 LPVOID lpContext)
{
	guid_string_map* map = reinterpret_cast<guid_string_map*>(lpContext);
	LPGUID lpTemp = NULL;

	if (lpGUID != NULL)  //  NULL only for "Primary Sound Driver".
	{
		if ((lpTemp = (LPGUID)malloc(sizeof(GUID))) == NULL) {
			return(TRUE);
		}
		memcpy(lpTemp, lpGUID, sizeof(GUID));

		OLECHAR* bstrGuid;
		StringFromCLSID(*lpGUID, &bstrGuid);

		map->operator[](bstrGuid) = lpszDesc;

	} else {
		map->operator[](primary_sound_driver) = lpszDesc;
	}

	

	//ComboBox_AddString(hCombo, lpszDesc);
	//ComboBox_SetItemData(hCombo,
	//					 ComboBox_FindString(hCombo, 0, lpszDesc),
	//					 lpTemp);
	free(lpTemp);
	return(TRUE);
}

int enum_direct_sound_drivers()
{
	guid_string_map map;
	if (FAILED(DirectSoundEnumerate((LPDSENUMCALLBACK)DSEnumProc,
									(VOID*)&map))) {

		return(-1);
	}

	//auto primary_sound_driver_ = map[primary_sound_driver];
	//std::wcout << *primary_sound_driver << L": " << primary_sound_driver_.c_str() << std::endl;
	for (auto driver : map) {
		//if (driver.first != primary_sound_driver) {
		std::wcout << driver.first.c_str() << L":" << driver.second.c_str() << std::endl;
		//}
	}

	return 0;
}

int main() 
{
	enum_direct_sound_drivers();

	std::system("pause");

	_CrtDumpMemoryLeaks();
	return 0;
}
