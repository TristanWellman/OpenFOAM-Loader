/*Copyright (c) 2024 Tristan Wellman*/

#ifndef VTK_PARSER_HPP
#define VTK_PARSER_HPP

#include <iostream>
#include <vector>

#define MAXLINESIZE 256
#define MAXFILELINES 100000

#define POLYDATANSIZE 3
#define MAXPOLY 100000

class vtkParser {
	public:
		// constructor
		vtkParser(char *vtkFile);

		void freeVtkData();
		int init();
		int parse();

		void dumpOFOAMPolyDataset();

	private:
		char *VTKFILE;
		
		enum dataScopes {
			DATASET,
			POINT_DATA,
			CELL_DATA
		};

		enum geometryTypes {
        	STRUCTURED_GRID,
        	UNSTRUCTURED_GRID,
        	POLYDATA,
        	STRUCTURED_POINTS,
        	RECTILINEAR_GRID,
        	FIELD
		};

		typedef struct {
			double polyDataset[MAXPOLY][POLYDATANSIZE];
		} openFoamVtkFileData;

		typedef struct {
			char fileBuffer[MAXFILELINES][MAXLINESIZE];
			int lineCount;
			openFoamVtkFileData *foamData;
		} vtkParseData;

		vtkParseData *globalVtkData;

		std::vector<std::string> tokenizeDataLine(char *currentLine);

		/* This function needs changed in future:
		 * vtk datasets are defined by (name) value type I.E. POINTS 104 float.
		 * this function is only catering to the polyData when it could grab everything for later use.
		 * */
		void getPolyDataset(vtkParseData *data);	
};

#endif
