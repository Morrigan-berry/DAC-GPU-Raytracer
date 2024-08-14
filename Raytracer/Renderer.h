#pragma once
#include <optional>
#include <memory>
#include <glm/glm.hpp>
#include "dacs.h"
#include "Shader.h"
#include "Texture.h"
#include "Scene.h"

struct AABB {
	std::vector<float> min;
	std::vector<float> size;
};

class Window;


class Renderer {
public:
	Renderer() = default;
	~Renderer() = default;

	void init() noexcept;
	void render() noexcept;
	void update() noexcept;
	void setWindow(Window* window) noexcept { this->window = window; }
	void handleKey(char key, int action) noexcept;
	void handleMouseMove(double xpos, double ypos) noexcept;
	void handleMouse(int button, int action, double xpos, double ypos) noexcept;
	void automaticCam(int& pos,bool& stop);
	void changeFlag(bool flag);
private:
	void renderUI() noexcept;
	void takeScreenshot();
	void checkForAccumulationFrameInvalidation() noexcept;
	void loadSVO(SVO& svo);
	void loadScenes();
	void writeVec(std::vector<int32_t>& vec);
	bool findNodeAt(FTRep* dac, bitRankDac* br, std::vector<float> pos, std::vector<int32_t> svdag, std::vector<SVO::Material> materials, int RootSize, uint* data, uint* rs);
	void camRead();
	void camWrite();

	std::optional<Shader> computeShader = std::nullopt, renderShader = std::nullopt;// Shader
	std::optional<Texture> texture = std::nullopt;
	Window* window;
	GLuint quadVAO = 0, quadVBO = 0; // for rendering the image (screen quad)
	
	GLuint svdagBuffer = 0, materialsBuffer, levelsBuffer,levelsIndexBuffer, baseBitsBuffer,iteratorIndexBuffer,tablebaseBuffer,autoFocusBuffer, dataBitRankBuffer,rSbitRankBuffer, bitRankBuffer,popcountBuffer,nLevelsBuffer,inilevelBuffer,rankLevelsBuffer;
	//GLushort ;
	
	glm::vec3 cameraPos = { -232.8f,117.8f, 92.2f };
	glm::vec3 cameraUp = { 0.0f, 1.0f, 0.0f };
	glm::vec3 cameraFront = { 0.8f, -0.4f, -0.3f };
	//camarapos, camarafront 
	std::vector<glm::vec3> camPos;
	std::vector<glm::vec3> camFro;
	int posi = 1; // determina las 20 pasadas de la camara
	bool camaraFlag = false; // flag para terminar con mov de camara utilizado para grabar nuevos mov de camara

	bool mouseClicked = false;
	glm::vec2 lastMousePos = { 0.0f, 0.0f };
	bool keyPressed[256] = { false };

	size_t currentFrameCount = 0;
	bool enableDepthOfField = false;
	float focalLength = 5.f;
	float lenRadius = 0.1f;

	bool fastMode = false;
	
	glm::vec3 sunDir { -0.5, 0.75, 0.8 };
	glm::vec3 sunColor { 1, 1, 1 };
	glm::vec3 skyColor { .53, .81, .92 };

	float* autoFocus = nullptr;

	//flag para escribir archivo
	bool flag = false;

	// stats
	size_t sceneSize = 0, nMaterials = 0, rootSize = 0;

	// scenes
	std::vector<std::unique_ptr<Scene>> scenes;
};
