/*Copyright (c) 2024 Tristan Wellman*/

#ifndef VTK_PARSER_HPP
#define VTK_PARSER_HPP

#define MAXLINESIZE 256
#define MAXFILELINES 100000
#define POLYDATANSIZE 3

class vtkParser {
	public:
		// constructor
		vtkParser(char *vtkFile);
		void freeVtkData();
		int init();
		void parse();

	private:
		char *VTKFILE;
		typedef struct {
			char fileBuffer[MAXFILELINES][MAXLINESIZE];
			int lineCount;
		} vtkParseData;

		vtkParseData globalVtkData;

		void addArrData();
		
};

#endif
