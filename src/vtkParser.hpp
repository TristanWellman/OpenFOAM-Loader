/*Copyright (c) 2024 Tristan Wellman*/

#ifndef VTK_PARSER_HPP
#define VTK_PARSER_HPP

#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>


#if defined __APPLE__
#define FMT_HEADER_ONLY
#endif
#include <fmt/core.h>

#define MAXLINESIZE 100000
#define MAXFILELINES 100000

#define POLYDATANSIZE 3
#define MAXPOLY 100000

#define VTKASSERT(err, ...) \
	if (!(err)) { fprintf(stderr, __VA_ARGS__); exit(1); }

#define VTKLOG(msg, ...) \
	std::cout << fmt::format( \
		"({}-{}):{}", __FILE__, __LINE__, __FUNCTION__)  \
		<< " " << (fmt::format(msg, ## __VA_ARGS__)) << std::endl;

// these types are dataset names used in OpenFoam Vtk files.
enum OFOAMdatasetTypes {
	OFNONE,
	POINTS,
	LINES,
	U,
	K,
	P,
};

class vtkParser {
public:

	enum dataScopes {
		NONE,
		DATASET,
		POINT_DATA,
		CELL_DATA
	};

	enum geometryTypes {
		STRUCTURED_GRID = 4,
		UNSTRUCTURED_GRID,
		POLYDATA,
		STRUCTURED_POINTS,
		RECTILINEAR_GRID,
		FIELD
	};

	struct vtkPoint {
		float x, y, z;
	};
	struct vtkLine {
		std::vector<int> indecies;
	};

	typedef struct {
		std::vector<std::vector<double> > polyData;
		int size; // the 104 number in the .vtk file: POINTS 104 float
		int expandedSize; //  104 * 3 = 312 : expanded
	} vtkPointDataset; // DATASET scope followed by POINTS scope

	typedef struct {
		vtkPointDataset points;
		std::vector<vtkParser::vtkLine> lines;
		vtkPointDataset uMagnitude;

		int depth;
		//std::vector<std::vector<double> > polyDataset;
	} openFoamVtkFileData;

	// constructors
	vtkParser();
	vtkParser(char* vtkFile);

	virtual ~vtkParser() {freeVtkData();}

	template<typename FSTR>
	void setVtkFile(FSTR fileName);
	void printVTKFILE();

	void freeVtkData();
	void freeFileBuffer();
	int init();
	int parseOpenFoam();

	// Enum Template so user can use geometryTypes, dataScopes, or just an int
	template<typename VTKENUM>
	vtkPointDataset getVtkData(VTKENUM dataType, std::string dataName);
	void dumpOFOAMPolyDataset();

	openFoamVtkFileData getOpenFoamData();

private:

	std::string VTKFILE;

	typedef struct {
		char **fileBuffer;
		int lineCount;
		openFoamVtkFileData* foamData;

		int currentScope; // EX: DATASET POLYDATA
		int currentSubScope; // EX: POINT_DATA
	} vtkParseData;

	vtkParseData* globalVtkData;

	void tokenizeDataLine(char* currentLine,
		std::vector<std::string>& ret);

	int polyPointSecParse(vtkParseData* p, vtkPointDataset* data, int line);
	/* This function needs changed in future:
	 * vtk datasets are defined by (name) value type I.E. POINTS 104 float.
	 * this function is only catering to the polyData when it could grab everything for later use.
	 * */
	void getPolyDataset(vtkParseData* data);
};

#endif
