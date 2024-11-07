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

vtkParser::vtkParser() {}
vtkParser::vtkParser(char* vtkFile) : VTKFILE(vtkFile) {}

template<typename FSTR>
void vtkParser::setVtkFile(FSTR fileName) {
	std::string str(fileName);
	try { VTKFILE = str; }
	catch (char* e) {
		VTKASSERT(e == NULL,
			"ERROR:: need to malloc vtkParser ptr objects\n%s\n", e);
	}
}
template void vtkParser::setVtkFile<std::string>(std::string);
template void vtkParser::setVtkFile<char*>(char*);
template void vtkParser::setVtkFile<const char*>(const char*);

void vtkParser::printVTKFILE() {
	std::cout << VTKFILE << std::endl;
}

void vtkParser::freeVtkData() {

	delete globalVtkData->foamData;
	delete globalVtkData;

}

int vtkParser::init() {

	globalVtkData = new vtkParseData;
	globalVtkData->foamData = new openFoamVtkFileData;

	std::FILE* file = std::fopen(
		VTKFILE.c_str(), "r");

	VTKASSERT(
		file != NULL,
		"ERROR:: Failed to Open file : %s\n", VTKFILE);

	int lineCount = 0;
	char line[256];
	for (; fgets(line, sizeof(line), file) != NULL; lineCount++);
	VTKASSERT(
		lineCount > 0,
		"Error:: OpenFoam File Buffer empty!\n");

	std::fseek(file, 0, SEEK_SET);
	globalVtkData->lineCount = lineCount;

	// copy data from file to fileBuffer
	for (lineCount = 0; fgets(line, sizeof(line), file) != NULL; lineCount++) {
		int i = 0;
		for (i = 0; i < MAXLINESIZE && line[i] != '\0'; i++) {
			globalVtkData->fileBuffer[lineCount][i] = line[i];
		}
		//std::cout << globalVtkData->fileBuffer[lineCount];
	}
	std::fclose(file);


	return (globalVtkData->lineCount > 0);
}

void vtkParser::dumpOFOAMPolyDataset() {
	int i, j;
	std::cout << "Total Polys: " << globalVtkData->foamData->points.size << std::endl;

	for (i = 0; i < globalVtkData->foamData->points.size; i++) {
		for (j = 0; j < POLYDATANSIZE; j++) {
			if (globalVtkData->foamData->points.polyData.at(i).empty() ||
				globalVtkData->foamData->points.polyData.at(i).size() <= j) continue;
			//std::cout << globalVtkData->foamData->points.polyData[i][j] << " ";
			VTKLOG("{}", globalVtkData->foamData->points.polyData.at(i).at(j));
		}
		std::cout << "Poly " << i << std::endl;
	}
}

vtkParser::openFoamVtkFileData vtkParser::getOpenFoamData() {
	return *globalVtkData->foamData;
}

int checkDataScope(char* line) {
	//std::cout << line << std::endl;
	std::string str(line);
	if (str.find("DATASET") != std::string::npos ||
		str.find("POINT_DATA") != std::string::npos ||
		str.find("CELL_DATA") != std::string::npos ||
		str.find("LINES") != std::string::npos) return 1;
	return 0;
}

// util function to seek to line in ifstream
void seekToLine(std::ifstream& stream, int lineNum) {
	std::string line;
	for (int i = 0; i < lineNum && std::getline(stream, line); i++);
}

std::vector<std::string> vtkParser::tokenizeDataLine(char* currentLine) {
	std::vector<std::string> ret;
	std::string line(currentLine);
	std::stringstream strstream(line);
	while (std::getline(strstream, line, ' '))
		ret.push_back(line);
	return ret;
}

void vtkParser::polyPointSecParse(vtkParseData* data, int lineNum) {

	if (data == nullptr ||
		data->foamData == nullptr) {
		VTKLOG("ERROR:: vtk parse data struct is nullptr!");
		return;
	}

	std::string dataBuffer;

	// initialize 2D vector
	int i, j;
	if (data->foamData->points.polyData.empty()) {
		data->foamData->points.polyData =
			std::vector<std::vector<double> >{};
		data->foamData->points.polyData.resize(
			data->foamData->points.size);
	}

	std::ifstream file(VTKFILE);
	VTKASSERT(!file.fail(), "ERROR:: Failed to re-open vtk file!");

	std::vector<std::string> pointBufferLines;
	std::string curLine;
	int totalSize = 0;

	// get all the data lines
	seekToLine(file, lineNum);
	for (i = 0; i < data->foamData->points.size && getline(file, curLine); i++) {
		if (checkDataScope(data->fileBuffer[lineNum + i])) break;
		pointBufferLines.push_back(curLine);
		totalSize += curLine.length();
	}

	// reserve and concat all strings to line buffer
	dataBuffer.reserve(totalSize);
	for (auto& str : pointBufferLines) {
		if (str.at(str.length() - 1) == '\n') str = str.substr(0, str.size() - 1);
		dataBuffer += str;
		if (str.at(str.length() - 1) != ' ') dataBuffer += ' ';
	}

	// tokenize the file buffer line into individual values
	std::vector<std::string> tokenizedLine;
	tokenizedLine = tokenizeDataLine((char*)dataBuffer.c_str());

	for (i = 0; i < tokenizedLine.size() &&
		i < data->foamData->points.size; i++) {
		for (j = 0; j < POLYDATANSIZE; j++) {
			if (tokenizedLine[i * POLYDATANSIZE + j].empty()) return;
			if (tokenizedLine[i * POLYDATANSIZE + j] == "\n") continue;
			int pos = (i * POLYDATANSIZE + j);
			data->foamData->points.polyData[i].push_back(
				std::stod(tokenizedLine[pos]));
		}
	}

	file.close();

}

/* This function needs changed in future:
 * vtk datasets are defined by (name) value type I.E. POINTS 104 float.
 * this function is only catering to the polyData when it could grab everything for later use.
 * */
 // WIP
void vtkParser::getPolyDataset(vtkParseData* data) {

	vtkParser::dataScopes currentDataScope = NONE;
	vtkParser::OFOAMdatasetTypes currentDatasetType = OFNONE;

	int i;

	int inDataset = 0, inPolyPointData = 0;
	int totalPoints = 0;
	for (i = 0; i < data->lineCount; i++) {

		if (currentDataScope == DATASET) {

			// run the parser for point data in vtk file
			if (currentDatasetType == POINTS) {
				if (checkDataScope(data->fileBuffer[i])) break;
				polyPointSecParse(data, i);
				data->foamData->depth++;
				break;
			}

			//if in just dataset portion
			char* polyCheck = strtok(data->fileBuffer[i], "POINTS");
			if (polyCheck != NULL) {
				polyCheck++;
				// polyCheck should be "104 float" with my test file
				char tmp[MAXLINESIZE];
				snprintf(tmp, sizeof(tmp), "%s", polyCheck);
				char* count = strtok(tmp, " ");
				char* type = strstr(polyCheck, " ");
				if (count != NULL && type != NULL) {
					type++;
					currentDatasetType = POINTS;

					data->foamData->points.size = atoi(count++);

					data->foamData->points.expandedSize =
						data->foamData->points.size * 3;
				}
				else {
					// invalid or wrong polydata point area
					continue;
				}
			}
		}

		if (strstr(data->fileBuffer[i], "DATASET POLYDATA")) currentDataScope = DATASET;
	}
}

template<typename VTKENUM>
vtkParser::vtkPointDataset vtkParser::getVtkData(VTKENUM dataType, std::string dataName) {

	vtkParser::vtkPointDataset ret;
	int T = dataType;
	if (T == DATASET) {

	}
	else if (T == POINT_DATA) {

	}
	else if (T == CELL_DATA) {

	}
	else if (T == STRUCTURED_GRID) {

	}
	else if (T == UNSTRUCTURED_GRID) {

	}
	else if (T == POLYDATA) {

	}
	else if (T == STRUCTURED_POINTS) {

	}
	else if (T == RECTILINEAR_GRID) {

	}
	else if (T == FIELD) {

	}

	return ret;
}
template vtkParser::vtkPointDataset vtkParser::getVtkData<int>(int, std::string);
template vtkParser::vtkPointDataset vtkParser::getVtkData<vtkParser::dataScopes>(
	vtkParser::dataScopes, std::string);
template vtkParser::vtkPointDataset vtkParser::getVtkData<vtkParser::geometryTypes>(
	vtkParser::geometryTypes, std::string);

int vtkParser::parseOpenFoam() {
	int isASCII = 0;
	int i;
	// make sure file is readable
	for (i = 0; i < globalVtkData->lineCount; i++) {
		if (strstr(globalVtkData->fileBuffer[i], "ASCII")) isASCII = 1;
		if (strstr(globalVtkData->fileBuffer[i], "DATASET POLYDATA")) break;
	}
	if (!isASCII) {
		std::cout << "ERROR:: .vtk file is not ASCII readable!" << std::endl;
		return 0;
	}

	// get poly data and put it into the foamData struct
	/*globalVtkData->foamData->points =
		vtkParser::getVtkData<vtkParser::dataScopes>(DATASET, "POINTS");
	globalVtkData->foamData->lines =
		vtkParser::getVtkData<vtkParser::dataScopes>(DATASET, "LINES");
	globalVtkData->foamData->u_velocity =
		vtkParser::getVtkData<vtkParser::dataScopes>(POINT_DATA, "U");*/

		// this is temporary while I work on getVtkData
	getPolyDataset(globalVtkData);

	return 1;
}
