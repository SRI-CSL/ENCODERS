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
#ifndef _TRAYNOTIFIER_H
#define _TRAYNOTIFIER_H

#include <aygshell.h>
#include <libcpphaggle/Thread.h>

class HaggleKernel;

/*
	Note: the tray notification will only display on devices running Windows Mobile Professional.
*/
class TrayNotifier : public Runnable {
	friend LRESULT CALLBACK NotifyCallback(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
	HINSTANCE hInstance;
	HaggleKernel *kernel;
	SHNOTIFICATIONDATA sn;
	HWND hDialog;
	bool run();
	void cleanup();
public:
	TrayNotifier(HINSTANCE _hInstance, HaggleKernel *_kernel);
	~TrayNotifier() {}
};

#endif /* _TRAYNOTIFIER_H */