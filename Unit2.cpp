//---------------------------------------------------------------------------

#include <vcl.h>                                                          
#include <list.h>
#include <filectrl.hpp>
#include <io.h>
#include <mmsystem.h>
#include <windows.h>
#include <Registry.hpp>
                                              
#pragma hdrstop

#include "Unit2.h"
#include "Unit1.h"
#include "Unit4.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TWINDOW_SETTINGS *WINDOW_SETTINGS;

#include "guiDBDefs.cpp"
#include "FileVersion.h"
#include "OFPMonitor.h"
using namespace OFPMonitor_Unit2;

String getGameName(int gameid) {
        if(gameid == GAMEID_OFPCWC) { return "OFP:CWC"; }
        if(gameid == GAMEID_OFPRES) { return "OFP:RES"; }
        if(gameid == GAMEID_ARMACWA) { return "ARMA:CWA"; }
        return "UNKNOWN GAME";
}

int getGameId(String name) {
        if(name == "OFP:CWC") { return GAMEID_OFPCWC; }
        if(name == "OFP:RES") { return GAMEID_OFPRES; }
        if(name == "ARMA:CWA") { return GAMEID_ARMACWA; }
        return -1;
}


/**
   Returns the number of game configurations the user has set
 */

int TWINDOW_SETTINGS::getConfAmount() {
        return confAmount;
}

/**
   Represents an game configuration, which the user can use to join an server
 */

class Configuration {
        public:
		bool set;
                String label;
                list<String> mods;
                String password;
                list<String> addParameters;
                int gameid;
                Configuration(int gameid, String l, list<String> &m, String pw, String aP, bool ns, bool nm) {
                        this->set = true;
                        this->gameid = gameid;
                        this->label = l;
                        this->mods = m;
                        this->password = pw;
                        this->addParameters = Form1->splitUpMessage(aP," ");
                        if(nm) {
                                this->addParameters.push_back("-nomap");
                        }
                        if(ns) {
                                this->addParameters.push_back("-nosplash");
                        }
                }

		Configuration () {
			this->set = false;
		}

                /**
                   Constructs the settings file's entry for this configuration
                 */

                TStringList* createFileEntry() {
                        TStringList *output = new TStringList;
                        output->Add("[Conf]");
                        output->Add("Game = " + getGameName(this->gameid));
                        if(!this->label.IsEmpty()) {
                                output->Add("Label = " + this->label);
                        }
                        if(!this->password.IsEmpty()) {
                                output->Add("Password = " + this->password);
                        }
                        if(this->mods.size() > 0) {
                                output->Add("Mods = " + this->createModLine());
                        }
                        if(this->addParameters.size() > 0) {
                                output->Add("Parameters = " + this->createParameterLine());
                        }
                        output->Add("[\\Conf]");
                        return output;
                }

                /**
                   Constructs the startup line, the game application is fed with
                   when OFPMonitor launches the application
                */

                String createStartLine(String ip, int port, String player) {
                        String out = "";
                        out += " " + this->createParameterLine();
                        out += " -connect=" + ip;
                        out += " -port=" + String(port);
                        String ml = this->createModLine();
                        if(!player.IsEmpty()) {
                                out += " \"-name=" + player + "\"";
                        }
                        if(!ml.IsEmpty()) {
                                out += " \"-mod="+ ml +"\"";
                        }
                        if(!this->password.IsEmpty()) {
                                out += " \"-password="  + this->password + "\"";
                        }
                        return out;
                }

                /**
                   Constructs this configuration's entry in the settings window 
                 */

                String createListEntry() {
                        String out = "";
                        if(this->set) {
                                if(!this->label.Trim().IsEmpty()) {
                                        out += this->label + "  ";
                                }
                                if(this->password.Length() > 0) {
                                        out += "pw:" + this->password + "  ";
                                }
                                out += this->createModLine();
                        }
                        return out;
                }

                /**
                   Concatenates all mod folders of this configuration, with ; as seperator,
                   as Operation Flashpoint expects it
                 */

                String createModLine () {
                        String modline = "";
                        unsigned int i = 0;
                        for (list<String>::iterator ci = this->mods.begin(); ci != this->mods.end(); ++ci) {
                                i++;
                                modline += *ci;
                                if(i < this->mods.size()) {
                                        modline += ";";
                                }
                        }
                        return modline;
                }

                /**
                   Concatenates all additional startup parameters of this configuration, with a space as seperator
                 */

                String createParameterLine() {
                        String paraline = "";
                        unsigned int j = 0;
                        for (list<String>::iterator cj = this->addParameters.begin(); cj != this->addParameters.end(); ++cj) {
                                j++;
                                paraline += *cj;
                                if(j < this->addParameters.size()) {
                                        paraline += " ";
                                }
                        }
                        return paraline;
                }

                /**
                   Creates and returns a copy of this configuration
                 */

                Configuration clone() {
                        list<String> c_mods;
                        for (list<String>::iterator cm = this->mods.begin(); cm != this->mods.end(); ++cm) {
                                c_mods.push_back(*cm);
                        }
                        list<String> c_addParameters;
                        for (list<String>::iterator cap = this->addParameters.begin(); cap != this->addParameters.end(); ++cap) {
                                c_addParameters.push_back(*cap);
                        }
                        Configuration copy = Configuration();
                        copy.set = true;
                        copy.label = this->label;
                        copy.password = this->password;
                        copy.mods = c_mods;
                        copy.addParameters = c_addParameters;
                        copy.gameid = this->gameid;
                        return copy;
                }
};

/**
   Converts an bool to its binary representation
 */

String checkBool(bool in) {
        if(in) {
                return "1";
        } else {
                return "0";
        }
}

/**
   Converts the binary representation of true and false (1 and 0) to a bool
 */

bool checkBool2(String in) {
        return (in == "1");
}

/**
   Reads a String from the OS registry. 'a' holds a list of keys to visit in order,
   'key' is the object which content is to read inside the last key
 */

String GetRegistryValue(void * root, list<String> a, String key) {
        String S = "";
        TRegistry *Registry = new TRegistry(KEY_READ);
        try
        {
                Registry->RootKey = root;
                while (a.size() > 0) {
                        Registry->OpenKey(a.front(),false);
                        a.pop_front();
                }
                S = Registry->ReadString(key);
        }
        __finally {
                delete Registry;
        }
        return S;
}

/**
   Returns the folder of the game exe file
 */

String getFolder(String in) {
        String out = "";
        for(int i = in.Length() - 1; i >= 0; i--) {
                if(in.SubString(i,1) == "\\") {
                        out = in.SubString(0, i).Trim();
                        break;
                }
        }
        return out;
}

/**
   Reads all player profile folders in OFP's \Users folder
 */

list<String> findPlayerProfiles(String ofpfolder) {
        list<String> profiles;
        if(!ofpfolder.IsEmpty()) {
                TSearchRec daten;
                if(0 == FindFirst((ofpfolder +"\\Users\\*").c_str(), faDirectory, daten)) {
                        try {
                                do {
                                        if(     daten.Size == 0 &&
                                                daten.Name != "." &&
                                                daten.Name != ".." &&
                                                FileExists(ofpfolder + "\\Users\\" + daten.Name + "\\UserInfo.cfg")) {
                                                profiles.push_back(String(daten.Name));
                                        }
                                } while(FindNext(daten) == 0);
                        }__finally
                        {
                                FindClose(daten);
                        }
                }
        }
        return profiles;
}


class Game {
        public:
                bool set;
                int gameid;
                String exe;
                String folder;
                String player;
                String fullName;
                int version;
                Configuration startupConfs[confAmount];

                String defaultExeName;
                String gamespyToken;
                Game () {
                        this->set = false;
                        this->exe = "";
                        this->folder = "";
                        this->version = 0;
                        this->fullName = "";
                        this->player = "";
                }

                /**
                   Checks if the correct exe is set
                 */

                bool isValid() {
                        return FileExists(this->exe);
                }

                void autodetect(String exe) {
                        this->autodetect(exe, "");
                }

                void autodetect(String exe, String player) {
                        String e = exe;
                        if(!FileExists(e)) {
                                list<String> regFolder;
                                if(this->gameid == GAMEID_OFPCWC || this->gameid == GAMEID_OFPRES) {
                                        regFolder.push_back("SOFTWARE");
                                        regFolder.push_back("Codemasters");
                                        regFolder.push_back("Operation Flashpoint");
                                } else if(this->gameid == GAMEID_ARMACWA) {
                                        regFolder.push_back("SOFTWARE");
                                        regFolder.push_back("Bohemia Interactive Studio");
                                        regFolder.push_back("ColdWarAssault");
                                }
                                String f = GetRegistryValue(HKEY_LOCAL_MACHINE, regFolder, "Main");
                                e = f + "\\" + this->defaultExeName;
                        }
                        if(FileExists(e)) {
                                this->exe = e;
                                this->folder = getFolder(this->exe);
                                this->queryVersion();
                                this->detectPlayer(player);
                                this->set = true;
                        }
                }

                void detectPlayer(String setP) {
                        list<String> regPlayer;
                        if(gameid == GAMEID_OFPCWC || gameid == GAMEID_OFPRES) {
                                regPlayer.push_back("SOFTWARE");
                                regPlayer.push_back("Codemasters");
                                regPlayer.push_back("Operation Flashpoint");
                         } else if(gameid == GAMEID_ARMACWA) {
                                regPlayer.push_back("SOFTWARE");
                                regPlayer.push_back("Bohemia Interactive Studio");
                                regPlayer.push_back("ColdWarAssault");
                        }
                        String regP = GetRegistryValue(HKEY_CURRENT_USER, regPlayer, "Player Name");
                        list<String> profiles = findPlayerProfiles(getFolder(this->exe));
                        bool fRegPlayer = false, fSetPlayer = false;
                        for (list<String>::iterator ci = profiles.begin(); ci != profiles.end(); ++ci) {
                                if((*ci) == setP && !setP.IsEmpty()) {
                                        fSetPlayer = true;
                                }
                                if((*ci) == regP && !regP.IsEmpty()) {
                                        fRegPlayer = true;
                                }
                        }
                        if(fSetPlayer) {
                                this->player = setP;
                        } else if(fRegPlayer) {
                                this->player = regP;
                        } else if(profiles.size() > 0) {
                                this->player = profiles.front();
                        }
                }

                void setGameId(int gameid) {
                        this->gameid = gameid;
                        if(this->gameid == GAMEID_OFPCWC) {
                                this->gamespyToken = "opflash";
                                this->defaultExeName = "OperationFlashpoint.exe";
                                this->fullName = "Operation Flashpoint: Cold War Crisis";
                        } else if(this->gameid == GAMEID_OFPRES) {
                                this->gamespyToken = "opflashr";
                                this->defaultExeName = "FLASHPOINTRESISTANCE.EXE";
                                this->fullName = "Operation Flaspoint: Resistance";
                        } else if(this->gameid == GAMEID_ARMACWA) {
                                this->gamespyToken = "opflashr";
                                this->defaultExeName = "ColdWarAssault.exe";
                                this->fullName = "ArmA: Cold War Assault";
                        }
                }

                void queryVersion() {
                        if(FileExists(this->exe)) {
                                FileVersion *fv = new FileVersion(this->exe);
                                this->version = fv->getVersion();
                                delete fv;
                        }
                }

                bool checkIfCorrectGame(int actVer, int reqVer) {
                        if(!this->set) { return false; }
                        return (reqVer <= this->version && this->version <= actVer && !this->player.IsEmpty());
                }


};

/**
   Stores the general program settings
 */

class Settings {
        public:
                bool changed;
                String workdir;
                String exe;
                String folder;
                String player;
                int interval;
                bool customNotifications;
                Game games[GAMES_TOTAL];
                String file;
                String languagefile;
                TStringList *watched;

                Settings() {
                        this->watched = new TStringList;
                        this->watched->Sorted = true;
                        this->watched->Duplicates = dupIgnore;
                        this->workdir = GetCurrentDir();
                        this->customNotifications = false;
                        this->interval = 1;
                        this->file = this->workdir + "\\OFPMonitor.ini";
                        this->languagefile = "Default";
                        this->changed = false;
                        for(int i = 0; i < GAMES_TOTAL; i++) {
                                this->games[i] = Game();
                                this->games[i].setGameId(i);
                        }
                }

                Configuration pSgetConf(int gameid, int i) {
                        return this->games[gameid].startupConfs[i];
                }
                void pSdeleteConf(int gameid, int i) {
                        this->games[gameid].startupConfs[i].set = false;
                        for(int j = i + 1; j < confAmount; j++) {
                                this->games[gameid].startupConfs[j - 1] = (this->games[gameid].startupConfs[j]);
                        }
                        this->games[gameid].startupConfs[confAmount - 1].set = false;
                        this->changed = true;
                }
                void pSaddConf(int gameid, Configuration c) {
                        for(int i = 0; i < confAmount; i++) {
                                if(!this->games[gameid].startupConfs[i].set) {
                                        this->games[gameid].startupConfs[i] = c;
                                        break;
                                }
                        }
                        this->changed = true;
                }

                void setInterval(int i) {
                        this->interval = i;
                        Form1->Timer1->Interval = i * 1000;
                        this->changed = true;
                }

                void writeToFile(list<String> &servers, list<String> &watchedServers, list<String> &font, list<String> &window, list<String> &chat) {
                        if(this->changed) {
                                TStringList *file = new TStringList;
                                file->Add("[General]");
                                file->Add("LangFile = " + this->languagefile);
                                file->Add("Interval = " + String(this->interval));
                                file->Add("customNotifications = " + checkBool(this->customNotifications));
                                file->Add("[\\General]");

                                for(int i = 0; i < GAMES_TOTAL; i++) {
                                        if(this->games[i].set) {
                                                file->Add("[Game]");
                                                if(i == GAMEID_OFPCWC) { file->Add("Name = OFP:CWC"); }
                                                if(i == GAMEID_OFPRES) { file->Add("Name = OFP:RES"); }
                                                if(i == GAMEID_ARMACWA) { file->Add("Name = ARMA:CWA"); }
                                                file->Add("Exe = " + this->games[i].exe);
                                                if(!this->games[i].player.IsEmpty()) { file->Add("LastPlayer = " + this->games[i].player); }
                                                file->Add("[\\Game]");
                                        }
                                }

                                for(int j = 0; j < GAMES_TOTAL; j++) {
                                        for(int i = 0; i < confAmount; i++) {
                                                if(this->games[j].startupConfs[i].set) {
                                                        TStringList *entry = this->games[j].startupConfs[i].createFileEntry();
                                                        while (entry->Count > 0) {
                                                                file->Add(entry->Strings[0]);
                                                                entry->Delete(0);
                                                        }
                                                        delete entry;
                                                }
                                        }
                                }
                                file->Add("[Filters]");
                                file->Add("Playing = " + checkBool(Form1->CHECKBOX_FILTER_PLAYING->Checked));
                                file->Add("Waiting= " + checkBool(Form1->CHECKBOX_FILTER_WAITING->Checked));
                                file->Add("Creating = " + checkBool(Form1->CHECKBOX_FILTER_CREATING->Checked));
                                file->Add("Settingup = " + checkBool(Form1->CHECKBOX_FILTER_SETTINGUP->Checked));
                                file->Add("Briefing = " + checkBool(Form1->CHECKBOX_FILTER_BRIEFING->Checked));
                                file->Add("Debriefing = " + checkBool(Form1->CHECKBOX_FILTER_DEBRIEFING->Checked));
                                file->Add("WithPW = " + checkBool(Form1->CHECKBOX_FILTER_WITHPASSWORD->Checked));
                                file->Add("WithoutPW = " + checkBool(Form1->CHECKBOX_FILTER_WITHOUTPASSWORD->Checked));
                                file->Add("minPlayers = " + IntToStr(Form1->UpDown1->Position));
                                file->Add("ServerName = " + Form1->Edit2->Text);
                                file->Add("MissionName = " + Form1->Edit1->Text);
                                file->Add("PlayerName = " + Form1->Edit4->Text);
                                file->Add("[\\Filters]");
                                String tmp;
                                while(font.size() > 0) {
                                        tmp = font.front();
                                        file->Add(tmp);
                                        font.pop_front();
                                }
                                while(window.size() > 0) {
                                        tmp = window.front();
                                        file->Add(tmp);
                                        window.pop_front();
                                }

                                while(chat.size() > 0) {
                                        tmp = chat.front();
                                        file->Add(tmp);
                                        chat.pop_front();
                                }

                                if (servers.size() > 0) {
                                        file->Add("[Servers]");
                                        if(watchedServers.size() > 0) {
                                                file->Add("[Watch]");
                                                while(watchedServers.size() > 0) {
                                                        tmp = watchedServers.front();
                                                        file->Add(tmp);
                                                        watchedServers.pop_front();
                                                }
                                                file->Add("[\\Watch]");
                                        }
                                        while(servers.size() > 0) {
                                                tmp = servers.front();
                                                file->Add(tmp);
                                                servers.pop_front();
                                        }
                                        file->Add("[\\Servers]");
                                }
                                TStringList *notifications = WINDOW_SETTINGS->getNotificationsFileEntries();
                                while(notifications->Count > 0) {
                                        file->Add(notifications->Strings[0]);
                                        notifications->Delete(0);
                                }
                                file->SaveToFile(this->file);
                                delete notifications;
                                delete file;
                        }
                }

                String createStartLine(String ip, int port, String player, String modline) {
                        String out = " -nosplash -nomap ";
                        out += " -connect=" + ip;
                        out += " -port=" + String(port);
                        if(!player.IsEmpty()) {
                                out += " \"-name=" + player + "\"";
                        }
                        if(!modline.IsEmpty()) {
                                out += " \"-mod="+ modline +"\"";
                        }
                        return out;
                }
                void setGame(int id, String exe, String player) {
                        if(id >= 0 && id < GAMES_TOTAL) {
                                if(FileExists(exe)) {
                                        this->games[id].autodetect(exe, player);
                                }
                        }
                }

                void removeGame(int id) {
                        this->games[id].set = false;
                }

                void setPlayer(int id, String player) {
                        if(id >= 0 && id < GAMES_TOTAL) {
                                if(this->games[id].isValid()) {
                                        this->games[id].detectPlayer(player);
                                }
                        }
                }

                String getPlayerForServer(int actVer, int reqVer) {
                        for(int i = 0; i < GAMES_TOTAL; i++) {
                                if(this->games[i].set && this->games[i].isValid()) {
                                        if(this->games[i].checkIfCorrectGame(actVer, reqVer)) {
                                                return this->games[i].player;
                                        }
                                }
                        }
                        return "";
                }

                int getMatchingGame(int actVer, int reqVer) {
                        for(int i = 0; i < GAMES_TOTAL; i++) {
                                if(this->games[i].set && this->games[i].isValid()) {
                                        if(this->games[i].checkIfCorrectGame(actVer, reqVer)) {
                                                return i;
                                        }
                                }
                        }
                        return -1;
                }

                void refreshGamesModList(TComboBox *box) {
                        String last = box->Text;
                        box->Items->Clear();
                        for(int i = 0; i < GAMES_TOTAL; i++) {
                                if(this->games[i].set && this->games[i].isValid()) {
                                        box->Items->AddObject(this->games[i].fullName, (TObject *)i);
                                }
                        }
                        if(box->Items->Count > 0) {
                                int index = box->Items->IndexOf(last);
                                if(index > -1) {
                                        box->ItemIndex = index;
                                } else {
                                        box->ItemIndex = 0;
                                }
                                box->Enabled = true;
                                WINDOW_SETTINGS->GROUPBOX_NEWCONFIGURATION->Enabled = true;
                        } else {
                                box->Enabled = false;
                        }
                        box->OnChange(WINDOW_SETTINGS);
                }
};
Settings programSettings = Settings();

void TWINDOW_SETTINGS::writeSettingToFile(list<String> servers, list<String> watchedServers, list<String> font, list<String> window, list<String> chat) {
        programSettings.writeToFile(servers, watchedServers, font, window, chat);
}

void TWINDOW_SETTINGS::setCustomNotifications(bool active) {
        programSettings.customNotifications = active;
}

bool TWINDOW_SETTINGS::areCustomNotificationsEnabled() {
        return programSettings.customNotifications;
}

void TWINDOW_SETTINGS::setSettingsChanged() {
        programSettings.changed = true;
}

String TWINDOW_SETTINGS::getConfListEntry(int gameid, int i) {
        return programSettings.games[gameid].startupConfs[i].createListEntry();
}

String TWINDOW_SETTINGS::getConfStartLine(int gameid, int i, String ip, int port) {
        return programSettings.games[gameid].startupConfs[i].createStartLine(ip, port, programSettings.games[gameid].player);
}

String TWINDOW_SETTINGS::getNoModsStartLine(int gameid, String ip, int port) {
        return programSettings.createStartLine(ip, port, programSettings.games[gameid].player, "");
}

String TWINDOW_SETTINGS::getSameModsStartLine(int gameid, String ip, int port, String servermods) {
        return programSettings.createStartLine(ip, port, programSettings.games[gameid].player, servermods);
}

String TWINDOW_SETTINGS::getConfModLine(int gameid, int i) {
        return programSettings.games[gameid].startupConfs[i].createModLine();
}

TStringList* TWINDOW_SETTINGS::getWatchedList() {
        return programSettings.watched;
}

int TWINDOW_SETTINGS::getGameId(int actVer, int reqVer) {
        return programSettings.getMatchingGame(actVer, reqVer);
}

/**
   Returns the currently set game exe file
 */

String TWINDOW_SETTINGS::getExe(int actVer, int reqVer) {
        for(int i = 0; i < GAMES_TOTAL; i++) {
                if(programSettings.games[i].set) {
                        if(programSettings.games[i].checkIfCorrectGame(actVer, reqVer)) {
                                return programSettings.games[i].exe;
                        }
                }
        }
        return "";
}

/**
   Returns the game folder
 */

String TWINDOW_SETTINGS::getExeFolder(int actVer, int reqVer) {
        for(int i = 0; i < GAMES_TOTAL; i++) {
                if(programSettings.games[i].set) {
                        if(programSettings.games[i].checkIfCorrectGame(actVer, reqVer)) {
                                return programSettings.games[i].folder;
                        }
                }
        }
        return "";
}

/**
   Refreshes the list box for the configurations in the settings window
 */

void updateConfList() {
        WINDOW_SETTINGS->LISTBOX_CONFIGURATIONS->Clear();
        int comboindex = WINDOW_SETTINGS->ComboBox2->Items->IndexOf(WINDOW_SETTINGS->ComboBox2->Text);
        if(comboindex >= 0) {
                int gameid = (int) WINDOW_SETTINGS->ComboBox2->Items->Objects[comboindex];
                for(int i = 0; i < confAmount; i++) {
                        if(programSettings.games[gameid].startupConfs[i].set) {
                                WINDOW_SETTINGS->LISTBOX_CONFIGURATIONS->Items->AddObject(programSettings.games[gameid].startupConfs[i].createListEntry(), (TObject *) i);
                        }
                }
        }
}

/**
   Returns the value of a String with the format "XYZ = VALUE"
 */

String getValue(String in) {
        String out = "";
        String tmp = in.Trim();
        for(int i = 1; i < tmp.Length(); i++) {
                if(tmp.SubString(i,1) == "=") {
                        out = tmp.SubString(i + 1, tmp.Length() - i).Trim();
                        break;
                }
        }
        return out;
}

/**
   Splits a String with 'diff' as seperator and returns the two parts in a list
 */

list<String> getVarAndValue(String in, String diff) {
        list<String> out;
        String tmp = in.Trim();
        for(int i = 0; i < tmp.Length(); i++) {
                if(tmp.SubString(i,diff.Length()) == diff) {
                        out.push_front(tmp.SubString(1, i - 1).Trim());
                        out.push_back(tmp.SubString(i + diff.Length(), tmp.Length() - (i + diff.Length() -1)).Trim());
                        break;
                }
        }
        return out;
}





/**
   Reads all folders within the game folder and displays them in the
   modfolder list box in the settings window
 */

void updateModFolderList(String ofpfolder) {
        WINDOW_SETTINGS->LISTBOX_MODFOLDERS_ALL->Clear();
        if(!ofpfolder.IsEmpty()) {
        	TSearchRec daten;
                if(0 == FindFirst((ofpfolder +"\\*").c_str(), faDirectory, daten)) {
                        try {
                                do {
                                        if(daten.Size == 0 && daten.Name != "." && daten.Name != "..") {
                                                WINDOW_SETTINGS->LISTBOX_MODFOLDERS_ALL->Items->Add(String(daten.Name));
                                        }
                                } while(FindNext(daten) == 0);
                        }__finally
                        {
                                FindClose(daten);
                        }
                }
        }
}

/**
   Returns a String of the current set language for an identifier 
 */

String TWINDOW_SETTINGS::getGuiString(String ident) {
        String out = "";
        for (list<guiString>::iterator ci = guiStrings.begin(); ci != guiStrings.end(); ++ci) {
                if((*ci).identifier == ident) {
                        out = (*ci).value;
                        break;
                }
        }
        return out;
}

void updateGames() {
        TComboBox *combobox;
        TCheckBox *checkbox;
        TEdit *edit;
        TLabel *label;
        for(int i = 0; i < GAMES_TOTAL; i++) {
                if(i == GAMEID_OFPCWC) {
                        combobox = WINDOW_SETTINGS->COMBOBOX_OFPCWC_PROFILE;
                        checkbox = WINDOW_SETTINGS->CHECKBOX_OFPCWC;
                        edit = WINDOW_SETTINGS->EDIT_OFPCWC_EXECUTABLE;
                        label = WINDOW_SETTINGS->LABEL_OFPCWC_DETECTEDVERSION;
                }
                if(i == GAMEID_OFPRES) {
                        combobox = WINDOW_SETTINGS->COMBOBOX_OFPRES_PROFILE;
                        checkbox = WINDOW_SETTINGS->CHECKBOX_OFPRES;
                        edit = WINDOW_SETTINGS->EDIT_OFPRES_EXECUTABLE;
                        label = WINDOW_SETTINGS->LABEL_OFPRES_DETECTEDVERSION;
                }
                if(i == GAMEID_ARMACWA) {
                        combobox = WINDOW_SETTINGS->COMBOBOX_ARMACWA_PROFILE;
                        checkbox = WINDOW_SETTINGS->CHECKBOX_ARMACWA;
                        edit = WINDOW_SETTINGS->EDIT_ARMACWA_EXECUTABLE;
                        label = WINDOW_SETTINGS->LABEL_ARMACWA_DETECTEDVERSION;
                }

                if(programSettings.games[i].set) {
                        checkbox->Checked = true;
                        edit->Text = programSettings.games[i].exe;
                        if(!programSettings.games[i].exe.IsEmpty()) {
                                label->Caption = WINDOW_SETTINGS->getGuiString("STRING_DETECTEDVERSION") + "  " + IntToStr(programSettings.games[i].version);
                        } else {
                                label->Caption = "";
                        }
                        combobox->Items->Clear();
                        list<String> profiles = findPlayerProfiles(programSettings.games[i].folder);
                        String textToSet = "";
                        if(profiles.size() > 0) {
                                combobox->Enabled = true;
                                bool playerMatching = false;
                                for (list<String>::iterator ci = profiles.begin(); ci != profiles.end(); ++ci) {
                                        combobox->Items->Add(*ci);
                                        if((*ci) == programSettings.games[i].player) {
                                                playerMatching = true;
                                        }
                                }
                                if(!playerMatching) {
                                        programSettings.games[i].detectPlayer("");
                                }
                                textToSet = programSettings.games[i].player;
                        } else {
                                combobox->Enabled = false;
                                programSettings.games[i].player = "";
                                textToSet = WINDOW_SETTINGS->getGuiString("STRING_NOPROFILES");
                                combobox->Items->Add(textToSet);
                        }
                        combobox->ItemIndex = combobox->Items->IndexOf(textToSet);
                } else {
                        checkbox->Checked = false;
                }
        }
        programSettings.refreshGamesModList(WINDOW_SETTINGS->ComboBox2);
}


/**
   Applys a language file to the Gui
 */

void updateLanguage(String languagefile) {
        TStringList *file = new TStringList;
        String pathAndFile = programSettings.workdir + "\\" + languagefile;
        guiStrings.push_back(guiString("STRING_YES","Yes"));
        guiStrings.push_back(guiString("STRING_NO","No"));
        guiStrings.push_back(guiString("STRING_NAME","Name"));
        guiStrings.push_back(guiString("STRING_ONLINE","Online:"));
        guiStrings.push_back(guiString("STRING_LISTED","Listed:"));
        guiStrings.push_back(guiString("STRING_ERRORS","Errors:"));
        guiStrings.push_back(guiString("STRING_NOPROFILES","No player profiles found!"));
        guiStrings.push_back(guiString("STRING_PLAYER_ALREADY_ON_SERVER","There's already a player with the same name on the server. Do you still want to start OFP?"));
        guiStrings.push_back(guiString("STRING_FROM","From:"));
        guiStrings.push_back(guiString("STRING_TO","To:"));
        guiStrings.push_back(guiString("STRING_PLAYBACKVOLUME","Volume:"));
        guiStrings.push_back(guiString("STRING_AUDIOFILE","MP3-File:"));
        guiStrings.push_back(guiString("STRING_MARKINGCOLOR","Marking color:"));
        guiStrings.push_back(guiString("STRING_MINIMUM","Min."));
        guiStrings.push_back(guiString("STRING_MAXIMUM","Max:"));
        guiStrings.push_back(guiString("STRING_CHAT_CONNECTINGTO","Connecting to:"));
        guiStrings.push_back(guiString("STRING_CHAT_CHANNEL","Channel:"));
        guiStrings.push_back(guiString("STRING_CHAT_DISCONNECTED","Disconnected."));
        guiStrings.push_back(guiString("STRING_CHAT_CONNECTING FAILED","Connecting failed."));
        guiStrings.push_back(guiString("STRING_CHAT_JOINED","joined"));
        guiStrings.push_back(guiString("STRING_CHAT_LEFT","left"));
        guiStrings.push_back(guiString("STRING_CHAT_CONNECTIONLOST","Connection lost. Reconnecting ..."));
        guiStrings.push_back(guiString("STRING_DETECTEDVERSION","Detected file version: "));
        guiStrings.push_back(guiString("STRING_CHAT_LEFT","left"));
        guiStrings.push_back(guiString("STRING_BROWSE","Browse ..."));
        guiStrings.push_back(guiString("STRING_OFPEXECUTABLE","Executable"));
        guiStrings.push_back(guiString("STRING_PROFILE","Player name:"));


        if(FileExists(pathAndFile)) {

                file->LoadFromFile(pathAndFile);
                String tmp;
                guiStrings.clear();
                list<String> val;
                for(int i = 0; i < file->Count; i++) {
                        Application->ProcessMessages();
                        val.clear();
                        tmp = file->Strings[i].Trim();
                        if(tmp.SubString(1,6) == "BUTTON") {
                                val = getVarAndValue(tmp, "=");
                                for(int j = 0; j < GetArrLength(guiButton); j++) {
                                        if(guiButton[j]->Name == val.front()) {
                                                guiButton[j]->Caption = val.back();
                                                break;
                                        }
                                }
                        } else if(tmp.SubString(1,5) == "LABEL") {
                                val = getVarAndValue(tmp, "=");
                                for(int j = 0; j < GetArrLength(guiLabel); j++) {
                                        if(guiLabel[j]->Name == val.front()) {
                                                guiLabel[j]->Caption = val.back();
                                                break;
                                        }
                                }
                        } else if(tmp.SubString(1,8) == "CHECKBOX") {
                                val = getVarAndValue(tmp, "=");
                                for(int j = 0; j < GetArrLength(guiCheckBox); j++) {
                                        if(guiCheckBox[j]->Name == val.front()) {
                                                guiCheckBox[j]->Caption = val.back();
                                                break;
                                        }
                                }
                        } else if(tmp.SubString(1,8) == "GROUPBOX") {
                                val = getVarAndValue(tmp, "=");
                                for(int j = 0; j < GetArrLength(guiGroupBox); j++) {
                                        if(guiGroupBox[j]->Name == val.front()) {
                                                guiGroupBox[j]->Caption = val.back();
                                                break;
                                        }
                                }
                        } else if(tmp.SubString(1,8) == "MENUITEM") {
                                val = getVarAndValue(tmp, "=");
                                for(int j = 0; j < GetArrLength(guiMenuItem); j++) {
                                        if(guiMenuItem[j]->Name == val.front()) {
                                                guiMenuItem[j]->Caption = val.back();
                                                break;
                                        }
                                }
                        } else if(tmp.SubString(1,6) == "WINDOW") {
                                val = getVarAndValue(tmp, "=");
                                for(int j = 0; j < GetArrLength(guiForm); j++) {
                                        if(guiForm[j]->Name == val.front()) {
                                                guiForm[j]->Caption = val.back();
                                                break;
                                        }
                                }
                        } else if(tmp.SubString(1,6) == "STRING") {
                                val = getVarAndValue(tmp, "=");
                                String f = val.front();
                                String b = val.back();
                                guiStrings.push_back(guiString(f,b));
                        } else if(tmp.SubString(1,8) == "TABSHEET") {
                                val = getVarAndValue(tmp, "=");
                                for(int j = 0; j < GetArrLength(guiTabSheet); j++) {
                                        if(guiTabSheet[j]->Name == val.front()) {
                                                guiTabSheet[j]->Caption = val.back();
                                                break;
                                        }
                                }
                        }
                }
                Form1->StringGrid1->Cells[0][0] = WINDOW_SETTINGS->getGuiString("STRING_ID");
                Form1->StringGrid1->Cells[1][0] = WINDOW_SETTINGS->getGuiString("STRING_NAME");
                Form1->StringGrid1->Cells[2][0] = WINDOW_SETTINGS->getGuiString("STRING_PLAYERS");
                Form1->StringGrid1->Cells[3][0] = WINDOW_SETTINGS->getGuiString("STRING_STATUS");
                Form1->StringGrid1->Cells[4][0] = WINDOW_SETTINGS->getGuiString("STRING_ISLAND");
                Form1->StringGrid1->Cells[5][0] = WINDOW_SETTINGS->getGuiString("STRING_MISSION");
                Form1->StringGrid1->Cells[6][0] = WINDOW_SETTINGS->getGuiString("STRING_PING");
                Form1->StringGrid2->Cells[0][0] = WINDOW_SETTINGS->getGuiString("STRING_NAME");
                Form1->StringGrid2->Cells[1][0] = WINDOW_SETTINGS->getGuiString("STRING_SCORE");
                Form1->StringGrid2->Cells[2][0] = WINDOW_SETTINGS->getGuiString("STRING_DEATHS");
                Form1->StringGrid2->Cells[3][0] = WINDOW_SETTINGS->getGuiString("STRING_TEAM");
                if(!WINDOW_SETTINGS->COMBOBOX_OFPRES_PROFILE->Enabled) {
                        WINDOW_SETTINGS->COMBOBOX_OFPRES_PROFILE->Text = WINDOW_SETTINGS->getGuiString("STRING_NOPROFILES");
                }
                if(!WINDOW_SETTINGS->COMBOBOX_OFPCWC_PROFILE->Enabled) {
                        WINDOW_SETTINGS->COMBOBOX_OFPCWC_PROFILE->Text = WINDOW_SETTINGS->getGuiString("STRING_NOPROFILES");
                }
                if(!WINDOW_SETTINGS->COMBOBOX_ARMACWA_PROFILE->Enabled) {
                        WINDOW_SETTINGS->COMBOBOX_ARMACWA_PROFILE->Text = WINDOW_SETTINGS->getGuiString("STRING_NOPROFILES");
                }
                WINDOW_SETTINGS->LABEL_FILTER_MISSIONNAME_BOX->Caption = Form1->LABEL_FILTER_MISSIONNAME->Caption;
                WINDOW_SETTINGS->LABEL_FILTER_SERVERNAME_BOX->Caption = Form1->LABEL_FILTER_SERVERNAME->Caption;
                WINDOW_SETTINGS->LABEL_FILTER_PLAYERNAME_BOX->Caption = Form1->LABEL_FILTER_PLAYERNAME->Caption;
                WINDOW_SETTINGS->BUTTON_EDITNOTIFICATION_CANCEL->Caption = WINDOW_SETTINGS->BUTTON_EDITCONFIGURATION_CANCEL->Caption;
                WINDOW_SETTINGS->BUTTON_EDITNOTIFICATION_EDIT->Caption = WINDOW_SETTINGS->BUTTON_EDITCONFIGURATION_EDIT->Caption;
                WINDOW_SETTINGS->BUTTON_EDITNOTIFICATION_OK->Caption = WINDOW_SETTINGS->BUTTON_EDITCONFIGURATION_OK->Caption;
                WINDOW_SETTINGS->BUTTON_NEWNOTIFICATION_ADD->Caption = WINDOW_SETTINGS->BUTTON_NEWCONFIGURATION_ADD->Caption;
                WINDOW_SETTINGS->BUTTON_NEWNOTIFICATION_CLEAR->Caption = WINDOW_SETTINGS->BUTTON_NEWCONFIGURATION_CLEAR->Caption;
                WINDOW_SETTINGS->BUTTON_NOTIFICATION_REMOVE->Caption = WINDOW_SETTINGS->BUTTON_CONFIGURATION_REMOVE->Caption;
                WINDOW_SETTINGS->CHECKBOX_FILTER_CREATING->Caption = Form1->CHECKBOX_FILTER_CREATING->Caption;
                WINDOW_SETTINGS->CHECKBOX_FILTER_WAITING->Caption = Form1->CHECKBOX_FILTER_WAITING->Caption;
                WINDOW_SETTINGS->CHECKBOX_FILTER_BRIEFING->Caption = Form1->CHECKBOX_FILTER_BRIEFING->Caption;
                WINDOW_SETTINGS->CHECKBOX_FILTER_SETTINGUP->Caption = Form1->CHECKBOX_FILTER_SETTINGUP->Caption;
                WINDOW_SETTINGS->CHECKBOX_FILTER_PLAYING->Caption = Form1->CHECKBOX_FILTER_PLAYING->Caption;
                WINDOW_SETTINGS->CHECKBOX_FILTER_DEBRIEFING->Caption = Form1->CHECKBOX_FILTER_DEBRIEFING->Caption;
                WINDOW_SETTINGS->CHECKBOX_FILTER_MINPLAYERS->Caption = WINDOW_SETTINGS->getGuiString("STRING_MINIMUM");
                WINDOW_SETTINGS->CHECKBOX_FILTER_MAXPLAYERS->Caption = WINDOW_SETTINGS->getGuiString("STRING_MAXIMUM");
                WINDOW_SETTINGS->BUTTON_BROWSE->Caption = WINDOW_SETTINGS->getGuiString("STRING_BROWSE");
                WINDOW_SETTINGS->BUTTON_OFPCWC_BROWSE->Caption = WINDOW_SETTINGS->getGuiString("STRING_BROWSE");
                WINDOW_SETTINGS->BUTTON_OFPRES_BROWSE->Caption = WINDOW_SETTINGS->getGuiString("STRING_BROWSE");
                WINDOW_SETTINGS->BUTTON_ARMACWA_BROWSE->Caption = WINDOW_SETTINGS->getGuiString("STRING_BROWSE");
                WINDOW_SETTINGS->LABEL_FILTER_PASSWORD->Caption = Form1->LABEL_FILTER_PASSWORD->Caption;
                WINDOW_SETTINGS->LABEL_FILTER_STATUS->Caption = Form1->LABEL_FILTER_STATUS->Caption;
                WINDOW_SETTINGS->LABEL_NOTIFICATION_NAME->Caption = WINDOW_SETTINGS->getGuiString("STRING_NAME");
                WINDOW_SETTINGS->LABEL_FILTER_PLAYERS->Caption = WINDOW_SETTINGS->getGuiString("STRING_PLAYERS");
                WINDOW_SETTINGS->CHECKBOX_FILTER_WITHOUTPASSWORD->Caption = Form1->CHECKBOX_FILTER_WITHOUTPASSWORD->Caption;
                WINDOW_SETTINGS->CHECKBOX_FILTER_WITHPASSWORD->Caption = Form1->CHECKBOX_FILTER_WITHPASSWORD->Caption;
                WINDOW_SETTINGS->LABEL_VOLUME->Caption = WINDOW_SETTINGS->getGuiString("STRING_PLAYBACKVOLUME");
                WINDOW_SETTINGS->LABEL_MARKINGCOLOR->Caption = WINDOW_SETTINGS->getGuiString("STRING_MARKINGCOLOR");
                WINDOW_SETTINGS->LABEL_AUDIOFILE->Caption = WINDOW_SETTINGS->getGuiString("STRING_AUDIOFILE");
                WINDOW_SETTINGS->LABEL_AUDIO_FROM->Caption = WINDOW_SETTINGS->getGuiString("STRING_FROM");
                WINDOW_SETTINGS->LABEL_AUDIO_TO->Caption = WINDOW_SETTINGS->getGuiString("STRING_TO");
                WINDOW_SETTINGS->LABEL_OFPCWC_EXECUTABLE->Caption = WINDOW_SETTINGS->getGuiString("STRING_OFPEXECUTABLE");
                WINDOW_SETTINGS->LABEL_OFPRES_EXECUTABLE->Caption = WINDOW_SETTINGS->getGuiString("STRING_OFPEXECUTABLE");
                WINDOW_SETTINGS->LABEL_ARMACWA_EXECUTABLE->Caption = WINDOW_SETTINGS->getGuiString("STRING_OFPEXECUTABLE");
                WINDOW_SETTINGS->LABEL_OFPCWC_PLAYERNAME->Caption = WINDOW_SETTINGS->getGuiString("STRING_PROFILE");
                WINDOW_SETTINGS->LABEL_OFPRES_PLAYERNAME->Caption = WINDOW_SETTINGS->getGuiString("STRING_PROFILE");
                WINDOW_SETTINGS->LABEL_ARMACWA_PLAYERNAME->Caption = WINDOW_SETTINGS->getGuiString("STRING_PROFILE");
        }
        updateGames();
}

/**
   Reads the programs config file
 */

list<String> readConfigFile() {
        String interval = "";
        String langfile = "";
        String notify = "0";
        bool gameSet = false;
        list<String> ipList;
        if(FileExists(programSettings.file)) {
                TStringList *file = new TStringList;
                file->LoadFromFile(programSettings.file);
                for(int i = 0; i < file->Count; i++) {
                        String tmp = file->Strings[i].Trim();
                        if(tmp.SubString(1,9) == "[General]") {
                                i++;
                                tmp = file->Strings[i].Trim();
                                while(tmp.SubString(1,10) != "[\\General]" && i < file->Count - 1) {
                                        if((tmp.SubString(1,8) == "Interval")) {
                                                interval = getValue(tmp);
                                        } else if((tmp.SubString(1,8) == "LangFile")) {
                                                langfile = getValue(tmp);
                                        } else if((tmp.SubString(1,19) == "customNotifications")) {
                                                notify = getValue(tmp);
                                        }
                                        i++;
                                        tmp = file->Strings[i].Trim();
                                }
                        } else if((tmp.SubString(1,6) == "[Game]")) {
                                i++;
                                tmp = file->Strings[i].Trim();
                                int gameid = -1;
                                String player = "", exe;

                                while(tmp.SubString(1,7) != "[\\Game]" && i < file->Count -1) {
                                        if(tmp.SubString(1,3) == "Exe") {
                                                exe = getValue(tmp);
                                        } else if(tmp.SubString(1,10) == "LastPlayer") {
                                                player = getValue(tmp);
                                        } else if(tmp.SubString(1,4) == "Name") {
                                                String n = getValue(tmp);
                                                gameid = getGameId(n);
                                        }
                                        i++;
                                        tmp = file->Strings[i].Trim();
                                }
                                if(gameid >= 0 && gameid < GAMES_TOTAL && !exe.IsEmpty()) {
                                        programSettings.setGame(gameid, exe, player);
                                        gameSet = true;
                                }
                        } else if((tmp.SubString(1,6) == "[Conf]")) {
                                Configuration c = Configuration();
                                c.set = true;
                                i++;
                                tmp = file->Strings[i].Trim();
                                int gameid = 1;
                                while((tmp.SubString(1,7) != "[\\Conf]") && i < file->Count - 1) {
                                        if((tmp.SubString(1,8) == "Password")) {
                                                c.password = getValue(tmp);
                                        } else if((tmp.SubString(1,4) == "Mods")) {
                                                c.mods = Form1->splitUpMessage(getValue(tmp), ";");
                                        } else if((tmp.SubString(1,10) == "Parameters")) {
                                                c.addParameters = Form1->splitUpMessage(getValue(tmp), " ");
                                        } else if((tmp.SubString(1,5) == "Label")) {
                                                c.label = getValue(tmp);
                                        } else if((tmp.SubString(1,4) == "Game")) {
                                                gameid = getGameId(getValue(tmp));
                                        }
                                        i++;
                                        tmp = file->Strings[i].Trim();
                                }
                                c.gameid = gameid;
                                programSettings.pSaddConf(gameid, c);
                        } else if((tmp.SubString(1,9) == "[Servers]")) {
                                i++;
                                tmp = file->Strings[i].Trim();
                                while((tmp.SubString(1,10) != "[\\Servers]") && i < file->Count - 1) {
                                        if(tmp.SubString(1,7) == "[Watch]") {
                                                i++;
                                                tmp = file->Strings[i].Trim();
                                                programSettings.watched->Clear();
                                                while(tmp.SubString(1,8) != "[\\Watch]" && i < file->Count -1 ) {
                                                        if(tmp.Length() > 8) {
                                                                programSettings.watched->Add(tmp.Trim());
                                                                ipList.push_back(tmp.Trim());
                                                        }
                                                        i++;
                                                        tmp = file->Strings[i].Trim();
                                                }
                                        } else if(tmp.Length() > 8) {
                                                ipList.push_back(tmp.Trim());
                                        }
                                        i++;
                                        tmp = file->Strings[i].Trim();
                                }
                        } else if((tmp.SubString(1,9) == "[Filters]")) {
                                i++;
                                tmp = file->Strings[i].Trim();
                                while((tmp.SubString(1,10) != "[\\Filters]") && i < file->Count - 1) {
                                        if((tmp.SubString(1,7) == "Playing")) {
                                                Form1->CHECKBOX_FILTER_PLAYING->Checked = checkBool2(getValue(tmp));
                                        } else if((tmp.SubString(1,7) == "Waiting")) {
                                                Form1->CHECKBOX_FILTER_WAITING->Checked = checkBool2(getValue(tmp));
                                        } else if((tmp.SubString(1,8) == "Creating")) {
                                                Form1->CHECKBOX_FILTER_CREATING->Checked = checkBool2(getValue(tmp));
                                        } else if((tmp.SubString(1,9) == "Settingup")) {
                                                Form1->CHECKBOX_FILTER_SETTINGUP->Checked = checkBool2(getValue(tmp));
                                        } else if((tmp.SubString(1,8) == "Briefing")) {
                                                Form1->CHECKBOX_FILTER_BRIEFING->Checked = checkBool2(getValue(tmp));
                                        } else if((tmp.SubString(1,10) == "Debriefing")) {
                                                Form1->CHECKBOX_FILTER_DEBRIEFING->Checked = checkBool2(getValue(tmp));
                                        } else if((tmp.SubString(1,6) == "WithPW")) {
                                                Form1->CHECKBOX_FILTER_WITHPASSWORD->Checked = checkBool2(getValue(tmp));
                                        } else if((tmp.SubString(1,9) == "WithoutPW")) {
                                                Form1->CHECKBOX_FILTER_WITHOUTPASSWORD->Checked = checkBool2(getValue(tmp));
                                        } else if((tmp.SubString(1,10) == "minPlayers")) {
                                                try {
                                                        int i = StrToInt(getValue(tmp));
                                                        Form1->UpDown1->Position = i;
                                                } catch (...) {}
                                        } else if((tmp.SubString(1,10) == "ServerName")) {
                                                Form1->Edit2->Text = getValue(tmp);
                                        } else if((tmp.SubString(1,11) == "MissionName")) {
                                                Form1->Edit1->Text = getValue(tmp);
                                        } else if((tmp.SubString(1,10) == "PlayerName")) {
                                                Form1->Edit4->Text = getValue(tmp);
                                        }
                                        i++;
                                        tmp = file->Strings[i].Trim();
                                }
                        } else if((tmp.SubString(1,14) == "[FontSettings]")) {
                                i++;
                                tmp = file->Strings[i].Trim();
                                int charset = 0;
                                String name = "";
                                int size = 0;
                                bool bold = false, italic = false;
                                while((tmp.SubString(1,15) != "[\\FontSettings]") && i < file->Count - 1) {
                                        if((tmp.SubString(1,4) == "Name")) {
                                                name = getValue(tmp);
                                        } else if((tmp.SubString(1,7) == "Charset")) {
                                                try {
                                                        charset = StrToInt(getValue(tmp));
                                                } catch (...) {}
                                        } else if((tmp.SubString(1,4) == "Size")) {
                                                try {
                                                        size = StrToInt(getValue(tmp));
                                                } catch (...) {}
                                        } else if((tmp.SubString(1,4) == "Bold")) {
                                                bold = checkBool2(getValue(tmp));
                                        } else if((tmp.SubString(1,6) == "Italic")) {
                                                italic = checkBool2(getValue(tmp));
                                        }
                                        i++;
                                        tmp = file->Strings[i].Trim();
                                }
                                Form1->setFont(name, size, charset,bold,italic);
                        } else if((tmp.SubString(1,14) == "[ChatSettings]")) {
                                i++;
                                tmp = file->Strings[i].Trim();
                                String host = "irc.freenode.net";
                                String channel = "operationflashpoint1";
                                int port = 6666;
                                String user = "";
                                bool autoConnect = false;
                                while((tmp.SubString(1,15) != "[\\ChatSettings]") && i < file->Count - 1) {
                                        if((tmp.SubString(1,11) == "AutoConnect")) {
                                                autoConnect = checkBool2(getValue(tmp));
                                        } else if((tmp.SubString(1,4) == "Host")) {
                                                host = getValue(tmp);
                                        } else if((tmp.SubString(1,4) == "Port")) {
                                                try {
                                                        port = StrToInt(getValue(tmp));
                                                } catch (...) {}
                                        } else if((tmp.SubString(1,7) == "Channel")) {
                                                channel = getValue(tmp);
                                        } else if((tmp.SubString(1,8) == "UserName")) {
                                                user = getValue(tmp);
                                        }
                                        i++;
                                        tmp = file->Strings[i].Trim();
                                }
                                Form1->setChat(host, port, channel, user, autoConnect);
                        } else if((tmp.SubString(1,16) == "[WindowSettings]")) {
                                i++;
                                tmp = file->Strings[i].Trim();
                                int     top = 0,
                                        left = 0,
                                        width = 0,
                                        height = 0,
                                        devider = 0;
                                float   ratioID = 0.0f,
                                        ratioSN = 0.0f,
                                        ratioPN = 0.0f,
                                        ratioST = 0.0f,
                                        ratioIS = 0.0f,
                                        ratioMN = 0.0f,
                                        ratioPI = 0.0f,
                                        ratioPL = 0.0f,
                                        ratioSC = 0.0f,
                                        ratioDE = 0.0f,
                                        ratioTE = 0.0f;
                                        
                                while((tmp.SubString(1,17) != "[\\WindowSettings]") && i < file->Count - 1) {
                                        if((tmp.SubString(1,4) == "Left")) {
                                                try {   left = StrToInt(getValue(tmp));  }catch(...) {}
                                        } else if((tmp.SubString(1,3) == "Top")) {
                                                try {   top = StrToInt(getValue(tmp));  }catch(...) {}
                                        } else if((tmp.SubString(1,5) == "Width")) {
                                                try {   width = StrToInt(getValue(tmp));  }catch(...) {}
                                        } else if((tmp.SubString(1,6) == "Height")) {
                                                try {   height = StrToInt(getValue(tmp));  }catch(...) {}
                                        } else if((tmp.SubString(1,7) == "ratioID")) {
                                                try {   ratioID = atof(getValue(tmp).c_str());  }catch(...) {}
                                        } else if((tmp.SubString(1,7) == "ratioSN")) {
                                                try {   ratioSN = atof(getValue(tmp).c_str());  }catch(...) {}
                                        } else if((tmp.SubString(1,7) == "ratioPN")) {
                                                try {   ratioPN = atof(getValue(tmp).c_str());  }catch(...) {}
                                        } else if((tmp.SubString(1,7) == "ratioST")) {
                                                try {   ratioST = atof(getValue(tmp).c_str());  }catch(...) {}
                                        } else if((tmp.SubString(1,7) == "ratioIS")) {
                                                try {   ratioIS = atof(getValue(tmp).c_str());  }catch(...) {}
                                        } else if(tmp.SubString(1,7) == "ratioMN") {
                                                try {   ratioMN = atof(getValue(tmp).c_str());  }catch(...) {}
                                        } else if(tmp.SubString(1,7) == "ratioPI") {
                                                try {   ratioPI = atof(getValue(tmp).c_str());  }catch(...) {}
                                        } else if(tmp.SubString(1,7) == "ratioPL") {
                                                try {   ratioPL = atof(getValue(tmp).c_str());  }catch(...) {}
                                        } else if(tmp.SubString(1,7) == "ratioSC") {
                                                try {   ratioSC = atof(getValue(tmp).c_str());  }catch(...) {}
                                        } else if(tmp.SubString(1,7) == "ratioDE") {
                                                try {   ratioDE = atof(getValue(tmp).c_str());  }catch(...) {}
                                        } else if(tmp.SubString(1,7) == "ratioTE") {
                                                try {   ratioTE = atof(getValue(tmp).c_str());  }catch(...) {}
                                        } else if(tmp.SubString(1,7) == "devider") {
                                                try {   devider = atof(getValue(tmp).c_str());  }catch(...) {}
                                        }
                                        i++;
                                        tmp = file->Strings[i].Trim();
                                }


                                Form1->setWindowSettings(top,left,height,width,ratioID,
                                        ratioSN,
                                        ratioPN,
                                        ratioST,
                                        ratioIS,
                                        ratioMN,
                                        ratioPI,
                                        ratioPL,
                                        ratioSC,
                                        ratioDE,
                                        ratioTE,
                                        devider);

                        } else if((tmp.SubString(1,20) == "[CustomNotification]")) {
                                i++;
                                tmp = file->Strings[i].Trim();
                                list<String> mission, server, player;
                                String name = "Unnamed", soundFile = "", color = clWindow;
                                int statusFilter = 0, volume = 100, minPlayers = -1, maxPlayers = -1, start = 0, end = -1, repeat = 0;
                                while((tmp.SubString(1,21) != "[\\CustomNotification]") && i < file->Count - 1) {
                                        if((tmp.SubString(1,4) == "name")) {
                                                name = getValue(tmp);
                                        } else if((tmp.SubString(1,13) == "missionFilter")) {
                                                mission = Form1->splitUpMessage(getValue(tmp),";");
                                        } else if((tmp.SubString(1,12) == "serverFilter")) {
                                                server = Form1->splitUpMessage(getValue(tmp),";");
                                        } else if((tmp.SubString(1,12) == "playerFilter")) {
                                                player = Form1->splitUpMessage(getValue(tmp),";");
                                        } else if((tmp.SubString(1,12) == "statusFilter")) {
                                                try {   statusFilter = StrToInt(getValue(tmp));  }catch(...) {}
                                        } else if((tmp.SubString(1,9) == "soundFile")) {
                                                soundFile = getValue(tmp);
                                        } else if((tmp.SubString(1,14) == "playbackVolume")) {
                                                try { volume = StrToInt(getValue(tmp)); } catch (...) {}
                                        } else if((tmp.SubString(1,13) == "playbackStart")) {
                                                try { start = StrToInt(getValue(tmp)); } catch (...) {}
                                        } else if((tmp.SubString(1,11) == "playbackEnd")) {
                                                try { end = StrToInt(getValue(tmp)); } catch (...) {}
                                        } else if((tmp.SubString(1,12) == "markingColor")) {
                                                color = getValue(tmp);
                                        } else if((tmp.SubString(1,14) == "minimumPlayers")) {
                                                try { minPlayers = StrToInt(getValue(tmp)); } catch (...) {}
                                        } else if((tmp.SubString(1,14) == "maximumPlayers")) {
                                                try { maxPlayers = StrToInt(getValue(tmp)); } catch (...) {}
                                        } else if((tmp.SubString(1,6) == "repeat")) {
                                                try { repeat = StrToInt(getValue(tmp)); } catch (...) {}
                                        }
                                        i++;
                                        tmp = file->Strings[i].Trim();
                                }
                                WINDOW_SETTINGS->addCustomNotification(name, statusFilter, mission, server, player,
                                        minPlayers, maxPlayers, soundFile, volume, start, end, color, repeat);                                  

                        }
                }
                delete file;
        }
        if(!gameSet) {
                for(int i = 0; i < GAMES_TOTAL; i++) {
                        programSettings.games[i].autodetect("");
                }                                              
        }
        if(!langfile.IsEmpty()) {
                if(FileExists(GetCurrentDir() + "\\" + langfile)) {
                        programSettings.languagefile = langfile;
                }
        }
        try {
                int a = StrToInt(interval);
                programSettings.setInterval(a);
                WINDOW_SETTINGS->UPDOWN_SERVERLIST_UPDATE->Position = a;
        }catch (...) {
                programSettings.setInterval(3);
                WINDOW_SETTINGS->UPDOWN_SERVERLIST_UPDATE->Position = 3;
        }
        programSettings.customNotifications = checkBool2(notify);
        WINDOW_SETTINGS->CHECKBOX_NOTIFICATIONS_ACTIVE->Checked = programSettings.customNotifications;
        WINDOW_SETTINGS->EDIT_SERVERLIST_UPDATE->Text = String(WINDOW_SETTINGS->UPDOWN_SERVERLIST_UPDATE->Position);
        programSettings.changed = false;
        return ipList;
}

/**
   Searches for languages files
 */

void findLanguageFiles() {
                WINDOW_SETTINGS->ComboBox1->Clear();
               	TSearchRec daten;
                if(0 == FindFirst("OFPM*.lang", faAnyFile, daten)) {
                        try {
                                do {
                                        WINDOW_SETTINGS->ComboBox1->Items->Add(daten.Name);
                                } while(FindNext(daten) == 0);
                        }__finally
                        {
                                FindClose(daten);
                        }
                }
                return;
}

void checkConfListState() {
        bool itemSelected = false;
        for(int i = 0; i < WINDOW_SETTINGS->LISTBOX_CONFIGURATIONS->Count; i++) {
                if(WINDOW_SETTINGS->LISTBOX_CONFIGURATIONS->Selected[i]) {
                        itemSelected = true;
                        break;
                }
        }
        bool limitReached = WINDOW_SETTINGS->LISTBOX_CONFIGURATIONS->Items->Count >= confAmount;
        WINDOW_SETTINGS->BUTTON_NEWCONFIGURATION_ADD->Enabled = !limitReached;
        WINDOW_SETTINGS->BUTTON_EDITCONFIGURATION_EDIT->Enabled = itemSelected;
        WINDOW_SETTINGS->BUTTON_CONFIGURATION_REMOVE->Enabled = itemSelected;
        WINDOW_SETTINGS->BUTTON_EDITCONFIGURATION_COPY->Enabled = itemSelected && !limitReached;
        WINDOW_SETTINGS->BUTTON_EDITCONFIGURATION_UP->Enabled = itemSelected;
        WINDOW_SETTINGS->BUTTON_EDITCONFIGURATION_DOWN->Enabled = itemSelected;
}

void exitEditMode() {
        WINDOW_SETTINGS->LISTBOX_CONFIGURATIONS->Enabled = true;
        WINDOW_SETTINGS->BUTTON_EDITCONFIGURATION_OK->Enabled = false;
        WINDOW_SETTINGS->BUTTON_EDITCONFIGURATION_CANCEL->Enabled = false;
        WINDOW_SETTINGS->BUTTON_EDITCONFIGURATION_OK->Visible = false;
        WINDOW_SETTINGS->BUTTON_EDITCONFIGURATION_CANCEL->Visible = false;
        WINDOW_SETTINGS->BUTTON_NEWCONFIGURATION_ADD->Visible = true;
        WINDOW_SETTINGS->BUTTON_NEWCONFIGURATION_ADD->Enabled = true;
        WINDOW_SETTINGS->BUTTON_NEWCONFIGURATION_CLEAR->Visible = true;
        WINDOW_SETTINGS->BUTTON_NEWCONFIGURATION_CLEAR->Enabled = true;
        WINDOW_SETTINGS->BUTTON_NEWCONFIGURATION_CLEAR->Click();
        checkConfListState();
}

String TWINDOW_SETTINGS::getPlayerName(int actVer, int reqVer) {
        for(int i = 0; i < GAMES_TOTAL; i++) {
                if(programSettings.games[i].set) {
                        if(programSettings.games[i].checkIfCorrectGame(actVer, reqVer)) {
                                return programSettings.games[i].player;
                        }
                }
        }
        return "";
}

void checkForAutoDetection(int gameid) {
        if(!programSettings.games[gameid].isValid()) {
                programSettings.games[gameid].autodetect("");
        } else {
                programSettings.games[gameid].set = true;
        }
}

TStringList* TWINDOW_SETTINGS::getGameSpyGames() {
        TStringList *out = new TStringList();
        out->Sorted = true;
        out->CaseSensitive = true;
        out->Duplicates = dupIgnore;
        for(int i = 0; i < GAMES_TOTAL; i++) {
                if(programSettings.games[i].set && programSettings.games[i].isValid()) {
                        out->Add(programSettings.games[i].gamespyToken);
                }
        }
        return out;
}

class CustomNotification {
        public:
                String name;
                bool statusCreating;
                bool statusWaiting;
                bool statusSettingUp;
                bool statusBriefing;
                bool statusPlaying;
                bool statusDebriefing;

                bool withPassword;
                bool withoutPassword;

                int minPlayers;
                int maxPlayers;
                list<String> serverFilter;
                list<String> missionFilter;
                list<String> playerFilter;

                String soundFile;
                int playbackVolume;
                int playbackStart;
                int playbackEnd;
                TColor markingColor;
                int repeat;
                bool set;

                CustomNotification () {
                        this->set = false;
                        this->statusCreating = false;
                }

                CustomNotification (String name, int filters, list<String> &mission,
                                    list<String> &server, list<String> &player, int minPlayers,
                                    int maxPlayers, String soundFile, int volume, int start, int end,
                                    String color, int repeat) {
                        this->name = name;
                        this->withoutPassword = ((filters & 1) == 1);
                        filters = filters >> 1;
                        this->withPassword = ((filters & 1) == 1);
                        filters = filters >> 1;
                        this->statusDebriefing = ((filters & 1) == 1);
                        filters = filters >> 1;
                        this->statusPlaying = ((filters & 1) == 1);
                        filters = filters >> 1;
                        this->statusBriefing = ((filters & 1) == 1);
                        filters = filters >> 1;
                        this->statusSettingUp = ((filters & 1) == 1);
                        filters = filters >> 1;
                        this->statusWaiting = ((filters & 1) == 1);
                        filters = filters >> 1;
                        this->statusCreating = ((filters & 1) == 1);
                        this->missionFilter = mission;
                        this->serverFilter = server;
                        this->playerFilter = player;
                        this->soundFile = soundFile;
                        this->minPlayers = minPlayers;
                        this->maxPlayers = maxPlayers;
                        this->playbackVolume = volume;
                        this->playbackStart = start;
                        this->playbackEnd = end;
                        this->markingColor = StringToColor(color);
                        this->repeat = repeat;
                        this->set = true;
                }

                CustomNotification (String name, bool statusC, bool statusW,
                        bool statusS, bool statusB, bool statusP, bool statusD,
                        bool statusWithPW, bool statusWithoutPW,
                        list<String> &mission, list<String> &server,
                        list<String> &player, int minPlayers, int maxPlayers,
                        String soundFile, int volume, int start, int end, String color, int repeat) {
                        this->name = name;
                        this->withoutPassword = statusWithoutPW;
                        this->withPassword = statusWithPW;
                        this->statusDebriefing = statusD;
                        this->statusPlaying = statusP;
                        this->statusBriefing = statusB;
                        this->statusSettingUp = statusS;
                        this->statusWaiting = statusW;
                        this->statusCreating = statusC;
                        this->missionFilter = mission;
                        this->serverFilter = server;
                        this->playerFilter = player;
                        this->soundFile = soundFile;
                        this->minPlayers = minPlayers;
                        this->maxPlayers = maxPlayers;
                        this->playbackVolume = volume;
                        this->playbackStart = start;
                        this->playbackEnd = end;
                        this->markingColor = StringToColor(color);
                        this->repeat = repeat;
                        this->set = true;
                }

                TStringList* createFileEntry() {
                        TStringList *output = new TStringList;
                        int filters = 0;
                        if(this->statusCreating) { filters = filters + 1; }
                        filters = filters << 1;
                        if(this->statusWaiting) { filters = filters + 1; }
                        filters = filters << 1;
                        if(this->statusSettingUp) { filters = filters + 1; }
                        filters = filters << 1;
                        if(this->statusBriefing) { filters = filters + 1; }
                        filters = filters << 1;
                        if(this->statusPlaying) { filters = filters + 1; }
                        filters = filters << 1;
                        if(this->statusDebriefing) { filters = filters + 1; }
                        filters = filters << 1;
                        if(this->withPassword) { filters = filters + 1; }
                        filters = filters << 1;
                        if(this->withoutPassword) { filters = filters + 1; }
                        output->Add("[CustomNotification]");
                        output->Add("name = " + this->name);
                        output->Add("statusFilter = " + String(filters));

                        String mission = "";
                        if(this->missionFilter.size() > 0) {
                                do {
                                        mission += this->missionFilter.front();
                                        this->missionFilter.pop_front();
                                        if(this->missionFilter.size() > 0) {
                                                mission += ";";
                                        }
                                } while (this->missionFilter.size() > 0);
                        }
                        String server = "";
                        if(this->serverFilter.size() > 0) {
                                do {
                                        server += this->serverFilter.front();
                                        this->serverFilter.pop_front();
                                        if(this->serverFilter.size() > 0) {
                                                server += ";";
                                        }
                                } while (this->serverFilter.size() > 0);
                        }
                        String player = "";
                        if(this->playerFilter.size() > 0) {
                                do {
                                        player += this->playerFilter.front();
                                        this->playerFilter.pop_front();
                                        if(this->playerFilter.size() > 0) {
                                                player += ";";
                                        }
                                } while (this->playerFilter.size() > 0);
                        }
                        output->Add("missionFilter = " + mission);
                        output->Add("serverFilter = " + server);
                        output->Add("playerFilter = " + player);
                        if(this->minPlayers >= 0) {
                                output->Add("minimumPlayers = " + String(this->minPlayers));
                        }
                        if(this->maxPlayers >= 0) {
                                output->Add("maximumPlayers = " + String(this->maxPlayers));
                        }
                        output->Add("markingColor = " + ColorToString(this->markingColor));
                        output->Add("soundFile = " + this->soundFile);
                        output->Add("playbackVolume = " + String(this->playbackVolume));
                        output->Add("playbackStart = " + String(this->playbackStart));
                        output->Add("playbackEnd = " + String(this->playbackEnd));
                        output->Add("repeat = " + IntToStr(this->repeat));
                        output->Add("[\\CustomNotification]");

                        return output;
                }
};

CustomNotification CustomNotify[32];

TStringList* TWINDOW_SETTINGS::getNotificationsFileEntries() {
        TStringList *entry = new TStringList;

        for(int i = 0; i < GetArrLength(CustomNotify); i++) {
                if(CustomNotify[i].set) {
                        TStringList *part = CustomNotify[i].createFileEntry();
                        while(part->Count > 0) {
                                entry->Add(part->Strings[0]);
                                part->Delete(0);
                        }
                        delete part;
                }
        }
        
        return entry;
}

class SongPosition {
     public:
        int milli;
        int sec;
        int min;
        SongPosition() {}
	void SongPosition::setMilli(int milli) {
		this->milli = milli;
	}
	void SongPosition::setSec(int sec) {
		this->sec = sec;
	}
	void SongPosition::setMin(int min) {
		this->min = min;
	}
};

SongPosition calcSongPosition(int position) {
        SongPosition out = SongPosition();
        out.setMilli(position % 1000);
        out.setSec(((position - out.milli)/1000) % 60);
        out.setMin((((position - out.milli)/1000) - out.sec) / 60);
        return out;
}

void printPlaybackRange(int start, int end) {
        SongPosition s = calcSongPosition(start);
        SongPosition e = calcSongPosition(end);
        WINDOW_SETTINGS->EDIT_SONGSTART_MIN->Text = IntToStr(s.min);
        WINDOW_SETTINGS->EDIT_SONGSTART_SEC->Text = IntToStr(s.sec);
        WINDOW_SETTINGS->EDIT_SONGSTART_MILL->Text = IntToStr(s.milli);
        WINDOW_SETTINGS->EDIT_SONGEND_MIN->Text = IntToStr(e.min);
        WINDOW_SETTINGS->EDIT_SONGEND_SEC->Text = IntToStr(e.sec);
        WINDOW_SETTINGS->EDIT_SONGEND_MILL->Text = IntToStr(e.milli);
}




void TWINDOW_SETTINGS::addCustomNotification(String name, int filters, list<String> &mission,
                                    list<String> &server, list<String> &player, int minPlayers,
                                    int maxPlayers, String soundFile, int volume, int start, int end,
                                    String color, int repeat) {
        for(int i = 0; i < GetArrLength(CustomNotify); i++) {
                if(!CustomNotify[i].set) {
                        CustomNotify[i] = CustomNotification(name, filters,
                                    mission, server, player,
                                    minPlayers, maxPlayers,
                                    soundFile, volume, start, end, color, repeat);
                        break;
                }
        }
}

int TWINDOW_SETTINGS::checkNotifications(String servername, int players, int status,
                                String missionname, bool passworded,
                                list<String> playerlist) {
        for(int i = 0; i < GetArrLength(CustomNotify); i++) {
            if(CustomNotify[i].set) {
                if(passworded && CustomNotify[i].withPassword ||
                        !passworded && CustomNotify[i].withoutPassword) {
                        if(     (CustomNotify[i].minPlayers <= players ||
                                 CustomNotify[i].minPlayers < 0) &&
                                (CustomNotify[i].maxPlayers >= players ||
                                 CustomNotify[i].maxPlayers < 0)) {
                                if(     CustomNotify[i].statusCreating && status == SERVERSTATE_CREATING ||
                                        CustomNotify[i].statusWaiting && status == SERVERSTATE_WAITING ||
                                        CustomNotify[i].statusBriefing && status == SERVERSTATE_BRIEFING ||
                                        CustomNotify[i].statusSettingUp && status == SERVERSTATE_SETTINGUP ||
                                        CustomNotify[i].statusPlaying && status == SERVERSTATE_PLAYING ||
                                        CustomNotify[i].statusDebriefing && status == SERVERSTATE_DEBRIEFING) {
                                        bool servertest = false;
                                        bool missiontest = false;
                                        bool playertest = false;
                                        if(CustomNotify[i].serverFilter.size() > 0) {
                                                for (list<String>::iterator ci = CustomNotify[i].serverFilter.begin(); ci != CustomNotify[i].serverFilter.end(); ++ci) {
                                                        if(Form1->doNameFilter(servername,*ci)) {
                                                                servertest = true;
                                                                break;
                                                        }
                                                }
                                        } else { servertest = true; }
                                        if(CustomNotify[i].missionFilter.size() > 0) {
                                                for (list<String>::iterator ci = CustomNotify[i].missionFilter.begin(); ci != CustomNotify[i].missionFilter.end(); ++ci) {
                                                        if(Form1->doNameFilter(missionname,*ci)) {
                                                                missiontest = true;
                                                                break;
                                                        }
                                                }
                                        } else { missiontest = true; }
                                        if(CustomNotify[i].playerFilter.size() > 0) {
                                                for (list<String>::iterator ci = playerlist.begin(); ci != playerlist.end(); ++ci) {
                                                        for (list<String>::iterator di = CustomNotify[i].playerFilter.begin(); di != CustomNotify[i].playerFilter.end(); ++di) {
                                                                if(Form1->doNameFilter(*ci,*di)) {
                                                                        playertest = true;
                                                                        break;
                                                                }
                                                        }
                                                }
                                        } else { playertest = true; }
                                        if(servertest && missiontest && playertest) {
                                                return i;
                                        }
                                }
                        }
                }
            }
        }
        return -1;
}


void updateNotificationsList() {
        WINDOW_SETTINGS->LISTBOX_NOTIFICATIONS->Clear();
        for(int i = 0; i < GetArrLength(CustomNotify); i++) {
                if(CustomNotify[i].set) {
                        WINDOW_SETTINGS->LISTBOX_NOTIFICATIONS->Items->AddObject(CustomNotify[i].name, (TObject *) i);
                }
        }
}
 

void checkNotificationListState() {
        WINDOW_SETTINGS->BUTTON_EDITNOTIFICATION_EDIT->Enabled = false;
        WINDOW_SETTINGS->BUTTON_NOTIFICATION_REMOVE->Enabled = false;
        for(int i = 0; i < WINDOW_SETTINGS->LISTBOX_NOTIFICATIONS->Count; i++) {
                if(WINDOW_SETTINGS->LISTBOX_NOTIFICATIONS->Selected[i]) {
                        WINDOW_SETTINGS->BUTTON_EDITNOTIFICATION_EDIT->Enabled = true;
                        WINDOW_SETTINGS->BUTTON_NOTIFICATION_REMOVE->Enabled = true;
                        break;
                }
        }
}

void updateChatSettings() {
        WINDOW_SETTINGS->COMBOBOX_CHAT_USERNAME->Clear();
        for(int i = 0; i < GAMES_TOTAL; i++) {
                if(programSettings.games[i].set) {
                        list<String> profiles = findPlayerProfiles(programSettings.games[i].folder);
                        for (list<String>::iterator ci = profiles.begin(); ci != profiles.end(); ++ci) {
                                if(WINDOW_SETTINGS->COMBOBOX_CHAT_USERNAME->Items->IndexOf(*ci) == -1) {
                                        WINDOW_SETTINGS->COMBOBOX_CHAT_USERNAME->Items->Add(*ci);
                                }
                        }
                }
        }
        String user = Form1->getChatUserName();
        if(!user.IsEmpty()) {
                WINDOW_SETTINGS->COMBOBOX_CHAT_USERNAME->Items->Add(user);
        }
        WINDOW_SETTINGS->COMBOBOX_CHAT_USERNAME->Text = Form1->getChatUserName();
        WINDOW_SETTINGS->EDIT_CHAT_IRCSERVER_ADDRESS->Text = Form1->getChatHost();
        WINDOW_SETTINGS->EDIT_CHAT_IRCSERVER_PORT->Text = Form1->getChatPort();
        WINDOW_SETTINGS->EDIT_CHAT_IRCSERVER_CHANNEL->Text = Form1->getChatChannel();
        WINDOW_SETTINGS->CHECKBOX_CHAT_AUTOCONNECT->Checked = Form1->getChatAutoConnect();
}

void exitEditNotificationMode() {
        WINDOW_SETTINGS->LISTBOX_NOTIFICATIONS->Enabled = true;
        WINDOW_SETTINGS->BUTTON_EDITNOTIFICATION_OK->Enabled = false;
        WINDOW_SETTINGS->BUTTON_EDITNOTIFICATION_CANCEL->Enabled = false;
        WINDOW_SETTINGS->BUTTON_EDITNOTIFICATION_OK->Visible = false;
        WINDOW_SETTINGS->BUTTON_EDITNOTIFICATION_CANCEL->Visible = false;
        WINDOW_SETTINGS->BUTTON_NEWNOTIFICATION_ADD->Visible = true;
        WINDOW_SETTINGS->BUTTON_NEWNOTIFICATION_ADD->Enabled = true;
        WINDOW_SETTINGS->BUTTON_NEWNOTIFICATION_CLEAR->Visible = true;
        WINDOW_SETTINGS->BUTTON_NEWNOTIFICATION_CLEAR->Enabled = true;
        WINDOW_SETTINGS->BUTTON_NEWNOTIFICATION_CLEAR->Click();
        checkNotificationListState();
}

      
void TWINDOW_SETTINGS::updateFontSettings(int charset) {
        WINDOW_SETTINGS->Font->Charset = charset;
}

class MP3Job {
    public:
        int notificationIndex;
        bool set;
        bool error;
        bool started;
        bool stopped;
        bool repeat;
        String file;
        String alias;
        int volume;
        int start;
        int end;

        MP3Job() {
                this->notificationIndex = -1;
                this->set = false;
        }

        MP3Job(int index) {
                this->notificationIndex = index;
                this->error = false;
                if(this->notificationIndex >= 0 && this->notificationIndex <= GetArrLength(CustomNotify)) {
                        if(CustomNotify[this->notificationIndex].set) {
                                this->file = CustomNotify[index].soundFile;
                                this->alias = "OFPM_MP3_" + IntToStr(this->notificationIndex);
                                this->volume = CustomNotify[index].playbackVolume;
                                this->start = CustomNotify[index].playbackStart;
                                this->end = CustomNotify[index].playbackEnd;
                                this->repeat = ((CustomNotify[index].repeat == 1) ? true : false);
                        }
                }
                this->started = false;
                this->stopped = false;
                this->set = true;
        }

        void play() {
                if(FileExists(this->file)) {
                        if(0 != mciSendString(("Open \"" + this->file + "\" alias " + this->alias).c_str(),0,0,0)) {
                                this->error = true;
                        }
                        if(0 != mciSendString(("play " + this->alias + " from " + String(this->start) + " to " + String(this->end)).c_str(), 0, 0, 0)) {
                                this->error = true;
                        }
                        if(0 != mciSendString(("setaudio " + this->alias + " volume to " + String(this->volume)).c_str(), 0, 0, 0)) {
                                this->error = true;
                        }
                        this->started = true;
                }
        }

        bool hasEnded() {
                if(FileExists(this->file)) {
                        char text[128];
                        mciSendString(("status " + this->alias + " position").c_str(), text, 128, 0);
                        int pos = 0;
                        try {
                                pos = StrToInt(String(text));
                        } catch (...) {
                        }
                        return (pos >= this->end || this->error || this->stopped);
                } else {
                        return (this->error || this->stopped);
                }
        }

        void stop() {
                if(FileExists(this->file)) {
                        mciSendString(("Close " + this->alias).c_str(),0,0,0);
                }
                this->stopped = true;
        }
};

/**
   A mp3 player
 */

class MP3Player {
    public:
        MP3Job jobs[5];
        MP3Job preview;
        int limit;

        MP3Player() {
                this->limit = 5;
        }

        /**
           Creates a new MP3Job for a certain notifiation @index
           If one already exists, nothing is done
         */

        void MP3add (int index) {
                for(int i = 0; i < this->limit; i++) {
                        if(this->jobs[i].notificationIndex == index) {
                                return;
                        }
                }
                for(int i = 0; i < this->limit; i++) {
                        if(!this->jobs[i].set) {
                                this->jobs[i] = MP3Job(index);
                                this->jobs[i].play();
                                break;
                        }
                }
        }

        void MP3startPreview() {
                MP3stopPreview();
                MP3Job p = MP3Job(-1);
                p.alias = "OFPM_MP3PREVIEW";
                p.file = WINDOW_SETTINGS->EDIT_NOTIFICATION_FILE->Text;
                p.volume = WINDOW_SETTINGS->TrackBar1->Position*10;
                p.start = StrToInt(WINDOW_SETTINGS->EDIT_SONGSTART_MIN->Text)*60000 +
                        StrToInt(WINDOW_SETTINGS->EDIT_SONGSTART_SEC->Text)*1000 +
                        StrToInt(WINDOW_SETTINGS->EDIT_SONGSTART_MILL->Text);
                p.end = StrToInt(WINDOW_SETTINGS->EDIT_SONGEND_MIN->Text)*60000 +
                        StrToInt(WINDOW_SETTINGS->EDIT_SONGEND_SEC->Text)*1000 +
                        StrToInt(WINDOW_SETTINGS->EDIT_SONGEND_MILL->Text);
                this->preview = p;
                this->preview.play();
                WINDOW_SETTINGS->MP3Timer->Enabled = true;
        }

        void MP3stopPreview() {
                if(this->preview.set) {
                        if(this->preview.started) {
                                this->preview.stop();
                                this->preview = MP3Job();
                        }
                }
        }

        /**
           Stops and removes all MP3Jobs
         */

        void reset() {
                for(int i = 0; i < this->limit; i++) {
                        if(this->jobs[i].set) {
                                if(this->jobs[i].started) {
                                        this->jobs[i].stop();
                                }
                                this->jobs[i] = MP3Job();
                        }
                }           
        }

        /**
           Checks the status of all active MP3Jobs and removes them when ended.
           Or repeats them, when the user has set it.
         */

        bool check() {
                bool hasJob = false;
                for(int i = 0; i < this->limit; i++) {
                        if(this->jobs[i].set) {
                                hasJob = true;
                                if(jobs[i].hasEnded()) {
                                        jobs[i].stop();
                                        jobs[i] = MP3Job();
                                        if(jobs[i].repeat) {
                                                Form1->resetNotifications(jobs[i].notificationIndex);
                                        }
                                }
                        }
                }
                if(this->preview.set) {
                        hasJob = true;
                        if(this->preview.hasEnded()) {
                                WINDOW_SETTINGS->STOP->Click();
                        } else {
                                char text[128];
                                mciSendString("status OFPM_MP3PREVIEW position", text, 128,0);
                                String out = "";
                                int minutes = StrToInt(text);
                                int milliseconds = minutes % 1000;
                                WINDOW_SETTINGS->LabelMilli->Caption = IntToStr(milliseconds);
                                minutes = (minutes - milliseconds)/1000;
                                int seconds = minutes % 60;
                                if(seconds < 10) {
                                        WINDOW_SETTINGS->LabelSeconds->Caption = "0" + IntToStr(seconds) + ":";
                                } else {
                                        WINDOW_SETTINGS->LabelSeconds->Caption = IntToStr(seconds) + ":";
                                }
                                minutes = (minutes - seconds) / 60;
                                if(minutes < 10) {
                                        WINDOW_SETTINGS->LabelMinutes->Caption = "0" + IntToStr(minutes)+ ":";
                                } else {
                                        WINDOW_SETTINGS->LabelMinutes->Caption = IntToStr(minutes)+ ":";
                                }
                        }   
                }
                return hasJob;
        }

        /**
           Removes the MP3Job that was created for a certain notification @index
         */

        void MP3remove(int index) {
                for(int i = 0; i < this->limit; i++) {
                        if(jobs[i].set) {
                                if(jobs[i].notificationIndex == index) {
                                        if(!Form1->isNotificationRuleActive(index)) {
                                                jobs[i].stop();
                                                jobs[i] = MP3Job();
                                        }
                                }
                        }
                }
        }
};

MP3Player mp3p = MP3Player();

TColor TWINDOW_SETTINGS::getMarkingColor(int index) {
        if(index >= 0 && index < GetArrLength(CustomNotify)) {
                if(CustomNotify[index].set) {
                        return CustomNotify[index].markingColor;
                }
        }
        return NULL;
}

void TWINDOW_SETTINGS::MP3remove(int index) {
        mp3p.MP3remove(index);
}
void TWINDOW_SETTINGS::MP3add(int index) {
        mp3p.MP3add(index);
        MP3Timer->Enabled = true;
}

void TWINDOW_SETTINGS::MP3shutdown() {
        mp3p.reset();
        Form1->resetNotifications(-1);
}

void profileChanged(TComboBox *box, int gameid) {
        if(gameid >= 0 && gameid < GAMES_TOTAL) {
                if(box->Items->IndexOf(box->Text) == -1) {
                        box->ItemIndex = box->Items->IndexOf(programSettings.games[gameid].player);
                } else {
                        programSettings.games[gameid].player = box->Text;
                }
        }
}

//---------------------------------------------------------------------------
__fastcall TWINDOW_SETTINGS::TWINDOW_SETTINGS(TComponent* Owner)
        : TForm(Owner)
{
}
//---------------------------------------------------------------------------

void __fastcall TWINDOW_SETTINGS::FormCreate(TObject *Sender)
{

        #include "guiDB.cpp"
        findLanguageFiles();
        list<String> ipList = readConfigFile();
        updateLanguage(programSettings.languagefile);
        Form1->readServerList(ipList);
        Form1->Timer3->Enabled = true;
        updateConfList();
        updateGames();
        WINDOW_SETTINGS->ComboBox1->ItemIndex = WINDOW_SETTINGS->ComboBox1->Items->IndexOf(programSettings.languagefile);
        if(Form1->getChatAutoConnect()) {
                Form1->MENUITEM_MAINMENU_CHAT_CONNECT->Click();
        }
}
//---------------------------------------------------------------------------

void __fastcall TWINDOW_SETTINGS::OpenDialog1CanClose(TObject *Sender,
      bool &CanClose)
{
        if(CanClose) {
                String exe = OpenDialog1->FileName;
                String folder = getFolder(exe);
                programSettings.setGame(OpenDialog1->Tag, exe, "");
                updateGames();
                OpenDialog1->InitialDir = "";
                OpenDialog1->Tag = 0;
        }
}
//---------------------------------------------------------------------------

void __fastcall TWINDOW_SETTINGS::BUTTON_NEWCONFIGURATION_MOVERIGHTClick(TObject *Sender)
{
        for(int i = 0; i < LISTBOX_MODFOLDERS_ALL->Count; i++) {
                if(LISTBOX_MODFOLDERS_ALL->Selected[i]) {
                        LISTBOX_MODFOLDERS_SELECTED->Items->Add(LISTBOX_MODFOLDERS_ALL->Items->Strings[i]);
                        LISTBOX_MODFOLDERS_ALL->Items->Delete(i);
                        i--;
                }
        }
}
//---------------------------------------------------------------------------

void __fastcall TWINDOW_SETTINGS::BUTTON_NEWCONFIGURATION_MOVELEFTClick(TObject *Sender)
{
        for(int i = 0; i < LISTBOX_MODFOLDERS_SELECTED->Count; i++) {
                if(LISTBOX_MODFOLDERS_SELECTED->Selected[i]) {
                        LISTBOX_MODFOLDERS_ALL->Items->Add(LISTBOX_MODFOLDERS_SELECTED->Items->Strings[i]);
                        LISTBOX_MODFOLDERS_SELECTED->Items->Delete(i);
                }
        }
}
//---------------------------------------------------------------------------

void __fastcall TWINDOW_SETTINGS::BUTTON_NEWCONFIGURATION_UPClick(TObject *Sender)
{
        for(int i = 0; i < LISTBOX_MODFOLDERS_SELECTED->Count; i++) {
                if(LISTBOX_MODFOLDERS_SELECTED->Selected[i]) {
                        if(i > 0) {
                                LISTBOX_MODFOLDERS_SELECTED->Items->Exchange(i, i - 1);
                                break;
                        }
                }
        }
}
//---------------------------------------------------------------------------

void __fastcall TWINDOW_SETTINGS::BUTTON_NEWCONFIGURATION_DOWNClick(TObject *Sender)
{
        for(int i = 0; i < LISTBOX_MODFOLDERS_SELECTED->Count; i++) {
                if(LISTBOX_MODFOLDERS_SELECTED->Selected[i]) {
                        if(i < LISTBOX_MODFOLDERS_SELECTED->Count - 1) {
                                LISTBOX_MODFOLDERS_SELECTED->Items->Exchange(i, i + 1);
                                break;
                        }
                }
        }
}
//---------------------------------------------------------------------------

void __fastcall TWINDOW_SETTINGS::BUTTON_CONFIGURATION_REMOVEClick(TObject *Sender)
{
        int comboindex = ComboBox2->Items->IndexOf(ComboBox2->Text);
        int gameid = (int) ComboBox2->Items->Objects[comboindex];

        for(int i = 0; i < LISTBOX_CONFIGURATIONS->Count; i++) {
                if(LISTBOX_CONFIGURATIONS->Selected[i]) {
                        TObject *t = LISTBOX_CONFIGURATIONS->Items->Objects[i];
                        int j = (int) t;
                        programSettings.pSdeleteConf(gameid, j);
                        LISTBOX_CONFIGURATIONS->Items->Delete(i);
                }
        }
        updateConfList();
        checkConfListState();
}
//---------------------------------------------------------------------------
        
void __fastcall TWINDOW_SETTINGS::BUTTON_NEWCONFIGURATION_ADDClick(TObject *Sender)
{
                int comboindex = ComboBox2->Items->IndexOf(ComboBox2->Text);
                int gameid = (int) ComboBox2->Items->Objects[comboindex];
                list<String> a;
                for(int i = 0; i < LISTBOX_MODFOLDERS_SELECTED->Count; i++) {
                        a.push_back(LISTBOX_MODFOLDERS_SELECTED->Items->Strings[i]);
                }
                Configuration newC = Configuration(gameid, EDIT_NEWCONFIGURATION_LABEL->Text, a, EDIT_NEWCONFIGURATION_PASSWORD->Text, EDIT_NEWCONFIGURATION_PARAMETERS->Text, CHECKBOX_NEWCONFIGURATION_NOSPLASH->Checked, CHECKBOX_NEWCONFIGURATION_NOMAP->Checked);
                programSettings.pSaddConf(gameid, newC);
                updateConfList();
                checkConfListState();
}
//---------------------------------------------------------------------------
          
void __fastcall TWINDOW_SETTINGS::BUTTON_NEWCONFIGURATION_CLEARClick(TObject *Sender)
{
        while(LISTBOX_MODFOLDERS_SELECTED->Count > 0) {
                LISTBOX_MODFOLDERS_ALL->Items->Add(LISTBOX_MODFOLDERS_SELECTED->Items->Strings[0]);
                LISTBOX_MODFOLDERS_SELECTED->Items->Delete(0);
        }
        EDIT_NEWCONFIGURATION_PASSWORD->Text = "";
        EDIT_NEWCONFIGURATION_PARAMETERS->Text = "";
        EDIT_NEWCONFIGURATION_LABEL->Text = "";
        CHECKBOX_NEWCONFIGURATION_NOSPLASH->Checked = true;
        CHECKBOX_NEWCONFIGURATION_NOMAP->Checked = true;
}
//---------------------------------------------------------------------------

void __fastcall TWINDOW_SETTINGS::FormClose(TObject *Sender, TCloseAction &Action)
{
        WINDOW_SETTINGS->setSettingsChanged();
        if(STOP->Visible) {
                STOP->Click();
        }
        try {
                Form1->setChat(EDIT_CHAT_IRCSERVER_ADDRESS->Text,
                StrToInt(EDIT_CHAT_IRCSERVER_PORT->Text),
                EDIT_CHAT_IRCSERVER_CHANNEL->Text,
                COMBOBOX_CHAT_USERNAME->Text,
                CHECKBOX_CHAT_AUTOCONNECT->Checked);
        } catch (...) {}
}
//---------------------------------------------------------------------------

void __fastcall TWINDOW_SETTINGS::FormKeyDown(TObject *Sender, WORD &Key,
      TShiftState Shift)
{
        if(Key == VK_ESCAPE) {
                WINDOW_SETTINGS->Close();
        } else if(Key == VK_F2) {
                WINDOW_SETTINGS->CHECKBOX_NOTIFICATIONS_ACTIVE->Checked = !WINDOW_SETTINGS->CHECKBOX_NOTIFICATIONS_ACTIVE->Checked;
        }
}
//---------------------------------------------------------------------------
void __fastcall TWINDOW_SETTINGS::FormShow(TObject *Sender)
{
        exitEditMode();
        updateChatSettings();
}
//---------------------------------------------------------------------------
void __fastcall TWINDOW_SETTINGS::ComboBox1Change(TObject *Sender)
{
        if(FileExists(programSettings.workdir + "\\" + ComboBox1->Text)) {
                ComboBox1->Enabled = false;
                programSettings.languagefile = ComboBox1->Text;
                updateLanguage(programSettings.languagefile);
                ComboBox1->Enabled = true;
                ComboBox1->SetFocus();
        }
}
//---------------------------------------------------------------------------
void __fastcall TWINDOW_SETTINGS::BUTTON_EDITCONFIGURATION_EDITClick(TObject *Sender)
{
        BUTTON_NEWCONFIGURATION_CLEAR->Click();
        LISTBOX_CONFIGURATIONS->Enabled = false;
        BUTTON_NEWCONFIGURATION_ADD->Visible = false;
        BUTTON_NEWCONFIGURATION_ADD->Enabled = false;
        BUTTON_NEWCONFIGURATION_CLEAR->Visible = false;
        BUTTON_NEWCONFIGURATION_CLEAR->Enabled = false;
        BUTTON_EDITCONFIGURATION_OK->Visible = true;
        BUTTON_EDITCONFIGURATION_OK->Enabled = true;
        BUTTON_EDITCONFIGURATION_CANCEL->Visible = true;
        BUTTON_EDITCONFIGURATION_CANCEL->Enabled = true;
        BUTTON_EDITCONFIGURATION_EDIT->Enabled = false;
        BUTTON_EDITCONFIGURATION_UP->Enabled = false;
        BUTTON_EDITCONFIGURATION_DOWN->Enabled = false;
        BUTTON_EDITCONFIGURATION_COPY->Enabled = false;
        BUTTON_CONFIGURATION_REMOVE->Enabled = false;

        int comboindex = ComboBox2->Items->IndexOf(ComboBox2->Text);
        int gameid = (int) ComboBox2->Items->Objects[comboindex];

        for(int i = 0; i < LISTBOX_CONFIGURATIONS->Count; i++) {
                if(LISTBOX_CONFIGURATIONS->Selected[i]) {
                        TObject *t = LISTBOX_CONFIGURATIONS->Items->Objects[i];
                        int j = (int) t;
                        Configuration edit = programSettings.games[gameid].startupConfs[j];
                        EDIT_NEWCONFIGURATION_LABEL->Text = edit.label;
                        EDIT_NEWCONFIGURATION_PASSWORD->Text = edit.password;
                        for (list<String>::iterator ci = edit.mods.begin(); ci != edit.mods.end(); ++ci) {
                                LISTBOX_MODFOLDERS_SELECTED->Items->Add(*ci);
                                for(int k = 0; k < LISTBOX_MODFOLDERS_ALL->Count; k++) {
                                        if(*ci == LISTBOX_MODFOLDERS_ALL->Items->Strings[k]) {
                                                LISTBOX_MODFOLDERS_ALL->Items->Delete(k);
                                                break;
                                        }
                                }
                        }
                        CHECKBOX_NEWCONFIGURATION_NOMAP->Checked = false;
                        CHECKBOX_NEWCONFIGURATION_NOSPLASH->Checked = false;
                        String line = "";
                        for (list<String>::iterator di = edit.addParameters.begin(); di != edit.addParameters.end(); ++di) {
                                String tmp = *di;
                                String nomap = "-nomap";
                                String nosplash = "-nosplash";
                                if((nomap == tmp) || nosplash == tmp) {
                                        if(nomap == tmp) {
                                                CHECKBOX_NEWCONFIGURATION_NOMAP->Checked = true;
                                        }
                                        if(nosplash == tmp) {
                                                CHECKBOX_NEWCONFIGURATION_NOSPLASH->Checked = true;
                                        }
                                } else {
                                        if(line.Length() > 0) {
                                                line += " ";
                                        }
                                        line += tmp;
                                }

                        }
                        EDIT_NEWCONFIGURATION_PARAMETERS->Text = line;
                        break;
                }
        }

}
//---------------------------------------------------------------------------
void __fastcall TWINDOW_SETTINGS::BUTTON_EDITCONFIGURATION_OKClick(TObject *Sender)
{
        int comboindex = ComboBox2->Items->IndexOf(ComboBox2->Text);
        int gameid = (int) ComboBox2->Items->Objects[comboindex];
        list<String> a;
        for(int i = 0; i < LISTBOX_MODFOLDERS_SELECTED->Count; i++) {
                a.push_back(LISTBOX_MODFOLDERS_SELECTED->Items->Strings[i]);
        }
        for(int i = 0; i < LISTBOX_CONFIGURATIONS->Count; i++) {
                if(LISTBOX_CONFIGURATIONS->Selected[i]) {
                        TObject *t = LISTBOX_CONFIGURATIONS->Items->Objects[i];
                        int j = (int) t;
                        programSettings.games[gameid].startupConfs[j].label = EDIT_NEWCONFIGURATION_LABEL->Text;
                        programSettings.games[gameid].startupConfs[j].mods = a;
                        programSettings.games[gameid].startupConfs[j].password = EDIT_NEWCONFIGURATION_PASSWORD->Text;
                        programSettings.games[gameid].startupConfs[j].addParameters = Form1->splitUpMessage(EDIT_NEWCONFIGURATION_PARAMETERS->Text," ");
                        if(CHECKBOX_NEWCONFIGURATION_NOMAP->Checked) {
                                programSettings.games[gameid].startupConfs[j].addParameters.push_back("-nomap");
                        }
                        if(CHECKBOX_NEWCONFIGURATION_NOSPLASH->Checked) {
                                programSettings.games[gameid].startupConfs[j].addParameters.push_back("-nosplash");
                        }
                        break;
                }
        }
        updateConfList();
        exitEditMode();
}
//---------------------------------------------------------------------------

void __fastcall TWINDOW_SETTINGS::BUTTON_EDITCONFIGURATION_CANCELClick(
      TObject *Sender)
{
        exitEditMode();
}
//---------------------------------------------------------------------------


void __fastcall TWINDOW_SETTINGS::BUTTON_EDITCONFIGURATION_UPClick(TObject *Sender)
{
        int comboindex = ComboBox2->Items->IndexOf(ComboBox2->Text);
        int gameid = (int) ComboBox2->Items->Objects[comboindex];
        for(int i = 0; i < LISTBOX_CONFIGURATIONS->Count; i++) {
                if(LISTBOX_CONFIGURATIONS->Selected[i]) {
                        if(i > 0) {
                                LISTBOX_CONFIGURATIONS->Items->Exchange(i, i - 1);
                                Configuration tmp = programSettings.games[gameid].startupConfs[i];
                                programSettings.games[gameid].startupConfs[i] = programSettings.games[gameid].startupConfs[i - 1];
                                programSettings.games[gameid].startupConfs[i - 1] = tmp;

                                TObject *t = LISTBOX_CONFIGURATIONS->Items->Objects[i];
                                LISTBOX_CONFIGURATIONS->Items->Objects[i] = LISTBOX_CONFIGURATIONS->Items->Objects[i - 1];
                                LISTBOX_CONFIGURATIONS->Items->Objects[i - 1] = t;
                                break;
                        }
                }
        }
}
//---------------------------------------------------------------------------

void __fastcall TWINDOW_SETTINGS::BUTTON_EDITCONFIGURATION_DOWNClick(TObject *Sender)
{
        int comboindex = ComboBox2->Items->IndexOf(ComboBox2->Text);
        int gameid = (int) ComboBox2->Items->Objects[comboindex];
        for(int i = 0; i < LISTBOX_CONFIGURATIONS->Count; i++) {
                if(LISTBOX_CONFIGURATIONS->Selected[i]) {
                        if(i < LISTBOX_CONFIGURATIONS->Count - 1) {
                                LISTBOX_CONFIGURATIONS->Items->Exchange(i, i + 1);
                                Configuration tmp = programSettings.games[gameid].startupConfs[i];
                                programSettings.games[gameid].startupConfs[i] = programSettings.games[gameid].startupConfs[i + 1];
                                programSettings.games[gameid].startupConfs[i + 1] = tmp;
                                TObject *t = LISTBOX_CONFIGURATIONS->Items->Objects[i];
                                LISTBOX_CONFIGURATIONS->Items->Objects[i] = LISTBOX_CONFIGURATIONS->Items->Objects[i + 1];
                                LISTBOX_CONFIGURATIONS->Items->Objects[i + 1] = t;
                                break;
                        }
                }
        }
}
//---------------------------------------------------------------------------

void __fastcall TWINDOW_SETTINGS::BUTTON_EDITCONFIGURATION_COPYClick(
      TObject *Sender)
{
        int comboindex = ComboBox2->Items->IndexOf(ComboBox2->Text);
        int gameid = (int) ComboBox2->Items->Objects[comboindex];
        for(int i = 0; i < LISTBOX_CONFIGURATIONS->Count; i++) {
                if(LISTBOX_CONFIGURATIONS->Selected[i]) {
                        TObject *t = LISTBOX_CONFIGURATIONS->Items->Objects[i];
                        int j = (int) t;
                        Configuration copy = programSettings.games[gameid].startupConfs[j].clone();
                        programSettings.pSaddConf(gameid, copy);
                }
        }
        updateConfList();
        checkConfListState();
}
//---------------------------------------------------------------------------


void __fastcall TWINDOW_SETTINGS::BUTTON_OFPRES_BROWSEClick(TObject *Sender)
{
        if(!EDIT_OFPRES_EXECUTABLE->Text.IsEmpty()) {
                OpenDialog1->InitialDir = getFolder(EDIT_OFPRES_EXECUTABLE->Text);
        }
        OpenDialog1->Filter = "Operation Flashpoint: Resistance|FLASHPOINTRESISTANCE.EXE;FLASHPOINTBETA.EXE";
        OpenDialog1->Tag = GAMEID_OFPRES;
        OpenDialog1->Execute();
}

//---------------------------------------------------------------------------

void __fastcall TWINDOW_SETTINGS::BUTTON_OFPCWC_BROWSEClick(
      TObject *Sender)
{
        if(!EDIT_OFPCWC_EXECUTABLE->Text.IsEmpty()) {
                OpenDialog1->InitialDir = getFolder(EDIT_OFPCWC_EXECUTABLE->Text);
        }
        OpenDialog1->Filter = "Operation Flashpoint: Cold War Crisis|OperationFlashpoint.exe;OperationFlashpointbeta.exe";
        OpenDialog1->Tag = GAMEID_OFPCWC;
        OpenDialog1->Execute();
}
//---------------------------------------------------------------------------

void __fastcall TWINDOW_SETTINGS::BUTTON_ARMACWA_BROWSEClick(
      TObject *Sender)
{
        if(!EDIT_ARMACWA_EXECUTABLE->Text.IsEmpty()) {
                OpenDialog1->InitialDir = getFolder(EDIT_ARMACWA_EXECUTABLE->Text);
        }
        OpenDialog1->Filter = "Armed Assault: Cold War Assault|ColdWarAssault.exe";
        OpenDialog1->Tag = GAMEID_ARMACWA;
        OpenDialog1->Execute();
}
//---------------------------------------------------------------------------

void __fastcall TWINDOW_SETTINGS::ComboBox2Change(TObject *Sender)
{
        exitEditMode();
        updateConfList();
        checkConfListState();
        int comboindex = ComboBox2->Items->IndexOf(ComboBox2->Text);
        if(comboindex >= 0) {
                int gameid = (int) ComboBox2->Items->Objects[comboindex];
                updateModFolderList(programSettings.games[gameid].folder);
                GROUPBOX_CONFIGURATIONS->Enabled = true;
                GROUPBOX_NEWCONFIGURATION->Enabled = true;
                BUTTON_NEWCONFIGURATION_UP->Enabled = true;
                BUTTON_NEWCONFIGURATION_DOWN->Enabled = true;
                BUTTON_EDITCONFIGURATION_OK->Enabled = true;
                BUTTON_NEWCONFIGURATION_ADD->Enabled = true;
                BUTTON_NEWCONFIGURATION_CLEAR->Enabled = true;
                BUTTON_EDITCONFIGURATION_CANCEL->Enabled = true;
                BUTTON_NEWCONFIGURATION_MOVELEFT->Enabled = true;
                BUTTON_NEWCONFIGURATION_MOVERIGHT->Enabled = true;
                EDIT_NEWCONFIGURATION_LABEL->Enabled = true;
                EDIT_NEWCONFIGURATION_PASSWORD->Enabled = true;
                EDIT_NEWCONFIGURATION_PARAMETERS->Enabled = true;
                CHECKBOX_NEWCONFIGURATION_NOSPLASH->Enabled = true;
                CHECKBOX_NEWCONFIGURATION_NOMAP->Enabled = true;
                LISTBOX_MODFOLDERS_ALL->Enabled = true;
                LISTBOX_MODFOLDERS_SELECTED->Enabled = true;
        } else {
                GROUPBOX_CONFIGURATIONS->Enabled = false;
                GROUPBOX_NEWCONFIGURATION->Enabled = false;
                BUTTON_NEWCONFIGURATION_MOVELEFT->Enabled = false;
                BUTTON_NEWCONFIGURATION_MOVERIGHT->Enabled = false;
                BUTTON_NEWCONFIGURATION_UP->Enabled = false;
                BUTTON_NEWCONFIGURATION_DOWN->Enabled = false;
                BUTTON_EDITCONFIGURATION_OK->Enabled = false;
                BUTTON_NEWCONFIGURATION_ADD->Enabled = false;
                BUTTON_NEWCONFIGURATION_CLEAR->Enabled = false;
                BUTTON_EDITCONFIGURATION_CANCEL->Enabled = false;
                EDIT_NEWCONFIGURATION_LABEL->Enabled = false;
                EDIT_NEWCONFIGURATION_PASSWORD->Enabled = false;
                EDIT_NEWCONFIGURATION_PARAMETERS->Enabled = false;
                CHECKBOX_NEWCONFIGURATION_NOSPLASH->Enabled = false;
                CHECKBOX_NEWCONFIGURATION_NOMAP->Enabled = false;
                LISTBOX_MODFOLDERS_ALL->Enabled = false;
                LISTBOX_MODFOLDERS_SELECTED->Enabled = false;
        }
}
//---------------------------------------------------------------------------


void __fastcall TWINDOW_SETTINGS::COMBOBOX_OFPCWC_PROFILEChange(
      TObject *Sender)
{
        profileChanged(COMBOBOX_OFPCWC_PROFILE, GAMEID_OFPCWC);
}
//---------------------------------------------------------------------------

void __fastcall TWINDOW_SETTINGS::COMBOBOX_ARMACWA_PROFILEChange(
      TObject *Sender)
{
        profileChanged(COMBOBOX_ARMACWA_PROFILE, GAMEID_ARMACWA);
}
//---------------------------------------------------------------------------

void __fastcall TWINDOW_SETTINGS::COMBOBOX_OFPRES_PROFILEChange(TObject *Sender)
{
        profileChanged(COMBOBOX_OFPRES_PROFILE, GAMEID_OFPRES);
}
//---------------------------------------------------------------------------

void __fastcall TWINDOW_SETTINGS::CHECKBOX_OFPCWCClick(TObject *Sender)
{
        if(!CHECKBOX_OFPCWC->Checked) {
                programSettings.removeGame(GAMEID_OFPCWC);
        } else {
                checkForAutoDetection(GAMEID_OFPCWC);
                programSettings.games[GAMEID_OFPCWC].set = true;
                updateGames();
        }
        GROUPBOX_OFPCWC->Visible = CHECKBOX_OFPCWC->Checked;
        GROUPBOX_OFPCWC->Enabled = CHECKBOX_OFPCWC->Checked;
}
//---------------------------------------------------------------------------

void __fastcall TWINDOW_SETTINGS::CHECKBOX_OFPRESClick(TObject *Sender)
{
        if(!CHECKBOX_OFPRES->Checked) {
                programSettings.removeGame(GAMEID_OFPRES);
        } else {
                checkForAutoDetection(GAMEID_OFPRES);
                programSettings.games[GAMEID_OFPRES].set = true;
                updateGames();
        }
        GROUPBOX_OFPRES->Visible = CHECKBOX_OFPRES->Checked;
        GROUPBOX_OFPRES->Enabled = CHECKBOX_OFPRES->Checked;
}
//---------------------------------------------------------------------------

void __fastcall TWINDOW_SETTINGS::TABSHEET_MODSShow(TObject *Sender)
{
        programSettings.refreshGamesModList(ComboBox2);
}
//---------------------------------------------------------------------------

void __fastcall TWINDOW_SETTINGS::BUTTON_BROWSEClick(TObject *Sender)
{
        OpenDialog2->Execute();
        STOP->Click();
}
//---------------------------------------------------------------------------

void __fastcall TWINDOW_SETTINGS::OpenDialog2CanClose(TObject *Sender,
      bool &CanClose)
{
        EDIT_NOTIFICATION_FILE->Tag = -1;
        EDIT_NOTIFICATION_FILE->Text = "";
        EDIT_NOTIFICATION_FILE->Text = OpenDialog2->FileName;
}
//---------------------------------------------------------------------------

void __fastcall TWINDOW_SETTINGS::EDIT_NOTIFICATION_FILEChange(TObject *Sender)
{
        LabelMilli->Visible = false;
        LabelSeconds->Visible = false;
        LabelMinutes->Visible = false;
        if(!FileExists(EDIT_NOTIFICATION_FILE->Text) && !EDIT_NOTIFICATION_FILE->Text.IsEmpty()) {
                EDIT_NOTIFICATION_FILE->Color = clRed;
        } else {
                EDIT_NOTIFICATION_FILE->Color = clWindow;
                if(FileExists(EDIT_NOTIFICATION_FILE->Text)) {
                        if(EDIT_NOTIFICATION_FILE->Tag > -1) {
                                printPlaybackRange(CustomNotify[EDIT_NOTIFICATION_FILE->Tag].playbackStart, CustomNotify[EDIT_NOTIFICATION_FILE->Tag].playbackEnd);
                        } else {
                                mciSendString(("open \"" + EDIT_NOTIFICATION_FILE->Text + "\" alias LengthCheck").c_str(),0,0,0);
                                mciSendString("set LengthCheck time format milliseconds",0,0,0);
                                char text[128];
                                mciSendString("status LengthCheck length", text, 128, 0);
                                printPlaybackRange(0, StrToInt(text));
                                mciSendString("close LengthCheck", 0, 0, 0);
                        }
                }
        }
}
//---------------------------------------------------------------------------

void __fastcall TWINDOW_SETTINGS::CHECKBOX_FILTER_MINPLAYERSClick(
      TObject *Sender)
{
        EDIT_FILTER_MINPLAYERS->Enabled = CHECKBOX_FILTER_MINPLAYERS->Checked;
        UPDOWN_MINPLAYERS->Enabled = CHECKBOX_FILTER_MINPLAYERS->Checked;
}
//---------------------------------------------------------------------------

void __fastcall TWINDOW_SETTINGS::CHECKBOX_FILTER_MAXPLAYERSClick(
      TObject *Sender)
{
        EDIT_FILTER_MAXPLAYERS->Enabled = CHECKBOX_FILTER_MAXPLAYERS->Checked;
        UPDOWN_MAXPLAYERS->Enabled = CHECKBOX_FILTER_MAXPLAYERS->Checked;
}
//---------------------------------------------------------------------------

void __fastcall TWINDOW_SETTINGS::BUTTON_NEWNOTIFICATION_CLEARClick(
      TObject *Sender)
{
        MEMO_FILTER_MISSIONNAME->Clear();
        MEMO_FILTER_SERVERNAME->Clear();
        MEMO_FILTER_PLAYERNAME->Clear();
        CHECKBOX_FILTER_CREATING->Checked = false;
        CHECKBOX_FILTER_WAITING->Checked = false;
        CHECKBOX_FILTER_SETTINGUP->Checked = false;
        CHECKBOX_FILTER_BRIEFING->Checked = false;
        CHECKBOX_FILTER_PLAYING->Checked = false;
        CHECKBOX_FILTER_DEBRIEFING->Checked = false;
        CHECKBOX_FILTER_WITHPASSWORD->Checked = false;
        CHECKBOX_FILTER_WITHOUTPASSWORD->Checked = true;
        CHECKBOX_FILTER_MINPLAYERS->Checked = false;
        CHECKBOX_FILTER_MAXPLAYERS->Checked = false;
        CHECKBOX_REPEAT->Checked = false;
        UPDOWN_MINPLAYERS->Position = 0;
        UPDOWN_MAXPLAYERS->Position = 0;
        EDIT_NOTIFICATION_FILE->Text = "";
        EDIT_NOTIFICATION_NAME->Text = "";
        EDIT_SONGSTART_MIN->Text = "0";
        EDIT_SONGSTART_SEC->Text = "0";
        EDIT_SONGSTART_MILL->Text = "0";
        EDIT_SONGEND_MIN->Text = "0";
        EDIT_SONGEND_SEC->Text = "0";
        EDIT_SONGEND_MILL->Text = "0";
        TrackBar1->Position = 100;
        ColorBox1->Selected = clMaroon;
}
//---------------------------------------------------------------------------

void __fastcall TWINDOW_SETTINGS::BUTTON_EDITNOTIFICATION_EDITClick(
      TObject *Sender)
{
        LISTBOX_NOTIFICATIONS->Enabled = false;
        BUTTON_NEWNOTIFICATION_ADD->Visible = false;
        BUTTON_NEWNOTIFICATION_ADD->Enabled = false;
        BUTTON_NEWNOTIFICATION_CLEAR->Visible = false;
        BUTTON_NEWNOTIFICATION_CLEAR->Enabled = false;
        BUTTON_EDITNOTIFICATION_OK->Visible = true;
        BUTTON_EDITNOTIFICATION_OK->Enabled = true;
        BUTTON_EDITNOTIFICATION_CANCEL->Visible = true;
        BUTTON_EDITNOTIFICATION_CANCEL->Enabled = true;
        BUTTON_EDITNOTIFICATION_EDIT->Enabled = false;
        BUTTON_NOTIFICATION_REMOVE->Enabled = false;

        for(int i = 0; i < LISTBOX_NOTIFICATIONS->Count; i++) {
                if(LISTBOX_NOTIFICATIONS->Selected[i]) {
                        TObject *t = LISTBOX_NOTIFICATIONS->Items->Objects[i];
                        int j = (int) t;
                        CustomNotification cN = CustomNotify[j];
                        EDIT_NOTIFICATION_NAME->Text = cN.name;
                        CHECKBOX_FILTER_BRIEFING->Checked = cN.statusBriefing;
                        CHECKBOX_FILTER_CREATING->Checked = cN.statusCreating;
                        CHECKBOX_FILTER_DEBRIEFING->Checked = cN.statusDebriefing;
                        CHECKBOX_FILTER_MAXPLAYERS->Checked = (cN.maxPlayers > 0);
                        CHECKBOX_FILTER_MINPLAYERS->Checked = (cN.minPlayers > 0);
                        CHECKBOX_FILTER_PLAYING->Checked = cN.statusPlaying;
                        CHECKBOX_FILTER_SETTINGUP->Checked = cN.statusSettingUp;
                        CHECKBOX_FILTER_WAITING->Checked = cN.statusWaiting;
                        CHECKBOX_FILTER_WITHOUTPASSWORD->Checked = cN.withoutPassword;
                        CHECKBOX_FILTER_WITHPASSWORD->Checked = cN.withPassword;

                        UPDOWN_MINPLAYERS->Position = max(cN.minPlayers, 0);
                        CHECKBOX_FILTER_MINPLAYERS->Checked = (cN.minPlayers >= 0);
                        UPDOWN_MAXPLAYERS->Position = max(cN.maxPlayers, 0);
                        CHECKBOX_FILTER_MAXPLAYERS->Checked = (cN.maxPlayers >= 0);
                        CHECKBOX_REPEAT->Checked = (cN.repeat == 1);
                        MEMO_FILTER_MISSIONNAME->Clear();
                        for (list<String>::iterator ci = cN.missionFilter.begin(); ci != cN.missionFilter.end(); ++ci) {
                                MEMO_FILTER_MISSIONNAME->Lines->Add(*ci);
                        }
                        MEMO_FILTER_SERVERNAME->Clear();
                        for (list<String>::iterator ci = cN.serverFilter.begin(); ci != cN.serverFilter.end(); ++ci) {
                                MEMO_FILTER_SERVERNAME->Lines->Add(*ci);
                        }
                        MEMO_FILTER_PLAYERNAME->Clear();
                        for (list<String>::iterator ci = cN.playerFilter.begin(); ci != cN.playerFilter.end(); ++ci) {
                                MEMO_FILTER_PLAYERNAME->Lines->Add(*ci);
                        }
                        EDIT_NOTIFICATION_FILE->Tag = j;
                        EDIT_NOTIFICATION_FILE->Text = cN.soundFile;
                        ColorBox1->Selected = cN.markingColor;
                        TrackBar1->Position = cN.playbackVolume;
                        break;
                }
        }
}
//---------------------------------------------------------------------------

void __fastcall TWINDOW_SETTINGS::BUTTON_EDITNOTIFICATION_OKClick(
      TObject *Sender)
{
    if(!EDIT_NOTIFICATION_NAME->Text.Trim().IsEmpty() && (FileExists(EDIT_NOTIFICATION_FILE->Text) || EDIT_NOTIFICATION_FILE->Text.Trim().IsEmpty())) {
        if(STOP->Visible) {
                STOP->Click();
        }
        list<String> mission;
        for(int i = 0; i < MEMO_FILTER_MISSIONNAME->Lines->Count; i++) {
                if(!MEMO_FILTER_MISSIONNAME->Lines->Strings[i].Trim().IsEmpty()) {
                        mission.push_back(MEMO_FILTER_MISSIONNAME->Lines->Strings[i]);
                }
        }
        list<String> server;
        for(int i = 0; i < MEMO_FILTER_SERVERNAME->Lines->Count; i++) {
                if(!MEMO_FILTER_SERVERNAME->Lines->Strings[i].Trim().IsEmpty()) {
                        server.push_back(MEMO_FILTER_SERVERNAME->Lines->Strings[i]);
                }
        }
        list<String> player;
        for(int i = 0; i < MEMO_FILTER_PLAYERNAME->Lines->Count; i++) {
                if(!MEMO_FILTER_PLAYERNAME->Lines->Strings[i].Trim().IsEmpty()) {
                        player.push_back(MEMO_FILTER_PLAYERNAME->Lines->Strings[i]);
                }
        }
        for(int i = 0; i < LISTBOX_NOTIFICATIONS->Count; i++) {
                if(LISTBOX_NOTIFICATIONS->Selected[i]) {
                        TObject *t = LISTBOX_NOTIFICATIONS->Items->Objects[i];
                        int j = (int) t;
                        CustomNotify[j].name = EDIT_NOTIFICATION_NAME->Text;
                        CustomNotify[j].missionFilter = mission;
                        CustomNotify[j].serverFilter = server;
                        CustomNotify[j].playerFilter = player;
                        CustomNotify[j].statusCreating = CHECKBOX_FILTER_CREATING->Checked;
                        CustomNotify[j].statusWaiting = CHECKBOX_FILTER_WAITING->Checked;
                        CustomNotify[j].statusSettingUp = CHECKBOX_FILTER_SETTINGUP->Checked;
                        CustomNotify[j].statusBriefing = CHECKBOX_FILTER_BRIEFING->Checked;
                        CustomNotify[j].statusPlaying = CHECKBOX_FILTER_PLAYING->Checked;
                        CustomNotify[j].statusDebriefing = CHECKBOX_FILTER_DEBRIEFING->Checked;
                        CustomNotify[j].withPassword = CHECKBOX_FILTER_WITHPASSWORD->Checked;
                        CustomNotify[j].withoutPassword = CHECKBOX_FILTER_WITHOUTPASSWORD->Checked;
                        if(CHECKBOX_FILTER_MINPLAYERS->Checked) {
                                CustomNotify[j].minPlayers = UPDOWN_MINPLAYERS->Position;
                        } else {
                                CustomNotify[j].minPlayers = -1;
                        }
                        if(CHECKBOX_FILTER_MAXPLAYERS->Checked) {
                                CustomNotify[j].maxPlayers = UPDOWN_MAXPLAYERS->Position;
                        } else {
                                CustomNotify[j].maxPlayers = -1;
                        }
                        CustomNotify[j].soundFile = EDIT_NOTIFICATION_FILE->Text;
                        CustomNotify[j].repeat = CHECKBOX_REPEAT->Checked;
                        CustomNotify[j].playbackVolume = TrackBar1->Position;
                        CustomNotify[i].playbackStart =
                                                StrToInt(EDIT_SONGSTART_MIN->Text)*60000 +
                                                StrToInt(EDIT_SONGSTART_SEC->Text)*1000 +
                                                StrToInt(EDIT_SONGSTART_MILL->Text);
                        CustomNotify[j].playbackEnd =
                                                StrToInt(EDIT_SONGEND_MIN->Text)*60000 +
                                                StrToInt(EDIT_SONGEND_SEC->Text)*1000 +
                                                StrToInt(EDIT_SONGEND_MILL->Text);
                        CustomNotify[j].markingColor = ColorBox1->Selected;
                        break;
                }
        }
        updateNotificationsList();
        exitEditNotificationMode();
    }
}
//---------------------------------------------------------------------------

void __fastcall TWINDOW_SETTINGS::BUTTON_NEWNOTIFICATION_ADDClick(
      TObject *Sender)
{
        if(!EDIT_NOTIFICATION_NAME->Text.Trim().IsEmpty() && (FileExists(EDIT_NOTIFICATION_FILE->Text) || EDIT_NOTIFICATION_FILE->Text.Trim().IsEmpty())) {
                if(STOP->Visible) {
                        STOP->Click();
                }
                list<String> mission, server, player;
                for(int i = 0; i < MEMO_FILTER_MISSIONNAME->Lines->Count; i++) {
                        mission.push_back(MEMO_FILTER_MISSIONNAME->Lines->Strings[i]);
                }
                for(int i = 0; i < MEMO_FILTER_SERVERNAME->Lines->Count; i++) {
                        server.push_back(MEMO_FILTER_SERVERNAME->Lines->Strings[i]);
                }
                for(int i = 0; i < MEMO_FILTER_PLAYERNAME->Lines->Count; i++) {
                        player.push_back(MEMO_FILTER_PLAYERNAME->Lines->Strings[i]);
                }
                int minP = -1, maxP = -1;
                if(CHECKBOX_FILTER_MINPLAYERS->Checked) {
                        minP = UPDOWN_MINPLAYERS->Position;
                }
                if(CHECKBOX_FILTER_MAXPLAYERS->Checked) {
                        maxP = UPDOWN_MAXPLAYERS->Position;
                }
                CustomNotification newCN = CustomNotification(EDIT_NOTIFICATION_NAME->Text,
                        CHECKBOX_FILTER_CREATING->Checked, CHECKBOX_FILTER_WAITING->Checked,
                        CHECKBOX_FILTER_SETTINGUP->Checked, CHECKBOX_FILTER_BRIEFING->Checked,
                        CHECKBOX_FILTER_PLAYING->Checked, CHECKBOX_FILTER_DEBRIEFING->Checked,
                        CHECKBOX_FILTER_WITHPASSWORD->Checked, CHECKBOX_FILTER_WITHOUTPASSWORD->Checked,
                        mission, server, player, minP, maxP,
                        EDIT_NOTIFICATION_FILE->Text, TrackBar1->Position,
                                StrToInt(EDIT_SONGSTART_MIN->Text)*60000 +
                                StrToInt(WINDOW_SETTINGS->EDIT_SONGSTART_SEC->Text)*1000 +
                                StrToInt(WINDOW_SETTINGS->EDIT_SONGSTART_MILL->Text),
                                StrToInt(WINDOW_SETTINGS->EDIT_SONGEND_MIN->Text)*60000 +
                                StrToInt(WINDOW_SETTINGS->EDIT_SONGEND_SEC->Text)*1000 +
                                StrToInt(WINDOW_SETTINGS->EDIT_SONGEND_MILL->Text),
                        ColorToString(ColorBox1->Selected),
                        CHECKBOX_REPEAT->Checked);

                for(int i = 0; i < GetArrLength(CustomNotify); i++) {
                        if(!CustomNotify[i].set) {
                                CustomNotify[i] = newCN;
                                break;
                        }
                }
                if(LISTBOX_NOTIFICATIONS->Items->Count >= GetArrLength(CustomNotify)) {
                        BUTTON_NEWNOTIFICATION_ADD->Enabled = false;
                }
                updateNotificationsList();
                checkNotificationListState();
        } else if(EDIT_NOTIFICATION_NAME->Text.Trim().IsEmpty()) {
                EDIT_NOTIFICATION_NAME->SetFocus();
        } else if(!FileExists(EDIT_NOTIFICATION_FILE->Text) && !EDIT_NOTIFICATION_FILE->Text.Trim().IsEmpty()) {
                EDIT_NOTIFICATION_FILE->SetFocus();
        }
}
//---------------------------------------------------------------------------

void __fastcall TWINDOW_SETTINGS::BUTTON_NOTIFICATION_REMOVEClick(
      TObject *Sender)
{
        for(int i = 0; i < LISTBOX_NOTIFICATIONS->Count; i++) {
                if(LISTBOX_NOTIFICATIONS->Selected[i]) {
                        TObject *t = LISTBOX_NOTIFICATIONS->Items->Objects[i];
                        int j = (int) t;
                        CustomNotify[j] = CustomNotification();
                        LISTBOX_NOTIFICATIONS->Items->Delete(i);
                }
        }
        if(LISTBOX_NOTIFICATIONS->Items->Count < GetArrLength(CustomNotify)) {
                BUTTON_NEWNOTIFICATION_ADD->Enabled = true;
        }
        updateNotificationsList();
        checkNotificationListState();
}
//---------------------------------------------------------------------------

void __fastcall TWINDOW_SETTINGS::TABSHEET_NOTIFICATIONSShow(TObject *Sender)
{
        exitEditNotificationMode();
        updateNotificationsList();
        checkNotificationListState();
}
//---------------------------------------------------------------------------

void __fastcall TWINDOW_SETTINGS::BUTTON_EDITNOTIFICATION_CANCELClick(
      TObject *Sender)
{
        exitEditNotificationMode();        
}
//---------------------------------------------------------------------------

void __fastcall TWINDOW_SETTINGS::PLAYClick(TObject *Sender)
{
        if(FileExists(EDIT_NOTIFICATION_FILE->Text)) {
                LabelMilli->Visible = true;
                LabelSeconds->Visible = true;
                LabelMinutes->Visible = true;
                PLAY->Visible = false;
                STOP->Visible = true;
                mp3p.MP3startPreview();
        }        
}
//---------------------------------------------------------------------------

void __fastcall TWINDOW_SETTINGS::TrackBar1Change(TObject *Sender)
{
        mciSendString(("setaudio OFPM_MP3PREVIEW volume to " + String(TrackBar1->Position)*10).c_str(), 0, 0,0);
        
}
//---------------------------------------------------------------------------

void __fastcall TWINDOW_SETTINGS::STOPClick(TObject *Sender)
{
        mp3p.MP3stopPreview();
        PLAY->Visible = true;
        STOP->Visible = false;        
}
//---------------------------------------------------------------------------


void __fastcall TWINDOW_SETTINGS::EDIT_FILTER_MINPLAYERSChange(TObject *Sender)
{
        try {
                int a = StrToInt(EDIT_FILTER_MINPLAYERS->Text);
                if(a < UPDOWN_MINPLAYERS->Min) {
                        EDIT_FILTER_MINPLAYERS->Text = UPDOWN_MINPLAYERS->Position;
                } else if(a > UPDOWN_MINPLAYERS->Max) {
                        EDIT_FILTER_MINPLAYERS->Text = UPDOWN_MINPLAYERS->Position;
                }
        } catch (...) {
                EDIT_FILTER_MINPLAYERS->Text = UPDOWN_MINPLAYERS->Position;
        }
}
//---------------------------------------------------------------------------

void __fastcall TWINDOW_SETTINGS::EDIT_FILTER_MAXPLAYERSChange(TObject *Sender)
{
        try {
                int a = StrToInt(EDIT_FILTER_MAXPLAYERS->Text);
                if(a < UPDOWN_MAXPLAYERS->Min) {
                        EDIT_FILTER_MAXPLAYERS->Text = UPDOWN_MAXPLAYERS->Position;
                } else if(a > UPDOWN_MAXPLAYERS->Max) {
                        EDIT_FILTER_MAXPLAYERS->Text = UPDOWN_MAXPLAYERS->Position;
                }
        } catch (...) {
                EDIT_FILTER_MAXPLAYERS->Text = UPDOWN_MAXPLAYERS->Position;
        }
}
//---------------------------------------------------------------------------

void __fastcall TWINDOW_SETTINGS::EDIT_SONGEND_MILLChange(TObject *Sender)
{
        try {
                StrToInt(EDIT_SONGEND_MILL->Text);
        } catch (...) {
                EDIT_SONGEND_MILL->Text = "0";
        }        
}
//---------------------------------------------------------------------------

void __fastcall TWINDOW_SETTINGS::EDIT_SONGSTART_MINChange(TObject *Sender)
{
        try {
                StrToInt(EDIT_SONGSTART_MIN->Text);
        } catch (...) {
                EDIT_SONGSTART_MIN->Text = "0";
        }
}
//---------------------------------------------------------------------------

void __fastcall TWINDOW_SETTINGS::EDIT_SONGSTART_SECChange(TObject *Sender)
{
        try {
                StrToInt(EDIT_SONGSTART_SEC->Text);
        } catch (...) {
                EDIT_SONGSTART_SEC->Text = "0";
        }
}
//---------------------------------------------------------------------------

void __fastcall TWINDOW_SETTINGS::EDIT_SONGSTART_MILLChange(TObject *Sender)
{
        try {
                StrToInt(EDIT_SONGSTART_MILL->Text);
        } catch (...) {
                EDIT_SONGSTART_MILL->Text = "0";
        }
}
//---------------------------------------------------------------------------

void __fastcall TWINDOW_SETTINGS::EDIT_SONGEND_MINChange(TObject *Sender)
{
        try {
                StrToInt(EDIT_SONGEND_MIN->Text);
        } catch (...) {
                EDIT_SONGEND_MIN->Text = "0";
        }
}
//---------------------------------------------------------------------------

void __fastcall TWINDOW_SETTINGS::EDIT_SONGEND_SECChange(TObject *Sender)
{
        try {
                StrToInt(EDIT_SONGEND_SEC->Text);
        } catch (...) {
                EDIT_SONGEND_SEC->Text = "0";
        }
}
//---------------------------------------------------------------------------

void __fastcall TWINDOW_SETTINGS::EDIT_NOTIFICATION_FILEKeyUp(TObject *Sender, WORD &Key,
      TShiftState Shift)
{
        if(Key == VK_DELETE) {
                EDIT_NOTIFICATION_FILE->Text = "";
        }
}
//---------------------------------------------------------------------------

void __fastcall TWINDOW_SETTINGS::MP3TimerTimer(TObject *Sender)
{
        MP3Timer->Enabled = mp3p.check();         
}
//---------------------------------------------------------------------------

void __fastcall TWINDOW_SETTINGS::LISTBOX_NOTIFICATIONSClick(
      TObject *Sender)
{
        checkNotificationListState();        
}
//---------------------------------------------------------------------------


void __fastcall TWINDOW_SETTINGS::CHECKBOX_NOTIFICATIONS_ACTIVEClick(TObject *Sender)
{
        WINDOW_SETTINGS->setCustomNotifications(CHECKBOX_NOTIFICATIONS_ACTIVE->Checked);
        if(!CHECKBOX_NOTIFICATIONS_ACTIVE->Checked) {
                WINDOW_SETTINGS->MP3shutdown();
        }
}
//---------------------------------------------------------------------------
void __fastcall TWINDOW_SETTINGS::EDIT_SERVERLIST_UPDATEChange(TObject *Sender)
{
        try {
                int a = StrToInt(EDIT_SERVERLIST_UPDATE->Text);
                if(a < UPDOWN_SERVERLIST_UPDATE->Min) {
                        EDIT_SERVERLIST_UPDATE->Text = UPDOWN_SERVERLIST_UPDATE->Position;
                } else if(a > UPDOWN_SERVERLIST_UPDATE->Max) {
                        EDIT_SERVERLIST_UPDATE->Text = UPDOWN_SERVERLIST_UPDATE->Position;
                }
        } catch (...) {
                EDIT_SERVERLIST_UPDATE->Text = UPDOWN_SERVERLIST_UPDATE->Position;
        }
        programSettings.setInterval(UPDOWN_SERVERLIST_UPDATE->Position);
}

void __fastcall TWINDOW_SETTINGS::UPDOWN_SERVERLIST_UPDATEClick(TObject *Sender, TUDBtnType Button)
{
        programSettings.setInterval(UPDOWN_SERVERLIST_UPDATE->Position);
}
//---------------------------------------------------------------------------

void __fastcall TWINDOW_SETTINGS::LISTBOX_CONFIGURATIONSClick(TObject *Sender)
{
        checkConfListState();
}

//---------------------------------------------------------------------------

void __fastcall TWINDOW_SETTINGS::CHECKBOX_ARMACWAClick(TObject *Sender)
{
        if(!CHECKBOX_ARMACWA->Checked) {
                programSettings.removeGame(GAMEID_ARMACWA);
        } else {
                checkForAutoDetection(GAMEID_ARMACWA);
                programSettings.games[GAMEID_ARMACWA].set = true;
                updateGames();
        }
        GROUPBOX_ARMACWA->Visible = CHECKBOX_ARMACWA->Checked;
        GROUPBOX_ARMACWA->Enabled = CHECKBOX_ARMACWA->Checked;
}                                                                            
//---------------------------------------------------------------------------

void __fastcall TWINDOW_SETTINGS::EDIT_CHAT_IRCSERVER_PORTChange(
      TObject *Sender)
{
        try {
                int p = StrToInt(EDIT_CHAT_IRCSERVER_PORT->Text);
        } catch (...) {
                EDIT_CHAT_IRCSERVER_PORT->Text = Form1->getChatPort();
        }
}
//---------------------------------------------------------------------------

void __fastcall TWINDOW_SETTINGS::BUTTON_CHAT_SETDEFAULTClick(
      TObject *Sender)
{
        WINDOW_SETTINGS->EDIT_CHAT_IRCSERVER_ADDRESS->Text = "irc.freenode.net";
        WINDOW_SETTINGS->EDIT_CHAT_IRCSERVER_PORT->Text = 6666;
        WINDOW_SETTINGS->EDIT_CHAT_IRCSERVER_CHANNEL->Text = "operationflashpoint1";
        WINDOW_SETTINGS->CHECKBOX_CHAT_AUTOCONNECT->Checked = false;
}
//---------------------------------------------------------------------------


