/*Copyright (c) 2024 Tristan Wellman*/
#include <iostream>
#include <cstdlib>
#include <fstream>
#include <cstring>
#include <memory>
#include <vector>
#include <sstream>
#include <type_traits>

#include "vtkParser.hpp"

vtkParser::vtkParser(){}
vtkParser::vtkParser(char *vtkFile) : VTKFILE(vtkFile){}

template<typename FSTR>
void vtkParser::setVtkFile(FSTR fileName) {	
	/* I'm not deleting this cuz it was almost beautiful...
	
	void *strptr = static_cast<void *>(fileName); 
	std::string *tester = static_cast<std::string *>(strptr);
	std::string outStr = *tester;
	if(tester) { //it's an std::string
		VTKFILE = static_cast<char *>(outStr.c_str());
	} else VTKFILE = static_cast<char *>(fileName);

	*/

	std::string str(fileName);
	VTKFILE = (char *)str.c_str();

}
template void vtkParser::setVtkFile<std::string>(std::string);
template void vtkParser::setVtkFile<char *>(char *);
template void vtkParser::setVtkFile<const char *>(const char *);


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
	std::cout << "Total Polys: " << globalVtkData->foamData->points.size << std::endl;

	for(i=0;i<globalVtkData->foamData->points.size;i++) {
		for(j=0;j<POLYDATANSIZE;j++) {
			if(globalVtkData->foamData->points.polyData[i].empty()) continue;
			std::cout << globalVtkData->foamData->points.polyData[i][j] << " ";
		}
		std::cout << std::endl;
	}
}

vtkParser::openFoamVtkFileData vtkParser::getOpenFoamData() {
	return *globalVtkData->foamData;
}

std::vector<std::string> vtkParser::tokenizeDataLine(char *currentLine) {
	std::vector<std::string> ret;
	std::string line(currentLine);
	std::stringstream strstream(line);
	while(std::getline(strstream, line, ' ')) 
		ret.push_back(line);
	return ret;
}

void vtkParser::polyPointSecParse(vtkParseData *data, int lineNum) {
	
	// initialize 2D vector
	int i,j;
	if(data->foamData->points.polyData.size()==0) {
		data->foamData->points.polyData.resize(
				data->foamData->points.size);
		//for(i=0;i<data->foamData->points.size;i++) 
		//	data->foamData->points.polyData[i].resize(POLYDATANSIZE);
	}
	std::vector<std::string> tokenizedLine;
	tokenizedLine = tokenizeDataLine(data->fileBuffer[lineNum]);	

	int startingVec=0;
	for(i=0;i<data->foamData->points.polyData.size();i++) {
		if(data->foamData->points.polyData[i].empty()) {
			if(i>0&&data->foamData->points.polyData[i-1][2]==0) i--;
			break;
		}
	}
	startingVec = i;
	//std::cout << startingVec << std::endl;
	if(tokenizedLine.size()==2) {
		data->foamData->points.polyData[startingVec].push_back(
				std::stod(tokenizedLine[0]));
		return;
	}
	for(i=0;i<tokenizedLine.size()&&
			i+startingVec<data->foamData->points.size;i++) {
		for(j=0;j<POLYDATANSIZE;j++) {
			if(tokenizedLine[i*POLYDATANSIZE+j].empty()) return;

			data->foamData->points.polyData[i+startingVec].push_back(
					std::stod(tokenizedLine[i*POLYDATANSIZE+j]));
		}
	}
	
}

int checkDataScope(char *line) {
	//std::cout << line << std::endl;
	std::string str(line);
	if(str.find("DATASET")!= std::string::npos||
			str.find("POINT_DATA")!= std::string::npos||
			str.find("CELL_DATA")!= std::string::npos ||
			str.find("LINES")!= std::string::npos) return 1;
	return 0;
} 

/* This function needs changed in future:
 * vtk datasets are defined by (name) value type I.E. POINTS 104 float.
 * this function is only catering to the polyData when it could grab everything for later use.
 * */
// WIP
void vtkParser::getPolyDataset(vtkParseData *data) {
	
	vtkParser::dataScopes currentDataScope = NONE;
	vtkParser::OFOAMdatasetTypes currentDatasetType = OFNONE;
	
	int i;

	int inDataset=0, inPolyPointData=0;
	int totalPoints=0;
	for(i=0;i<data->lineCount;i++) {

		if(currentDataScope == DATASET){
			
			// run the parser for point data in vtk file
			if(currentDatasetType == POINTS) {
				if(checkDataScope(data->fileBuffer[i])) break;
				polyPointSecParse(data, i);
				data->foamData->depth++;
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
					currentDatasetType = POINTS;

					data->foamData->points.size = atoi(count++);

					data->foamData->points.expandedSize =
						data->foamData->points.size * 3;
				} else {
					// invalid or wrong polydata point area
					continue;
				}
			}
		}

		if(strstr(data->fileBuffer[i], "DATASET POLYDATA")) currentDataScope = DATASET;
	}
}

template<typename VTKENUM>
vtkParser::vtkPointDataset vtkParser::getVtkData(VTKENUM dataType, std::string dataName) {

	vtkParser::vtkPointDataset ret;
	int T = dataType;
	if(T==DATASET) {

	} else if(T==POINT_DATA) {

	} else if(T==CELL_DATA) {

	} else if(T==STRUCTURED_GRID) {

	} else if(T==UNSTRUCTURED_GRID) {

	} else if(T==POLYDATA) {

	} else if(T==STRUCTURED_POINTS) {

	} else if(T==RECTILINEAR_GRID) {

	} else if(T==FIELD) {

	}

	return ret;	
}
template vtkParser::vtkPointDataset vtkParser::getVtkData<int>(int, std::string);
template vtkParser::vtkPointDataset vtkParser::getVtkData<vtkParser::dataScopes>(
		vtkParser::dataScopes, std::string);
template vtkParser::vtkPointDataset vtkParser::getVtkData<vtkParser::geometryTypes>(
		vtkParser::geometryTypes, std::string);

int vtkParser::parseOpenFoam() {
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
	globalVtkData->foamData->points	= 
		vtkParser::getVtkData<vtkParser::dataScopes>(DATASET, "POINTS");
	globalVtkData->foamData->lines = 
		vtkParser::getVtkData<vtkParser::dataScopes>(DATASET, "LINES");
	globalVtkData->foamData->u_velocity = 
		vtkParser::getVtkData<vtkParser::dataScopes>(POINT_DATA, "U");

	// this is temporary while I work on getVtkData
	getPolyDataset(globalVtkData);	

	return 1;
}




