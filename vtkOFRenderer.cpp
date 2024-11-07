/*Created by Tristan Wellman 2024*/

#include <filesystem>
#include <functional>

#include "vtkOFRenderer.hpp"

using namespace Aftr;

std::vector<std::string> getDirectoryListing(std::string filePath) {
	std::vector<std::string> dirs;
	if(!std::filesystem::is_directory(filePath)) return dirs;
	for (auto& curDir : std::filesystem::recursive_directory_iterator(filePath)) {
		if (curDir.is_directory()) dirs.push_back(curDir.path().string());
	}
	return dirs;
}

std::string splitStr(std::string & str, char delim) {
	int i;
	for (i = 0; i < str.length(); i++) {
		if (str.at(i) == delim) {
			// return the first half of split string and set original to rest of string
			std::string ret = str.substr(0, i);
			str = str.substr(i + 1, str.length());
			return ret;
		}
	}
	return "";
}

std::vector<std::string> vtkOFRenderer::getOpenFoamTimeStamps(std::vector<std::string> dirs) {
	std::vector<std::string> ret;
	for (std::string i : dirs) {
		while(splitStr(i, '/')!="");
		if(splitStr(i, '\\') != "") continue;
		try {
			if(std::stoi(i)||i=="0") ret.push_back(i);
		} catch (const std::invalid_argument &e) { continue; }
	}
	return ret;
}

vtkOFRenderer::vtkOFRenderer(std::string openFoamPath) : filePath(openFoamPath) {
	
	//parser = (vtkParser*)malloc(sizeof(vtkParser*));

	std::vector<std::string> listing = getDirectoryListing(openFoamPath);

	VTKASSERT(!listing.empty(),
		"ERROR:: Failed to open OPENFOAM test case folder");

	timeStamps = getOpenFoamTimeStamps(listing);

	VTKASSERT(!timeStamps.empty(),
		"ERROR:: Failed to retrieve OpenFoam case Time Stamps!");
	
	if (openFoamPath.at(openFoamPath.length() - 1) != '/') openFoamPath += '/';
	for (int i = 0; i < timeStamps.size();i++) {
		std::string fullPath = openFoamPath +
			"postProcessing/streamlines/" + timeStamps.at(i) + "/tracks.vtk";
		tracksFiles.push_back(fullPath);
		std::cout << fullPath << std::endl;
	}
	isReady = false; // will be ready after parser is ran
}

void vtkOFRenderer::parseThread(int index) {
	
	vtkParser* parser = &threadParsers.at(index);
	
	parser->setVtkFile(tracksFiles.at(index));
	//parser->printVTKFILE();
	parser->init();
	parser->parseOpenFoam();
	tracksFileData.push_back(parser->getOpenFoamData());
	//parser->dumpOFOAMPolyDataset();
	parser->freeVtkData();

	threadStates.at(index) = 1;
}

int vtkOFRenderer::parseTracksFiles() {

	threadStates.resize(tracksFiles.size(), 0);
	threadParsers.resize(tracksFiles.size());
	threads.resize(tracksFiles.size());

	int i, finished=0;
	for (i = 0; i < tracksFiles.size(); i++) {
		threads.at(i) = std::thread(std::mem_fn(&vtkOFRenderer::parseThread), this, i);
		VTKLOG("INFO:: Started parser thread for: {}", tracksFiles.at(i));
	}
	
	while (!finished) {
		int totalFinished = 0;
		for (i = 0; i < threadStates.size();i++) {
			if (threadStates.at(i)) totalFinished++;
		}
		if (totalFinished == threadStates.size() - 1) finished = 1;
	}
	for (i = 0; i < tracksFiles.size(); i++) {
		threads.at(i).join();
		VTKLOG("INFO:: Finished parser thread for: {}", tracksFiles.at(i));
	}

	isReady = true;
	return 0;
}

WO *vtkOFRenderer::renderTimeStampTrack() {
	WO* wo = WO::New();
	
	VTKASSERT(currentSelectedTimeStamp != NULL, 
		"ERROR:: Uninitialized vtk timestamps!");



	return wo;
}


/*This must be ran in already initialized WOImGui istance*/
void vtkOFRenderer::renderImGuivtkSettings() {


	static int curTime = 0;
	static const char* curItem = timeStamps.at(0).c_str();

	ImGui::SetNextWindowSize(ImVec2(400, 200));
	if (ImGui::Begin("Vtk View", NULL)) {

		ImGui::BulletText("Select a timestamp to view!");
		if (ImGui::BeginCombo("TimeStamps", curItem)) {
			for (int n = 0; n < timeStamps.size(); n++)
			{
				bool is_selected = (curItem == timeStamps.at(n).c_str()); 
				if (ImGui::Selectable(timeStamps.at(n).c_str(), is_selected))
					curItem = timeStamps.at(n).c_str();
					if (is_selected)
						ImGui::SetItemDefaultFocus(); 
			}
			ImGui::EndCombo();
		}
	}
	ImGui::End();

	currentSelectedTimeStamp = curItem;

}