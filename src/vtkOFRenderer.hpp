/*Created by Tristan Wellman 2024

  NOTE: vtkOFRenderer is a library strictly made for the AfterBurner engine
		BUT some functionality can be used in whatever you are making
		EX: getOpenFoamTimeStamps().
*/

#pragma once

#include <thread>
#include "GLViewNewModule.h"

#include "WorldList.h"
#include "ManagerOpenGLState.h" 
#include "Axes.h" 
#include "PhysicsEngineODE.h"

#include "WO.h"
#include "WOImGui.h"
#include "AftrImGuiIncludes.h"
#include "AftrGLRendererBase.h"
#include "AftrUtilities.h"

#include "MGLAxes.h"
#include "IndexedGeometryTriangles.h"
#include "MGLIndexedGeometry.h"
#include "MGLPointSetShaderAccelerated.h"
#include "GLSLShaderPointTesselatorBillboard.h"
#include "MGLPointCloud.h"
#include "GLSLShaderDefaultGL32PerVertexColor.h"

#include "vtkParser.hpp"
#include "meshParse.h"

using namespace Aftr;

typedef double(*HSVFUN)(double val);

// This determines how many points to skip each iteration while rendering the model to save ram and cpu/gpu usage
#define RENDER_RESOLUTION 10
// l,w,h size of rendered points
#define POINT_SIZE 5
// position scaling from those super tiny values
#define POSMUL 80

#define MAXTHREADS 40

/* 
*  loads all WO models for every time stamp to speed up loading time.
*  Warning: When enabled this loads ALL objects, it WILL use a lot of RAM be carful on low-end systems.
*/
#define PRELOAD_TIMESTAMPS true

/*The constructor NEEDS to be initialized
   BEFORE AfterBurner render loop or it'll parse all openFOAM
   files every frame!
 */
class vtkOFRenderer : vtkParser {
public:

	bool isReady;

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

	std::vector<std::string> getOpenFoamTimeStamps(std::vector<std::string> dirs);

	// Keeps model up to date with imgui selection
	void updateVtkTrackModel(WorldContainer* wl);

	/*Returns WO for you to push back in the world list*/
	WO *renderTimeStampTrack(WorldContainer* worldList, Camera** cam);

	/*This must be ran in already initialized WOImGui istance*/
	void renderImGuivtkSettings();
	
private:

	bool runLoop;
	bool enableStreamLines;
	const char* currentSelectedTimeStamp;


	std::vector<int> threadStates;
	std::vector<vtkParser> threadParsers;
	std::vector<std::thread> threads;

	std::string filePath;

	std::vector<std::string> timeStamps;

	std::vector<std::string> tracksFiles;
	std::vector<vtkParser::openFoamVtkFileData> tracksFileData;

	std::vector<unsigned int> WOIDS;
	std::vector<unsigned int> MESHWOIDS;

	std::vector< Vector > curVertexList;
	std::vector< unsigned int > curIndexList;

	std::vector<WO*> preLoadedWOs;
	std::vector<WO*> preLoadedOFMeshTS;

	OEFOAMMesh *model;

	void parseThread(int index);
};