#include <stdio.h>
#include <objbase.h>

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>




// Code to display the memory leak report 
// We use a custom report hook to filter out Qt's own memory leaks
// Credit to Andreas Schmidts - http://www.schmidt-web-berlin.de/winfig/blog/?p=154

_CRT_REPORT_HOOK prevHook;

int customReportHook(int /* reportType */, char* message, int* /* returnValue */) {
	// This function is called several times for each memory leak.
	// Each time a part of the error message is supplied.
	// This holds number of subsequent detail messages after
	// a leak was reported
	const int numFollowupDebugMsgParts = 2;
	static bool ignoreMessage = false;
	static int debugMsgPartsCount = 0;

	// check if the memory leak reporting starts
	if ((strncmp(message, "Detected memory leaks!\n", 10) == 0)
		|| ignoreMessage) {
		// check if the memory leak reporting ends
		if (strncmp(message, "Object dump complete.\n", 10) == 0) {
			_CrtSetReportHook(prevHook);
			ignoreMessage = false;
		} else
			ignoreMessage = true;

		// something from our own code?
		if (strstr(message, ".cpp") == NULL) {
			if (debugMsgPartsCount++ < numFollowupDebugMsgParts)
				// give it back to _CrtDbgReport() to be printed to the console
				return FALSE;
			else
				return TRUE;  // ignore it
		} else {
			debugMsgPartsCount = 0;
			// give it back to _CrtDbgReport() to be printed to the console
			return FALSE;
		}
	} else
		// give it back to _CrtDbgReport() to be printed to the console
		return FALSE;
}




// GUIDS /////////////////////////////////////////////////////////////////////////////////////

// these were all generated with GUIDGEN.EXE

// {B9B8ACE1-CE14-11d0-AE58-444553540000}
const IID IID_IX =
{ 0xb9b8ace1, 0xce14, 0x11d0, { 0xae, 0x58, 0x44, 0x45, 0x53, 0x54, 0x0, 0x0 } };


// {B9B8ACE2-CE14-11d0-AE58-444553540000}
const IID IID_IY =
{ 0xb9b8ace2, 0xce14, 0x11d0, { 0xae, 0x58, 0x44, 0x45, 0x53, 0x54, 0x0, 0x0 } };

// {B9B8ACE3-CE14-11d0-AE58-444553540000}
const IID IID_IZ =
{ 0xb9b8ace3, 0xce14, 0x11d0, { 0xae, 0x58, 0x44, 0x45, 0x53, 0x54, 0x0, 0x0 } };



// INTERFACES ////////////////////////////////////////////////////////////////////////////////

__interface Ix : IUnknown
{
	virtual void __stdcall fx() = 0;
};

__interface Iy : IUnknown
{
	virtual void __stdcall fy() = 0;
};

class ComObject : public Ix, public Iy
{
public:
	ComObject() {
		printf("ComObject ctor\n");
	}

	virtual ~ComObject() {
		printf("ComObject dtor\n");
	}

private:
	virtual HRESULT __stdcall QueryInterface(const IID& iid, void** iface) override {
		if (iid == IID_IUnknown) {
			printf("QueryInterface for IUnknown\n");
			*iface = (Ix*)this;
		} else if (iid == IID_IX) {
			printf("QueryInterface for Ix\n");
			*iface = (Ix*)this;
		} else if (iid == IID_IY) {
			printf("QueryInterface for Iy\n");
			*iface = (Iy*)this; // 这里是什么黑魔法？写成 this 或 (Ix*)this，调用iy->fy()都会输出fx，只有写成(Iy*)this才能输出fy???
			// 应该是vtable吧？
			// 才发现强转类型可以修改void**类型的虚表
		} else {
			printf("QueryInterface for unknown interface\n");
			*iface = nullptr;
			return E_NOINTERFACE;
		}

		((IUnknown*)(*iface))->AddRef();
		return S_OK;
	}

	virtual ULONG __stdcall AddRef() override {
		++ref_count;
		printf("AddRef, ref_count=%d\n", ref_count);
		return ref_count;
	}

	virtual ULONG __stdcall Release() override {
		int n = --ref_count;
		printf("Release, ref_count=%d\n", n); 
		if (n == 0) {
			delete this;
		}
		return n;
	}

	virtual void __stdcall fx() override { printf("fx\n"); }
	virtual void __stdcall fy() override { printf("fy\n"); }

private:
	int ref_count = 0;
};

IUnknown* myCoCreateInstance()
{
	printf("myCoCreateInstance\n");
	IUnknown* obj = (Ix*)new ComObject();
	obj->AddRef();
	return obj;
}

int main()
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	prevHook = _CrtSetReportHook(customReportHook);


	{
		IUnknown* iunknown = myCoCreateInstance();
		Ix* ix = nullptr;
		Iy* iy = nullptr;

		iunknown->QueryInterface(IID_IX, (void**)&ix);
		ix->fx();
		ix->Release();

		iunknown->QueryInterface(IID_IY, (void**)&iy);
		iy->fy();
		iy->Release();

		iunknown->Release();
	}


	_CrtDumpMemoryLeaks();
}
