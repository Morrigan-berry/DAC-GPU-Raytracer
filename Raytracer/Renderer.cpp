#define _CRT_SECURE_NO_WARNINGS

#include "Renderer.h"

#include <cassert>
#include <iostream>
#include <fstream>
//#include <string>

#include "Window.h"
#include "SVO.h"
#include <algorithm>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <sstream>

#define _USE_MATH_DEFINES
#include <math.h>

#include <filesystem>

#define Epsilon 0.0005


unsigned int __popcount_tab_[] = { // de basics.cpp
0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4,1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,4,5,5,6,5,6,6,7,5,6,6,7,6,7,7,8,
};


void error(int i = 0) {
	auto err = glGetError();
	if (err != GL_NO_ERROR) {
		std::cerr << i << "Error: " << err << std::endl;
		exit(1);
	}
}

void Renderer::writeVec(std::vector<int32_t>& vec) {
	std::fstream file;
	if (flag) {
		file.open("text.txt",std::fstream::out);
		flag = false;
	}else {
		file.open("text.txt",std::ios::app);
	}

	std::cout << "open" << std::endl;
	file << vec.size() << std::endl;
	for (int i = 0; i < vec.size(); ++i) {
		std::string aux = std::to_string(vec[i]);
		file << aux << std::endl;
	}
	file.close();
	
}

void transformAABB( int& childrenIndex, AABB& box) {// tambien parte de test funcion gpu en cpu
	for (int i = 0; i < box.size.size(); i++){
		box.size[i] /= 2;
	}
	if (childrenIndex % 2 == 1) // 1,3,5,7
		box.min[2] += box.size[2];
	if (childrenIndex == 2 || childrenIndex == 3 || childrenIndex == 6 ||
		childrenIndex == 7)
		box.min[1] += box.size[1];
	if (childrenIndex >= 4) // 4,5,6,7
		box.min[0] += box.size[0];
}

int positionToIndex(std::vector<float> position) {
	return int(position[0]) * 4 + int(position[1]) * 2 + int(position[2]);
}

bool Renderer::findNodeAt(FTRep* dac,bitRankDac* br,std::vector<float> pos, std::vector<int32_t> svdag, std::vector<SVO::Material> materials,int RootSize,uint* data, uint* rs) {//test funcion en gpu a cpu
	int level = 0;
	int index = 0;
	int level1 = 0;
	int index1 = 0;
	bool filled;
	SVO::Material mat;
	
	float var = float(RootSize);
	AABB box;
	AABB boxout;
	box.min = { Epsilon, Epsilon, Epsilon };
	box.size = {var,var,var };
	
	for (int i = 0; i < 32; ++i) {
		std::vector<float> aux;
		
		for (int i = 0; i < pos.size(); i++){
			float val = 2 * (pos[i] - box.min[i]) / (RootSize >> level);
			aux.push_back(val);
		}

		int childrenIndex = positionToIndex(aux);
		transformAABB(childrenIndex, box);
		std::cout << "childrenindex: " << childrenIndex << std::endl;


		int bitmask = svdag[index];
		uint bitmask1 = accessFT_Dac(dac,index1 + 1,br,data,rs );

		std::cout <<"valores bitmask en ciclo "<< i<< ": bitmask svdag:" << bitmask << " , bitmask dac:" << bitmask1 << std::endl;
		// if no children at all, this entire node is filled
		if ((bitmask & 255) == 0) {
			filled = true;
			boxout = box;
			mat = materials[bitmask >> 8];
			return true;
		}

		// check if it has the specific children
		if (((bitmask >> childrenIndex) & 1) == 1) {
			index1 = int(accessFT_Dac(dac,index1 + 2 + glm::bitCount(bitmask & ((1 << childrenIndex) - 1)), br, data, rs));
			level1 += 1;
			
			index =
				svdag[index + 1 + glm::bitCount(bitmask & ((1 << childrenIndex) - 1))];
			level += 1;

			std::cout << "valor index y level" << std::endl;
			std::cout << "svdag, index:" << index << " ,level:" << level << std::endl;
			std::cout << "dac, index:" << index1 << " ,level:" << level1 << std::endl;
		}
		else {
			filled = false;
			boxout = box;
			return true;
		}
	}
	return false; // shouldn't be here
}


void Renderer::loadSVO(SVO& svo) { 
	std::cout << "Scene loaded / generated!" << std::endl;
	std::vector<int32_t> svdag; //vector enteros
	std::vector<SVO::Material> materials;
	svo.toSVDAG(svdag, materials); // funcion SVO 
	std::cout << "SVDAG size " << svdag.size()
		<< " with " << materials.size() << " materials" << std::endl;
		
	sceneSize = svdag.size();
	nMaterials = materials.size();
	rootSize = svo.getSize();
	std::cout <<"tam root size: "<< rootSize << std::endl;
	//writeVec(svdag);
	int count = 0;

	FTRep* dac = createFT(svdag, svdag.size());
	uint * base_bitsGpu = (uint*)malloc(sizeof(uint) * dac->nLevels); // necesario para uso en gpu

	for (int i = 0; i < dac->nLevels; i++) {
		base_bitsGpu[i] = (uint)dac->base_bits[i];
		//std::cout << " bb GPU: " << base_bitsGpu[i] << " bb dac: " << dac->base_bits[i]<<std::endl;
	}
	
	bitRankDac* bitRank_gpu = (bitRankDac*)malloc(sizeof(struct sbitRank_DAC));
	bitRank_gpu->b = dac->bS->b;
	bitRank_gpu->s = dac->bS->s;
	bitRank_gpu->n = dac->bS->n;
	bitRank_gpu->factor = dac->bS->factor;
	bitRank_gpu->integers = dac->bS->integers;
	bitRank_gpu->owner = dac->bS->owner;

	uint* pr = &(dac->n_levels);

	//uint* prlevels = (uint*)malloc(sizeof(uint) * (dac->tamCode / W + 1));
	std::vector<float> position = { 2.60, 0.70, -0.50 };
	
	//findNodeAt(dac, bitRank_gpu, position, svdag, materials, svo.getSize(), dac->bS->data, dac->bS->Rs);

	
	
	//int count = 0;
	//uint ini1 = 0;
	//uint ini2 = 0;

	/*for (int i = 0; i < 3; i++) {
		
		uint t1 = rank(dac->bS, dac->levelsIndex[i] + ini1 - 1);
		uint t2 = rank_t(bitRank_gpu, dac->levelsIndex[i] + ini2 - 1, dac->bS->data, dac->bS->Rs);
		std::cout << "test 1 cpu" << t1 << std::endl;
		std::cout << "test 2 gpu antes de entrar a compute" << t2 << std::endl;
		ini1 = ini1 - t1;
		ini2 = ini2 - t2;

	}*/

	/*
	for (int i = 0; i < svdag.size(); i++) {
		if (svdag[i] != accessFT(dac, i + 1)) count++;
	}
	std::cout << "errores: " << count << " tam "<< svdag.size() << std::endl;
	*/
	/*
	std::cout << "nLevels: "<< (int)dac->nLevels << std::endl;
	for (int i = 0; i < dac->nLevels; i++){
		std::cout << dac->base_bits[i] << std::endl;
	}*/

	//if (svdagBuffer) glDeleteBuffers(1, &svdagBuffer);


	if (materialsBuffer) glDeleteBuffers(1, &materialsBuffer);
	if (levelsBuffer) glDeleteBuffers(1, &levelsBuffer);
	if (levelsIndexBuffer) glDeleteBuffers(1, &levelsIndexBuffer);
	if (baseBitsBuffer) glDeleteBuffers(1, &baseBitsBuffer);
	if (iteratorIndexBuffer) glDeleteBuffers(1, &iteratorIndexBuffer);
	if (tablebaseBuffer) glDeleteBuffers(1, &tablebaseBuffer);
	if (nLevelsBuffer) glDeleteBuffers(1, &nLevelsBuffer);
	if (inilevelBuffer) glDeleteBuffers(1, &inilevelBuffer);
	if (rankLevelsBuffer)glDeleteBuffers(1, &rankLevelsBuffer);
	if (dataBitRankBuffer) glDeleteBuffers(1, &dataBitRankBuffer);
	if (rSbitRankBuffer) glDeleteBuffers(1, &rSbitRankBuffer);
	if (bitRankBuffer) glDeleteBuffers(1, &bitRankBuffer);
	if (popcountBuffer) glDeleteBuffers(1, &popcountBuffer);
	
	glCreateBuffers(1, &materialsBuffer);
	glNamedBufferStorage(materialsBuffer, materials.size() * sizeof(SVO::Material), materials.data(), 0); // pasa materiales a opengl
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, materialsBuffer);

	// de DACS
	glCreateBuffers(1, &levelsBuffer);
	glNamedBufferStorage(levelsBuffer, sizeof(uint) * (dac->tamCode / W + 1), dac->levels, 0); // carga svdag a opengl
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, levelsBuffer);

	glCreateBuffers(1, &levelsIndexBuffer);
	glNamedBufferStorage(levelsIndexBuffer, sizeof(uint) * (dac->nLevels + 1), dac->levelsIndex, 0);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, levelsIndexBuffer);

	glCreateBuffers(1, &baseBitsBuffer);
	glNamedBufferStorage(baseBitsBuffer, sizeof(uint) * dac->nLevels, base_bitsGpu, 0);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, baseBitsBuffer);

	glCreateBuffers(1, &iteratorIndexBuffer);
	glNamedBufferStorage(iteratorIndexBuffer, sizeof(uint) * dac->nLevels, dac->iteratorIndex, 0);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, iteratorIndexBuffer);

	glCreateBuffers(1, &tablebaseBuffer);
	glNamedBufferStorage(tablebaseBuffer, sizeof(uint) * dac->tamtablebase, dac->tablebase, 0);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, tablebaseBuffer);

	glCreateBuffers(1, &nLevelsBuffer);
	glNamedBufferStorage(nLevelsBuffer, sizeof(uint), pr, 0);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 9, nLevelsBuffer);

	glCreateBuffers(1, &inilevelBuffer);
	glNamedBufferStorage(inilevelBuffer, sizeof(uint) * dac->nLevels, dac->iniLevel, 0);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 10, inilevelBuffer);

	glCreateBuffers(1, &rankLevelsBuffer);
	glNamedBufferStorage(rankLevelsBuffer, sizeof(uint) * dac->nLevels, dac->rankLevels, 0);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 11, rankLevelsBuffer);


	//De Bitrank
	glCreateBuffers(1, &dataBitRankBuffer);
	glNamedBufferStorage(dataBitRankBuffer, sizeof(uint) * (dac->levelsIndex[dac->nLevels - 1] + 1 / W + 1), dac->bS->data, 0);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 12, dataBitRankBuffer);

	glCreateBuffers(1, &rSbitRankBuffer);
	glNamedBufferStorage(rSbitRankBuffer, sizeof(uint) * ((dac->bS->n / dac->bS->s) + 1), dac->bS->Rs, 0);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 13, rSbitRankBuffer);
	//de bitrank modificado
	glCreateBuffers(1, &bitRankBuffer);
	glNamedBufferStorage(bitRankBuffer, sizeof(struct sbitRank_DAC), bitRank_gpu, 0);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 14, bitRankBuffer);
	//popcount mod para gpu
	glCreateBuffers(1, &popcountBuffer);
	glNamedBufferStorage(popcountBuffer, sizeof(__popcount_tab_) / sizeof(__popcount_tab_[0]) * sizeof(uint), __popcount_tab_, 0);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 15, popcountBuffer);
	//

	computeShader->use();
	computeShader->setInt("RootSize", svo.getSize());
	currentFrameCount = 0;

	std::cout << " mem usage svdag: " << svo.memoryusageSVDAG(svdag)<< std::endl;
	std::cout << " mem usage dac: " << memoryUsage(dac) << std::endl;
	std::cout << " mem usage dac GPU : " << memoryUsageGPU(dac) << std::endl;
	
}


void Renderer::loadScenes() {// itera sobre archivos .vox
	scenes.clear();
	scenes.push_back(std::make_unique<TestScene>());
	scenes.push_back(std::make_unique<TerrainScene>());
	scenes.push_back(std::make_unique<StairScene>());
	// iter vox files
	if (std::filesystem::exists("vox")) {
		for (auto& p : std::filesystem::recursive_directory_iterator("vox")) {
			if (p.path().extension() == ".vox") {
				scenes.push_back(std::make_unique<VoxModelScene>(p.path().string()));
			}
		}
	}
}

void Renderer::init() noexcept { // utiliza svo load revisalo
	flag = true;
	renderShader.emplace("shaders/vertex.glsl", "shaders/fragment.glsl", nullptr); //Inicializa clase shader dandole los argumentos requeridos para su construccion 
	computeShader.emplace(nullptr, nullptr, "shaders/compute.glsl");// lo mismo que la anterior
	computeShader->use(); // gluseprogram
	texture.emplace(window->width(), window->height()); // texture.h

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	ImGui_ImplGlfw_InitForOpenGL(window->getGLFWwindow(), true);
	ImGui_ImplOpenGL3_Init();

	float quadVertices[] = {
		// positions        // texture Coords
		-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
		-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
		 1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
		 1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
	};
	// setup plane VAO
	glGenVertexArrays(1, &quadVAO); //variable interna de renderer
	glGenBuffers(1, &quadVBO);
	glBindVertexArray(quadVAO);
	glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);//  vertex shader 
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

	texture.value().bind();
	glBindVertexArray(quadVAO);

	computeShader->use();// gluseprogram
	// autoFocus buffer for receving output
	glCreateBuffers(1, &autoFocusBuffer);
	glNamedBufferStorage(autoFocusBuffer, sizeof(float), nullptr, GL_MAP_READ_BIT);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, autoFocusBuffer);
	// map to autoFocus
	autoFocus = static_cast<float*>(glMapNamedBufferRange(autoFocusBuffer, 0, sizeof(float), GL_MAP_READ_BIT));
	
	

	loadScenes();
	loadSVO(*(scenes[0]->load(0))); // load primera escena
	//loadSVO(*(scenes[6]->load(32)));
	//camRead(); // funcion para leer text.txt posee mov camara
}

void Renderer::renderUI() noexcept { //
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	ImGui::Begin("SVO Raytracer");

	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
	ImGui::Text("Frames accumulated %d", currentFrameCount);
	ImGui::Text("Scene size: %d bytes, materials count: %d, root size: %d", sceneSize, nMaterials, rootSize);
	ImGui::Spacing();
	ImGui::DragFloat3("Camera Position", &cameraPos[0]);
	ImGui::DragFloat3("Camera front", &cameraFront[0]);
	ImGui::Spacing();
	ImGui::ColorEdit3("Sky color", &skyColor[0]);
	ImGui::ColorEdit3("Sun color", &sunColor[0]);
	ImGui::DragFloat3("Sun direction", &sunDir[0]);
	ImGui::Spacing();
	ImGui::Checkbox("Enable Depth of Field", &enableDepthOfField);
	ImGui::DragFloat("Focal Length", &focalLength);
	ImGui::DragFloat("Lens Radius", &lenRadius);
	ImGui::Text("Look-at distance: %.3f", *autoFocus);
	ImGui::SameLine();
	if (ImGui::Button("Auto focus")) {
		focalLength = *autoFocus;
	}
	ImGui::Spacing();
	ImGui::Checkbox("Fast Mode", &fastMode);
	ImGui::Spacing();
	// hasta aqui todas eran funciones de imgui que es la interfaz que se puede ver al correr la aplicacion

	static int currentSelection = 0;
	auto& currentScene = scenes[currentSelection];
	if (ImGui::BeginCombo("Scene", currentScene->getDisplayName(), 0))// escena elegida
	{
		for (size_t n = 0; n < scenes.size(); n++)
		{
			const bool isSelected = (currentSelection == n); // esta seleccionado
			if (ImGui::Selectable(scenes[n]->getDisplayName(), isSelected)) {
				currentSelection = n;
			    loadSVO(*(scenes[n]->load(32)));
				scenes[n]->release();
			}
			if (isSelected)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}
	// cuando escojo la escalera se cambia automaticamente a set 32
	static char paramInput[64] = "32";
	if (currentScene->hasParam()) {
		ImGui::InputText(currentScene->getParamName(), paramInput, 64, ImGuiInputTextFlags_CharsDecimal);
		ImGui::SameLine();
		if (ImGui::Button("Set")) {
			loadSVO(*(currentScene->load(std::stoi(paramInput))));
		}
	}

	if (ImGui::Button("Screenshot")) {
		takeScreenshot();
	}
	ImGui::SameLine();
	if (ImGui::Button("Re-render")) {
		currentFrameCount = 0;
	}

	ImGui::End();
}

void Renderer::checkForAccumulationFrameInvalidation() noexcept { // no se que hace pero no es mi campo
	static glm::vec3 cameraPosLastFrame = cameraPos, cameraFrontLastFrame = cameraFront;
	static bool enableDepthOfFieldLastFrame = enableDepthOfField;
	static float focalLengthLastFrame = focalLength, lenRadiusLastFrame = lenRadius;
	static glm::vec3 skyColorLastFrame = skyColor, sunColorLastFrame = sunColor, sunDirLastFrame = sunDir;
	static bool fastModeLastFrame = fastMode;
	if (cameraPos != cameraPosLastFrame ||
		cameraFront != cameraFrontLastFrame ||
		enableDepthOfField != enableDepthOfFieldLastFrame ||
		focalLength != focalLengthLastFrame ||
		lenRadius != lenRadiusLastFrame ||
		skyColor != skyColorLastFrame ||
		sunColor != sunColorLastFrame ||
		sunDir != sunDirLastFrame ||
		fastMode != fastModeLastFrame
		)  currentFrameCount = 0;
	cameraPosLastFrame = cameraPos;
	cameraFrontLastFrame = cameraFront;
	enableDepthOfFieldLastFrame = enableDepthOfField;
	focalLengthLastFrame = focalLength;
	lenRadiusLastFrame = lenRadius;
	skyColorLastFrame = skyColor;
	sunColorLastFrame = sunColor;
	sunDirLastFrame = sunDir;
	fastModeLastFrame = fastMode;
}

void Renderer::render() noexcept {
	renderUI();
	checkForAccumulationFrameInvalidation();

	// Raytrace with compute shader
	computeShader->use();
	computeShader->setVec3("CameraPos", cameraPos);
	computeShader->setVec3("CameraUp", cameraUp);
	computeShader->setVec3("CameraFront", cameraFront);
	computeShader->setVec3("RandomSeed", glm::vec3(rand(), rand(), rand()));
	computeShader->setInt("CurrentFrameCount", currentFrameCount);
	computeShader->setBool("DepthOfField", enableDepthOfField);
	computeShader->setFloat("FocalLength", focalLength);
	computeShader->setFloat("LenRadius", lenRadius);
	computeShader->setVec3("SkyColor", skyColor);
	computeShader->setVec3("SunColor", sunColor);
	computeShader->setVec3("SunDir", sunDir);
	computeShader->setBool("FastMode", fastMode);

	currentFrameCount += 1;

	glDispatchCompute(window->width(), window->height(), 1);// dispatch compute
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	// Render the result of the compute shader
	renderShader->use();// VERTEX Y FRAG SHADER en 1
	renderShader->setInt("tex", 0);// vertex y frag
	glActiveTexture(GL_TEXTURE0);
	glClear(GL_COLOR_BUFFER_BIT);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	
	ImGui::Render();// esta funcion es de ImGui
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData()); //
}


void Renderer::camRead() {
	std::fstream file("text.txt", std::fstream::in);
	if (!file.is_open()) {
		std::cerr << "Error al abrir el archivo." << std::endl;
	}else{
		glm::vec3 val;
		while (file >> val.x >> val.y >> val.z) {
			camPos.push_back(val);
			//std::cout << "camarapos x: " << camPos.back().x << " camarapos y: " << camPos.back().y << " camara pos z: " << camPos.back().z << std::endl;
			file >> val.x >> val.y >> val.z;
			camFro.push_back(val);
			//std::cout << "camarafront x: " << camFro.back().x << " camarafront y: " << camFro.back().y << " camarafront z: " << camFro.back().z << std::endl;
		}
	}
	file.close();
}


void Renderer::camWrite() {
	std::fstream file("text.txt", std::fstream::out);
	if (file.is_open()) {
		file << std::setprecision(5);
		for (int l = 0; l < camPos.size(); ++l) {
			file << camPos[l].x << " " << camPos[l].y << " " << camPos[l].z << '\n';
			file << camFro[l].x << " " << camFro[l].y << " " << camFro[l].z << '\n';
		}
	}
	
}

void Renderer::changeFlag(bool flag) {
	camaraFlag = flag;
	camWrite();
}

void Renderer::automaticCam(int& pos,bool& stop) {
	pos = pos % camPos.size();
	if (pos == 0) {
		std::cout << "vuelta " << posi << std::endl;
		posi++;
	}
	if (posi >= 21) stop = true;
	cameraPos = camPos[pos];
	cameraFront = camFro[pos];

}

using namespace glm;
static vec3 rotate(float theta, const vec3& v, const vec3& w) {//rotaciones de camara , no es necesario tocar esto
	const float c = cosf(theta);
	const float s = sinf(theta);

	const vec3 v0 = dot(v, w) * w;
	const vec3 v1 = v - v0;
	const vec3 v2 = cross(w, v1);

	return v0 + c * v1 + s * v2;
}

void Renderer::update() noexcept { //automatizacion

	float speed = keyPressed['X'] ? 1.f : 0.1f;
	if (keyPressed['C']) speed *= 100;
	if (keyPressed['W']) { 
		cameraPos += cameraFront * speed; 
		if (camaraFlag) {
			camPos.push_back(cameraPos);
			camFro.push_back(cameraFront);
		}
	}
	if (keyPressed['S']) {
		cameraPos -= cameraFront * speed;
		if (camaraFlag) {
			camPos.push_back(cameraPos);
			camFro.push_back(cameraFront);
		}
	}
	if (keyPressed['A']) {
		cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * speed;
		if (camaraFlag) {
			camPos.push_back(cameraPos);
			camFro.push_back(cameraFront);
		}
	}
	if (keyPressed['D']) {
		cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * speed;
		if (camaraFlag) {
			camPos.push_back(cameraPos);
			camFro.push_back(cameraFront);
		}
	}
	if (keyPressed[' ']) cameraPos += cameraUp * speed;
	if (keyPressed['Z']) cameraPos -= cameraUp * speed;
	if (keyPressed['O']) focalLength += 1.f;
	if (keyPressed['P']) focalLength = std::max(focalLength - 1.f, 0.0f);
	if (keyPressed['I']) focalLength = *autoFocus;
	if (keyPressed['K']) lenRadius += 0.01f;
	if (keyPressed['L']) lenRadius = std::max(lenRadius - 0.01f, 0.0f);

	//std::cout << "camara pos x: "<< cameraPos.x<< " camara pos y: "<< cameraPos.y<< " camara pos z: " << cameraPos.z << std::endl;
}


void Renderer::handleKey(char key, int action) noexcept {
	keyPressed[key] = action != GLFW_RELEASE;

	if (key == 'M' && action == GLFW_PRESS) {
		printf("Camera position: (%f, %f, %f)\n", cameraPos.x, cameraPos.y, cameraPos.z);
		printf("CameraFront: (%f, %f, %f)\n", cameraFront.x, cameraFront.y, cameraFront.z);
		printf("DOF (%d): focalLength = %f, lenRadius = %f, autoFocus=%f\n", enableDepthOfField, focalLength, lenRadius, *autoFocus);
	}
	if (key == 'Q' && action == GLFW_PRESS) {
		enableDepthOfField = !enableDepthOfField;
		printf("Depth of field %s\n", enableDepthOfField ? "enabled" : "disabled");
		currentFrameCount = 0;
	}
	if (key == 'E' && action == GLFW_PRESS) { // take a screenshot
		takeScreenshot();
	}
}

void Renderer::takeScreenshot() {
	std::stringstream filename;
	filename << "screenshots/screenshot_" << time(nullptr) << "_" << currentFrameCount << ".png";
	printf("Saving screenshot to %s\n", filename.str().data());
	auto data = texture->dump();
	// convert to int and also flip and apply gamma correction
	constexpr float Gamma = 2.2f;
	std::vector<char> idata(data.size());
	for (size_t x = 0; x < window->width(); x++)
		for (size_t y = 0; y < window->height(); y++)
			for (size_t c = 0; c < 4; c++) {
				float floatVal = data[(x + (window->height() - y - 1) * window->width()) * 4 + c];
				floatVal = clamp(floatVal, 0.f, 1.f);
				floatVal = powf(floatVal, 1.f / Gamma);
				idata[(x + y * window->width()) * 4 + c] =
					int(floatVal * 255);
			}
	stbi_write_png(filename.str().data(), window->width(), window->height(), 4, idata.data(), window->width() * 4);
}

void Renderer::handleMouseMove(double xpos, double ypos) noexcept {
	if (!mouseClicked) return;
	const float sensitivity = 0.01f;
	const float xoffset = lastMousePos.x - xpos;
	const float yoffset = lastMousePos.y - ypos;
	lastMousePos = { xpos, ypos };
	const float yaw = xoffset * sensitivity;
	const float pitch = yoffset * sensitivity;
	cameraFront = rotate(pitch, cameraFront, glm::normalize(glm::cross(cameraFront, cameraUp)));
	//std::cout << " 1 camara pos x: " << cameraFront.x << " camara pos y: " << cameraFront.y << " camara pos z: " << cameraFront.z << std::endl;
	if (camaraFlag) {
		camPos.push_back(cameraPos);
		camFro.push_back(cameraFront);
	}
	cameraFront = rotate(yaw, cameraFront, cameraUp);
	if (camaraFlag) {
		camPos.push_back(cameraPos);
		camFro.push_back(cameraFront);
	}
	//std::cout << " 2 camara pos x: " << cameraFront.x << " camara pos y: " << cameraFront.y << " camara pos z: " << cameraFront.z << std::endl;
}

void Renderer::handleMouse(int button, int action, double xpos, double ypos) noexcept {
	mouseClicked = action == GLFW_PRESS;
	lastMousePos = { xpos, ypos };
}