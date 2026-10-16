#include "nxmp-i18n.h"

#include "nxmp-en.h"
#include "nxmp-it.h"
#include "logger.h"
#include "utils.h"

#include <fstream>

namespace nxlangs{
	
	
	std::vector<lang_struct> langslist;
	std::vector<std::string> langfileslist;
	
	unsigned int selectedlang = 0;
	
	std::string get_mainmenu_str(NX_MAINMENU_STR strval){
		return langslist[selectedlang].mainmenu[strval];
	};
	
	std::string get_common_str(NX_COMMON_STR strval){
		return langslist[selectedlang].common[strval];
	};
	
	std::string get_popup_str(NX_POPUP_STR strval){
		return langslist[selectedlang].popup[strval];
	};
	
	std::string get_settingsmenu_str(NX_SETTINGS_STR strval){
		return langslist[selectedlang].settingsmenu[strval];
	};
	
	std::string get_network_str(NX_NETWORK_STR strval){
		return langslist[selectedlang].network[strval];
	};
	
	std::string GetCurrentLangName(){
		return langslist[selectedlang].name;
	}
	
	void ParseAddLangfile(std::string _path){
		// Slim build: embedded languages used, json external parsing disabled
	}
	
	
	void FetchUserLangs(std::string _path){
	
		langfileslist.clear();
		struct dirent *ent;
		DIR *dir;
		NXLOG::DEBUGLOG("Open Langs Dir: %s\n",_path.c_str());
		if (!_path.empty()) {
			if ((dir = opendir(_path.c_str())) != nullptr) {
				FsFileSystem sdmc;
				fsOpenSdCardFileSystem(&sdmc);
				while ((ent = readdir(dir)) != nullptr) {
					if ((_path == "/" || strlen(ent->d_name) == 1) && ent->d_name[0] == '.') {
						continue;
					}
					if ((_path == "/" || strlen(ent->d_name) == 2) && ent->d_name[0] == '.' && ent->d_name[1] == '.') {
						continue;
					}
					
					if (Utility::endsWith(ent->d_name, ".json", false)) {
						NXLOG::DEBUGLOG("FILE %s\n",(_path + std::string("/") + ent->d_name).c_str());
						ParseAddLangfile(_path + std::string("/") + ent->d_name);
						//langfileslist.push_back(_path + std::string("/") + ent->d_name);
					}
						
				}
				fsFsClose(&sdmc);
				closedir(dir);
					
			}
		}
		
	}
	
	void Init_Langs(){
		
		lang_struct tmp_eng;
		tmp_eng.id = 0;
		tmp_eng.name = "English";
		tmp_eng.author = en_i18n_author;
		tmp_eng.status = en_i18n_status;
		tmp_eng.native = en_i18n_native;
		tmp_eng.mainmenu = MainMenu_STR_EN;
		tmp_eng.common = Common_STR_EN;
		tmp_eng.popup = Popup_STR_EN;
		tmp_eng.settingsmenu = SettingsMenu_STR_EN;
		tmp_eng.network = Network_STR_EN;
		tmp_eng.filepath = "0";
		langslist.push_back(tmp_eng);
		
		lang_struct tmp_it;
		tmp_it.id = 1;
		tmp_it.name = "Italiano";
		tmp_it.author = it_i18n_author;
		tmp_it.status = it_i18n_status;
		tmp_it.native = it_i18n_native;
		tmp_it.mainmenu = MainMenu_STR_IT;
		tmp_it.common = Common_STR_IT;
		tmp_it.popup = Popup_STR_IT;
		tmp_it.settingsmenu = SettingsMenu_STR_IT;
		tmp_it.network = Network_STR_IT;
		tmp_eng.filepath = "1";
		
		langslist.push_back(tmp_it);
		
		
		FetchUserLangs("./langs");
		
		
		
		
		
		
	}


}