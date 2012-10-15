//---------------------------------------------------------------------------

#include <vcl.h>
#include <windows.h>
#include <list.h>
#include "FileVersion.h"
#include "OFPMonitorModel.h"
#include "OFPMonitorDefinitions.h"
#include "ServerList.h"
#include "GameControl.h"
#include "ProcessFinder.h"
#include "FontSettings.h"
#include "Unit1.h"
#include "Unit2.h"
#include "Unit3.h"
#pragma hdrstop

//---------------------------------------------------------------------------
USEFORM("Unit1.cpp", Form1);
USEFORM("Unit2.cpp", WINDOW_SETTINGS);
USEFORM("Unit4.cpp", WINDOW_INFO);
USEFORM("Unit3.cpp", WINDOW_LOCALGAME);
USEFORM("Unit5.cpp", WINDOW_UPDATE);
//---------------------------------------------------------------------------
HANDLE hMutex;

bool MyAppAlreadyRunning() {
        hMutex = CreateMutex(NULL,true,"OFPMonitor");
        if (GetLastError() == ERROR_ALREADY_EXISTS ) {
                CloseHandle(hMutex);
                return true; // Already running
        } else {
                return false; // First instance
        }
}

void releaseMutex() {
        CloseHandle(hMutex);
}

String getExeFromFullPath(String in) {
        String out = in;
        for(int i = in.Length() - 1; i >= 0; i--) {
                if(in.SubString(i,1) == "\\") {
                        out = in.SubString(i + 1, in.Length() - i);
                        break;
                }
        }
        return out;
}

WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
        if(!MyAppAlreadyRunning()) {
	        try {
                        String applicationDir = ExtractFileDir(Application->ExeName);
                        String soundDir = applicationDir + "\\sound\\";
                        if(!DirectoryExists(soundDir)) {
                                CreateDir(soundDir);
                                for(int i = 1; i <= 5; i++) {
                                        String files[5] = { "creating.wav", "waiting.wav", "briefing.wav", "playing.wav", "debriefing.wav" };
                                        String fileName = soundDir + files[i - 1];
                                        if(!FileExists(fileName)) {
                                                HMODULE hModule = GetModuleHandle(NULL);
                                                HRSRC resourceInfo = FindResource(hModule, MAKEINTRESOURCE(i), "WAVE");
                                                if(hModule && resourceInfo) {
                                                        HGLOBAL data = LoadResource(hModule, resourceInfo);
                                                        DWORD size = SizeofResource(hModule, resourceInfo);
                                                        if(data && size > 0) {
                                                                TMemoryStream *ms = new TMemoryStream();
                                                                ms->WriteBuffer(data, size);
                                                                ms->SaveToFile(fileName);
                                                                ms->Clear();
                                                                delete ms;
                                                        }
                                                }
                                        }
                                }
                        }
                        Application->Initialize();
                        FileVersion *fv = new FileVersion(Application->ExeName);
                        Application->Title = "OFPMonitor " + fv->getFullVersion();
                        delete fv;
                        Application->CreateForm(__classid(TForm1), &Form1);
                 Application->CreateForm(__classid(TWINDOW_UPDATE), &WINDOW_UPDATE);
                 Application->CreateForm(__classid(TWINDOW_INFO), &WINDOW_INFO);
                 Application->CreateForm(__classid(TWINDOW_LOCALGAME), &WINDOW_LOCALGAME);
                 Application->CreateForm(__classid(TWINDOW_SETTINGS), &WINDOW_SETTINGS);
                 String settingsFile = applicationDir + "\\OFPMonitor.ini";

                 ServerList *sL = new ServerList();
                 OFPMonitorModel *ofpm = new OFPMonitorModel(settingsFile, sL);
                 GameControl *gameControl = new GameControl(ofpm);
                 FontSettings *fontSettings = new FontSettings(Form1->Font);
                 WindowSettings *windowSettings = new WindowSettings();
                 ServerFilter *serverFilter = new ServerFilter();
                 Form1->setFontSettings(fontSettings);
                 Form1->setWindowSettings(windowSettings);
                 Form1->setServerFilter(serverFilter);
                 Form1->setGameControl(gameControl);
                 Form1->setModel(ofpm);
                 WINDOW_SETTINGS->setModel(ofpm);
                 WINDOW_LOCALGAME->setModel(ofpm);

                 TStringList *file = new TStringList;
                 if(FileExists(settingsFile)) {
                        file->LoadFromFile(settingsFile);
                 }
                 sL->readSettings(file);
                 ofpm->readSettings(file);
                 gameControl->readSettings(file);
                 fontSettings->readSettings(file);
                 windowSettings->readSettings(file);
                 serverFilter->readSettings(file);
                 Form1->readSettings(file);
                 delete file;

                 Form1->applyWindowSettings();
                 Application->Run();
                } catch (Exception &exception) {
                        Application->ShowException(&exception);
                } catch (...) {
                        try {
                                throw Exception("");
                        } catch (Exception &exception) {
                                Application->ShowException(&exception);
                        }
                }
	} else {
                ProcessFinder *finder = new ProcessFinder();
                TStringList *startsWith = new TStringList();
                startsWith->Add("OFPMonitor");
                TStringList *moduleIncludes = new TStringList();
                moduleIncludes->Add(getExeFromFullPath(Application->ExeName));
                if(finder->enumerate(startsWith, moduleIncludes)) {
                        ProcessInfo p = finder->output.front();
                        SendMessage(p.hWindow, WM_KEYDOWN, VK_F13, NULL);
                        SendMessage(p.hWindow, WM_KEYUP,   VK_F13, NULL);
                        SetForegroundWindow(p.hWindow);
                }
                delete finder;
                delete startsWith;
                delete moduleIncludes;
        }
        return 0;
}
//---------------------------------------------------------------------------

