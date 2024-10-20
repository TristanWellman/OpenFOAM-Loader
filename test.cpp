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
	parser.freeVtkData();
	return 0;
}
