/*Copyright (c) 2024 Tristan Wellman*/
#include <iostream>
#include "vtkParser.hpp"

using namespace std;

int main() {
	vtkParser parser((char *)"tracks.vtk");
	if(!parser.init()) {
		cout << "Error: failed to initialize file!" << endl;
		exit(1);
	};	
	if(!parser.parse()) {
		cout << "Error: failed to parse file!" << endl;
		exit(1);
	}

	// parser.dumpOFOAMPolyDataset();
	
	// manually dump dataset points
	vtkParser::openFoamVtkFileData OFD = parser.getOpenFoamData();
	int i,j;
	std::cout << "Total Polys: " << OFD.points.size << std::endl;

	for(i=0;i<OFD.points.size;i++) {
		for(j=0;j<POLYDATANSIZE;j++) {
			if(OFD.points.polyData[i].empty()) continue;
			std::cout << OFD.points.polyData[i][j] << " ";
		}
		std::cout << std::endl;
	}


	parser.freeVtkData();
	return 0;
}
