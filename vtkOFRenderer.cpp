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

std::vector<std::string> vtkOFRenderer::getOpenFoamTimeStamps() {

}

vtkOFRenderer::vtkOFRenderer(std::string openFoamPath) : filePath(openFoamPath) {
	
	std::vector<std::string> listing = getDirectoryListing(openFoamPath);
	if (listing.size() == 0) {
		//TODO see if there is a log system in afterburner
		std::cout << "ERROR:: Failed to open OPENFOAM test case folder" << std::endl;
	}




}

int vtkOFRenderer::parseTracksFiles() {

	int i;
	for (i = 0; i < tracksFiles.size(); i++) {
		setVtkFile(tracksFiles.at(i));
		parseOpenFoam();
		tracksFileData.push_back(getOpenFoamData());
	}

	return 0;
}

void renderTimeStampTrack(int timeStamp) {

}

void renderImGuiTimeSelector() {

}