/*Created by Tristan Wellman 2024

  NOTE: vtkOFRenderer is a library strictly made for the AfterBurner engine
		BUT some functionality can be used in whatever you are making
		EX: getOpenFoamTimeStamps().
*/

#pragma once

#include "vtkParser.hpp"
//#ifdef AFTR_USE_VTKOF
#include "MGLAxes.h"
#include "IndexedGeometryTriangles.h"
//#endif

/*The constructor NEEDS to be initialized
   BEFORE AfterBurner render loop or it'll parse all openFOAM
   files every frame!
 */
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

	int parseTracksFiles();

	std::vector<std::string> getOpenFoamTimeStamps();

	void renderTimeStampTrack(int timeStamp);
	void renderImGuiTimeSelector();

private:
	std::string filePath;

	std::vector<std::string> timeStamps;

	std::vector<std::string> tracksFiles;
	std::vector<vtkParser::openFoamVtkFileData> tracksFileData;
};