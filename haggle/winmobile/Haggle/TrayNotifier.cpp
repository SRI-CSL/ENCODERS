/* Copyright 2008 Uppsala University
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/
#include <HaggleKernel.h>
#include <Debug.h>
#include "TrayNotifier.h"
#include "resource.h"

// Application specific dialog message
#define WM_HAGGLE_MSG (WM_APP + 1)
#define MY_NOTIFICATION_ID 1

// {4C74E045-FC2B-41a7-8262-6B180AA1B255}
static const GUID trayguid = { 0x4c74e045, 0xfc2b, 0x41a7, { 0x82, 0x62, 0x6b, 0x18, 0xa, 0xa1, 0xb2, 0x55 } };

#ifdef DEBUG

struct msginfo {
	UINT msg;
	const char *msg_name;
};

static struct msginfo msgnames[] = {
	{ WM_INITDIALOG, "WM_INITDIALOG" },
	{ DM_GETDEFID, "DM_GETDEFID" },
	{ DM_SETDEFID, "DM_SETDEFID" },
	{ WM_CTLCOLORDLG, "WM_CTLCOLORDLG" },
	{ WM_CTLCOLORSTATIC, "WM_CTLCOLORSTATIC" },
	{ WM_GETDLGCODE, "WM_GETDLGCODE" },
	{ WM_NEXTDLGCTL, "WM_NEXTDLGCTL" },
	{ WM_CANCELMODE, "WM_CANCELMODE" },
	{ WM_CLOSE, "WM_CLOSE" },
	{ WM_HIBERNATE, "WM_HIBERNATE" },
	{ WM_ACTIVATE, "WM_ACTIVATE" },
	{ WM_DESTROY, "WM_DESTROY" },
	{ WM_CREATE, "WM_CREATE" },
	{ WM_DESTROY, "WM_DESTROY" },
	{ WM_ENABLE, "WM_ENABLE" },
	{ WM_ERASEBKGND, "WM_ERASEBKGND" },
	{ WM_MOVE, "WM_MOVE" },
	{ WM_QUIT, "VM_QUIT" },
	{ WM_SIZE, "WM_SIZE" },
	{ WM_COMMAND, "WM_COMMAND" },
	{ WM_NOTIFY, "WM_NOTIFY" },
	{ WM_WINDOWPOSCHANGED, "WM_WINDOWPOSCHANGED" },
	{ WM_TIMER, "WM_TIMER" },
	{ WM_SETFONT, "WM_SETFONT" },
	{ WM_GETFONT, "WM_GETFONT" }
};

static const char *msgname(UINT msg)
{
	int i;

	for (i = 0; i < sizeof(msgnames) / sizeof(struct msginfo); i++) {
		if (msgnames[i].msg == msg)
			return msgnames[i].msg_name;
	}
	return "Unknown message";
}

#endif /* DEBUG */

TrayNotifier::TrayNotifier(HINSTANCE _hInstance, HaggleKernel *_kernel) : 
	Runnable("TrayNotifier"), hInstance(_hInstance), kernel(_kernel)
{
	memset(&sn, 0, sizeof(SHNOTIFICATIONDATA));

	sn.dwID = MY_NOTIFICATION_ID;
	sn.clsid = trayguid;
	sn.cbStruct = sizeof(SHNOTIFICATIONDATA);
	sn.grfFlags = SHNF_STRAIGHTTOTRAY;
	sn.hicon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_HAGGLE_NOTIFY_ICON));
	sn.npPriority = SHNP_INFORM;
	sn.csDuration = -1;
	sn.pszHTML = _T("<html><body>This is a stray notification for Haggle. It probably means that Haggle crashed or was shutdown inappropriately.<br>Restart Haggle to reset the notification.</body></html>");
}
bool TrayNotifier::run()
{
	HANDLE h[1] = { Thread::selfGetExitSignal()->getHandle() };
	MSG msg;

	// Remove any lingering tray notifications due to bad shutdown
	SHNotificationRemove(&trayguid, 1);

	hDialog = CreateDialogParam(hInstance, (LPCTSTR)IDD_HAGGLE_DIALOG, 
		NULL, (DLGPROC)NotifyCallback, (LPARAM)this); 

	if (!hDialog) {
		fprintf(stderr, "Could not create tray dialog window\n");
		return false;
	}

	// Hide the dialog by default. It should be shown when we click the
	// notification icon in the tray
	ShowWindow(hDialog, SW_HIDE); 

	// Set the dialog icon
	SendMessage(hDialog, WM_SETICON, ICON_SMALL, (LPARAM)LoadIcon(NULL, MAKEINTRESOURCE(IDI_HAGGLE_LAUNCH_ICON)));
	SendMessage(hDialog, WM_SETICON, ICON_BIG, (LPARAM)LoadIcon(NULL, MAKEINTRESOURCE(IDI_HAGGLE_LAUNCH_ICON)));

	// Set the dialog as the sink for the notification
	sn.hwndSink = hDialog;

	// Add the notification to the tray
	SHNotificationAdd(&sn);

	// Thread runloop that handles all messages
	while (!shouldExit()) {

		if (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)) {
			if (GetMessage(&msg, NULL, 0, 0) && !IsDialogMessage(hDialog, &msg)) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		} else {
			DWORD ret = MsgWaitForMultipleObjects(1, h, TRUE, INFINITE, QS_ALLEVENTS | QS_ALLINPUT | QS_POSTMESSAGE | QS_SENDMESSAGE | QS_INPUT | QS_TIMER);

			if (ret == WAIT_OBJECT_0) {
				break;
			}
		}
	}
	
	// Doing cleanup here instead of cleanup function. See FIXME note below...
	SHNotificationRemove(&trayguid, 1);
	DestroyWindow(hDialog);

	return false;
}

/*
	FIXME: For some reason the cleanup function here is never called on thread exit.
	It must be some bug in the vtable lookup for TrayNotifier, 
	because cleanup works for all other threads.
*/
void TrayNotifier::cleanup() 
{ 
	/*
	SHNotificationRemove(&trayguid, 1);
	DestroyWindow(hDialog);
	*/
}

static void print_status(HaggleKernel *kernel)
{
	MEMORYSTATUS memInfo;
	
	memInfo.dwLength = sizeof(memInfo);
	
	GlobalMemoryStatus(&memInfo);

	HAGGLE_DBG("Memory status - Total RAM: %lu bytes Free: %lu Used: %lu\n", 
		memInfo.dwTotalPhys, memInfo.dwAvailPhys, memInfo.dwTotalPhys - memInfo.dwAvailPhys);

#if defined(DEBUG)
	printf("- Threads\n");
	Thread::registryPrint();

	printf("- Managers:\n");
	kernel->printRegisteredManagers();

	printf("- Interfaces:\n");
	kernel->getInterfaceStore()->print();

	printf("- Nodes:\n");
	kernel->getNodeStore()->print();
	
	printf("- Protocols:\n");
	DebugCmdRef dbgCmd = new DebugCmd(DBG_CMD_PRINT_PROTOCOLS);
	kernel->addEvent(new Event(dbgCmd));
#endif
}

LRESULT CALLBACK NotifyCallback(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	// This is kind of ugly. We use this static variable to access
	// the TrayNotifier object. We set this pointer when the dialog 
	// initializes. We need to do this as there seems to be no way to pass objects
	// in callbacks. The other option would be to use a global variable.
	static TrayNotifier *tn = NULL;

	//HAGGLE_DBG("NotifyCallback : %s\n", msgname(m essage));
	
	switch (message) {
	/* Dialog box messages: */
	case WM_INITDIALOG:
		/*
			This message is sent to the dialog box procedure immediately before 
			a dialog box is displayed.
		*/
		tn = (TrayNotifier *)lParam;
		return TRUE;
	case DM_GETDEFID:
		/* 
			This message is sent by an application to retrieve the identifier 
			of the default push button control for a dialog box. 
		*/
		break;
	case DM_SETDEFID:
		/*
			This message is sent by an application to change the identifier of 
			the default push button for a dialog box.
		*/
		break;
	case WM_CTLCOLORDLG:
		return (LRESULT)CreateSolidBrush(RGB(255, 255, 255));
	case WM_CTLCOLORSTATIC:
		/*
			This message is sent to a dialog box before Windows draws the dialog box.
		*/
		{
			// We set a white background
			HBRUSH hbrBackground = CreateSolidBrush(RGB(255, 255, 255));
			HDC hdcStatic = (HDC)wParam;
			SetTextColor(hdcStatic, RGB(0, 0, 0));
			SetBkMode(hdcStatic, TRANSPARENT);
			return (LRESULT)hbrBackground;
		}
	case WM_GETDLGCODE:
		/*
			This message is sent to the dialog box procedure associated with a control.
		*/
		break;
	case WM_NEXTDLGCTL:
		/*
			This message is sent to a dialog box procedure to set the keyboard focus to 
			a different control in the dialog box.
		*/
		break;
	/* Window messages: */
	case WM_CANCELMODE:	
		/*
			This message is sent to the focus window when a dialog box or message box 
			is displayed; this enables the focus window to cancel modes, such as stylus capture.
		*/
		break;
	case WM_CREATE:
		/*
			This message is sent when an application requests that a window be created by 
			calling the CreateWindowEx or CreateWindow function.
		*/
		break;
	case WM_DESTROY:
		/*
			This message is sent when a window is being destroyed.
		*/
		PostQuitMessage(0);
		break;
	case WM_ENABLE:
		/*
			This message is sent when an application changes the enabled state of a window.
		*/
		break;
	case WM_ERASEBKGND:
		/*
			This message is sent by an application when the window background must be erased, 
			for example, when a window is resized.
		*/
		break;
	case WM_MOVE:
		/*
			This message is sent after a window has been moved.
		*/
		break;
	case WM_QUIT:
		/*
			This message indicates a request to terminate an application and is generated 
			when the application calls the PostQuitMessage function.
		*/
		break;
	case WM_SIZE:
		/*
			This message is sent to a window after its size has changed.
		*/
		break;
	case WM_WINDOWPOSCHANGED:
		/*
			This message is sent to a window whose size, position, or place in the 
			z-order has changed as a result of a call to the SetWindowPos function 
			or another window-management function.
		*/
		break;
	/* Notification messages (ShNotificationAdd) */
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_BUTTON_SHUTDOWN:
			 switch (HIWORD(wParam)) {
			 case BN_CLICKED:
				HAGGLE_DBG("Shutdown button clicked\n");
				tn->kernel->shutdown();
				ShowWindow(hDlg, SW_HIDE); 
				PostQuitMessage(0);
				return TRUE;
			 default:
				 break;
			}
		case IDC_BUTTON_DISMISS:
			switch (HIWORD(wParam)) {
			case BN_CLICKED:
				HAGGLE_DBG("Dismiss button clicked\n");
				print_status(tn->kernel);
				ShowWindow(hDlg, SW_HIDE); 
				return TRUE;
			default:
				break;
			}
		}
		break;	
	case WM_TIMER:
		break;
		/* Common control messages: */
	case WM_NOTIFY:
		switch(wParam) {
		case MY_NOTIFICATION_ID:
			{
				NMSHN* pnmshn = (NMSHN*)lParam;

				switch (pnmshn->hdr.code) {
				case SHNN_HOTKEY:
					break;
				case SHNN_SHOW:
					{
						Timeval runtime = Timeval::now() - tn->kernel->getStartTime();
						long secs = runtime.getSeconds() % 60;
						long minutes = (runtime.getSeconds() / 60) % 60;
						long hours = (runtime.getSeconds() / 3600) % 24;
						long days = runtime.getSeconds() / 3600 / 24;
						wchar_t buf[50];

						_snwprintf(buf, 50, L"Runnning for %ld days %ld h %ld min %ld s", days, hours, minutes, secs);

						SetDlgItemText(hDlg, IDC_TEXT_STARTTIME, buf);

						SetForegroundWindow(hDlg);
						ShowWindow(hDlg, SW_SHOW);

						// Do not show the bubble. Yes, the API is cryptic and sucks...
						SetWindowLong(hDlg, DWL_MSGRESULT, 1);
						return TRUE;
					}
				case SHNN_DISMISS:
					break;
				case SHNN_LINKSEL:
					break;
				case SHNN_ICONCLICKED:
					break;
				default:
					HAGGLE_DBG("default\n");
					break;
				}
			}
			break;
		}
	case WM_SETFONT:
		break;
	case WM_GETFONT:
		break;
	case WM_CLOSE:
		/*
			This message is sent as a signal that a window or an application should terminate.
		*/
		ShowWindow(hDlg, SW_HIDE);
		HAGGLE_DBG("NotifyCallback : %s\n", msgname(message));
		return TRUE;
	case WM_ACTIVATE:
	case WM_HIBERNATE:
		HAGGLE_DBG("NotifyCallback : %s\n", msgname(message));
		break;
	default:
		break;
	}
	return FALSE;
}
