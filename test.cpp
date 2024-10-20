#include <iostream>
#include "vtkParser.hpp"

using namespace std;

int main() {
	vtkParser parser((char *)"tracks.vtk");
	if(!parser.init()) {
		cout << "Error: failed to initialize file!" << endl;
		exit(1);
	};	
	parser.parse();	
	parser.freeVtkData();
	return 0;
}
