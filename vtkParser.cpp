#include <iostream>
#include <cstdlib>
#include <fstream>
#include <cstring>
#include <memory>
#include "vtkParser.hpp"

vtkParser::vtkParser(char *vtkFile) : VTKFILE(vtkFile){}

void vtkParser::freeVtkData() {
	free(globalVtkData);
}


int vtkParser::init() {
	// ptr so I don't have to type all that shit out every time
	globalVtkData = (vtkParser::vtkParseData *)malloc(sizeof(vtkParser::vtkParseData));
	//vtkSectionData<int, 25> polyData;
	std::FILE *file = std::fopen(
			vtkParser::VTKFILE, "r");
	if(file==NULL) return 0;	
	int lineCount = 0;
	char line[256];
	for(;fgets(line,sizeof(line),file)!=NULL;lineCount++);
	
	std::fseek(file, 0, SEEK_SET);
	globalVtkData->lineCount = lineCount;
	
	// copy data from file to fileBuffer
	for(lineCount=0;fgets(line,sizeof(line),file)!=NULL;lineCount++) {
		int i=0;
		for(i=0;i<MAXLINESIZE&&line[i]!='\0';i++) {
			globalVtkData->fileBuffer[lineCount][i] = line[i];
		}
		std::cout << globalVtkData->fileBuffer[lineCount];
	}
	std::fclose(file);
	return (globalVtkData->lineCount>0);	
}

void vtkParser::parse() {

	int i;
	for(i=0;i<globalVtkData->lineCount;i++) {
		std::cout << globalVtkData->fileBuffer[i];
	}
}
