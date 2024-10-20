/*Copyright (c) 2024 Tristan Wellman*/

#ifndef VTK_PARSER_HPP
#define VTK_PARSER_HPP

#define POLYDATANSIZE 3

class vtkParser {
	public:

		typedef struct {
			char **fileBuffer;
			int lineCount;
		} vtkParseData;

		vtkParseData globalVtkData;

		// used to hold "dynamic" array sizes for different data
		template<typename ARRT, int len>
		struct vtkSectionData {
			ARRT dataArray[len];
		};
		// constructor
		vtkParser(char *vtkFile);
		void freeVtkData();
		int init();
		void parse();

	private:
		char *VTKFILE;

		void addArrData();
		
};

#endif
