/*Copyright (c) 2024 Tristan Wellman*/
#include <iostream>
#include <cstdlib>
#include <fstream>
#include <cstring>
#include <memory>
#include <vector>
#include <sstream>
#include <type_traits>
#include <unordered_map>
#include <stdexcept>

#include "vtkParser.hpp"

vtkParser::vtkParser() {}
vtkParser::vtkParser(char* vtkFile) : VTKFILE(vtkFile) {}

template<typename FSTR>
void vtkParser::setVtkFile(FSTR fileName) {
	std::string str(fileName);
	try { 
		VTKFILE = std::string(str);
	} catch (char* e) {
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
	if (globalVtkData && globalVtkData->foamData) {
		delete globalVtkData->foamData;
		globalVtkData->foamData = nullptr;
	}
	delete globalVtkData;
	globalVtkData = nullptr;
}

int vtkParser::init() {

	globalVtkData = new vtkParseData;
	globalVtkData->foamData = new openFoamVtkFileData;
	std::FILE* file = std::fopen(
		VTKFILE.c_str(), "r");

	if (file == NULL) {
		VTKLOG("ERROR:: Failed to Open File {}", VTKFILE);
		exit(1);
	}

	int lineCount = 0;
	// I had to up this to 100000 because the .vtk files sometimes contain stupidly large lines for one dataset.
	char line[100000];

	std::fseek(file, 0, SEEK_SET);

	// copy data from file to fileBuffer
	for (lineCount = 0; fgets(line, sizeof(line), file) != NULL; lineCount++) {
		int i = 0;
		for (i = 0; i < MAXLINESIZE && line[i] != '\0'; i++) {
			globalVtkData->fileBuffer[lineCount][i] = line[i];
			globalVtkData->fileBuffer[lineCount][i+1] = '\0';
		}
		//std::cout << globalVtkData->fileBuffer[lineCount];
	}
	globalVtkData->lineCount = lineCount;
	std::fclose(file);

	VTKASSERT(
		lineCount > 0,
		"Error:: OpenFoam File Buffer empty!\n");


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
	if (!globalVtkData || !globalVtkData->foamData) {
		throw std::runtime_error("ERROR: Attempted to access uninitialized globalVtkData->foamData");
	}
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
// We do not do anything fancy for performance because we only skip 5 lines for this purpose.
// This means that indexing and things for entire files actually make it slower. We only use once.
void seekToLine(std::ifstream& stream, int lineNum) {
	std::string line;
	for (int i = 0; i < lineNum && std::getline(stream, line); i++);
}

void vtkParser::tokenizeDataLine(char* currentLine,
	std::vector<std::string> &ret) {

	std::string line(currentLine);
	std::istringstream stream(line);
	ret = { std::istream_iterator<std::string>(stream), std::istream_iterator<std::string>() };
	std::stringstream strstream(line);
}


int vtkParser::polyPointSecParse(vtkParseData* p, vtkPointDataset* data, int lineNum) {

	if (data == nullptr) {
		VTKLOG("ERROR:: vtk parse data struct is nullptr!");
		return lineNum;
	}

	std::string dataBuffer;

	// initialize 2D vector
	int i, j, finalLineNum = lineNum;
	if (data->polyData.empty()) {
		data->polyData =
			std::vector<std::vector<double> >{};
		data->polyData.resize(
			data->size);
	}

	std::ifstream file(VTKFILE);
	VTKASSERT(!file.fail(), "ERROR:: Failed to re-open vtk file!");

	std::vector<std::string> pointBufferLines;
	pointBufferLines.reserve(data->size);
	std::string curLine;
	int totalSize = 0;

	// get all the data lines
	seekToLine(file, lineNum);
	for (i = 0; i < data->size && getline(file, curLine); i++) {
		if (checkDataScope(p->fileBuffer[lineNum + i])) break;
		//std::cout << i << ": " << curLine << "\n";
		pointBufferLines.push_back(curLine);
		totalSize += curLine.length();
	}
	finalLineNum = i;

	// reserve and concat all strings to line buffer
	dataBuffer.reserve(totalSize);
	for (auto& str : pointBufferLines) {
		if (str.at(str.length() - 1) == '\n') str = str.substr(0, str.size() - 1);
		dataBuffer += str;
		if (str.at(str.length() - 1) != ' ') dataBuffer += ' ';
	}

	// tokenize the file buffer line into individual values
	std::vector<std::string> tokenizedLine;
	tokenizeDataLine((char*)dataBuffer.c_str(), tokenizedLine);

	for (i = 0; i < tokenizedLine.size() &&
		i < data->size; i++) {
		data->polyData[i].reserve(POLYDATANSIZE);
		for (j = 0; j < POLYDATANSIZE; j++) {
			if (tokenizedLine[i * POLYDATANSIZE + j].empty()) return finalLineNum;
			if (tokenizedLine[i * POLYDATANSIZE + j] == "\n") continue;
			int pos = (i * POLYDATANSIZE + j);
			data->polyData[i].push_back(
				std::stod(tokenizedLine[pos]));
		}
	}

	file.close();
	return finalLineNum;
}

/*This is the function that obtains all the data for a set in the dataset I.E. POINTS*/
void vtkParser::getPolyDataset(vtkParseData* data) {

	vtkParser::dataScopes currentDataScope = NONE;
	OFOAMdatasetTypes currentDatasetType = OFNONE;

	int i;

	int inDataset = 0, inPolyPointData = 0;
	int totalPoints = 0;
	for (i = 0; i < data->lineCount; i++) {

		if (currentDataScope == DATASET) {

			// run the parser for point data in vtk file
			if (currentDatasetType == POINTS) {
				if(checkDataScope(data->fileBuffer[i])) break;
				int skip = polyPointSecParse(data, &data->foamData->points, i);
				i += skip;
				data->foamData->depth++;
				currentDatasetType = OFNONE;
				continue;
			} else if(currentDatasetType == U) {
				/*Get the Magnitude to be used for coloring*/
				if(checkDataScope(data->fileBuffer[i])) break;
				polyPointSecParse(data, &data->foamData->uMagnitude, i);
				break;
			}

			//check subscopes
			std::string *line = new std::string(data->fileBuffer[i]);
			if (line->find("POINTS")!=std::string::npos) {
				// I know strtok treats each letter as a delim but it works so...
				char *polyCheck = strtok(data->fileBuffer[i], "POINTS");
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
				} else continue;

			} else {
				if(line->find("U ") != std::string::npos) {
					// U 3 11015 float
					std::vector<std::string> tokens;
					tokenizeDataLine(data->fileBuffer[i], tokens);

					data->foamData->uMagnitude.size = std::stoi(tokens.at(2));

					data->foamData->uMagnitude.expandedSize =
						data->foamData->points.size * 3;

					currentDatasetType = U;
				}
			}
			delete line;
		}

		if (strstr(data->fileBuffer[i], "DATASET POLYDATA")) currentDataScope = DATASET;
	}
}

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

	getPolyDataset(globalVtkData);

	return 1;
}
