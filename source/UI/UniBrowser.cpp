#include "gui.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "appwindows.h"
#include "utils.h"
#include "localfiles.h"
#include "Enigma2.h"

#include "nxmp-i18n.h"

namespace Windows {
	
	bool movenavfocus = false;
	std::string navfocusstring = "";
	
	void SetBrowserNav(std::string foldername){
		movenavfocus = true;
		navfocusstring = foldername;
	}

	struct BrowserThumb {
		Texture tex;
		bool loaded = false;
		bool custom = false;
	};
	static std::map<std::string, BrowserThumb> g_thumb_cache;

	static Texture GetEntryThumbnail(const FS::FileEntry& entry) {
		if (g_thumb_cache.find(entry.path) != g_thumb_cache.end()) {
			return g_thumb_cache[entry.path].tex;
		}

		BrowserThumb bt;
		bt.loaded = true;

		if (entry.type == FS::FileEntryType::Directory) {
			std::string candidates[] = {
				entry.path + "/cover.jpg",
				entry.path + "/cover.png",
				entry.path + "/folder.jpg",
				entry.path + "/folder.png",
				entry.path + "/poster.jpg",
				entry.path + ".jpg",
				entry.path + ".png"
			};
			for (const auto& c : candidates) {
				FILE* f = fopen(c.c_str(), "rb");
				if (f) {
					fclose(f);
					bt.tex = imgloader->OpenImageFile(c);
					if (bt.tex.id != (DkResHandle)-1 && bt.tex.id != 0) {
						bt.custom = true;
						g_thumb_cache[entry.path] = bt;
						return bt.tex;
					}
				}
			}
			bt.tex = imgloader->icons.FolderTexture;
		} else {
			std::string base_no_ext = entry.path;
			size_t dot_pos = base_no_ext.find_last_of('.');
			if (dot_pos != std::string::npos) {
				base_no_ext = base_no_ext.substr(0, dot_pos);
			}
			std::string candidates[] = {
				base_no_ext + ".jpg",
				base_no_ext + ".png",
				base_no_ext + ".jpeg"
			};
			for (const auto& c : candidates) {
				FILE* f = fopen(c.c_str(), "rb");
				if (f) {
					fclose(f);
					bt.tex = imgloader->OpenImageFile(c);
					if (bt.tex.id != (DkResHandle)-1 && bt.tex.id != 0) {
						bt.custom = true;
						g_thumb_cache[entry.path] = bt;
						return bt.tex;
					}
				}
			}
			if (entry.mediatype == FS::FileMediaType::Image) {
				bt.tex = imgloader->OpenImageFile(entry.path);
				if (bt.tex.id != (DkResHandle)-1 && bt.tex.id != 0) {
					bt.custom = true;
					g_thumb_cache[entry.path] = bt;
					return bt.tex;
				}
				bt.tex = imgloader->icons.ImageTexture;
			} else if (entry.mediatype == FS::FileMediaType::PDF) {
				bt.tex = imgloader->icons.PDFTexture;
			} else if (entry.mediatype == FS::FileMediaType::Archive) {
				bt.tex = imgloader->icons.ArchiveTexture;
			} else {
				bt.tex = imgloader->icons.FileTexture;
			}
		}

		g_thumb_cache[entry.path] = bt;
		return bt.tex;
	}

	static void HandleOpenEntry(const FS::FileEntry& curEntry, int n, bool& changefolder, bool& triggerselect) {
		if (curEntry.type == FS::FileEntryType::Directory) {
			triggerselect = true;
			changefolder = true;
			filebrowser->DirList(curEntry.path, configini->getshowHidden(false), Utility::getMediaExtensions());
		} else {
			item.laststate = item.state;
			std::string fullurl = filebrowser->getOpenUrlPart() + curEntry.path;
			if (Utility::isImageExtension(fullurl)) {
				if (filebrowser->getfileContentsThreaded(curEntry.path)) {
					item.state = MENU_STATE_IMGVIEWER;
					Windows::setImageZoom(1.0f);
					Windows::loadimage = true;
				}
			} else if (Utility::isPDFExtension(fullurl)) {
				if (filebrowser->getfileContentsThreaded(curEntry.path)) {
					item.state = MENU_STATE_PDFVIEWER;
					Windows::setPDFImageZoom(1.0f);
					Windows::pdf_loadimage = true;
				}
			} else if (Utility::isISOExtension(fullurl)) {
				item.popupstate = POPUP_STATE_ISOOPENCHOOSE;
				item.popuparg = fullurl;
			} else if (Utility::isArchiveExtension(fullurl)) {
				filebrowser->OpenArchive(fullurl);
				filebrowser->DirList(filebrowser->getBasePath(), configini->getshowHidden(false), Utility::getMediaExtensions());
			} else if (Utility::isM3UExtension(fullurl) && !Utility::startWith(fullurl, "m3u0:/", false)) {
				filebrowser->OpenM3U(fullurl);
				filebrowser->DirList(filebrowser->getBasePath(), configini->getshowHidden(false), Utility::getMediaExtensions());
			} else {
				filebrowser->clearChecked();
				libmpv->loadFile(fullurl);
				if (configini->getDbActive(true)) {
					libmpv->getFileInfo()->resume = sqlitedb->getResume(fullurl);
					if (libmpv->getFileInfo()->resume > 0) {
						item.popupstate = POPUP_STATE_RESUME;
					}
				}
			}
		}
	}
	
	void UniBrowserWindow(/*bool *focus, bool *first_item*/) {
		 Windows::SetupWindow();
		 if(BrowserTextScroller == nullptr){
			 BrowserTextScroller=new CTextScroller("##filescroller");
		 }
		GUI::cloktimeText(ImVec2((1180.0f*multiplyRes)-ImGui::CalcTextSize(nxmpstats->currentTime).x-(10.0*multiplyRes),2.0f*multiplyRes),true,nxmpstats->currentTime);
		GUI::newbatteryIcon(ImVec2(1180.0f*multiplyRes,2.0f*multiplyRes),true,batteryPercent,40,20,true);
		
		 if (ImGui::Begin(filebrowser->getTitle().c_str(), nullptr, ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_MenuBar )) {
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, NavHover_color);
			ImGui::PushStyleColor(ImGuiCol_NavHighlight, Active_color);
			ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, {0, 5});
			
			if (ImGui::BeginMenuBar()) {
				ImGui::Text("path: %s", filebrowser->getCurrentPath().c_str());

				float cur_w = ImGui::GetWindowWidth();
				ImGui::SameLine(cur_w - 340.0f * multiplyRes);
				std::string viewlabel = (browserViewMode == 1) ? "[模式: 缩略图]" : "[模式: 列表]";
				if (ImGui::SmallButton(viewlabel.c_str())) {
					browserViewMode = 1 - browserViewMode;
				}
				ImGui::SameLine();
				std::string themelabel = (configini->getThemeColor(true) == 1) ? "[主题: 白天]" : "[主题: 黑夜]";
				if (ImGui::SmallButton(themelabel.c_str())) {
					int nxt = (configini->getThemeColor(true) == 1) ? 0 : 1;
					configini->setThemeColor(nxt);
					SetColorTheme(nxt);
				}

				ImGui::EndMenuBar();
			}
			float total_w = ImGui::GetContentRegionAvail().x;
			float total_h = ImGui::GetContentRegionAvail().y;
			std::vector<FS::FileEntry> thislist = filebrowser->getCurrList();
			bool triggerselect = false;
			

			if(!libmpv->Stopped() && item.state != MENU_STATE_PLAYER){
#ifdef OPENGL_BACKEND
				ImVec2 image_pnew	= ImVec2((1280.0*multiplyRes-300.0f*multiplyRes-2 * ImGui::GetStyle().ItemSpacing.x),(720.0*multiplyRes-200.0f*multiplyRes-2 * ImGui::GetStyle().ItemSpacing.y));
				ImVec2 image_p1 = ImVec2((1280.0*multiplyRes-2 * ImGui::GetStyle().ItemSpacing.x), (720.0*multiplyRes-2 * ImGui::GetStyle().ItemSpacing.y));
		
				image_pnew = image_pnew - ImVec2(10,10);
				image_p1 = image_p1 - ImVec2(10,10);
				
				ImVec2 image_pnew_border = image_pnew - ImVec2(1,1);
				ImVec2 image_p1_border = image_p1 + ImVec2(1,1);
				
				ImGui::GetForegroundDrawList()->AddRect(image_pnew_border, image_p1_border, ImGui::GetColorU32(ImVec4(1.0f,0.0f,0.0f,1.0f)), 0.0f);
				ImGui::GetForegroundDrawList()->AddImage((void*)(intptr_t)getFBO_Texture(), image_pnew, image_p1);
				
#endif
			}

			
			
			ImGui::BeginChild("##tablecontainer",ImVec2(total_w,total_h-45*multiplyRes));
			if(!filebrowser->connected){
				ImGui::SetCursorPosX( (ImGui::GetWindowWidth() - ImGui::CalcTextSize("Error Connecting").x) / 2.f);
				ImGui::TextColored(ImVec4(1.0f,0.0f,0.0f,1.0f),"Error Connecting");
				
				ImGui::SetCursorPosX( (ImGui::GetWindowWidth() - ImGui::CalcTextSize(filebrowser->errormsg.c_str()).x) / 2.f);
				ImGui::SetCursorPosY( (ImGui::GetWindowHeight() - ImGui::CalcTextSize(filebrowser->errormsg.c_str()).y) / 2.f);
				ImGui::Text("%s",filebrowser->errormsg.c_str());
			}else{
				bool changefolder = false;
				if (browserViewMode == 0) {
					if (ImGui::BeginTable("table1", 3,/*ImGuiTableFlags_RowBg|*/ImGuiTableFlags_ScrollY)){
					ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, (910.0f*multiplyRes -2 * ImGui::GetStyle().ItemSpacing.x)); 
					ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, 125.0f*multiplyRes);
					ImGui::TableSetupColumn("Date", ImGuiTableColumnFlags_WidthFixed,255.f*multiplyRes);       
					ImGui::TableSetupScrollFreeze(0, 1);
					//ImGui::TableHeadersRow();
					ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
					ImGui::TableSetColumnIndex(0);
					ImGui::Text(nxlangs::get_common_str(nxlangs::NXCOMMON_NAME).c_str());
					ImGui::TableSetColumnIndex(1);
					ImGui::Text(nxlangs::get_common_str(nxlangs::NXCOMMON_SIZE).c_str());
					ImGui::TableSetColumnIndex(2);
					ImGui::Text(nxlangs::get_common_str(nxlangs::NXCOMMON_DATE).c_str());
					
					ImGuiListClipper clipper;
					clipper.Begin(thislist.size());
					
					
					int navfocusid = 0;
					if(movenavfocus){
						for(int i=0;i<thislist.size();i++){
							//NXLOG::DEBUGLOG("%s %s",thislist[i].name.c_str(),navfocusstring.c_str());
						
							if(thislist[i].name == navfocusstring){
								navfocusid = i;
							}
						}
						//movenavfocus=false;
						clipper.ForceDisplayRangeByIndices( navfocusid, navfocusid+41 );
						 
					}
					
					changefolder = false;
					while (clipper.Step())
					{
						for (unsigned int n = clipper.DisplayStart; n < clipper.DisplayEnd; n++){
							static int selected = -1;
							
							if(movenavfocus){
								if(navfocusid == n){
									ImGui::SetKeyboardFocusHere();
									
								}
								
							}
							
							
							ImGui::TableNextRow();
							ImGui::TableSetColumnIndex(0);
							
							std::string itemid = "##cbox" + std::to_string(n);
							
							ImGui::Dummy(ImVec2(0,30 + ImGui::GetStyle().FramePadding.y * 2));
							ImGui::SameLine();
							
							if(item.selectionstate == FILE_SELECTION_CHECKBOX){
								if(filebrowser->remotefs)FILE_SELECTION_NONE;
								if(thislist[n].type != FS::FileEntryType::Directory){
									if(ImGui::Checkbox(itemid.c_str(), filebrowser->checked(n))){
										
									}
									ImGui::SameLine();
								}
								
							}
							
							if(thislist[n].type == FS::FileEntryType::Directory){
								GUI::NXMPImage((void*)(intptr_t)imgloader->icons.FolderTexture.id, ImVec2(30,30));
							}else{
								if(thislist[n].mediatype == FS::FileMediaType::Media){
									GUI::NXMPImage((void*)(intptr_t)imgloader->icons.FileTexture.id, ImVec2(30,30));
								}else if(thislist[n].mediatype == FS::FileMediaType::Image){
									GUI::NXMPImage((void*)(intptr_t)imgloader->icons.ImageTexture.id, ImVec2(30,30));
								}else if(thislist[n].mediatype == FS::FileMediaType::Archive){
									GUI::NXMPImage((void*)(intptr_t)imgloader->icons.ArchiveTexture.id, ImVec2(30,30));
								}else if(thislist[n].mediatype == FS::FileMediaType::PDF){
									GUI::NXMPImage((void*)(intptr_t)imgloader->icons.PDFTexture.id, ImVec2(30,30));
								}
							}
							
							
							ImGui::SameLine();
							ImGuiSelectableFlags selectable_flags = ImGuiSelectableFlags_SpanAllColumns;
							if(item.selectionstate != FILE_SELECTION_CHECKBOX){
								if (ImGui::Selectable(itemid.c_str(), selected == n,selectable_flags,ImVec2(0,30.0f*multiplyRes))){
									HandleOpenEntry(thislist[n], n, changefolder, triggerselect);
								}
							}else{
								ImGui::Dummy(ImVec2(0,ImGui::CalcTextSize("A").y + ImGui::GetStyle().FramePadding.y * 2));
								ImGui::SameLine();
							
							}
							
							if (ImGui::IsItemHovered()){
								if(item.fileHoveredidx != n){
									BrowserTextScroller->ResetPosition();
									
								}
								item.fileHoveredidx = n;
							}
							ImVec4* colors = ImGui::GetStyle().Colors;
							ImVec4 textcolor = colors[ImGuiCol_Text];
							if(sqlitedb != nullptr && thislist[n].dbread == -1){
								int dbfilestatus = sqlitedb->getFileDbStatus(filebrowser->getOpenUrlPart()+thislist[n].path);
								if(dbfilestatus == 2){
									textcolor = ImVec4(0.0f,1.0f,0.0f,1.0f);
								} else if(dbfilestatus == 1){
									textcolor = ImVec4(0.0f,1.0f,1.0f,1.0f);
								}
								filebrowser->SetFileDbStatus(n,dbfilestatus);
							}else if(sqlitedb != nullptr){
								if(thislist[n].dbread == 2){
									textcolor = ImVec4(0.0f,1.0f,0.0f,1.0f);
								} else if(thislist[n].dbread == 1){
									textcolor = ImVec4(0.0f,1.0f,1.0f,1.0f);
								}
							}
							ImGui::SameLine();
							//ImGui::SetCursorPosY(ImGui::GetCursorPosY() +5.0f);
							
							std::string bsid =std::string("##bs") + std::to_string(n);
							if(item.fileHoveredidx == n){
								ImGui::Dummy(ImVec2(-10.0,(30.0f*multiplyRes-2 * ImGui::GetStyle().ItemSpacing.y)));
								ImGui::SameLine();
								ImGui::SetCursorPosX(ImGui::GetCursorPosX()-10.0f);
								ImGui::SetCursorPosY(ImGui::GetCursorPosY() +5.0f*multiplyRes);
								BrowserTextScroller->DrawColor(bsid.c_str(),textcolor,(910.0f*multiplyRes -2* ImGui::GetStyle().ItemSpacing.x),(30.0f*multiplyRes-2 * ImGui::GetStyle().ItemSpacing.y),thislist[n].name.c_str());	
							}else{
								ImGui::Dummy(ImVec2(-10.0,(30.0f*multiplyRes-2 * ImGui::GetStyle().ItemSpacing.y)));
								
								ImGui::SameLine();
								ImGui::SetCursorPosX(ImGui::GetCursorPosX()-10.0f);
								ImGui::SetCursorPosY(ImGui::GetCursorPosY()+5.0f);
								ImGui::TextColored(textcolor,"%s",thislist[n].name.c_str());
							}
							ImGui::TableSetColumnIndex(1);
							if(thislist[n].type == FS::FileEntryType::File){
								ImGui::SetCursorPosY(ImGui::GetCursorPosY() +5.0f*multiplyRes);
								std::string strsize = Utility::humanSize(thislist[n].size);
								ImVec2 textSize = ImGui::CalcTextSize(strsize.c_str());
								ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetColumnWidth() -textSize.x- ImGui::GetScrollX() - 2 * ImGui::GetStyle().ItemSpacing.x);
								ImGui::TextColored(textcolor,"%s",strsize.c_str());
							}else{
								ImGui::SetCursorPosY(ImGui::GetCursorPosY() +5.0f*multiplyRes);
							}
							
							ImGui::TableSetColumnIndex(2);
							//ImGui::SetCursorPosY(ImGui::GetCursorPosY() +5.0f*multiplyRes);
							
							if(thislist[n].is_valid){
								ImGui::SetCursorPosY(ImGui::GetCursorPosY() +5.0f*multiplyRes);
								std::string strdate = Utility::formatTimeStamp(thislist[n].modified);
								//ImVec2 textSize = ImGui::CalcTextSize(strdate.c_str());
								ImGui::TextColored(textcolor,"%s",strdate.c_str());
							}
							
							
						}
					}
					ImGui::EndTable();
				} else {
					// 缩略图海报墙网格视图 (Grid View)
					float avail_w = ImGui::GetContentRegionAvail().x;
					float spacing = 14.0f * multiplyRes;
					int cols = 4;
					if (avail_w > 0) {
						cols = (int)(avail_w / (270.0f * multiplyRes));
						if (cols < 2) cols = 2;
						if (cols > 5) cols = 5;
					}
					float card_w = (avail_w - (cols - 1) * spacing) / cols;
					float card_h = 205.0f * multiplyRes;
					float thumb_h = 135.0f * multiplyRes;

					ImDrawList* draw_list = ImGui::GetWindowDrawList();
					bool is_dark = (configini->getThemeColor(true) == 0 || (configini->getThemeColor(true) == 2 && Window_Bg_color.x < 0.5f));

					for (size_t n = 0; n < thislist.size(); n++) {
						ImGui::PushID((int)n);
						ImVec2 p = ImGui::GetCursorScreenPos();
						std::string btn_id = "##gridcard" + std::to_string(n);

						bool is_selected = (item.fileHoveredidx == (int)n);
						if (movenavfocus && navfocusstring == thislist[n].name) {
							ImGui::SetKeyboardFocusHere();
						}

						bool clicked = ImGui::Selectable(btn_id.c_str(), is_selected, ImGuiSelectableFlags_None, ImVec2(card_w, card_h));
						bool is_hovered = ImGui::IsItemHovered();
						bool is_focused = ImGui::IsItemFocused();

						if (is_hovered || is_focused) {
							item.fileHoveredidx = (int)n;
						}

						// 1. 卡片底色 (昼夜自适应)
						ImU32 card_bg = is_dark ? IM_COL32(38, 38, 42, 235) : IM_COL32(245, 246, 248, 255);
						if (is_hovered || is_focused) {
							card_bg = is_dark ? IM_COL32(52, 54, 60, 255) : IM_COL32(235, 238, 244, 255);
						}
						draw_list->AddRectFilled(p, ImVec2(p.x + card_w, p.y + card_h), card_bg, 8.0f * multiplyRes);

						// 2. 聚焦高亮外框 (手柄与触控状态)
						if (is_hovered || is_focused) {
							draw_list->AddRect(p, ImVec2(p.x + card_w, p.y + card_h), ImGui::GetColorU32(Active_color), 8.0f * multiplyRes, 0, 3.0f * multiplyRes);
						} else {
							ImU32 border_col = is_dark ? IM_COL32(65, 65, 72, 100) : IM_COL32(215, 218, 224, 180);
							draw_list->AddRect(p, ImVec2(p.x + card_w, p.y + card_h), border_col, 8.0f * multiplyRes, 0, 1.0f);
						}

						// 3. 绘制缩略图
						Texture thumb = GetEntryThumbnail(thislist[n]);
						ImVec2 img_p1 = ImVec2(p.x + 8.0f * multiplyRes, p.y + 8.0f * multiplyRes);
						ImVec2 img_p2 = ImVec2(p.x + card_w - 8.0f * multiplyRes, p.y + 8.0f * multiplyRes + thumb_h);
						draw_list->AddRectFilled(img_p1, img_p2, is_dark ? IM_COL32(20, 20, 24, 255) : IM_COL32(225, 227, 232, 255), 6.0f * multiplyRes);
						draw_list->AddImage((void*)(intptr_t)thumb.id, img_p1, img_p2);

						// 4. 徽标 (目录/大小)
						std::string tag_str = (thislist[n].type == FS::FileEntryType::Directory) ? "目录" : Utility::humanSize(thislist[n].size);
						ImVec2 tag_sz = ImGui::CalcTextSize(tag_str.c_str());
						ImVec2 tag_p1 = ImVec2(img_p2.x - tag_sz.x - 10.0f * multiplyRes, img_p2.y - tag_sz.y - 6.0f * multiplyRes);
						ImVec2 tag_p2 = ImVec2(img_p2.x - 4.0f * multiplyRes, img_p2.y - 4.0f * multiplyRes);
						draw_list->AddRectFilled(tag_p1, tag_p2, IM_COL32(0, 0, 0, 190), 4.0f * multiplyRes);
						draw_list->AddText(ImVec2(tag_p1.x + 3.0f * multiplyRes, tag_p1.y + 1.0f * multiplyRes), IM_COL32(255, 255, 255, 240), tag_str.c_str());

						// 5. 标题文本 (截短显示防超出)
						std::string display_name = thislist[n].name;
						if (display_name.length() > 22) {
							display_name = display_name.substr(0, 19) + "...";
						}
						ImVec2 text_pos = ImVec2(p.x + 10.0f * multiplyRes, p.y + thumb_h + 16.0f * multiplyRes);
						draw_list->AddText(text_pos, ImGui::GetColorU32(Text_color), display_name.c_str());

						if (clicked) {
							HandleOpenEntry(thislist[n], (int)n, changefolder, triggerselect);
						}

						ImGui::PopID();

						if ((n + 1) % cols != 0 && (n + 1) < thislist.size()) {
							ImGui::SameLine(0.0f, spacing);
						}
					}
				}

				movenavfocus = false;
				if(changefolder){
					SetBrowserNav("");
					changefolder = false;
					g_thumb_cache.clear();
				}
				if(item.popupstate == POPUP_STATE_NONE){
					ImGui::SetWindowFocus();
				}
			}
			ImGui::EndChild();
			ImGui::BeginChild("##helpchild",ImVec2(total_w,ImGui::GetContentRegionAvail().y));
			ImGuiWindow* window = ImGui::GetCurrentWindow();
			
			ImGui::Dummy(ImVec2(0,5));
			ImVec2 startpos =  ImGui::GetCursorScreenPos();
			ImGui::Dummy(ImVec2(0,5));
			ImGui::Text(FONT_DPADUP_BUTTON_FILLED);
			ImGui::SameLine();
			ImGui::Text(FONT_DPADDOWN_BUTTON_FILLED);
			ImGui::SameLine();
			ImGui::Text(nxlangs::get_common_str(nxlangs::NXCOMMON_NAVIGATION).c_str());
			ImGui::SameLine();
			ImGui::SetCursorPosX(ImGui::GetCursorPosX()+50.0f*multiplyRes);
			ImGui::Text(FONT_DPADLEFT_BUTTON_FILLED);
			ImGui::SameLine();
			ImGui::Text(nxlangs::get_common_str(nxlangs::NXCOMMON_CHECKBOX).c_str());
			ImGui::SameLine();
			ImGui::SetCursorPosX(ImGui::GetCursorPosX()+50.0f*multiplyRes);
			ImGui::Text(FONT_A_BUTTON_FILLED);
			ImGui::SameLine();
			ImGui::Text(nxlangs::get_common_str(nxlangs::NXCOMMON_SELECTPLAY).c_str());
			ImGui::SameLine();
			ImGui::SetCursorPosX(ImGui::GetCursorPosX()+50.0f*multiplyRes);
			ImGui::Text(FONT_B_BUTTON_FILLED);
			ImGui::SameLine();
			ImGui::Text(nxlangs::get_common_str(nxlangs::NXCOMMON_BACK).c_str());
			ImGui::SameLine();
			ImGui::SetCursorPosX(ImGui::GetCursorPosX()+50.0f*multiplyRes);
			ImGui::Text(FONT_X_BUTTON_FILLED);
			ImGui::SameLine();
			ImGui::Text(nxlangs::get_common_str(nxlangs::NXCOMMON_CONTEXTMENU).c_str());
			ImGui::SameLine();
			ImGui::SetCursorPosX(ImGui::GetCursorPosX()+50.0f*multiplyRes);
			ImGui::Text(FONT_Y_BUTTON_FILLED);
			ImGui::SameLine();
			ImGui::Text(nxlangs::get_common_str(nxlangs::NXCOMMON_HOME).c_str());
			
			window->DrawList->AddLine(startpos,ImVec2(startpos.x+1280*multiplyRes,startpos.y) , ImGui::GetColorU32(ImVec4(1.0f,1.0f,1.0f,1.0f)), 1.0f);
			
			ImGui::EndChild();
			
			/*
			
			if (*first_item && thislist.size() >0) {
				std::string itemid = "##" + std::to_string(0);
				ImGui::SetFocusID(ImGui::GetID(itemid.c_str()), ImGui::GetCurrentWindow());
				ImGui::SetScrollY(0.0f);
				*first_item = false;
			}
			if(*focus){
				if(item.selectionstate == FILE_SELECTION_NONE){
					std::string itemid = "##" + std::to_string(item.fileHoveredidx);
					ImGui::SetFocusID(ImGui::GetID((itemid.c_str())), ImGui::GetCurrentWindow());
				}
				if(item.selectionstate == FILE_SELECTION_CHECKBOX){
					std::string checkitemid = "##check" + std::to_string(item.fileHoveredidx);
					ImGui::SetFocusID(ImGui::GetID((checkitemid.c_str())), ImGui::GetCurrentWindow());
				}
				*focus = false;
			}
			if(triggerselect == true){
				*first_item = true;
			}
			*/
			
			
			
			ImGui::PopStyleColor(2);
			ImGui::PopStyleVar();
        }
        Windows::ExitWindow();
		 
	}
	
	
	void UniUPNPServersWindow(/*bool *focus, bool *first_item*/) {
		Windows::SetupWindow();
		if (ImGui::Begin("UPNP Server List", nullptr, ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_MenuBar )) {
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, NavHover_color);
			ImGui::PushStyleColor(ImGuiCol_NavHighlight, Active_color);
			ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, {0, 5});
			
			float total_w = ImGui::GetContentRegionAvail().x;
			float total_h = ImGui::GetContentRegionAvail().y;
			
			static int selected = -1;	
			std::vector<Device *> thislist = nxupnp->getDevicesList();
			for (unsigned int n = 0; n < thislist.size(); n++){
				std::string itemid = "##" + std::to_string(n);
				/*
				if(thislist[n]->devIcon.id == -1){
							
					MemoryStruct *chunk = (MemoryStruct *)malloc(sizeof(MemoryStruct));
					curlDownloader * curldownload = new curlDownloader();
					curldownload->Download((char *)thislist[n]->iconUrl.c_str(),chunk);
					thislist[n]->devIcon = imgloader->OpenImageMemory(chunk->memory,chunk->size);
					//Utility::TxtLoadFromMemory(chunk->memory,chunk->size,&thislist[n]->devIcon.id,&thislist[n]->devIcon.width,&thislist[n]->devIcon.height);
					free(chunk->memory);
					free(chunk);
					delete curldownload;
							
					}
				*/
				if (ImGui::Selectable(itemid.c_str(), selected == n,0,ImVec2(0,100))){
					thislist[n]->parentList.push_back("0");
					thislist[n]->friendlyparentList.push_back("0");
					
					thislist[n]->browseOID();
					nxupnp->setSelDevice(n);
							
				}
				ImGui::SameLine();
				ImGui::Image((void*)(intptr_t)thislist[n]->devIcon.id, ImVec2(80,80));
				ImGui::SameLine();
				float currstartXpos = ImGui::GetCursorPosX();
				ImGui::SetCursorPosY(ImGui::GetCursorPosY()+20.0f);				
				ImGui::Text("%s",thislist[n]->getfriendlyName().c_str());
				ImGui::SetCursorPosY(ImGui::GetCursorPosY()-60.0f);
				ImGui::SetCursorPosX(currstartXpos);
				ImGui::Text("%s",thislist[n]->getmodelName().c_str());
				ImGui::SetCursorPosX(currstartXpos);
				ImGui::Text("%s",thislist[n]->getmodelDescription().c_str());
				
			}
			
			ImGui::PopStyleColor(2);
			ImGui::PopStyleVar();
		}
		
		
		Windows::ExitWindow();
		
		
	}
	
		
	
	void UniUPNPBrowserWindow(/*bool *focus, bool *first_item*/) {
		 Windows::SetupWindow();
		 if(BrowserTextScroller == nullptr){
			 BrowserTextScroller=new CTextScroller("##filescroller");
		 }
		GUI::cloktimeText(ImVec2((1180.0f*multiplyRes)-ImGui::CalcTextSize(nxmpstats->currentTime).x-(10.0*multiplyRes),2.0f*multiplyRes),true,nxmpstats->currentTime);
		GUI::newbatteryIcon(ImVec2(1180.0f*multiplyRes,2.0f*multiplyRes),true,batteryPercent,40,20,true);
		
		 if (ImGui::Begin("UPNP Browser", nullptr, ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_MenuBar )) {
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, NavHover_color);
			ImGui::PushStyleColor(ImGuiCol_NavHighlight, Active_color);
			ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, {0, 5});
			
			if (ImGui::BeginMenuBar()) {
				
				std::string txtpath = Utility::pathfromarray(nxupnp->getDevice(nxupnp->getSelDevice())->friendlyparentList,1);
				ImGui::Text("current path: %s",txtpath.c_str());
				
				
				ImGui::EndMenuBar();
			}
			float total_w = ImGui::GetContentRegionAvail().x;
			float total_h = ImGui::GetContentRegionAvail().y;
			std::vector<upnpres_struct> thislist = nxupnp->getDevice(nxupnp->getSelDevice())->currentlist;
			
			//std::vector<upnpres_struct> thislist = {};//nxupnp->getDevice(nxupnp->getSelDevice())->currentlist;
			
			
			//std::vector<FS::FileEntry> thislist = filebrowser->getCurrList();
			bool triggerselect = false;
			
			
			ImGui::BeginChild("##tablecontainer",ImVec2(total_w,total_h-45*multiplyRes));
			if(false){
				//ImGui::SetCursorPosX( (ImGui::GetWindowWidth() - ImGui::CalcTextSize("Error Connecting").x) / 2.f);
				//ImGui::TextColored(ImVec4(1.0f,0.0f,0.0f,1.0f),"Error Connecting");
				
				//ImGui::SetCursorPosX( (ImGui::GetWindowWidth() - ImGui::CalcTextSize(filebrowser->errormsg.c_str()).x) / 2.f);
				//ImGui::SetCursorPosY( (ImGui::GetWindowHeight() - ImGui::CalcTextSize(filebrowser->errormsg.c_str()).y) / 2.f);
				//ImGui::Text("%s",filebrowser->errormsg.c_str());
			}else{
				if (ImGui::BeginTable("table1", 2,/*ImGuiTableFlags_RowBg|*/ImGuiTableFlags_ScrollY)){
					ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, (910.0f*multiplyRes -2 * ImGui::GetStyle().ItemSpacing.x)); 
					ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, 125.0f*multiplyRes);
					//ImGui::TableSetupColumn("Date", ImGuiTableColumnFlags_WidthFixed,255.f*multiplyRes);       
					ImGui::TableSetupScrollFreeze(0, 1);
					//ImGui::TableHeadersRow();
					ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
					ImGui::TableSetColumnIndex(0);
					ImGui::Text(nxlangs::get_common_str(nxlangs::NXCOMMON_NAME).c_str());
					ImGui::TableSetColumnIndex(1);
					ImGui::Text(nxlangs::get_common_str(nxlangs::NXCOMMON_SIZE).c_str());
					
					
					ImGuiListClipper clipper;
					clipper.Begin(thislist.size());
					
					
					int navfocusid = 0;
					if(movenavfocus){
						for(int i=0;i<thislist.size();i++){
							//NXLOG::DEBUGLOG("%s %s",thislist[i].name.c_str(),navfocusstring.c_str());
						
							if(thislist[i].name == navfocusstring){
								navfocusid = i;
							}
						}
						//movenavfocus=false;
						clipper.ForceDisplayRangeByIndices( navfocusid, navfocusid+41 );
						 
					}
					
					bool changefolder = false;
					while (clipper.Step())
					{
						for (unsigned int n = clipper.DisplayStart; n < clipper.DisplayEnd; n++){
							static int selected = -1;
							
							if(movenavfocus){
								if(navfocusid == n){
									ImGui::SetKeyboardFocusHere();
									
								}
								
							}
							
							
							ImGui::TableNextRow();
							ImGui::TableSetColumnIndex(0);
							
							std::string itemid = "##" + std::to_string(n);
							
							ImGui::Dummy(ImVec2(0,30 + ImGui::GetStyle().FramePadding.y * 2));
							ImGui::SameLine();
							
							if(item.selectionstate == FILE_SELECTION_CHECKBOX){
								item.selectionstate == FILE_SELECTION_NONE;
								
							}
							
							if(thislist[n].type == UPNPTYPE::UPNPContainer){
								GUI::NXMPImage((void*)(intptr_t)imgloader->icons.FolderTexture.id, ImVec2(30,30));
							}else{
								if(thislist[n].type == UPNPTYPE::UPNPItem){
									GUI::NXMPImage((void*)(intptr_t)imgloader->icons.FileTexture.id, ImVec2(30,30));
								}
							}
							
							
							ImGui::SameLine();
							ImGuiSelectableFlags selectable_flags = ImGuiSelectableFlags_SpanAllColumns;
							if(item.selectionstate != FILE_SELECTION_CHECKBOX){
								if (ImGui::Selectable(itemid.c_str(), selected == n,selectable_flags,ImVec2(0,30.0f*multiplyRes))){
										if(thislist[n].type == UPNPTYPE::UPNPContainer){
										nxupnp->getDevice(nxupnp->getSelDevice())->parentList.push_back(thislist[n].uri);
										nxupnp->getDevice(nxupnp->getSelDevice())->friendlyparentList.push_back(thislist[n].name);
										nxupnp->getDevice(nxupnp->getSelDevice())->browseOID();
										triggerselect = true;
										changefolder = true;
										
										//filebrowser->DirList(thislist[n].path,configini->getshowHidden(false),Utility::getMediaExtensions());
									} else{
										item.laststate = item.state;
										libmpv->loadFileLive(thislist[n].uri,thislist[n].name);
										
									}
								}
							}else{
								ImGui::Dummy(ImVec2(0,ImGui::CalcTextSize("A").y + ImGui::GetStyle().FramePadding.y * 2));
								ImGui::SameLine();
							
							}
							
							if (ImGui::IsItemHovered()){
								if(item.fileHoveredidx != n){
									BrowserTextScroller->ResetPosition();
									
								}
								item.fileHoveredidx = n;
							}
							ImVec4* colors = ImGui::GetStyle().Colors;
							ImVec4 textcolor = colors[ImGuiCol_Text];
							
							ImGui::SameLine();
							//ImGui::SetCursorPosY(ImGui::GetCursorPosY() +5.0f);
							
							std::string bsid =std::string("##bs") + std::to_string(n);
							if(item.fileHoveredidx == n){
								ImGui::Dummy(ImVec2(-10.0,(30.0f*multiplyRes-2 * ImGui::GetStyle().ItemSpacing.y)));
								ImGui::SameLine();
								ImGui::SetCursorPosX(ImGui::GetCursorPosX()-10.0f);
								ImGui::SetCursorPosY(ImGui::GetCursorPosY() +5.0f*multiplyRes);
								BrowserTextScroller->DrawColor(bsid.c_str(),textcolor,(910.0f*multiplyRes -2* ImGui::GetStyle().ItemSpacing.x),(30.0f*multiplyRes-2 * ImGui::GetStyle().ItemSpacing.y),thislist[n].name.c_str());	
							}else{
								ImGui::Dummy(ImVec2(-10.0,(30.0f*multiplyRes-2 * ImGui::GetStyle().ItemSpacing.y)));
								
								ImGui::SameLine();
								ImGui::SetCursorPosX(ImGui::GetCursorPosX()-10.0f);
								ImGui::SetCursorPosY(ImGui::GetCursorPosY()+5.0f);
								ImGui::TextColored(textcolor,"%s",thislist[n].name.c_str());
							}
							ImGui::TableSetColumnIndex(1);
							if(thislist[n].type == UPNPTYPE::UPNPItem){
								ImGui::SetCursorPosY(ImGui::GetCursorPosY() +5.0f*multiplyRes);
								std::string strsize = Utility::humanSize(thislist[n].size);
								ImVec2 textSize = ImGui::CalcTextSize(strsize.c_str());
								ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetColumnWidth() -textSize.x- ImGui::GetScrollX() - 2 * ImGui::GetStyle().ItemSpacing.x);
								ImGui::TextColored(textcolor,"%s",strsize.c_str());
							}else{
								ImGui::SetCursorPosY(ImGui::GetCursorPosY() +5.0f*multiplyRes);
							}
							
							
							
						}
					}
					movenavfocus = false;
					if(changefolder){
						SetBrowserNav("");
						changefolder = false;
					}
					if(item.popupstate == POPUP_STATE_NONE){
						ImGui::SetWindowFocus();
					}
					ImGui::EndTable();
					
				}
			}
			ImGui::EndChild();
			ImGui::BeginChild("##helpchild",ImVec2(total_w,ImGui::GetContentRegionAvail().y));
			ImGuiWindow* window = ImGui::GetCurrentWindow();
			
			ImGui::Dummy(ImVec2(0,5));
			ImVec2 startpos =  ImGui::GetCursorScreenPos();
			ImGui::Dummy(ImVec2(0,5));
			ImGui::Text(FONT_DPADUP_BUTTON_FILLED);
			ImGui::SameLine();
			ImGui::Text(FONT_DPADDOWN_BUTTON_FILLED);
			ImGui::SameLine();
			ImGui::Text(nxlangs::get_common_str(nxlangs::NXCOMMON_NAVIGATION).c_str());
			ImGui::SameLine();
			ImGui::SetCursorPosX(ImGui::GetCursorPosX()+50.0f*multiplyRes);
			ImGui::Text(FONT_DPADLEFT_BUTTON_FILLED);
			ImGui::SameLine();
			ImGui::Text(nxlangs::get_common_str(nxlangs::NXCOMMON_CHECKBOX).c_str());
			ImGui::SameLine();
			ImGui::SetCursorPosX(ImGui::GetCursorPosX()+50.0f*multiplyRes);
			ImGui::Text(FONT_A_BUTTON_FILLED);
			ImGui::SameLine();
			ImGui::Text(nxlangs::get_common_str(nxlangs::NXCOMMON_SELECTPLAY).c_str());
			ImGui::SameLine();
			ImGui::SetCursorPosX(ImGui::GetCursorPosX()+50.0f*multiplyRes);
			ImGui::Text(FONT_B_BUTTON_FILLED);
			ImGui::SameLine();
			ImGui::Text(nxlangs::get_common_str(nxlangs::NXCOMMON_BACK).c_str());
			ImGui::SameLine();
			ImGui::SetCursorPosX(ImGui::GetCursorPosX()+50.0f*multiplyRes);
			ImGui::Text(FONT_X_BUTTON_FILLED);
			ImGui::SameLine();
			ImGui::Text(nxlangs::get_common_str(nxlangs::NXCOMMON_CONTEXTMENU).c_str());
			ImGui::SameLine();
			ImGui::SetCursorPosX(ImGui::GetCursorPosX()+50.0f*multiplyRes);
			ImGui::Text(FONT_Y_BUTTON_FILLED);
			ImGui::SameLine();
			ImGui::Text(nxlangs::get_common_str(nxlangs::NXCOMMON_HOME).c_str());
			
			window->DrawList->AddLine(startpos,ImVec2(startpos.x+1280*multiplyRes,startpos.y) , ImGui::GetColorU32(ImVec4(1.0f,1.0f,1.0f,1.0f)), 1.0f);
			
			ImGui::EndChild();
			
			
			ImGui::PopStyleColor(2);
			ImGui::PopStyleVar();
        }
        Windows::ExitWindow();
		 
	}
	
	

	void USBMountWindow(bool *focus, bool *first_item){
		Windows::SetupWindow();
#ifdef USB_USE_CALLBACK_SYSTEM
		if(MyUSBMount != nullptr)MyUSBMount->usbMscTestDevices();
#endif
        if (ImGui::Begin("USB Mount", nullptr, ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_MenuBar)) {
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, NavHover_color);
			ImGui::PushStyleColor(ImGuiCol_NavHighlight, Active_color);
			ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, {0, 5});
			ImGui::SetNextWindowFocus();
            	float total_w = ImGui::GetContentRegionAvail().x;
				float total_h = ImGui::GetContentRegionAvail().y;
				ImGui::BeginChild("##tablecontainer",ImVec2(total_w,total_h-45*multiplyRes));
				//if (ImGui::BeginListBox("USB Browser Menu",ImVec2(total_w, total_h))){
					bool triggerselect = false;
					static int selected = -1;
					
					//std::vector<usb_devices> thislist = usbmounter->mounted_devs;
					std::vector<usb_devices> thislist = filebrowser->getUsbDev(configini->getEmuOverrides());
					for (unsigned int n = 0; n < thislist.size(); n++){
						std::string itemid = "##" + std::to_string(n);
						
						if (ImGui::Selectable(itemid.c_str(), selected == n,0, ImVec2(0, 80))){
							//item.usbpath = thislist[n].mount_point + std::string("/");
							filebrowser->setBasePath(thislist[n].mount_point + std::string("/"));
							filebrowser->DirList(filebrowser->getBasePath(),configini->getshowHidden(false),Utility::getMediaExtensions());
							triggerselect = true;
							item.state = MENU_STATE_USB_BROWSER;
						}
						float currstartYpos = ImGui::GetCursorPosY();
						ImGui::SameLine();
						ImGui::SetCursorPosY(ImGui::GetCursorPosY()+20.0f);
						ImGui::Image((void*)(intptr_t)imgloader->icons.UsbTexture.id, ImVec2(40,40));
						ImGui::SameLine();
						ImGui::SetCursorPosY(currstartYpos);
						float currstartXpos = ImGui::GetCursorPosX();
						std::string devname = Utility::trim(thislist[n].name)  + std::string(" -> ") + thislist[n].mount_point;
						ImGui::SetCursorPosY(ImGui::GetCursorPosY()-70.0f);
						ImGui::Text("%s",devname.c_str());
						ImGui::SetCursorPosY(ImGui::GetCursorPosY()-40.0f);
						ImGui::SetCursorPosX(currstartXpos);
						ImGui::Text("%s %s",thislist[n].fstype.c_str(),Utility::humanSize(thislist[n].capacity).c_str());
						ImGui::SetCursorPosY(currstartYpos);
					
						if (selected)
							ImGui::SetItemDefaultFocus();
					}
					if (*first_item) {
						std::string itemid = "##" + std::to_string(0);
						ImGui::SetFocusID(ImGui::GetID(itemid.c_str()), ImGui::GetCurrentWindow());
						*first_item = false;
					}
					if(triggerselect == true){
						*first_item = true;
					}
				//}	
				//ImGui::EndListBox();
				
				ImGui::EndChild();
				ImGui::BeginChild("##helpchild",ImVec2(total_w,ImGui::GetContentRegionAvail().y));
				ImGuiWindow* window = ImGui::GetCurrentWindow();
				
				ImGui::Dummy(ImVec2(0,5));
				ImVec2 startpos =  ImGui::GetCursorScreenPos();
				ImGui::Dummy(ImVec2(0,5));
				ImGui::Text(FONT_DPADUP_BUTTON_FILLED);
				ImGui::SameLine();
				ImGui::Text(FONT_DPADDOWN_BUTTON_FILLED);
				ImGui::SameLine();
				ImGui::Text(nxlangs::get_common_str(nxlangs::NXCOMMON_NAVIGATION).c_str());
				ImGui::SameLine();
				ImGui::SetCursorPosX(ImGui::GetCursorPosX()+50.0f*multiplyRes);
				ImGui::Text(FONT_A_BUTTON_FILLED);
				ImGui::SameLine();
				ImGui::Text(nxlangs::get_common_str(nxlangs::NXCOMMON_SELECT).c_str());
				ImGui::SameLine();
				ImGui::SetCursorPosX(ImGui::GetCursorPosX()+50.0f*multiplyRes);
				ImGui::Text(FONT_Y_BUTTON_FILLED);
				ImGui::SameLine();
				ImGui::Text(nxlangs::get_common_str(nxlangs::NXCOMMON_HOME).c_str());
				
				window->DrawList->AddLine(startpos,ImVec2(startpos.x+1280*multiplyRes,startpos.y) , ImGui::GetColorU32(ImVec4(1.0f,1.0f,1.0f,1.0f)), 1.0f);
				
				ImGui::EndChild();
				
				ImGui::PopStyleColor(2);
				ImGui::PopStyleVar();
        }
        Windows::ExitWindow();
	}
}