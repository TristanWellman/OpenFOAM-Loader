/*Created by Tristan Wellman 2024*/

#include <filesystem>
#include <fstream>
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
		if(i.find(".orig")!=std::string::npos) continue;
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
	
	std::sort(timeStamps.begin(), timeStamps.end(), [](const std::string& a, const std::string& b) {
		return std::stod(a) < std::stod(b);});

	if (openFoamPath.at(openFoamPath.length() - 1) != '/') openFoamPath += '/';
	for (int i = 0; i < timeStamps.size();i++) {
		// This is /track0_U.vtk on older versions of OpenFOAM I.E. v2012
		std::string fullPath = openFoamPath +
			"postProcessing/sets/streamlines/" + timeStamps.at(i) + "/track0.vtk";
		tracksFiles.push_back(fullPath);
	}
	isReady = false; // will be ready after parser is ran
	runLoop = false;
	enableStreamLines = false;
}

std::mutex tracksFileDataMutex;

void vtkOFRenderer::parseThread(int index) {
	vtkParser *parser = threadParsers[index].get();
	std::ifstream s(tracksFiles.at(index));
	if(s.fail()) { threadStates.at(index) = 1; return;}
	s.close();
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

	int i, j = 1, finished = 0;
	for (i = 0; i < timeStamps.size(); i++) {
		std::string tpath = filePath + timeStamps.at(i) + "/U";
		OEParseMagnitudeTimeStamp((char *)tpath.c_str(), std::stoi(timeStamps.at(i)), model);
	}

	threadStates.resize(tracksFiles.size(), 0);
	threadParsers.clear();
	threadParsers.reserve(tracksFiles.size());
	for(i=0;i<tracksFiles.size();i++) threadParsers.push_back(std::make_unique<vtkParser>());

	int nextIndex = 0;
	std::vector<std::thread> activeThreads;

	while(nextIndex < tracksFiles.size() || !activeThreads.empty()) {
		while(activeThreads.size() < MAXTHREADS && nextIndex < tracksFiles.size()) {
			if(!tracksFiles.at(nextIndex).empty()) {
				std::string ts = tracksFiles.at(nextIndex);
				activeThreads.push_back(std::thread(std::mem_fn(&vtkOFRenderer::parseThread), this, nextIndex));
				VTKLOG("INFO:: Started parser thread for: {}", ts);
			}
			nextIndex++;
		}
		for(auto t = activeThreads.begin();t!=activeThreads.end();) {
			if(t->joinable()) {
				t->join();
				t = activeThreads.erase(t);
			} else t++;
		}
		//std::this_thread::sleep_for(std::chrono::milliseconds(10));
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
	vtkParser::openFoamVtkFileData* pptr = new vtkParser::openFoamVtkFileData();
	int i = 0;

	if(runLoop) currentSelectedTimeStamp = timeStamps.at(curTime).c_str();

	if(currentSelectedTimeStamp != pastTS) {
		for (i = 0; i < WOIDS.size()&&i<MESHWOIDS.size(); i++) {
			WO* tmp = wl->getWOByID(WOIDS.at(i));
			WO* MeshTmp = wl->getWOByID(MESHWOIDS.at(i));
			wl->eraseViaWOptr(MeshTmp);
			wl->eraseViaWOptr(tmp);
#if !PRELOAD_TIMESTAMPS
			delete tmp;
#endif
		}
		WOIDS.clear();
		MESHWOIDS.clear();
		int i = 0;
		for (i = 0; i < timeStamps.size() && i<tracksFileData.size()-1; i++) {
			if (i>0&&preLoadedWOs.at(i) == NULL) {
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
		WO* tmp = preLoadedWOs.at(i);
		WO* MeshTmp = preLoadedOFMeshTS.at(i);
		// If the WO's label is not a timestamp then something is invalid
		if((MeshTmp!=nullptr&&MeshTmp->getLabel()!="") &&
			(tmp != nullptr && tmp->getLabel() != "")) {
			wl->push_back(tmp);
			wl->push_back(MeshTmp);
			VTKLOG("LOADED {} : {} - TimeStamp: {}", 
				MeshTmp->getLabel(), MeshTmp->getID(), timeStamps.at(i));
			MESHWOIDS.push_back(MeshTmp->getID());
			WOIDS.push_back(tmp->getID());
		}
#endif
	}
	// remove any unwanted streamlines if disabled.
	if(!enableStreamLines) {
		for (i = 0; i < WOIDS.size(); i++) {
			WO* tmp = wl->getWOByID(WOIDS.at(i));
			wl->eraseViaWOptr(tmp);
		}
	}
	// update clock
	pastTS = currentSelectedTimeStamp;
	curClock = clock();
	if (runLoop) {
		if (curClock >= (pastClock+CLOCKS_PER_SEC)/1.5) {
			pastClock = clock();
			if (curTime <= timeStamps.size()) curTime++;
			if (curTime == timeStamps.size()) curTime = 0;
		}
	}
}

double modelHueCalc(double val) {
	return (double)(240.0f * (1.0f - val) / 360.0f);
}
double streamLinesHueCalc(double val) {
	return (double)((1.0f - pow(val,2)) * (240.0f / 360.0f));
}

/*
 * Map a magnitude value (3 per vertex) to RGB
 * Calc is the function to determine Hue. It must take in and return a double.
 */
void mapMagnitudeToHSV(double mean, double stddev, std::vector<double> magnitude,
						std::vector<Vector> &rgbVals, HSVFUN calc) {
	int l;
	for(l = 0; l < magnitude.size(); l++) {
		double val = magnitude.at(l);
		double min = mean - stddev * 2.5;
		double max = mean + stddev * 2.5;
		double fin = 0.0;
		rgbVals.reserve(rgbVals.size() + magnitude.size());
		if (val < min) fin = 0;
		else if (val > max) fin = 1;
		else {
			fin = (val - min) / (max - min);
			/*1 - fin is inverting the color wheel, this is for testing with more extreme values right now */
			Vector hsv = { static_cast<float>(calc(fin)),1.0f,1.0f };
			rgbVals.push_back(AftrUtilities::convertHSVtoRGB(hsv));
		}
	}
}

WO *vtkOFRenderer::renderTimeStampTrack(WorldContainer *worldList, Camera** cam) {

	/*Load the model OBJ*/
	int tloc;
	int i = 0, j = 0, k = 0;

	std::vector< Vector > verts;
	std::vector< std::vector<int> > faces;
	std::vector< unsigned int > indices;
	std::vector< int > owner;
	std::vector< int > neighbour;
	for(i = 0; i < model->verts.size; i++) {
		verts.push_back(Vector(
			model->verts.data[i][0]*(POSMUL * POINT_SIZE),
			model->verts.data[i][2]*(POSMUL * POINT_SIZE),
			model->verts.data[i][1]*(POSMUL * POINT_SIZE)));
	}	
	for(i = 0; i < model->indices.size; i++) {
		for (int j = 0; j < 6; j++) 
			indices.push_back((unsigned int)model->indices.data[i][j]);
	}
	for (i = 0; i < model->faces.size;i++) {
		std::vector<int> face;
		for (j = 0; j < 4; j++) face.push_back((int)model->faces.data[i][j]);
		faces.push_back(face);
	}
	for (i = 0; i < model->osize;i++) owner.push_back(model->owner[i]);
	for (i = 0; i < model->nsize; i++) neighbour.push_back(model->neighbour[i]);

	float mean = 0, stddev = 0, sizex = 0, sizey = 0;

	/*WO* wmodel = WO::New();
	IndexedGeometryTriangles* igt = IndexedGeometryTriangles::New(verts, indices);
	MGLIndexedGeometry* mgl = MGLIndexedGeometry::New(wmodel);
	mgl->setIndexedGeometry(igt);

	wmodel->setModel(mgl);
	wmodel->setLabel("OFMesh");*/
	//worldList->push_back(wmodel);
	//wmodel->render(**cam);

	VTKASSERT(currentSelectedTimeStamp != NULL, 
		"ERROR:: Uninitialized vtk timestamps!");

	static const char* pastTS;
	vtkParser::openFoamVtkFileData* pptr = new vtkParser::openFoamVtkFileData();

	for (i = 0; i < timeStamps.size(); i++) {
		if (timeStamps.at(i).c_str() == currentSelectedTimeStamp) break;
	}
	pptr = &tracksFileData.at(i);
	tloc = i;
#if PRELOAD_TIMESTAMPS
		preLoadedWOs = std::vector<WO* >{};
		preLoadedWOs.resize(timeStamps.size());
		preLoadedOFMeshTS = std::vector<WO*>{};
		preLoadedOFMeshTS.resize(timeStamps.size());
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
	std::vector<aftrColor4ub> meshMagnitudeColors;

// load up rest of object into memory
#if PRELOAD_TIMESTAMPS
	pointLoc.clear();
	magnitude.clear();

	for (i = 1; i < timeStamps.size() && i < tracksFileData.size(); i++) {
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
			pointLoc.push_back(Vector((pptr->points.polyData.at(j).at(0) * POSMUL),
				(pptr->points.polyData.at(j).at(2) * POSMUL)+0.1f,
				pptr->points.polyData.at(j).at(1) * POSMUL));

			std::vector<Vector> rgbVals;
			Vector finalColor = Vector();
			mapMagnitudeToHSV(mean, stddev, pptr->uMagnitude.polyData.at(i), rgbVals, (HSVFUN)streamLinesHueCalc);
			for(int d = 0; d < rgbVals.size(); d++) finalColor += rgbVals.at(d);
			finalColor /= rgbVals.size();
			aftrColor4ub fin = aftrColor4ub(finalColor);
			fin.a = 50.0f;
			magnitude.push_back(fin);

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
		int mi, mj;
		for (mi = 0; mi < model->magnitudeTS[i].values.size; mi++) {
			for (mj = 0; mj < VSIZE; mj++) mean += model->magnitudeTS[i].values.data[mi][mj];
		}
		sizex = model->magnitudeTS[i].values.size;
		sizey = VSIZE;
		mean /= (sizex * sizey);
		for (mi = 0; mi < model->magnitudeTS[i].values.size; mi++) {
			for (mj= 0; mj < VSIZE; mj++) stddev += pow(model->magnitudeTS[i].values.data[mi][mj] - mean, 2);
		}
		stddev /= ((sizex * sizey) - 1);
		stddev = sqrt(stddev);

		meshMagnitudeColors.clear();
		for (mi = 0; mi < model->magnitudeTS[i].values.size; mi++) {
			std::vector<Vector> rgbVals;
			std::vector<double> meshMag;
			Vector finalColor = Vector();
			for (mj = 0; mj < VSIZE; mj++) meshMag.push_back(model->magnitudeTS[i].values.data[mi][mj]);
			mapMagnitudeToHSV(mean, stddev, meshMag, rgbVals, (HSVFUN)modelHueCalc);
			for (k = 0; k < rgbVals.size(); k++) finalColor = finalColor + rgbVals.at(k);
			meshMagnitudeColors.push_back(aftrColor4ub(finalColor));
		}

		std::vector<aftrColor4ub> faceColors(faces.size());
		std::vector<aftrColor4ub> vertexColors(verts.size(), aftrColor4ub(0.0f, 0.0f, 0.0f, 255.0f));
		std::vector<int> vertexColorCounts(verts.size(), 0);
		int faceIdx,v;
		for(faceIdx = 0; faceIdx < faces.size(); faceIdx++) {
			int cellOwner = owner[faceIdx];
			int cellNeighbour = -1;

			if(faceIdx < neighbour.size()) cellNeighbour = neighbour[faceIdx];

			if(cellNeighbour >= 0) {
				int r = (meshMagnitudeColors[cellOwner].r + meshMagnitudeColors[cellNeighbour].r) / 2;
				int g = (meshMagnitudeColors[cellOwner].g + meshMagnitudeColors[cellNeighbour].g) / 2;
				int b = (meshMagnitudeColors[cellOwner].b + meshMagnitudeColors[cellNeighbour].b) / 2;
				faceColors[faceIdx] = aftrColor4ub(r, g, b, 255);
			} else faceColors[faceIdx] = meshMagnitudeColors[cellOwner];
		}

		for(faceIdx = 0;faceIdx < faces.size();faceIdx++) {
			for(int v : faces[faceIdx]) {
				vertexColors[v].r += faceColors[faceIdx].r;
				vertexColors[v].g += faceColors[faceIdx].g;
				vertexColors[v].b += faceColors[faceIdx].b;
				vertexColorCounts[v]++;
			}
		}

		for(v = 0; v < verts.size(); v++) {
			if(vertexColorCounts[v] > 0) {
				vertexColors[v].r = (vertexColors[v].r / vertexColorCounts[v]);
				vertexColors[v].g = (vertexColors[v].g / vertexColorCounts[v]);
				vertexColors[v].b = (vertexColors[v].b / vertexColorCounts[v]);
				vertexColors[v].a = 255.0f;
			}
		}

		preLoadedWOs.at(i) = WO::New();
		preLoadedOFMeshTS.at(i) = WO::New();
		ModelMeshSkin cloudskin(GLSLShaderDefaultGL32PerVertexColor::New());
		cloudskin.setGLPrimType(GL_TRIANGLES);
		cloudskin.setMeshShadingType(MESH_SHADING_TYPE::mstFLAT);
		MGLPointCloud *cloud = MGLPointCloud::New(preLoadedWOs.at(i), cam, true, false, false);
		cloud->addSkin(std::move(cloudskin));
		cloud->useNextSkin();
		cloud->setPoints(pointLoc, magnitude);
		cloud->setScale(Vector(POINT_SIZE, POINT_SIZE, POINT_SIZE));
		preLoadedWOs.at(i)->setModel(cloud);
		preLoadedWOs.at(i)->setLabel(timeStamps.at(i));
		WOIDS.push_back(preLoadedWOs.at(i)->getID());

		// OFMODEL
		ModelMeshSkin skin(GLSLShaderDefaultGL32PerVertexColor::New());
		skin.setGLPrimType(GL_TRIANGLES);
		skin.setMeshShadingType(MESH_SHADING_TYPE::mstFLAT);
		skin.setAmbient(aftrColor4f(255.0f, 255.0f, 255.0f, 255.0f));
		skin.setColor(aftrColor4ub(255.0f, 255.0f, 255.0f, 255.0f));
		IndexedGeometryTriangles* igt = IndexedGeometryTriangles::New(verts, indices, vertexColors);
		MGLIndexedGeometry* mgl = MGLIndexedGeometry::New(preLoadedOFMeshTS.at(i));
		mgl->addSkin(std::move(skin));
		mgl->useNextSkin();
		mgl->setIndexedGeometry(igt);

		preLoadedOFMeshTS.at(i)->setModel(mgl);
		preLoadedOFMeshTS.at(i)->setLabel("OFMesh"+timeStamps.at(i));
		worldList->push_back(preLoadedOFMeshTS.at(i));
		MESHWOIDS.push_back(preLoadedOFMeshTS.at(i)->getID());

		//VTKLOG("LOADED:: WO#{} at timestamp: {}", preLoadedWOs.at(i)->getID(), timeStamps.at(i)); 
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
	if(ImGui::Begin("Vtk View", NULL)) {

		ImGui::Text("Select a timestamp to view");
		if(ImGui::BeginCombo("TimeStamps", currentSelectedTimeStamp)) {
			for (int n = 0; n < timeStamps.size(); n++) {

				bool is_selected = (currentSelectedTimeStamp == timeStamps.at(n).c_str());
				if(ImGui::Selectable(timeStamps.at(n).c_str(), is_selected))
					currentSelectedTimeStamp = timeStamps.at(n).c_str();
				
				if(is_selected) ImGui::SetItemDefaultFocus(); 
			}
			ImGui::EndCombo();
		}

		ImGui::Checkbox("Enable StreamLines", &enableStreamLines);
		ImGui::Checkbox("Play timeStamps", &runLoop);

	}
	ImGui::End();

	//currentSelectedTimeStamp = curItem;

}