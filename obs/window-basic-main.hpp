/******************************************************************************
    Copyright (C) 2013 by Hugh Bailey <obs.jim@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#pragma once

#include "forms/OBSWindows.h"

#include <obs.hpp>

#include <unordered_map>

class OBSBasic : public OBSBasicBase {
	std::unordered_map<obs_source_t, int> sourceSceneRefs;

	obs_scene_t GetCurrentScene();
	void AddSceneItem(obs_sceneitem_t item);
	void RemoveSceneItem(obs_sceneitem_t item);
	void AddScene(obs_source_t scene);
	void RemoveScene(obs_source_t scene);
	void UpdateSources(obs_scene_t scene);
	void UpdateSceneSelection(obs_source_t source);

	/* OBS Callbacks */
	static void SceneItemAdded(void *data, calldata_t params);
	static void SceneItemRemoved(void *data, calldata_t params);
	static void SourceAdded(void *data, calldata_t params);
	static void SourceDestroyed(void *data, calldata_t params);
	static void ChannelChanged(void *data, calldata_t params);

	void ResizePreview(uint32_t cx, uint32_t cy);

	void AddSource(obs_scene_t scene, const char *id);
	void AddSourcePopupMenu();

	bool InitGraphics();
	bool InitAudio();

	void NewProject();
	void SaveProject();
	void LoadProject();

protected:
	/* wxWidgets callbacks */
	virtual void OnClose(wxCloseEvent &event);
	virtual void OnMinimize(wxIconizeEvent &event);
	virtual void OnSize(wxSizeEvent &event);
	virtual void OnResizePreview(wxSizeEvent &event);
	virtual void fileNewClicked(wxCommandEvent &event);
	virtual void fileOpenClicked(wxCommandEvent &event);
	virtual void fileSaveClicked(wxCommandEvent &event);
	virtual void fileExitClicked(wxCommandEvent &event);
	virtual void scenesClicked(wxCommandEvent &event);
	virtual void scenesRDown(wxMouseEvent &event);
	virtual void sceneAddClicked(wxCommandEvent &event);
	virtual void sceneRemoveClicked(wxCommandEvent &event);
	virtual void scenePropertiesClicked(wxCommandEvent &event);
	virtual void sceneUpClicked(wxCommandEvent &event);
	virtual void sceneDownClicked(wxCommandEvent &event);
	virtual void sourcesClicked(wxCommandEvent &event);
	virtual void sourcesToggled(wxCommandEvent &event);
	virtual void sourcesRDown(wxMouseEvent &event);
	virtual void sourceAddClicked(wxCommandEvent &event);
	virtual void sourceRemoveClicked(wxCommandEvent &event);
	virtual void sourcePropertiesClicked(wxCommandEvent &event);
	virtual void sourceUpClicked(wxCommandEvent &event);
	virtual void sourceDownClicked(wxCommandEvent &event);
	virtual void settingsClicked(wxCommandEvent &event);
	virtual void exitClicked(wxCommandEvent &event);

public:
	inline OBSBasic() : OBSBasicBase(NULL) {}
	virtual ~OBSBasic();

	bool Init();

	inline wxPanel *GetPreviewPanel() {return previewPanel;}
	inline wxSizer *GetPreviewContainer() {return previewContainer;}
};
