#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <random>

using namespace std;

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "Shader.h"
#include "Camera.h"
#include "Mesh.h" // Assuming you have a Mesh class for better object handling

// Global variables (consider encapsulating in a scene class for larger projects)
vector<GLfloat> global_vertices;
vector<GLfloat> global_textures;
vector<GLfloat> global_normals;

glm::vec3 Ka; // Ambient material color
glm::vec3 Kd; // Diffuse material color
glm::vec3 Ks; // Specular material color
float Ns;     // Shininess exponent

string mtlFilePath = "";
string textureFilePath = "";
string basePath = "../assets/"; // Adjust this if your assets are elsewhere

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 700;

bool rotateX = false;
bool rotateY = false;
bool rotateZ = false;

// Removed global objectScale as it will be per-object

int verticesToDraw = 0; // Number of vertices for the loaded OBJ model

// Structure to hold properties of each object in the scene
struct SceneObject {
    GLuint VAO;
    GLuint textureID;
    int numVertices;
    glm::vec3 position;
    glm::vec3 scale;
    float rotationAngle;
    glm::vec3 rotationAxis;
    // You might want to add material properties here too if they differ per object
    // glm::vec3 Ka, Kd, Ks;
    // float Ns;
};

std::vector<SceneObject> sceneObjects; // List of objects in the scene
int selectedObjectIndex = 0;          // Index of the currently selected object

Camera camera;

// Function Prototypes
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void setupWindow(GLFWwindow*& window);
void resetAllRotateFlags(); // Renamed from resetAllRotate to avoid confusion
void readFromMtl(string path);
int setupGeometry(const vector<GLfloat>& vertices, const vector<GLfloat>& textures, const vector<GLfloat>& normals, int& numVertices);
int loadTexture(string path);
void readFromObj(string path, vector<GLfloat>& out_vertices, vector<GLfloat>& out_textures, vector<GLfloat>& out_normals, string& out_mtlFilePath);

int main()
{
    GLFWwindow* window;
    setupWindow(window);

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    // Initialize Shader
    Shader shader("../shaders/sprite.vs", "../shaders/sprite.fs");
    glUseProgram(shader.ID);
    shader.setInt("tex_buffer", 0);

    // Setup camera
    camera.initialize(&shader, WINDOW_WIDTH, WINDOW_HEIGHT);

    // --- Object 1: Suzanne (main object) ---
    vector<GLfloat> suzanne_vertices, suzanne_textures, suzanne_normals;
    string suzanne_mtlPath;
    readFromObj(basePath + "Modelos3D/Suzanne.obj", suzanne_vertices, suzanne_textures, suzanne_normals, suzanne_mtlPath);
    readFromMtl(basePath + "Modelos3D/" + suzanne_mtlPath); // Load Suzanne's material
    GLuint suzanne_texID = loadTexture(basePath + "Modelos3D/Suzanne.png"); // Load Suzanne's texture

    SceneObject suzanne;
    suzanne.position = glm::vec3(0.0f, 0.0f, 0.0f);
    suzanne.scale = glm::vec3(0.5f);
    suzanne.rotationAngle = 0.0f;
    suzanne.rotationAxis = glm::vec3(0.0f, 1.0f, 0.0f); // Default rotation axis
    suzanne.textureID = suzanne_texID;
    suzanne.VAO = setupGeometry(suzanne_vertices, suzanne_textures, suzanne_normals, suzanne.numVertices);
    sceneObjects.push_back(suzanne);

    // --- Object 2: Cube ---
    vector<GLfloat> cube_vertices, cube_textures, cube_normals;
    string cube_mtlPath;
    readFromObj(basePath + "Modelos3D/Cube.obj", cube_vertices, cube_textures, cube_normals, cube_mtlPath);
    // For simplicity, let's reuse Suzanne's material/texture for the cube or define new ones if needed.
    // For this example, I'll just use Suzanne's texture for the cube as well.
    // In a real application, you'd load Cube.mtl and Cube.png
    GLuint cube_texID = loadTexture(basePath + "Modelos3D/Suzanne.png"); // Using Suzanne's texture for cube

    SceneObject cube;
    cube.position = glm::vec3(1.5f, 0.0f, 0.0f); // Position cube next to Suzanne
    cube.scale = glm::vec3(0.3f);
    cube.rotationAngle = 0.0f;
    cube.rotationAxis = glm::vec3(0.0f, 1.0f, 0.0f);
    cube.textureID = cube_texID;
    cube.VAO = setupGeometry(cube_vertices, cube_textures, cube_normals, cube.numVertices);
    sceneObjects.push_back(cube);


    // Set initial lighting properties (these are general for the scene, not per-object for now)
    shader.setVec3("material.Ka", Ka);
    shader.setVec3("material.Kd", Kd);
    shader.setVec3("material.Ks", Ks);
    shader.setFloat("material.Ns", Ns);

    shader.setVec3("light.position", 1.0f, 1.0f, 1.0f);
    shader.setVec3("light.ambient", 0.1f, 0.1f, 0.1f);
    shader.setVec3("light.diffuse", 0.8f, 0.8f, 0.8f);
    shader.setVec3("light.specular", 1.0f, 1.0f, 1.0f);

    glEnable(GL_DEPTH_TEST); // Enable depth testing

    // Game loop
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        int currentWidth, currentHeight;
        glfwGetFramebufferSize(window, &currentWidth, &currentHeight);
        glViewport(0, 0, currentWidth, currentHeight);

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        camera.update(); // Update camera's view and projection matrices in the shader

        // Render all objects
        for (size_t i = 0; i < sceneObjects.size(); ++i) {
            SceneObject& obj = sceneObjects[i];

            glm::mat4 model = glm::mat4(1);

            // Apply transformations
            model = glm::translate(model, obj.position);
            model = glm::scale(model, obj.scale);

            // Apply rotation only if this is the currently selected object and a rotation flag is true
            if (i == selectedObjectIndex) {
                if (rotateX) {
                    obj.rotationAxis = glm::vec3(1.0f, 0.0f, 0.0f);
                    obj.rotationAngle += 0.05f;
                } else if (rotateY) {
                    obj.rotationAxis = glm::vec3(0.0f, 1.0f, 0.0f);
                    obj.rotationAngle += 0.05f;
                } else if (rotateZ) {
                    obj.rotationAxis = glm::vec3(0.0f, 0.0f, 1.0f);
                    obj.rotationAngle += 0.05f;
                }
            }
            model = glm::rotate(model, obj.rotationAngle, obj.rotationAxis);

            shader.setMat4("model", model); // Send model matrix to shader

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, obj.textureID);
            glBindVertexArray(obj.VAO);
            glDrawArrays(GL_TRIANGLES, 0, obj.numVertices);
            glBindVertexArray(0);
        }

        glBindTexture(GL_TEXTURE_2D, 0); // Unbind texture

        glfwSwapBuffers(window);
    }

    // Clean up
    for (const auto& obj : sceneObjects) {
        glDeleteVertexArrays(1, &obj.VAO);
    }
    glfwTerminate();
    return 0;
}

// Keyboard callback function
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    const float scaleStep = 0.1f;
    const float translateStep = 0.1f;

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    // Object selection
    if (key == GLFW_KEY_TAB && action == GLFW_PRESS) {
        selectedObjectIndex = (selectedObjectIndex + 1) % sceneObjects.size();
        resetAllRotateFlags(); // Reset rotation flags when changing selected object
        cout << "Selected object: " << selectedObjectIndex << endl;
    }

    // Transformations for the selected object
    if (!sceneObjects.empty()) {
        SceneObject& currentObject = sceneObjects[selectedObjectIndex];

        if (action == GLFW_PRESS || action == GLFW_REPEAT) {
            // Translation
            if (key == GLFW_KEY_W) currentObject.position.z -= translateStep;
            if (key == GLFW_KEY_S) currentObject.position.z += translateStep;
            if (key == GLFW_KEY_A) currentObject.position.x -= translateStep;
            if (key == GLFW_KEY_D) currentObject.position.x += translateStep;
            if (key == GLFW_KEY_Q) currentObject.position.y += translateStep; // Up
            if (key == GLFW_KEY_E) currentObject.position.y -= translateStep; // Down

            // Scaling
            if (key == GLFW_KEY_UP) { // Uniform scale up
                currentObject.scale += scaleStep;
            }
            if (key == GLFW_KEY_DOWN) { // Uniform scale down
                currentObject.scale -= scaleStep;
                if (currentObject.scale.x < 0.1f) currentObject.scale = glm::vec3(0.1f);
            }

            // Rotation axis selection (toggle)
            if (key == GLFW_KEY_X && action == GLFW_PRESS) {
                rotateX = !rotateX; rotateY = false; rotateZ = false;
            }
            if (key == GLFW_KEY_Y && action == GLFW_PRESS) {
                rotateY = !rotateY; rotateX = false; rotateZ = false;
            }
            if (key == GLFW_KEY_Z && action == GLFW_PRESS) {
                rotateZ = !rotateZ; rotateX = false; rotateY = false;
            }
            if (key == GLFW_KEY_P && action == GLFW_PRESS) { // Pause rotation
                resetAllRotateFlags();
            }
        }
    }

    // Camera movement (independent of object selection)
    camera.setCameraPos(key);
}

// Mouse callback function
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    camera.mouseCallback(window, xpos, ypos);
}

// Setup GLFW window
void setupWindow(GLFWwindow*& window) {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Vivencial 1 - Selecao e Transformacoes", nullptr, nullptr);
    glfwMakeContextCurrent(window);

    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
    }

    const GLubyte* renderer = glGetString(GL_RENDERER);
    const GLubyte* version = glGetString(GL_VERSION);
    cout << "Renderer: " << renderer << endl;
    cout << "OpenGL version supported " << version << endl;
}

// Reset rotation flags
void resetAllRotateFlags() {
    rotateX = false;
    rotateY = false;
    rotateZ = false;
}

// Reads material properties from MTL file
void readFromMtl(string path)
{
    string line, readValue;
    ifstream mtlFile(path);

    if (!mtlFile.is_open()) {
        cerr << "Erro ao abrir arquivo MTL: " << path << endl;
        // Set default material properties if file not found
        Ka = glm::vec3(0.1f, 0.1f, 0.1f);
        Kd = glm::vec3(0.7f, 0.7f, 0.7f);
        Ks = glm::vec3(1.0f, 1.0f, 1.0f);
        Ns = 32.0f;
        textureFilePath = ""; // Clear any previous texture path
        return;
    }

    // Default values in case they are not specified in the MTL
    Ka = glm::vec3(0.1f, 0.1f, 0.1f);
    Kd = glm::vec3(0.7f, 0.7f, 0.7f);
    Ks = glm::vec3(1.0f, 1.0f, 1.0f);
    Ns = 32.0f;

    while (getline(mtlFile, line))
    {
        istringstream iss(line);
        string prefix;
        iss >> prefix;

        if (prefix == "map_Kd")
        {
            iss >> textureFilePath;
        }
        else if (prefix == "Ka")
        {
            iss >> Ka.x >> Ka.y >> Ka.z;
        }
        else if (prefix == "Kd")
        {
            iss >> Kd.x >> Kd.y >> Kd.z;
        }
        else if (prefix == "Ks")
        {
            iss >> Ks.x >> Ks.y >> Ks.z;
        }
        else if (prefix == "Ns")
        {
            iss >> Ns;
        }
    }
    mtlFile.close();
}

// Setup VAO and VBOs for object geometry
int setupGeometry(const vector<GLfloat>& vertices_in, const vector<GLfloat>& textures_in, const vector<GLfloat>& normals_in, int& numVertices)
{
    GLuint VAO, VBO[3];

    glGenVertexArrays(1, &VAO);
    glGenBuffers(3, VBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
    glBufferData(GL_ARRAY_BUFFER, vertices_in.size() * sizeof(GLfloat), vertices_in.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0); // Position
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, VBO[1]);
    glBufferData(GL_ARRAY_BUFFER, textures_in.size() * sizeof(GLfloat), textures_in.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0); // Texture coordinates
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, VBO[2]);
    glBufferData(GL_ARRAY_BUFFER, normals_in.size() * sizeof(GLfloat), normals_in.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0); // Normals
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    numVertices = vertices_in.size() / 3; // Each vertex has 3 floats for position

    return VAO;
}

// Loads a texture from a file
int loadTexture(string path)
{
    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int width, height, nrChannels;
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrChannels, 0);

    if (data) {
        GLenum format = (nrChannels == 3) ? GL_RGB : GL_RGBA;
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    } else {
        cout << "Failed to load texture: " << path << endl;
    }

    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);

    return texID;
}

// Reads OBJ file data
void readFromObj(string path, vector<GLfloat>& out_vertices, vector<GLfloat>& out_textures, vector<GLfloat>& out_normals, string& out_mtlFilePath) {
    std::ifstream file(path);

    if (!file.is_open()) {
        std::cerr << "Failed to open OBJ file: " << path << std::endl;
        return;
    }

    std::vector<glm::vec3> temp_vertices;
    std::vector<glm::vec2> temp_textures;
    std::vector<glm::vec3> temp_normais;

    std::string line;

    // Clear output vectors for new object
    out_vertices.clear();
    out_textures.clear();
    out_normals.clear();

    while (std::getline(file, line)) {
        if (line.length() > 0) {
            std::istringstream iss(line);
            std::string prefix;
            iss >> prefix;

            if (prefix == "v") {
                glm::vec3 values;
                iss >> values.x >> values.y >> values.z;
                temp_vertices.push_back(values);
            } else if (prefix == "vt") {
                glm::vec2 values;
                iss >> values.x >> values.y;
                temp_textures.push_back(values);
            } else if (prefix == "vn") {
                glm::vec3 values;
                iss >> values.x >> values.y >> values.z;
                temp_normais.push_back(values);
            } else if (prefix == "f") {
                for (int i = 0; i < 3; ++i) { // Assuming triangular faces
                    unsigned int vertexIndex, textIndex, normalIndex;
                    char slash;

                    iss >> vertexIndex >> slash >> textIndex >> slash >> normalIndex;

                    // OBJ indices are 1-based, convert to 0-based
                    out_vertices.push_back(temp_vertices[vertexIndex - 1].x);
                    out_vertices.push_back(temp_vertices[vertexIndex - 1].y);
                    out_vertices.push_back(temp_vertices[vertexIndex - 1].z);

                    out_textures.push_back(temp_textures[textIndex - 1].x);
                    out_textures.push_back(1.0f - temp_textures[textIndex - 1].y); // Flip V coordinate for OpenGL

                    out_normals.push_back(temp_normais[normalIndex - 1].x);
                    out_normals.push_back(temp_normais[normalIndex - 1].y);
                    out_normals.push_back(temp_normais[normalIndex - 1].z);
                }
            } else if (prefix == "mtllib") {
                iss >> out_mtlFilePath;
            }
        }
    }
    file.close();
    std::cout << "OBJ file loaded: " << path << std::endl;
}                 out_normals.push_back(temp_normais[normalIndex - 1].x);
                    out_normals.push_back(temp_normais[normalIndex - 1].y);
                    out_normals.push_back(temp_normais[normalIndex - 1].z);
                }
            } else if (prefix == "mtllib") {
                iss >> out_mtlFilePath;
            }
        }
    }
    file.close();
    std::cout << "OBJ file loaded: " << path << std::endl;
}