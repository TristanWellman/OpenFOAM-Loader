/*Copyright (c) 2024 Tristan Wellman*/

#ifndef VTK_PARSER_HPP
#define VTK_PARSER_HPP

class vtkParser {
	public:
		// used to hold "dynamic" array sizes for different data
		template<typename ARRT, int len>
		struct vtkSectionData {
			ARRT dataArray[len];
		};

		vtkParser(char *vtkFile) : VTKFILE(vtkFile){};
		void init();

	private:
		void addArrData();
		char *VTKFILE;
		
};

#endif
