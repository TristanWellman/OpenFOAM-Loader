#include <iostream>
#include <cstdlib>
#include <fstream>
#include "vtkParser.hpp"

void vtkParser::init() {
	//vtkSectionData<int, 25> polyData;
	std::ifstream  file;
	file.open(vtkParser::VTKFILE);
	
	// test out
	std::string test;
	while(!file.eof()) {
		std::getline(file, test);
		std::cout << test << std::endl;
	}

}
