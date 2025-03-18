/*Created by Tristan Wellman 2024*/

#include <filesystem>
#include <functional>

#include "vtkOFRenderer.hpp"

using namespace Aftr;

std::vector<std::string> getDirectoryListing(std::string filePath) {
	std::vector<std::string> dirs;
	if(!std::filesystem::is_directory(filePath)) return dirs;
	for (auto& curDir : std::filesystem::recursive_directory_iterator(filePath)) {
		if (curDir.is_directory()) dirs.push_back(curDir.path().string());
	}
	return dirs;
}

std::string splitStr(std::string & str, char delim) {
	int i;
	for (i = 0; i < str.length(); i++) {
		if (str.at(i) == delim) {
			// return the first half of split string and set original to rest of string
			std::string ret = str.substr(0, i);
			str = str.substr(i + 1, str.length());
			return ret;
		}
	}
	return "";
}

std::vector<std::string> vtkOFRenderer::getOpenFoamTimeStamps(std::vector<std::string> dirs) {
	std::vector<std::string> ret;
	for (std::string i : dirs) {
		while (splitStr(i, '/') != "");
		if(splitStr(i, '\\') != "") continue;

		try {
			if(std::stoi(i)||i=="0") ret.push_back(i);
		} catch (const std::invalid_argument &e) { 
			VTKLOG("ERROR:: Failed to convert {} to timeStamp!", i);
			continue; 
		}
	}
	return ret;
}

vtkOFRenderer::vtkOFRenderer(std::string openFoamPath) : filePath(openFoamPath) {
	
	//parser = (vtkParser*)malloc(sizeof(vtkParser*));

	std::vector<std::string> listing = getDirectoryListing(openFoamPath);

	VTKASSERT(!listing.empty(),
		"ERROR:: Failed to open OPENFOAM test case folder: %s", openFoamPath.c_str());

	timeStamps = getOpenFoamTimeStamps(listing);

	VTKASSERT(!timeStamps.empty(),
		"ERROR:: Failed to retrieve OpenFoam case Time Stamps!");
	
	if (openFoamPath.at(openFoamPath.length() - 1) != '/') openFoamPath += '/';
	for (int i = 0; i < timeStamps.size();i++) {
		std::string fullPath = openFoamPath +
			"postProcessing/sets/streamlines/" + timeStamps.at(i) + "/track0_U.vtk";
		tracksFiles.push_back(fullPath);
		std::cout << tracksFiles.at(i) << std::endl;
	}
	isReady = false; // will be ready after parser is ran
	runLoop = false;
}

std::mutex tracksFileDataMutex;

void vtkOFRenderer::parseThread(int index) {
	vtkParser* parser = &threadParsers.at(index);
	parser->setVtkFile(tracksFiles.at(index));
	parser->init();
	parser->parseOpenFoam();

	{
		std::lock_guard<std::mutex> lock(tracksFileDataMutex);
		tracksFileData.push_back(parser->getOpenFoamData());
	}

	parser->freeVtkData();
	threadStates.at(index) = 1;
}

int vtkOFRenderer::parseTracksFiles() {

/*
 * Parse the mesh files, get the model.
 */
	model = new OEFOAMMesh;
	std::string mpath = filePath + "constant/polyMesh";
	OEParseFOAMObj((char*)mpath.c_str(), model);

	threadStates.resize(tracksFiles.size(), 0);
	threadParsers.resize(tracksFiles.size());
	threads.resize(tracksFiles.size());

	int i,j=1, finished=0;
	for (i = 0; i < MAXTHREADS&&i<tracksFiles.size()&&!tracksFiles.at(i).empty(); i++) {
		threads.at(i) = std::thread(std::mem_fn(&vtkOFRenderer::parseThread), this, i);
		VTKLOG("INFO:: Started parser thread for: {}", tracksFiles.at(i));
	}
	
	while (!finished) {
		int totalFinished = 0;
		for (i = 0;  i < MAXTHREADS && i < tracksFiles.size();i++) {
			if (threadStates.at(i)) totalFinished++;
		}
		if (totalFinished == MAXTHREADS - 1 || totalFinished==tracksFiles.size()-1) {
			finished = 1; 
			break;
		}
		/*for (; i < MAXTHREADS * j && i<tracksFiles.size() && !tracksFiles.at(i).empty(); i++) {
			threads.at(i) = std::thread(std::mem_fn(&vtkOFRenderer::parseThread), this, i);
			VTKLOG("INFO:: Started parser thread for: {}", tracksFiles.at(i));
		}
		j++;*/

	}
	for (i = 0; i < MAXTHREADS && i < tracksFiles.size(); i++) {
		//threads.at(i).join();
		VTKLOG("INFO:: Finished parser thread for: {}", tracksFiles.at(i));
	}

	isReady = true;
	currentSelectedTimeStamp = timeStamps.at(0).c_str();
#if PRELOAD_TIMESTAMPS
	VTKLOG("INFO:: Preloading OpenFOAM timestamps is enabled");
#endif

	return 0;
}

void vtkOFRenderer::updateVtkTrackModel(WorldContainer* wl) {

	static int curTime = 0;
	static size_t pastClock = clock();
	static size_t curClock = clock();

	static const char* pastTS = "0";
	openFoamVtkFileData* pptr = new openFoamVtkFileData;
	int i = 0;

	if (runLoop) currentSelectedTimeStamp = timeStamps.at(curTime).c_str();

	if (currentSelectedTimeStamp != pastTS) {
		for (i = 0; i < WOIDS.size(); i++) {
			WO* tmp = wl->getWOByID(WOIDS.at(i));
			wl->eraseViaWOptr(tmp);
#if !PRELOAD_TIMESTAMPS
			delete tmp;
#endif
		}
		WOIDS.clear();
		int i = 0;
		for (i = 0; i < timeStamps.size() && i < MAXTHREADS && i<tracksFileData.size()-1; i++) {
			if (preLoadedWOs.at(i) == NULL) {
				i--; break;
			}
			if (timeStamps.at(i).c_str() == currentSelectedTimeStamp) break;
		}
		pptr = &tracksFileData.at(i);
		//std::string point(ManagerEnvironmentConfiguration::getSMM() + "/models/planetSunR10.wrl");
#if !PRELOAD_TIMESTAMPS
		for (i = 0; i < pptr->points.polyData.size(); i += RENDER_RESOLUTION) {
			WO* wo = WO::New(point, Vector(POINT_SIZE, POINT_SIZE, POINT_SIZE), MESH_SHADING_TYPE::mstFLAT);
			wo->setPosition(Vector(
				pptr->points.polyData.at(i).at(0) * POSMUL,
				pptr->points.polyData.at(i).at(1) * POSMUL,
				pptr->points.polyData.at(i).at(2) * POSMUL));
			wo->renderOrderType = RENDER_ORDER_TYPE::roOPAQUE;
			std::string id = "point";
			wo->setLabel(id);
			wl->push_back(wo);
			WOIDS.push_back(wo->getID());

		}
#else 
		//for (int j = 0; j < preLoadedWOs.size()-1; j++) {
			WO* tmp = preLoadedWOs.at(i);
			// If the WO's label is not a timestamp then something is invalid
			if(tmp != nullptr && tmp->getLabel() != "") {
				wl->push_back(tmp);
				WOIDS.push_back(tmp->getID());
			}
		//}
#endif
	}
	pastTS = currentSelectedTimeStamp;
	curClock = clock();
	if (runLoop) {
		if (curClock >= pastClock+CLOCKS_PER_SEC) {
			pastClock = clock();
			if (curTime <= timeStamps.size()) curTime++;
			if (curTime == timeStamps.size()) curTime = 0;
		}
	}
}

/*
 * Map a magnitude value (3 per vertex) to RGB
 */
void mapMagnitudeToHSV(double mean, double stddev, std::vector<double> magnitude,
						std::vector<Vector> &rgbVals) {
	int l;
	for(l = 0; l < magnitude.size(); l++) {
		double val = magnitude.at(l);
		double min = mean - stddev * 2.5;
		double max = mean + stddev * 2.5;
		double fin = 0.0;

		if (val < min) fin = 0;
		else if (val > max) fin = 1;
		else {
			fin = (val - min) / (max - min);
			/* 240 / 360 is blue to magenta
			 * 1 - fin is inverting the color wheel, this is for testing with more extreme values right now */
			Vector hsv = { static_cast<float>((1-pow(fin,2)) * (240.0f/360.0f)),1.0f,1.0f };
			rgbVals.push_back(AftrUtilities::convertHSVtoRGB(hsv));
		}
	}
}

WO *vtkOFRenderer::renderTimeStampTrack(WorldContainer *worldList, Camera** cam) {
	//WO* wo = WO::New();
	
	/*Load the model OBJ*/
	std::vector< Vector > verts;
	std::vector< unsigned int > indices;
	for (int i = 0; i < model->verts.size; i++) {
		verts.push_back(Vector(
			model->verts.data[i][0]*(POSMUL * POINT_SIZE),
			model->verts.data[i][1]*(POSMUL * POINT_SIZE),
			model->verts.data[i][2]*(POSMUL * POINT_SIZE)));
	}	
	for (int i = 0; i < model->indices.size; i++) {
		for (int j = 0; j < 6; j++) 
			indices.push_back((unsigned int)model->indices.data[i][j]);
	}

	WO* wmodel = WO::New();
	IndexedGeometryTriangles* igt = IndexedGeometryTriangles::New(verts, indices);
	MGLIndexedGeometry* mgl = MGLIndexedGeometry::New(wmodel);
	mgl->setIndexedGeometry(igt);

	wmodel->setModel(mgl);
	worldList->push_back(wmodel);
	wmodel->render(**cam);

	VTKASSERT(currentSelectedTimeStamp != NULL, 
		"ERROR:: Uninitialized vtk timestamps!");

	static const char* pastTS;
	openFoamVtkFileData* pptr = new openFoamVtkFileData;

	int tloc;
	int i = 0,j=0,k=0;
	for (i = 0; i < timeStamps.size(); i++) {
		if (timeStamps.at(i).c_str() == currentSelectedTimeStamp) break;
	}
	pptr = &tracksFileData.at(i);
	tloc = i;
#if PRELOAD_TIMESTAMPS
		preLoadedWOs = std::vector<WO* >{};
		preLoadedWOs.resize(timeStamps.size());
		/* This was for when we were rendering spheres for each point.
		* 
		* for (i = 0; i < preLoadedWOs.size() && i < MAXTHREADS; i++) {
		*	pptr = &tracksFileData.at(i);
		*	preLoadedWOs.at(i).resize(pptr->points.polyData.size());
		* }
		*/
		pptr = &tracksFileData.at(tloc);
#endif

	//std::string point(ManagerEnvironmentConfiguration::getSMM() + "/models/planetSunR10.wrl");
	std::vector<Vector> pointLoc;
	std::vector<aftrColor4ub> magnitude;
	
	//compute mean and standard deviation for the magnitude colors
	float mean = 0, stddev = 0;
	for(const auto &x : pptr->uMagnitude.polyData) {
		for(double y : x) mean += y;
	}
	int sizex = pptr->uMagnitude.polyData.size();
	int sizey = pptr->uMagnitude.polyData[0].size();
	mean /= (sizex * sizey);
	for (const auto& x : pptr->uMagnitude.polyData) {
		for (double y : x) stddev += pow(y - mean, 2);
	}
	stddev /= ((sizex * sizey) - 1);
	stddev = sqrt(stddev);

	for (i = 0; i < pptr->points.polyData.size() && 
		(pptr->points.polyData.size()==pptr->uMagnitude.polyData.size()); i += RENDER_RESOLUTION,j++) {
#if !PRELOAD_TIMESTAMPS
		WO* wo = WO::New(point, Vector(POINT_SIZE, POINT_SIZE, POINT_SIZE), MESH_SHADING_TYPE::mstFLAT);
		wo->setPosition(Vector(
			pptr->points.polyData.at(i).at(0) * POSMUL,
			pptr->points.polyData.at(i).at(1) * POSMUL,
			pptr->points.polyData.at(i).at(2) * POSMUL));
		wo->renderOrderType = RENDER_ORDER_TYPE::roOPAQUE;
		std::string id = "point";
		wo->setLabel(id);
		worldList->push_back(wo);
		WOIDS.push_back(wo->getID());
	}
#else 

		pointLoc.push_back(Vector(pptr->points.polyData.at(i).at(0) * POSMUL,
			pptr->points.polyData.at(i).at(1) * POSMUL,
			pptr->points.polyData.at(i).at(2) * POSMUL));

		/*VTKLOG("{},{},{}\n", pptr->uMagnitude.polyData.at(i).at(0),
			pptr->uMagnitude.polyData.at(i).at(1),
			pptr->uMagnitude.polyData.at(i).at(2));*/
		std::vector<Vector> rgbVals;
		Vector finalColor = Vector();
		mapMagnitudeToHSV(mean, stddev, pptr->uMagnitude.polyData.at(i),rgbVals);
		for (int d = 0; d < rgbVals.size(); d++) finalColor = finalColor + rgbVals.at(d);
		magnitude.push_back(aftrColor4ub(finalColor));

		/* This was for when we were rendering spheres for each point.
		preLoadedWOs.at(tloc).at(i) = WO::New(point, Vector(POINT_SIZE, POINT_SIZE, POINT_SIZE), MESH_SHADING_TYPE::mstFLAT);
		preLoadedWOs.at(tloc).at(i)->setPosition(Vector(
			pptr->points.polyData.at(i).at(0) * POSMUL,
			pptr->points.polyData.at(i).at(1) * POSMUL,
			pptr->points.polyData.at(i).at(2) * POSMUL));
		preLoadedWOs.at(tloc).at(i)->renderOrderType = RENDER_ORDER_TYPE::roOPAQUE;
		std::string id = "point";
		preLoadedWOs.at(tloc).at(i)->setLabel(id);
		worldList->push_back(preLoadedWOs.at(tloc).at(i));
		WOIDS.push_back(preLoadedWOs.at(tloc).at(i)->getID());
		*/
	}
	preLoadedWOs.at(tloc) = WO::New();
	MGLPointCloud *cloud = 
		MGLPointCloud::New(preLoadedWOs.at(tloc), cam, true, false, false);
	cloud->setPoints(pointLoc, magnitude);
	cloud->setScale(Vector(POINT_SIZE, POINT_SIZE, POINT_SIZE));
	preLoadedWOs.at(tloc)->setModel(cloud);
	worldList->push_back(preLoadedWOs.at(tloc));
	WOIDS.push_back(preLoadedWOs.at(tloc)->getID());
	VTKLOG("LOADED:: WO#{} at timestamp: {}", preLoadedWOs.at(tloc)->getID(), timeStamps.at(tloc));
#endif

// load up rest of object into memory
#if PRELOAD_TIMESTAMPS
	pointLoc.clear();
	magnitude.clear();

	for (i = 1; i < timeStamps.size() && i < MAXTHREADS && i < tracksFileData.size(); i++) {
		pptr = &tracksFileData.at(i);

		mean = 0; stddev = 0;
		for (const auto& x : pptr->uMagnitude.polyData) {
			for(double y : x) mean += y;
		}
		sizex = pptr->uMagnitude.polyData.size();
		sizey = pptr->uMagnitude.polyData[0].size();
		mean /= (sizex * sizey);
		for (const auto& x : pptr->uMagnitude.polyData) {
			for (double y : x) stddev += pow(y - mean, 2);
		}
		stddev /= ((sizex * sizey) - 1);
		stddev = sqrt(stddev);

		for (j = 0; j < pptr->points.polyData.size(); j += RENDER_RESOLUTION,k++) {
			pointLoc.push_back(Vector(pptr->points.polyData.at(j).at(0) * POSMUL,
				pptr->points.polyData.at(j).at(1) * POSMUL,
				pptr->points.polyData.at(j).at(2) * POSMUL));

			std::vector<Vector> rgbVals;
			Vector finalColor = Vector();
			mapMagnitudeToHSV(mean, stddev, pptr->uMagnitude.polyData.at(i), rgbVals);
			for(int d = 0; d < rgbVals.size(); d++) finalColor += rgbVals.at(d);
			finalColor /= rgbVals.size();
			magnitude.push_back(aftrColor4ub(finalColor));

			/*preLoadedWOs.at(i).at(j) =
				WO::New(point, Vector(POINT_SIZE, POINT_SIZE, POINT_SIZE), MESH_SHADING_TYPE::mstFLAT);
			preLoadedWOs.at(i).at(j)->setPosition(Vector(
				pptr->points.polyData.at(j).at(0) * POSMUL,
				pptr->points.polyData.at(j).at(1) * POSMUL,
				pptr->points.polyData.at(j).at(2) * POSMUL));
			preLoadedWOs.at(i).at(j)->renderOrderType = RENDER_ORDER_TYPE::roOPAQUE;
			std::string id = "point";
			preLoadedWOs.at(i).at(j)->setLabel(id);
			//preLoadedWOs.at(i).push_back(wo);*/
		}
		preLoadedWOs.at(i) = WO::New();
		cloud = MGLPointCloud::New(preLoadedWOs.at(i), cam, true, false, false);
		cloud->setPoints(pointLoc, magnitude);
		cloud->setScale(Vector(POINT_SIZE, POINT_SIZE, POINT_SIZE));
		preLoadedWOs.at(i)->setModel(cloud);
		preLoadedWOs.at(i)->setLabel(timeStamps.at(i));
		WOIDS.push_back(preLoadedWOs.at(i)->getID());
		VTKLOG("LOADED:: WO#{} at timestamp: {}", preLoadedWOs.at(i)->getID(), timeStamps.at(i)); 
	};
#endif

	pastTS = currentSelectedTimeStamp;

	return nullptr;
}


/*This must be ran in already initialized WOImGui istance*/
void vtkOFRenderer::renderImGuivtkSettings() {


	static int curTime = 0;
	static const char* curItem = timeStamps.at(0).c_str();

	ImGui::SetNextWindowSize(ImVec2(400, 200));
	if (ImGui::Begin("Vtk View", NULL)) {

		ImGui::Text("Select a timestamp to view");
		if (ImGui::BeginCombo("TimeStamps", currentSelectedTimeStamp)) {
			for (int n = 0; n < timeStamps.size(); n++)
			{
				bool is_selected = (currentSelectedTimeStamp == timeStamps.at(n).c_str());
				if (ImGui::Selectable(timeStamps.at(n).c_str(), is_selected))
					currentSelectedTimeStamp = timeStamps.at(n).c_str();
					if(is_selected) ImGui::SetItemDefaultFocus(); 
			}
			ImGui::EndCombo();
		}

		ImGui::Checkbox("Play timeStamps", &runLoop);

	}
	ImGui::End();

	//currentSelectedTimeStamp = curItem;

}