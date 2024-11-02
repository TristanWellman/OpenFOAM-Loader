/*Created by Tristan Wellman 2024*/

#include <filesystem>
#include "vtkOFRenderer.hpp"


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
			if(std::stoi(i)) ret.push_back(i);
		} catch (const std::invalid_argument&) { continue; }
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

}

int vtkOFRenderer::parseTracksFiles() {

	int i;
	for (i = 0; i < tracksFiles.size(); i++) {
		setVtkFile(tracksFiles.at(i));
		printVTKFILE();
		init();
		parseOpenFoam();
		tracksFileData.push_back(getOpenFoamData());
		dumpOFOAMPolyDataset();
		freeVtkData();
	}

	return 0;
}

void renderTimeStampTrack(int timeStamp) {

}

void renderImGuiTimeSelector() {

}