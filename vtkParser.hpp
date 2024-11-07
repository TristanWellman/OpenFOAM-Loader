/*Copyright (c) 2024 Tristan Wellman*/

#ifndef VTK_PARSER_HPP
#define VTK_PARSER_HPP

#include <iostream>
#include <vector>

#if defined __APPLE__
#define FMT_HEADER_ONLY
#endif
#include <fmt/core.h>

#define MAXLINESIZE 256
#define MAXFILELINES 100000

#define POLYDATANSIZE 3
#define MAXPOLY 100000

#define VTKASSERT(err, ...) \
	if (!(err)) { fprintf(stderr, __VA_ARGS__); exit(1); }

#define VTKLOG(msg, ...) \
	std::cout << fmt::format( \
		"({}-{}):{}", __FILE__, __LINE__, __FUNCTION__)  \
		<< " " << (fmt::format(msg, ## __VA_ARGS__)) << std::endl;

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
		vtkPointDataset u_velocity;

		int depth;
		//std::vector<std::vector<double> > polyDataset;
	} openFoamVtkFileData;

	// constructors
	vtkParser();
	vtkParser(char* vtkFile);

	template<typename FSTR>
	void setVtkFile(FSTR fileName);
	void printVTKFILE();

	void freeVtkData();
	int init();
	int parseOpenFoam();

	// Enum Template so user can use geometryTypes, dataScopes, or just an int
	template<typename VTKENUM>
	vtkPointDataset getVtkData(VTKENUM dataType, std::string dataName);
	void dumpOFOAMPolyDataset();

	openFoamVtkFileData getOpenFoamData();

private:
	// has to be std string instead of ptr because of local ptr return garbage.
	std::string VTKFILE;

	// these types are dataset names used in OpenFoam Vtk files.
	enum OFOAMdatasetTypes {
		OFNONE,
		POINTS,
		LINES,
		U,
		K,
		P,
	};

	typedef struct {
		char fileBuffer[MAXFILELINES][MAXLINESIZE];
		int lineCount;
		openFoamVtkFileData* foamData;

		int currentScope; // EX: DATASET POLYDATA
		int currentSubScope; // EX: POINT_DATA
	} vtkParseData;

	vtkParseData* globalVtkData;

	std::vector<std::string> tokenizeDataLine(char* currentLine);

	void polyPointSecParse(vtkParseData* data, int line);
	/* This function needs changed in future:
	 * vtk datasets are defined by (name) value type I.E. POINTS 104 float.
	 * this function is only catering to the polyData when it could grab everything for later use.
	 * */
	void getPolyDataset(vtkParseData* data);
};

#endif
