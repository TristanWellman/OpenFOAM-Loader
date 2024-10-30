/*Created by Tristan Wellman 2024

  NOTE: vtkOFRenderer is a library strictly made for the AfterBurner engine
		BUT some functionality can be used in whatever you are making
		EX: getOpenFoamTimeStamps().
*/

#pragma once

#include "vtkParser.hpp"
#ifdef AFTR_USE_VTKOF
#include "MGLAxes.h"
#include "IndecedGeometryTriangles.h"
#endif

class vtkOFRenderer : vtkParser {
public:
	/* openFoamPath - directory holding OpenFOAM case data entries
	*  Tree Ex:
	*  OpenFoamTestCase/
	*	- 0
	*   - 287
	*   - constant
	*   - logs (log.blockmesh etc.)
	*   - .OpenFOAM or .foam file
	*   - postProcessing
	*   - system
	*/
	vtkOFRenderer(std::string openFoamPath);

	std::vector<std::string> getOpenFoamTimeStamps();

private:
	std::string filePath;
};