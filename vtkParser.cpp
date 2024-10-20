/*Copyright (c) 2024 Tristan Wellman*/
#include <iostream>
#include <cstdlib>
#include <fstream>
#include <cstring>
#include <memory>
#include "vtkParser.hpp"

vtkParser::vtkParser(char *vtkFile) : VTKFILE(vtkFile){}

void vtkParser::freeVtkData() {
	free(globalVtkData->foamData);
	free(globalVtkData);
}

int vtkParser::init() {

	globalVtkData = (vtkParser::vtkParseData *)malloc(sizeof(vtkParser::vtkParseData));
	globalVtkData->foamData = 
		(vtkParser::openFoamVtkFileData *)malloc(sizeof(vtkParser::openFoamVtkFileData));
	
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
		//std::cout << globalVtkData->fileBuffer[lineCount];
	}
	std::fclose(file);
	return (globalVtkData->lineCount>0);	
}

/* This function needs changed in future:
 * vtk datasets are defined by (name) value type I.E. POINTS 104 float.
 * this function is only catering to the polyData when it could grab everything for later use.
 * */
// WIP
void vtkParser::getPolyDataset(vtkParseData *data) {
	int i;
	int inDataset=0, inPolyPointData=0;
	int totalPoints=0;
	for(i=0;i<data->lineCount;i++) {

		if(inDataset){
			if(inPolyPointData) {

				continue;
			}
			//if in just dataset portion
			char *polyCheck = strtok(data->fileBuffer[i], "POINTS");
			if(polyCheck!=NULL) {
				polyCheck++;
				// polyCheck should be "104 float" with my test file
				char *type = strtok(polyCheck, " ");
				char *count = strstr(polyCheck, " ");
				if(type!=NULL&&count!=NULL) {
					totalPoints = atoi(count++);
					
				} else {
					// invalid or wrong polydata point area
					continue;
				}
			}
		}

		if(strstr(data->fileBuffer[i], "DATASET POLYDATA")) inDataset=1;
	}
}

int vtkParser::parse() {
	int isASCII=0;
	int i;
	// make sure file is readable
	for(i=0;i<globalVtkData->lineCount;i++) {
		if(strstr(globalVtkData->fileBuffer[i], "ASCII")) isASCII = 1;
		if(strstr(globalVtkData->fileBuffer[i], "DATASET POLYDATA")) break;
	}
	if(!isASCII) {
		std::cout << "ERROR:: .vtk file is not ASCII readable!" << std::endl;
		return 0;
	}

	// get poly data and put it into the foamData struct
	getPolyDataset(globalVtkData);	
	return 1;
}
