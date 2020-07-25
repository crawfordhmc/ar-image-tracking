#pragma once

//model loading and rendering based on code from https://github.com/avmeer/ComputerVisionAugmentedReality

#include "Renderer.h"

/*
*Include GLM - OpenGL Maths
*/
#include <glm/glm.hpp>
using namespace glm;

// auxiliary C file to read the shader text files
#include "textfile.h"

// assimp include files. These three are usually needed.
#include "assimp/Importer.hpp"    //OO version Header!
#include "assimp/postprocess.h"
#include "assimp/scene.h"

#include "helpers.h"

//Window Default size
static int windowWidth, windowHeight;

static GLuint matricesUniBuffer;



class OpenGLRenderer : public Renderer {

public:

	OpenGLRenderer();
	~OpenGLRenderer();

	void init();

	bool loadModelFile(std::string filename);
	GLuint extractEdges(cv::Mat frame);
	GLuint extractColorEdges(cv::Mat frame);

	Event render(const CameraCalibration& calibration, const CameraPose& pose, const cv::Mat& image);
	void pushMatrix();
	void popMatrix();
	void setModelMatrix();
	void translate(float x, float y, float z);
	void rotate(float angle, float x, float y, float z);
	void scale(float x, float y, float z);
	static void buildProjectionMatrix(float fov, float ratio, float nearp, float farp);
	void setCamera(cv::Mat viewMatrix);
	void recursive_render(aiScene * sc, const aiNode * nd, std::vector<struct MyMesh>& myMeshes);
	void prepareTexture(int w, int h, unsigned char * data);
	void updateMatrices(const CameraPose &pose);
	

private:

	//intialise window with a name
	bool initWindow(std::string windowName);
	

	//window handle
	GLFWwindow* window;

	cv::Mat distCoeffs;
	cv::Mat imageMat;
	cv::Mat imageMatMask;
	cv::Mat imageMatGL;
	cv::Mat imageMatMaskGL;
	cv::Mat edges;
	

	// Model Matrix (part of the OpenGL Model View Matrix)
	float modelMatrix[16];
	// For push and pop matrix
	std::vector<float *> matrixStack;

	// Vertex Attribute Locations
	GLuint vertexLoc, normalLoc, texCoordLoc;

	// Uniform Bindings Points
	GLuint matricesUniLoc, materialUniLoc;

	// The sampler uniform for textured models
	// we are assuming a single texture so this will
	//always be texture unit 0
	GLuint texUnit;

	// Program and Shader Identifiers
	GLuint program, vertexShader, fragmentShader;
	GLuint videoProgram, vertexShader2D, fragmentShader2D;


	// holder for the vertex array object id
	GLuint videoVAO, videotextureID;
   
	// Shader Names
	char *vertexFileName;
	char *fragmentFileName;;


	// all the models we want to render
	std::map<int, MyModel> models;

	// images / texture
	// map image filenames to textureIds
	// pointer to texture Array
	std::map<std::string, GLuint> textureIdMap;

	// different marker ids - in case you have multiple markers or different checkerboards
	std::vector< int > markerIds;


	//mapping between markerid and model
	std::map<int, std::string> modelMap;
	std::map<int, float> scaleMap;
	std::string modelDir;


	//render flipped video
	bool flipped;

	// Frame counting and FPS computation
	double timebase;
	int frame;
	std::string windowTitle;



	//private methods 
	static void changeSize(GLFWwindow * window, int w, int h);
	void printShaderInfoLog(GLuint obj);
	void printProgramInfoLog(GLuint obj);
	GLuint setupShaders();
	void setupShaders2D();
	int init2D();
	bool import3DFromFile(const std::string & pFile, aiScene *& scene, Assimp::Importer & importer, float & scaleFactor);
	void genVAOsAndUniformBuffer(aiScene * sc, std::vector<struct MyMesh>& myMeshes);
	
	
	int loadModels();
	int loadGLTextures(aiScene * scene);

	void renderScene(void);
	void displayFPS();
};
