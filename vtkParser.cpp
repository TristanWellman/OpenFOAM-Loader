#include <iostream>
#include <cstdlib>
#include <fstream>
#include "vtkParser.hpp"

vtkParser::vtkParser(char *vtkFile) : VTKFILE(vtkFile){}

void vtkParser::freeVtkData() {
	free(globalVtkData.fileBuffer);
}

int vtkParser::init() {
	// ptr so I don't have to type all that shit out every time
	vtkParseData *vtkDataPtr;
	vtkDataPtr = &globalVtkData;
	//vtkSectionData<int, 25> polyData;
	std::FILE *file = std::fopen(
			vtkParser::VTKFILE, "r");
	if(file==NULL) return 0;	
	int lineCount = 0;
	char line[100];
	for(;fgets(line,sizeof(line),file)!=NULL;lineCount++);
	
	std::fseek(file, 0, SEEK_SET);
	vtkDataPtr->fileBuffer = (char **)malloc(sizeof(char*)*lineCount);
	vtkDataPtr->lineCount = lineCount;
	
	// copy data from file to fileBuffer
	for(lineCount=0;fgets(line,sizeof(line),file)!=NULL;lineCount++) {
		vtkDataPtr->fileBuffer[lineCount] = line;
		std::cout << vtkDataPtr->fileBuffer[lineCount];
	}
	std::fclose(file);
	return (vtkDataPtr->fileBuffer!=NULL);	
}

void vtkParser::parse() {

	int i;
	for(i=0;i<globalVtkData.lineCount;i++) {
		std::cout << globalVtkData.fileBuffer[i] << std::endl;
	}
}
