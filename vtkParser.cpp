/*Copyright (c) 2024 Tristan Wellman*/
#include <iostream>
#include <cstdlib>
#include <fstream>
#include <cstring>
#include <memory>
#include <vector>
#include <bits/stdc++.h>
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

void vtkParser::dumpOFOAMPolyDataset() {
	int i,j;
	for(i=0;i<MAXFILELINES;i++) {
		for(j=0;j<POLYDATANSIZE;j++) {
			if(globalVtkData->foamData->polyDataset[i][j]==0) return;
			std::cout << globalVtkData->foamData->polyDataset[i][j] << " ";
		}
		std::cout << std::endl;
	}
}

std::vector<std::string> vtkParser::tokenizeDataLine(char *currentLine) {
	std::vector<std::string> ret;
	std::string line(currentLine);
	std::stringstream strstream(line);
	while(std::getline(strstream, line, ' ')) 
		ret.push_back(line);
	return ret;
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
				std::vector<std::string> tokenizedLine;
				tokenizedLine = tokenizeDataLine(data->fileBuffer[i]);
				if(tokenizedLine.size()<3) {
					std::cout << "ERROR:: invalid poly-array size: line " << i << std::endl;
					return;
				}
				// this bs moves the vector space to polygons.
				// I'm sorry this looks the way it does...
				for(int k=0;k<tokenizedLine.size()&&k<MAXPOLY;k++) {
					if(data->foamData->polyDataset[k][0]!=0) continue;
					for(int j=0;j<POLYDATANSIZE&&(k*POLYDATANSIZE+j)<tokenizedLine.size();j++) {
						if(data->foamData->polyDataset[k][j]==0) {
							if(k*POLYDATANSIZE+j<=tokenizedLine.size()&&
									!tokenizedLine[k*POLYDATANSIZE+j].empty()) {
								std::string tmp = tokenizedLine[k*POLYDATANSIZE+j];
								//std::cout << tokenizedLine[k*POLYDATANSIZE+j] << " ";
								data->foamData->polyDataset[k][j] = std::stod(tmp);
							} else break;
						} 
					}
				}
				continue;
			}
			//if in just dataset portion
			char *polyCheck = strtok(data->fileBuffer[i], "POINTS");
			if(polyCheck!=NULL) {
				polyCheck++;
				// polyCheck should be "104 float" with my test file
				char tmp[MAXLINESIZE];
				snprintf(tmp, sizeof(tmp), "%s", polyCheck);
				char *count = strtok(tmp, " ");
				char *type = strstr(polyCheck, " ");
				if(count!=NULL&&type!=NULL) {
					type++;
					inPolyPointData=1;
					totalPoints = atoi(count++);
					// todo make use of count and type for different geometry types
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
