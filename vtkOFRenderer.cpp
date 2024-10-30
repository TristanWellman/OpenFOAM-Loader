/*Created by Tristan Wellman 2024*/

#include "vtkOFRenderer.hpp"



std::vector<std::string> getDirectoryListing(std::string filePath) {
	
}

std::vector<std::string> vtkOFRenderer::getOpenFoamTimeStamps() {

}

vtkOFRenderer::vtkOFRenderer(std::string openFoamPath) : filePath(openFoamPath) {
	
	std::vector<std::string> listing = getDirectoryListing(openFoamPath);



}