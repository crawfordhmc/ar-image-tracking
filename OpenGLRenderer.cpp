
#include "OpenGLRenderer.h"

#include <GLFW/glfw3.h>

using namespace std;

OpenGLRenderer::OpenGLRenderer() : Renderer() {
    
    // Frame counting and FPS computation
    timebase = 0;
    frame = 0;
    
    //we only work with one marker in our exampl
    markerIds.push_back(0);
    
    init();
    
}

OpenGLRenderer::~OpenGLRenderer() {
    // Close OpenGL window and terminate GLFW
    glfwTerminate();
}


void OpenGLRenderer::init() {
    
    // Distortion coeffs (fill in your actual values here).
    double dist_[] = { 0, 0, 0, 0, 0 };
    distCoeffs = cv::Mat(5, 1, CV_64F, dist_).clone();
    // Vertex Attribute Locations
    vertexLoc = 0;
    normalLoc = 1;
    texCoordLoc = 2;
    
    // Uniform Bindings Points
    matricesUniLoc = 1;
    materialUniLoc = 2;
    
    // The sampler uniform for textured models
    // we are assuming a single texture so this will
    //always be texture unit 0
    texUnit = 0;
    
    // Shader Names
    vertexFileName = (char *)"shader\\modelShader.vert";
    fragmentFileName = (char *)"shader\\modelShader.frag";
    modelDir = "models\\";
    
    flipped = false;
    
    
    // Init a basic window
    windowTitle = "SimpleWindow";
    bool windowInitSucess = initWindow(windowTitle.c_str());
    if (!windowInitSucess) {
        return;
    }
    
    //    Init GLEW
    glewExperimental = GL_TRUE;
    
    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "Failed to initialize GLEW\n");
        getchar();
        glfwTerminate();
        return;
    }
    if (glewIsSupported("GL_VERSION_3_3"))
        printf("Ready for OpenGL 3.3\n");
    else {
        printf("OpenGL 3.3 not supported\n");
        return;
    }
    
    // Ensure we can capture the escape key being pressed below
    //glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
    // Ensure we can capture the escape key being pressed below
    //glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
    // Hide the mouse and enable unlimited mouvement
    //glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    
    // Set the mouse at the center of the screen
    //glfwPollEvents();
    //glfwSetCursorPos(window, 1024 / 2, 768 / 2);
    
    // Dark blue background
    glClearColor(0.0f, 0.0f, 0.4f, 0.0f);
    // Enable depth test
    glEnable(GL_DEPTH_TEST);
    
    
    loadModelFile((std::string)"modelToMarker.txt");
    
    
    //  Init the app (load model and textures) and OpenGL
    if (!loadModels())
        printf("Could not Load the Model\n");
    
    
    glGetUniformBlockIndex = (PFNGLGETUNIFORMBLOCKINDEXPROC)glfwGetProcAddress("glGetUniformBlockIndex");
    glUniformBlockBinding = (PFNGLUNIFORMBLOCKBINDINGPROC)glfwGetProcAddress("glUniformBlockBinding");
    glGenVertexArrays = (PFNGLGENVERTEXARRAYSPROC)glfwGetProcAddress("glGenVertexArrays");
    glBindVertexArray = (PFNGLBINDVERTEXARRAYPROC)glfwGetProcAddress("glBindVertexArray");
    glBindBufferRange = (PFNGLBINDBUFFERRANGEPROC)glfwGetProcAddress("glBindBufferRange");
    glDeleteVertexArrays = (PFNGLDELETEVERTEXARRAYSPROC)glfwGetProcAddress("glDeleteVertexArrays");
    
    
    
    
    glEnable(GL_DEPTH_TEST);
    glClearColor(0, 0, 0, 0.0f);
    
    //
    // Uniform Block
    //
    glGenBuffers(1, &matricesUniBuffer);
    glBindBuffer(GL_UNIFORM_BUFFER, matricesUniBuffer);
    glBufferData(GL_UNIFORM_BUFFER, MatricesUniBufferSize, NULL, GL_DYNAMIC_DRAW);
    glBindBufferRange(GL_UNIFORM_BUFFER, matricesUniLoc, matricesUniBuffer, 0, MatricesUniBufferSize);    //setUniforms();
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    
    glEnable(GL_MULTISAMPLE);
    
    init2D();
    
    printf("Vendor: %s\n", glGetString(GL_VENDOR));
    printf("Renderer: %s\n", glGetString(GL_RENDERER));
    printf("Version: %s\n", glGetString(GL_VERSION));
    printf("GLSL: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
    
    
    // return from main loop
    //glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);
    
    glfwSetWindowSizeCallback(window, &OpenGLRenderer::changeSize);
    
    // Set the viewport to be the entire window
    int w, h;
    glfwGetFramebufferSize(window, &w, &h);
    
    glViewport(0, 0, w, h);
    
    float ratio = (1.0f * w) / h;
    buildProjectionMatrix(53.13f, ratio, 0.1f, 10.0f);
    
    program = setupShaders();
     // Initialize GLEW
    
    
    // Ensure we can capture the escape key being pressed below
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
    
    // Dark blue background
    glClearColor(0.0f, 0.0f, 0.4f, 0.0f);
    
    
    
}



bool OpenGLRenderer::loadModelFile(std::string filename) {
    filename = modelDir + filename;
    //open file stream to the file
    std::ifstream infile(filename);
    
    //if opened successfully, read in the data
    if (infile.is_open()) {
        int marker;
        std::string file;
        float scaleFactor;
        
        while (infile >> file >> marker >>scaleFactor)
        {
            modelMap[marker] = file;
            scaleMap[marker] = scaleFactor;
        }
    }
    else {
        //if here, file was not opened correctly, notify user
        printf("Error opening file, %s exiting", filename.c_str());
        exit(0);
    }
    return true;
}


GLuint OpenGLRenderer::extractEdges(cv::Mat frame) {
    cv::Mat edges;
    int threshold = 50;

    // convert to greyscale and extract edges
    cv::cvtColor(frame, edges, cv::COLOR_RGB2GRAY);
    cv::Canny(edges, edges, threshold, threshold * 3);
    //int dilation_size = 5;
    //cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(2 * dilation_size + 1, 2 * dilation_size + 1),
        //cv::Point(dilation_size, dilation_size));
    //cv::dilate(edges, edges, kernel);
    cv::Mat binary;
    cv::threshold(edges, binary, 100, 255, cv::THRESH_BINARY);

    //First create the image with alpha channel
    cv::cvtColor(edges, edges, cv::COLOR_GRAY2RGBA);
    //Split the image for access to alpha channel
    std::vector<cv::Mat>channels(4);
    cv::split(edges, channels);
    //Assign the mask to the last channel of the image
    channels[3] = binary;
    //Finally concat channels for rgba image
    cv::merge(channels, edges);

    // prepare texture
    GLuint edgesID;
    glGenTextures(1, &edgesID);
    glBindTexture(GL_TEXTURE_2D, edgesID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, edges.size().width,
        edges.size().height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
        edges.data);
    glGenerateMipmap(GL_TEXTURE_2D);

    // Test code to display edges
    cv::imshow("beep boop", edges);
    cv::waitKey(0);
    cv::destroyWindow("beep boop");

    return edgesID;
}


GLuint OpenGLRenderer::extractColorEdges(cv::Mat frame) {
    cv::Mat edges;
    int threshold = 50;

    // convert to greyscale, extract edges and convert to edges to binary
    cv::cvtColor(frame, edges, cv::COLOR_RGB2GRAY);
    cv::Canny(edges, edges, threshold, threshold*3);
    cv::Mat binary;
    cv::threshold(edges, binary, 100, 255, cv::THRESH_BINARY);

    // copy frame with binary edge mask to coloured edges matrix
    cv::Mat colorEdges;
    frame.copyTo(colorEdges, binary);

    //First create the image with alpha channel
    cv::cvtColor(colorEdges, colorEdges, cv::COLOR_RGB2RGBA);
    //Split the image for access to alpha channel
    std::vector<cv::Mat>channels(4);
    cv::split(colorEdges, channels);
    //Assign the mask to the last channel of the image
    channels[3] = binary;
    //Finally concat channels for rgba image
    cv::merge(channels, colorEdges);
    
    // prepare texture
    GLuint edgesID;
    glGenTextures(1, &edgesID);
    glBindTexture(GL_TEXTURE_2D, edgesID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, colorEdges.size().width,
        colorEdges.size().height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
        colorEdges.data);
    glGenerateMipmap(GL_TEXTURE_2D);

    // Test code to display edges
    /*cv::Mat reversed;
    cv::cvtColor(colorEdges, reversed, cv::COLOR_BGRA2RGB);
    cv::imshow("beep boop", reversed);
    cv::waitKey(0);
    cv::destroyWindow("beep boop");*/

    return edgesID;
}


// ------------------------------------------------------------
//
// Reshape Callback Function
//
void OpenGLRenderer::changeSize(GLFWwindow* window, int w, int h)
{
    
    float ratio;
    // Prevent a divide by zero, when window is too short
    // (you cant make a window of zero width).
    if (h == 0)
        h = 1;
    
    windowWidth = w;
    windowHeight = h;
    
    // Set the viewport to be the entire window
    glViewport(0, 0, w, h);
    
    ratio = (1.0f * w) / h;
    buildProjectionMatrix(53.13f, ratio, 0.1f, 10.0f);
}



// --------------------------------------------------------
//
// Shader Stuff
//

void OpenGLRenderer::printShaderInfoLog(GLuint obj)
{
    int infologLength = 0;
    int charsWritten = 0;
    char *infoLog;
    
    glGetShaderiv(obj, GL_INFO_LOG_LENGTH, &infologLength);
    
    if (infologLength > 0)
    {
        infoLog = (char *)malloc(infologLength);
        glGetShaderInfoLog(obj, infologLength, &charsWritten, infoLog);
        printf("%s\n", infoLog);
        free(infoLog);
    }
}


void OpenGLRenderer::printProgramInfoLog(GLuint obj)
{
    int infologLength = 0;
    int charsWritten = 0;
    char *infoLog;
    
    glGetProgramiv(obj, GL_INFO_LOG_LENGTH, &infologLength);
    
    if (infologLength > 0)
    {
        infoLog = (char *)malloc(infologLength);
        glGetProgramInfoLog(obj, infologLength, &charsWritten, infoLog);
        printf("%s\n", infoLog);
        free(infoLog);
    }
}


GLuint OpenGLRenderer::setupShaders() {
    
    char *vs = NULL, *fs = NULL;
    
    GLuint p, v, f;
    
    v = glCreateShader(GL_VERTEX_SHADER);
    f = glCreateShader(GL_FRAGMENT_SHADER);
    
    vs = textFileRead(vertexFileName);
    fs = textFileRead(fragmentFileName);
    
    const char * vv = vs;
    const char * ff = fs;
    
    glShaderSource(v, 1, &vv, NULL);
    glShaderSource(f, 1, &ff, NULL);
    
    free(vs); free(fs);
    
    glCompileShader(v);
    glCompileShader(f);
    
    p = glCreateProgram();
    glAttachShader(p, v);
    glAttachShader(p, f);
    
    glBindFragDataLocation(p, 0, "output");
    
    glBindAttribLocation(p, vertexLoc, "position");
    glBindAttribLocation(p, normalLoc, "normal");
    glBindAttribLocation(p, texCoordLoc, "texCoord");
    
    glLinkProgram(p);
    glValidateProgram(p);
    
    program = p;
    vertexShader = v;
    fragmentShader = f;
    
    GLuint k = glGetUniformBlockIndex(p, "Matrices");
    glUniformBlockBinding(p, k, matricesUniLoc);
    glUniformBlockBinding(p, glGetUniformBlockIndex(p, "Material"), materialUniLoc);
    
    texUnit = glGetUniformLocation(p, "texUnit");
    
    return(p);
}



int OpenGLRenderer::init2D() {
    
    // Data for the two triangles
    float position[] = {
        1.0f, -1.0f, 0.0f, 1.0f,
        -1.0f, 1.0f, 0.0f, 1.0f,
        1.0f,  1.0f, 0.0f, 1.0f,
        
        -1.0f, 1.0f, 0.0f, 1.0f,
        1.0f, -1.0f, 0.0f, 1.0f,
        -1.0f,  -1.0f, 0.0f, 1.0f,
        
    };
    
    float textureCoord[] = {
        1.0f, 1.0f,
        0.0f, 0.0f,
        1.0f, 0.0f,
        
        0.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 1.0f,
        
    };
    
    
    // variables to hold the shader's source code
    char *vs = NULL, *fs = NULL;
    
    // holders for the shader's ids
    GLuint v, f;
    
    // create the two shaders
    v = glCreateShader(GL_VERTEX_SHADER);
    f = glCreateShader(GL_FRAGMENT_SHADER);
    
    // read the source code from file
    vs = textFileRead((char *)"shader\\videotexture.vert");
    fs = textFileRead((char *)"shader\\videotexture.frag");
    
    // castings for calling the shader source function
    const char * vv = vs;
    const char * ff = fs;
    
    // setting the source for each shader
    glShaderSource(v, 1, &vv, NULL);
    glShaderSource(f, 1, &ff, NULL);
    
    // free the source strings
    free(vs); free(fs);
    
    // compile the sources
    glCompileShader(v);
    glCompileShader(f);
    
    // create a program and attach the shaders
    videoProgram = glCreateProgram();
    glAttachShader(videoProgram, v);
    glAttachShader(videoProgram, f);
    
    // Bind the fragment data output variable location
    // requires linking afterwards
    glBindFragDataLocation(videoProgram, 0, "outputF");
    
    // link the program
    glLinkProgram(videoProgram);
    
    GLint myLoc = glGetUniformLocation(videoProgram, "texUnit");
    glProgramUniform1d(videoProgram, myLoc, 0);
    
    GLuint vertexLoc, texCoordLoc;
    
    // Get the locations of the attributes in the current program
    vertexLoc = glGetAttribLocation(videoProgram, "position");
    texCoordLoc = glGetAttribLocation(videoProgram, "texCoord");
    
    // Generate and bind a Vertex Array Object
    // this encapsulates the buffers used for drawing the triangle
    glGenVertexArrays(1, &videoVAO);
    glBindVertexArray(videoVAO);
    
    // Generate two slots for the position and color buffers
    GLuint buffers[2];
    glGenBuffers(2, buffers);
    
    // bind buffer for vertices and copy data into buffer
    glBindBuffer(GL_ARRAY_BUFFER, buffers[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(position), position, GL_STATIC_DRAW);
    glEnableVertexAttribArray(vertexLoc);
    glVertexAttribPointer(vertexLoc, 4, GL_FLOAT, 0, 0, 0);
    
    // bind buffer for normals and copy data into buffer
    glBindBuffer(GL_ARRAY_BUFFER, buffers[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(textureCoord), textureCoord, GL_STATIC_DRAW);
    glEnableVertexAttribArray(texCoordLoc);
    glVertexAttribPointer(texCoordLoc, 2, GL_FLOAT, 0, 0, 0);
    
    
    glGenTextures(1, &videotextureID); //Gen a new texture and store the handle in texname
    
    //These settings stick with the texture that's bound. You only need to set them
    //once.
    glBindTexture(GL_TEXTURE_2D, videotextureID);
    
    //allocate memory on the graphics card for the texture. It's fine if
    //texture_data doesn't have any data in it, the texture will just appear black
    //until you update it.
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, {});
    
    
    return true;
}





bool OpenGLRenderer::import3DFromFile(const std::string& pFile, aiScene*& scene, Assimp::Importer& importer, float& scaleFactor)
{
    std::string fileDir = modelDir + pFile;
    //check if file exists
    std::ifstream fin(fileDir.c_str());
    if (!fin.fail()) {
        fin.close();
    }
    else {
        printf("Couldn't open file: %s\n", fileDir.c_str());
        printf("%s\n", importer.GetErrorString());
        return false;
    }
    
    scene = const_cast<aiScene*>(importer.ReadFile(fileDir, aiProcessPreset_TargetRealtime_Quality));
    
    
    // If the import failed, report it
    if (!scene)
    {
        printf("%s\n", importer.GetErrorString());
        return false;
    }
    
    // Now we can access the file's contents.
    printf("Import of scene %s succeeded.\n", fileDir.c_str());
    
    aiVector3D scene_min, scene_max, scene_center;
    get_bounding_box(&scene_min, &scene_max, scene);
    /*float tmp;
     tmp = scene_max.x - scene_min.x;
     tmp = scene_max.y - scene_min.y > tmp ? scene_max.y - scene_min.y : tmp;
     tmp = scene_max.z - scene_min.z > tmp ? scene_max.z - scene_min.z : tmp;
     scaleFactor = 1.f / tmp;*/
    
    // We're done. Everything will be cleaned up by the importer destructor
    return true;
}


int OpenGLRenderer::loadGLTextures(aiScene* scene)
{
    //ILboolean success;
    
    /* initialization of DevIL */
    //ilInit();
    
    /* scan scene's materials for textures */
    for (unsigned int m = 0; m < scene->mNumMaterials; ++m)
    {
        int texIndex = 0;
        aiString path;    // filename
        
        aiReturn texFound = scene->mMaterials[m]->GetTexture(aiTextureType_DIFFUSE, texIndex, &path);
        while (texFound == AI_SUCCESS) {
            //fill map with textures, OpenGL image ids set to 0
            textureIdMap[path.data] = 0;
            // more textures?
            texIndex++;
            texFound = scene->mMaterials[m]->GetTexture(aiTextureType_DIFFUSE, texIndex, &path);
        }
    }
    
    int numTextures = textureIdMap.size();
    
    /* create and fill array with DevIL texture ids */
    //ILuint* imageIds = new ILuint[numTextures];
    //ilGenImages(numTextures, imageIds);
    
    /* create and fill array with GL texture ids */
    GLuint* textureIds = new GLuint[numTextures];
    glGenTextures(numTextures, textureIds); /* Texture name generation */
    
    /* get iterator */
    std::map<std::string, GLuint>::iterator itr = textureIdMap.begin();
    int i = 0;
    for (; itr != textureIdMap.end(); ++i, ++itr)
    {
        //save IL image ID
        std::string filename = (*itr).first;  // get filename
        //std::replace(filename.begin(), filename.end(), '\\', '/'); //Replace backslash with forward slash so linux can find the files NOPE
        filename = modelDir + filename;
        (*itr).second = textureIds[i];      // save texture id for filename in map
        
        //ilBindImage(imageIds[i]); /* Binding of DevIL image name */
        //ilEnable(IL_ORIGIN_SET);
        //ilOriginFunc(IL_ORIGIN_LOWER_LEFT);
        cv::Mat textureImg = cv::imread(filename);
        bool success = !textureImg.empty();
        
        if (success) {
            /* Convert image to RGBA */
            if(textureImg.channels()==3)
                cv::cvtColor(textureImg, textureImg, cv::COLOR_BGR2RGBA, 4);
            
            /* Create and load textures to OpenGL */
            glBindTexture(GL_TEXTURE_2D, textureIds[i]);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, textureImg.size().width,
                         textureImg.size().height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                         textureImg.data);
        }
        else
            printf("Couldn't load Image: %s\n", filename.c_str());
    }
    /* Opencv will automatically release memory not any longer used. */
    
    //Cleanup
    //delete[] imageIds;
    //delete[] textureIds;
    
    //return success;
    return true;
}


void OpenGLRenderer::genVAOsAndUniformBuffer(aiScene *sc, std::vector<struct MyMesh> &myMeshes) {
    
    struct MyMesh aMesh;
    struct MyMaterial aMat;
    GLuint buffer;
    
    // For each mesh
    for (unsigned int n = 0; n < sc->mNumMeshes; ++n)
    {
        const aiMesh* mesh = sc->mMeshes[n];
        
        // create array with faces
        // have to convert from Assimp format to array
        unsigned int *faceArray;
        faceArray = (unsigned int *)malloc(sizeof(unsigned int) * mesh->mNumFaces * 3);
        unsigned int faceIndex = 0;
        
        for (unsigned int t = 0; t < mesh->mNumFaces; ++t) {
            const aiFace* face = &mesh->mFaces[t];
            
            memcpy(&faceArray[faceIndex], face->mIndices, 3 * sizeof(unsigned int));
            faceIndex += 3;
        }
        aMesh.numFaces = sc->mMeshes[n]->mNumFaces;
        
        // generate Vertex Array for mesh
        glGenVertexArrays(1, &(aMesh.vao));
        glBindVertexArray(aMesh.vao);
        
        // buffer for faces
        glGenBuffers(1, &buffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * mesh->mNumFaces * 3, faceArray, GL_STATIC_DRAW);
        
        // buffer for vertex positions
        if (mesh->HasPositions()) {
            glGenBuffers(1, &buffer);
            glBindBuffer(GL_ARRAY_BUFFER, buffer);
            glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * mesh->mNumVertices, mesh->mVertices, GL_STATIC_DRAW);
            glEnableVertexAttribArray(vertexLoc);
            glVertexAttribPointer(vertexLoc, 3, GL_FLOAT, 0, 0, 0);
        }
        
        // buffer for vertex normals
        if (mesh->HasNormals()) {
            glGenBuffers(1, &buffer);
            glBindBuffer(GL_ARRAY_BUFFER, buffer);
            glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * mesh->mNumVertices, mesh->mNormals, GL_STATIC_DRAW);
            glEnableVertexAttribArray(normalLoc);
            glVertexAttribPointer(normalLoc, 3, GL_FLOAT, 0, 0, 0);
        }
        
        // buffer for vertex texture coordinates
        if (mesh->HasTextureCoords(0)) {
            float *texCoords = (float *)malloc(sizeof(float) * 2 * mesh->mNumVertices);
            for (unsigned int k = 0; k < mesh->mNumVertices; ++k) {
                
                texCoords[k * 2] = mesh->mTextureCoords[0][k].x;
                texCoords[k * 2 + 1] = mesh->mTextureCoords[0][k].y;
                
            }
            glGenBuffers(1, &buffer);
            glBindBuffer(GL_ARRAY_BUFFER, buffer);
            glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 2 * mesh->mNumVertices, texCoords, GL_STATIC_DRAW);
            glEnableVertexAttribArray(texCoordLoc);
            glVertexAttribPointer(texCoordLoc, 2, GL_FLOAT, 0, 0, 0);
        }
        
        // unbind buffers
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        
        // create material uniform buffer
        aiMaterial *mtl = sc->mMaterials[mesh->mMaterialIndex];
        
        aiString texPath;    //contains filename of texture
        if (AI_SUCCESS == mtl->GetTexture(aiTextureType_DIFFUSE, 0, &texPath)) {
            //bind texture
            unsigned int texId = textureIdMap[texPath.data];
            aMesh.texIndex = texId;
            aMat.texCount = 1;
        }
        else
            aMat.texCount = 0;
        
        float c[4];
        set_float4(c, 0.8f, 0.8f, 0.8f, 1.0f);
        aiColor4D diffuse;
        if (AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_DIFFUSE, &diffuse))
            color4_to_float4(&diffuse, c);
        memcpy(aMat.diffuse, c, sizeof(c));
        
        set_float4(c, 0.2f, 0.2f, 0.2f, 1.0f);
        aiColor4D ambient;
        if (AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_AMBIENT, &ambient))
            color4_to_float4(&ambient, c);
        memcpy(aMat.ambient, c, sizeof(c));
        
        set_float4(c, 0.0f, 0.0f, 0.0f, 1.0f);
        aiColor4D specular;
        if (AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_SPECULAR, &specular))
            color4_to_float4(&specular, c);
        memcpy(aMat.specular, c, sizeof(c));
        
        set_float4(c, 0.0f, 0.0f, 0.0f, 1.0f);
        aiColor4D emission;
        if (AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_EMISSIVE, &emission))
            color4_to_float4(&emission, c);
        memcpy(aMat.emissive, c, sizeof(c));
        
        float shininess = 0.0;
        unsigned int max;
        aiGetMaterialFloatArray(mtl, AI_MATKEY_SHININESS, &shininess, &max);
        aMat.shininess = shininess;
        
        glGenBuffers(1, &(aMesh.uniformBlockIndex));
        glBindBuffer(GL_UNIFORM_BUFFER, aMesh.uniformBlockIndex);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(aMat), (void *)(&aMat), GL_STATIC_DRAW);
        
        myMeshes.push_back(aMesh);
    }
}


// ------------------------------------------------------------
//
// Model loading and OpenGL setup
//

int OpenGLRenderer::loadModels() {
    
    
    map<int, string>::iterator it;
    
    for (it = modelMap.begin(); it != modelMap.end(); it++)
    {
        int markerNum = it->first;
        string modelName = it->second;
        
        models[markerNum].marker = markerNum;
        models[markerNum].scaleFactor = scaleMap.find(markerNum)->second;
        
        if (!import3DFromFile(modelName, models[markerNum].scene, models[markerNum].importer, models[markerNum].scaleFactor))
            return(0);
        // changed to a 0 for false...
        
        //load the textures for each model
        loadGLTextures(models[markerNum].scene);
        
        genVAOsAndUniformBuffer(models[markerNum].scene, models[markerNum].myMeshes);
        
    }
    //should be a boolean but I won't change the header file
    // 1 is true...
    return 1;
}


/* ---- Helper Functions  ------------------------------------------------------- */

/*
 *  initWindow
 *
 *  This is used to set up a simple window using GLFW.
 *  Returns true if sucessful otherwise false.
 */
bool OpenGLRenderer::initWindow(std::string windowName) {
    
    // Initialise GLFW
    if (!glfwInit())
    {
        fprintf(stderr, "Failed to initialize GLFW\n");
        getchar();
        return false;
    }
    
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    
    
    // Open a window and create its OpenGL context
    window = glfwCreateWindow(1280, 780, windowName.c_str(), NULL, NULL);
    if (window == NULL) {
        fprintf(stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. .\n");
        getchar();
        glfwTerminate();
        return false;
    }
    glfwMakeContextCurrent(window);
    
    
    
    return true;
    
}



// ----------------------------------------------------
// MATRIX STUFF
//

// Push and Pop for modelMatrix

void OpenGLRenderer::pushMatrix() {
    
    float *aux = (float *)malloc(sizeof(float) * 16);
    memcpy(aux, modelMatrix, sizeof(float) * 16);
    matrixStack.push_back(aux);
}

void OpenGLRenderer::popMatrix() {
    
    float *m = matrixStack[matrixStack.size() - 1];
    memcpy(modelMatrix, m, sizeof(float) * 16);
    matrixStack.pop_back();
    free(m);
}
// ----------------------------------------------------
// Model Matrix
//
// Copies the modelMatrix to the uniform buffer


void  OpenGLRenderer::setModelMatrix() {
    
    glBindBuffer(GL_UNIFORM_BUFFER, matricesUniBuffer);
    glBufferSubData(GL_UNIFORM_BUFFER,
                    ModelMatrixOffset, MatrixSize, modelMatrix);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    
}

// The equivalent to glTranslate applied to the model matrix
void OpenGLRenderer::translate(float x, float y, float z) {
    
    float aux[16];
    
    setTranslationMatrix(aux, x, y, z);
    multMatrix(modelMatrix, aux);
    setModelMatrix();
}

// The equivalent to glRotate applied to the model matrix
void  OpenGLRenderer::rotate(float angle, float x, float y, float z) {
    
    float aux[16];
    
    setRotationMatrix(aux, angle, x, y, z);
    multMatrix(modelMatrix, aux);
    setModelMatrix();
}

// The equivalent to glScale applied to the model matrix
void  OpenGLRenderer::scale(float x, float y, float z) {
    
    float aux[16];
    
    setScaleMatrix(aux, x, y, z);
    multMatrix(modelMatrix, aux);
    setModelMatrix();
}

// ----------------------------------------------------
// Projection Matrix
//
// Computes the projection Matrix and stores it in the uniform buffer
// for more information: http://www.songho.ca/opengl/gl_projectionmatrix.html

void  OpenGLRenderer::buildProjectionMatrix(float fov, float ratio, float nearp, float farp) {
    
    float projMatrix[16];
    
    float t_i = 1.0f / tan(fov * (M_PI / 360.0f));
    
    setIdentityMatrix(projMatrix, 4);
    
    projMatrix[0] = t_i / ratio;
    projMatrix[1 * 4 + 1] = t_i;
    projMatrix[2 * 4 + 2] = (farp + nearp) / (nearp - farp);
    projMatrix[3 * 4 + 2] = (2.0f * farp * nearp) / (nearp - farp);
    projMatrix[2 * 4 + 3] = -1.0f;
    projMatrix[3 * 4 + 3] = 0.0f;
    
    glBindBuffer(GL_UNIFORM_BUFFER, matricesUniBuffer);
    glBufferSubData(GL_UNIFORM_BUFFER, ProjMatrixOffset, MatrixSize, projMatrix);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    
}


// ----------------------------------------------------
// View Matrix
//
// Computes the viewMatrix and stores it in the uniform buffer
//
// note: it assumes the camera is not tilted,
// i.e. a vertical up vector along the Y axis (remember gluLookAt?)
//

void OpenGLRenderer::setCamera(cv::Mat viewMatrix) {
    
    //Set these to make the view matrix happy
    
    viewMatrix.at<float>(0, 3) = 0;
    viewMatrix.at<float>(1, 3) = 0;
    viewMatrix.at<float>(2, 3) = 0;
    viewMatrix.at<float>(3, 3) = 1;
    
    glBindBuffer(GL_UNIFORM_BUFFER, matricesUniBuffer);
    glBufferSubData(GL_UNIFORM_BUFFER, ViewMatrixOffset, MatrixSize, (float*)viewMatrix.data);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}


// ------------------------------------------------------------
//
// Render stuff
//

// Render Assimp Model

void OpenGLRenderer::recursive_render(aiScene *sc, const aiNode* nd, std::vector<struct MyMesh> &myMeshes)
{
    
    // Get node transformation matrix
    aiMatrix4x4 m = nd->mTransformation;
    // OpenGL matrices are column major
    m.Transpose();
    
    // save model matrix and apply node transformation
    pushMatrix();
    
    float aux[16];
    memcpy(aux, &m, sizeof(float) * 16);
    multMatrix(modelMatrix, aux);
    setModelMatrix();
    
    
    // draw all meshes assigned to this node
    for (unsigned int n = 0; n < nd->mNumMeshes; ++n) {
        // bind material uniform
        glBindBufferRange(GL_UNIFORM_BUFFER, materialUniLoc, myMeshes[nd->mMeshes[n]].uniformBlockIndex, 0, sizeof(struct MyMaterial));
        // bind texture
        glBindTexture(GL_TEXTURE_2D, myMeshes[nd->mMeshes[n]].texIndex);
        // bind VAO
        glBindVertexArray(myMeshes[nd->mMeshes[n]].vao);
        // draw
        glDrawElements(GL_TRIANGLES, myMeshes[nd->mMeshes[n]].numFaces * 3, GL_UNSIGNED_INT, 0);
        
    }
    
    // draw all children
    for (unsigned int n = 0; n < nd->mNumChildren; ++n) {
        recursive_render(sc, nd->mChildren[n], myMeshes);
    }
    popMatrix();
}



// ----------------------------------------------------------------------------


void OpenGLRenderer::prepareTexture(int w, int h, unsigned char* data) {
    
    /* Create and load texture to OpenGL */
    glBindTexture(GL_TEXTURE_2D, videotextureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                 w, h,
                 0, GL_RGB, GL_UNSIGNED_BYTE,
                 data);
    glGenerateMipmap(GL_TEXTURE_2D);
}


void OpenGLRenderer::updateMatrices(const CameraPose &pose) {
    
    map<int, MyModel>::iterator it;
    
    for (it = models.begin(); it != models.end(); it++) {
        it->second.seennow = false;
    }
    
    if (pose.rvec.empty()|| countNonZero(pose.rvec) < 1) {
        return;
    }
    
    for (unsigned int i = 0; i < markerIds.size(); i++) {
        cv::Mat viewMatrix = cv::Mat::zeros(4, 4, CV_32F);;
        cv::Mat viewMatrixavg = cv::Mat::zeros(4, 4, CV_32F);
        
        cv::Mat rV, tV;
        cv::transpose(pose.rvec, rV);
        cv::transpose(pose.tvec, tV);
        cv::Vec3d r((double*)rV.data);
        cv::Vec3d t((double*)tV.data);
        
        cv::Mat rot;
        Rodrigues(r, rot);
        
        for (unsigned int row = 0; row < 3; ++row)
        {
            for (unsigned int col = 0; col < 3; ++col)
            {
                viewMatrix.at<float>(row, col) = (float)rot.at<double>(row, col);
            }
            viewMatrix.at<float>(row, 3) = (float)t[row] * 0.1; //add scale to camera translation
        }
        viewMatrix.at<float>(3, 3) = 1.0f;
        
        cv::Mat cvToGl = cv::Mat::zeros(4, 4, CV_32F);
        cvToGl.at<float>(0, 0) = 1.0f;
        cvToGl.at<float>(1, 1) = -1.0f; // Invert the y axis
        cvToGl.at<float>(2, 2) = -1.0f; // invert the z axis
        cvToGl.at<float>(3, 3) = 1.0f;
        viewMatrix = cvToGl * viewMatrix;
        cv::transpose(viewMatrix, viewMatrix);
        
        
        
        if (modelMap.count(markerIds[i])) {
            models[markerIds[i]].seennow = true;
            
            models[markerIds[i]].viewMatrix[0] = viewMatrix;
            models[markerIds[i]].viewMatrix[1] = viewMatrix;
            models[markerIds[i]].viewMatrix[2] = viewMatrix;
            
        }
        
    }
    
    
    for (it = models.begin(); it != models.end(); it++) {
        if (!it->second.seennow) {
            if (it->second.seenlast == -10) {
                it->second.viewMatrix[0] = cv::Mat::zeros(4, 4, CV_32F);
                it->second.seenlast = 0;
            }
            else if (it->second.seenlast < 0) {
                it->second.seenlast = it->second.seenlast - 1;
            }
            else if (it->second.seenlast > 0) {
                it->second.seenlast = -1;
            }
            
        }
        else if (it->second.seenlast<100) {
            it->second.seenlast++;
        }
        
        
    }
}


/*
 *  render
 *
 *  This function renders the actual scene with all the models and video background
 */

void OpenGLRenderer::renderScene(void) {
    // Create Texture
    prepareTexture(imageMatGL.cols, imageMatGL.rows, imageMatGL.data);
    //gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, imageMat.cols, imageMat.rows, GL_RGB, GL_UNSIGNED_BYTE, imageMat.data);
    // clear the framebuffer (color and depth)
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // disable blending for the background and model
    glDisable(GL_BLEND);
    
    //1. Render video background
    
    // DRAW 2D VIDEO
    // Use the program p
    glUseProgram(videoProgram);
    // Bind the vertex array object
    glBindVertexArray(videoVAO);
    // Bind texture
    glBindTexture(GL_TEXTURE_2D, videotextureID); //TEST
    // draw the 6 vertices
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    
    //2. Compute Pose and set matrices (we did this already in the main loop)
    
    //3. Draw 3D model
    glClear(GL_DEPTH_BUFFER_BIT);
    
    map<int, MyModel>::iterator it;
    
    for (it = models.begin(); it != models.end(); it++)
    {
        auto currentModel = it->second;
        if (currentModel.seennow) {
            
            setCamera(currentModel.viewMatrix[0]);
            
            // set the model matrix to the identity Matrix
            setIdentityMatrix(modelMatrix, 4);
            
            // sets the model matrix to a scale matrix so that the model fits in the window
            scale(currentModel.scaleFactor, currentModel.scaleFactor, currentModel.scaleFactor);
            
            // keep rotating the model
            rotate(270.0f, 1.0f, 0.0f, 0.0f);
            
            // use our shadershader
            glUseProgram(program);
            
            // we are only going to use texture unit 0
            // unfortunately samplers can't reside in uniform blocks
            // so we have set this uniform separately
            glUniform1i(texUnit, 0);
            
            
            //glLoadMatrixf((float*)viewMatrix.data);
            recursive_render(currentModel.scene, currentModel.scene->mRootNode, currentModel.myMeshes);
        }
        
        
    }
     
    // TODO: Implement 3rd rendering pass here for edge blending
    extractColorEdges(imageMatGL);

    glClear(GL_DEPTH_BUFFER_BIT); // clear both?
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // Use the program p
    glUseProgram(videoProgram);
    // Bind the vertex array object
    glBindVertexArray(videoVAO);
    // draw the 6 vertices
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
}

//

/*
 *
 *
 *  FPS computation and display
 
 */
void OpenGLRenderer::displayFPS() {
    
    frame++;
    double timet = glfwGetTime();
    //cout << timet - timebase << std::endl;
    if (timet - timebase >= 1.0) {
        char s[32];
        sprintf(s, "ARRenderer FPS:%4.2f",
                frame / (timet - timebase));
        timebase = timet;
        frame = 0;
        glfwSetWindowTitle(window, s);
    }
    
}



/*
 *  render
 *
 *  This function is doing the actual render work. Also passes the pose information to the OpenGL renderer
 */

Event OpenGLRenderer::render(const CameraCalibration& calibration, const CameraPose& pose, const cv::Mat& image) {
    cv::cvtColor(image, imageMatGL, cv::COLOR_BGR2RGB);


    // Check if the ESC key was pressed or the window was closed
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS && glfwWindowShouldClose(window) == 0) {
        // Clear the screen.
        glClear(GL_COLOR_BUFFER_BIT);
        // pass the pose data and convert this to opengl coordinates systems
        updateMatrices(pose);
        renderScene();
        
        // Swap buffers
        glfwSwapBuffers(window);
        //as for events
        glfwPollEvents();
        
        displayFPS();
        
        return Event::NONE;
        
    }
    
    else {
        return Event::QUIT;
    }
}

