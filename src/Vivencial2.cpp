

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


glm::vec3 Ka_material; 
glm::vec3 Kd_material; 
glm::vec3 Ks_material; 
float Ns_material;     

string mtlFilePath = "";
string textureFilePath = "";
string basePath = "../assets/";

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 700;


bool rotateX = false;
bool rotateY = false;
bool rotateZ = false;
float objectScale = 0.5f; 

int verticesToDraw = 0; 


struct PointLight {
    glm::vec3 position;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
    float constant;
    float linear;
    float quadratic;
    bool enabled; 
};


PointLight keyLight;
PointLight fillLight;
PointLight backLight;

Camera camera; 


void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void setupWindow(GLFWwindow*& window);
void setupShaderLightsAndMaterials(Shader& shader); 
void readFromMtl(string path);
int setupGeometry();
int loadTexture(string path);
void readFromObj(string path);
void configureLights(const glm::vec3& objectPosition, float objectRadius);
void updateLightUniforms(Shader& shader);

int main()
{
    GLFWwindow* window;
    setupWindow(window);

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    Shader shader("../shaders/sprite.vs", "../shaders/sprite.fs");
    glUseProgram(shader.ID);
    shader.setInt("tex_buffer", 0);

    
    camera.initialize(&shader, WINDOW_WIDTH, WINDOW_HEIGHT);

    readFromObj(basePath + "Modelos3D/Suzanne.obj"); 
    readFromMtl(basePath + "Modelos3D/Suzanne.mtl"); 
    GLuint texID = loadTexture(basePath + "Modelos3D/Suzanne.png"); 

    GLuint VAO = setupGeometry();

    
    glm::vec3 suzannePosition = glm::vec3(0.0f, 0.0f, 0.0f);
    float suzanneRadius = 0.5f; 

    
    configureLights(suzannePosition, suzanneRadius);

    
    setupShaderLightsAndMaterials(shader); 

    glEnable(GL_DEPTH_TEST);

    
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        int currentWidth, currentHeight;
        glfwGetFramebufferSize(window, &currentWidth, &currentHeight);
        glViewport(0, 0, currentWidth, currentHeight);

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        camera.update(); 

        
        glm::mat4 model = glm::mat4(1);
        model = glm::translate(model, suzannePosition);
        model = glm::scale(model, glm::vec3(objectScale));

        float angle = (GLfloat)glfwGetTime(); 
        if (rotateX) model = glm::rotate(model, angle, glm::vec3(1.0f, 0.0f, 0.0f));
        if (rotateY) model = glm::rotate(model, angle, glm::vec3(0.0f, 1.0f, 0.0f));
        if (rotateZ) model = glm::rotate(model, angle, glm::vec3(0.0f, 0.0f, 1.0f));

        shader.setMat4("model", model); 

        
        updateLightUniforms(shader);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texID);
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, verticesToDraw);
        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);

        glfwSwapBuffers(window);
    }

    glDeleteVertexArrays(1, &VAO);
    glfwTerminate();
    return 0;
}


void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    
    if (key == GLFW_KEY_1 && action == GLFW_PRESS) keyLight.enabled = !keyLight.enabled;
    if (key == GLFW_KEY_2 && action == GLFW_PRESS) fillLight.enabled = !fillLight.enabled;
    if (key == GLFW_KEY_3 && action == GLFW_PRESS) backLight.enabled = !backLight.enabled;

    
    if (key == GLFW_KEY_X && action == GLFW_PRESS) { rotateX = !rotateX; rotateY = false; rotateZ = false; }
    if (key == GLFW_KEY_Y && action == GLFW_PRESS) { rotateY = !rotateY; rotateX = false; rotateZ = false; }
    if (key == GLFW_KEY_Z && action == GLFW_PRESS) { rotateZ = !rotateZ; rotateX = false; rotateY = false; }
    if (key == GLFW_KEY_P && action == GLFW_PRESS) { rotateX = false; rotateY = false; rotateZ = false; } 

    
    camera.setCameraPos(key);
}


void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    camera.mouseCallback(window, xpos, ypos);
}


void setupWindow(GLFWwindow*& window) {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Vivencial 2 - Conrado, Gabriel Figueiredo e Julia Martins", nullptr, nullptr);
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


void configureLights(const glm::vec3& objectPosition, float objectRadius) {
    
    keyLight.position = objectPosition + glm::vec3(objectRadius * 2.0f, objectRadius * 2.0f, objectRadius * 2.0f);
    keyLight.ambient = glm::vec3(0.2f);
    keyLight.diffuse = glm::vec3(0.8f);
    keyLight.specular = glm::vec3(1.0f);
    keyLight.constant = 1.0f;
    keyLight.linear = 0.09f;
    keyLight.quadratic = 0.032f;
    keyLight.enabled = true;

    
    fillLight.position = objectPosition + glm::vec3(-objectRadius * 1.5f, objectRadius * 0.5f, objectRadius * 1.0f);
    fillLight.ambient = glm::vec3(0.05f);
    fillLight.diffuse = glm::vec3(0.4f);
    fillLight.specular = glm::vec3(0.5f);
    fillLight.constant = 1.0f;
    fillLight.linear = 0.09f;
    fillLight.quadratic = 0.032f;
    fillLight.enabled = true;

    
    backLight.position = objectPosition + glm::vec3(0.0f, objectRadius * 2.5f, -objectRadius * 2.0f);
    backLight.ambient = glm::vec3(0.01f);
    backLight.diffuse = glm::vec3(0.3f);
    backLight.specular = glm::vec3(0.4f);
    backLight.constant = 1.0f;
    backLight.linear = 0.09f;
    backLight.quadratic = 0.032f;
    backLight.enabled = true;
}


void setupShaderLightsAndMaterials(Shader& shader) {
    
    shader.setVec3("material.Ka", Ka_material);
    shader.setVec3("material.Kd", Kd_material);
    shader.setVec3("material.Ks", Ks_material);
    shader.setFloat("material.Ns", Ns_material);

    
    shader.setVec3("keyLight.position", keyLight.position);
    shader.setVec3("keyLight.ambient", keyLight.ambient);
    shader.setVec3("keyLight.diffuse", keyLight.diffuse);
    shader.setVec3("keyLight.specular", keyLight.specular);
    shader.setFloat("keyLight.constant", keyLight.constant);
    shader.setFloat("keyLight.linear", keyLight.linear);
    shader.setFloat("keyLight.quadratic", keyLight.quadratic);

    
    shader.setVec3("fillLight.position", fillLight.position);
    shader.setVec3("fillLight.ambient", fillLight.ambient);
    shader.setVec3("fillLight.diffuse", fillLight.diffuse);
    shader.setVec3("fillLight.specular", fillLight.specular);
    shader.setFloat("fillLight.constant", fillLight.constant);
    shader.setFloat("fillLight.linear", fillLight.linear);
    shader.setFloat("fillLight.quadratic", fillLight.quadratic);

    
    shader.setVec3("backLight.position", backLight.position);
    shader.setVec3("backLight.ambient", backLight.ambient);
    shader.setVec3("backLight.diffuse", backLight.diffuse);
    shader.setVec3("backLight.specular", backLight.specular);
    shader.setFloat("backLight.constant", backLight.constant);
    shader.setFloat("backLight.linear", backLight.linear);
    shader.setFloat("backLight.quadratic", backLight.quadratic);
}


void updateLightUniforms(Shader& shader) {
    shader.setBool("keyLight.enabled", keyLight.enabled);
    shader.setBool("fillLight.enabled", fillLight.enabled);
    shader.setBool("backLight.enabled", backLight.enabled);
}


void readFromMtl(string path)
{
    string line, readValue;
    ifstream mtlFile(path);

    if (!mtlFile.is_open()) {
        cerr << "Erro ao abrir arquivo MTL: " << path << endl;
        
        Ka_material = glm::vec3(0.1f, 0.1f, 0.1f);
        Kd_material = glm::vec3(0.7f, 0.7f, 0.7f);
        Ks_material = glm::vec3(1.0f, 1.0f, 1.0f);
        Ns_material = 32.0f;
        textureFilePath = ""; 
        return;
    }

    
    Ka_material = glm::vec3(0.1f, 0.1f, 0.1f);
    Kd_material = glm::vec3(0.7f, 0.7f, 0.7f);
    Ks_material = glm::vec3(1.0f, 1.0f, 1.0f);
    Ns_material = 32.0f;

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
            iss >> Ka_material.x >> Ka_material.y >> Ka_material.z;
        }
        else if (prefix == "Kd")
        {
            iss >> Kd_material.x >> Kd_material.y >> Kd_material.z;
        }
        else if (prefix == "Ks")
        {
            iss >> Ks_material.x >> Ks_material.y >> Ks_material.z;
        }
        else if (prefix == "Ns")
        {
            iss >> Ns_material;
        }
    }
    mtlFile.close();
}


vector<GLfloat> global_vertices;
vector<GLfloat> global_textures;
vector<GLfloat> global_normals;


int setupGeometry()
{
    GLuint VAO, VBO[3];

    glGenVertexArrays(1, &VAO);
    glGenBuffers(3, VBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
    glBufferData(GL_ARRAY_BUFFER, global_vertices.size() * sizeof(GLfloat), global_vertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0); 
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, VBO[1]);
    glBufferData(GL_ARRAY_BUFFER, global_textures.size() * sizeof(GLfloat), global_textures.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0); 
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, VBO[2]);
    glBufferData(GL_ARRAY_BUFFER, global_normals.size() * sizeof(GLfloat), global_normals.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0); 
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return VAO;
}


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


void readFromObj(string path) {
    std::ifstream file(path);

    if (!file.is_open()) {
        std::cerr << "Failed to open OBJ file: " << path << std::endl;
        return;
    }

    std::vector<glm::vec3> temp_vertices;
    std::vector<glm::vec2> temp_textures;
    std::vector<glm::vec3> temp_normais;

    std::string line;

    
    global_vertices.clear();
    global_textures.clear();
    global_normals.clear();

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
                for (int i = 0; i < 3; ++i) { 
                    unsigned int vertexIndex, textIndex, normalIndex;
                    char slash;

                    iss >> vertexIndex >> slash >> textIndex >> slash >> normalIndex;

                    
                    global_vertices.push_back(temp_vertices[vertexIndex - 1].x);
                    global_vertices.push_back(temp_vertices[vertexIndex - 1].y);
                    global_vertices.push_back(temp_vertices[vertexIndex - 1].z);

                    global_textures.push_back(temp_textures[textIndex - 1].x);
                    global_textures.push_back(1.0f - temp_textures[textIndex - 1].y); 

                    global_normals.push_back(temp_normais[normalIndex - 1].x);
                    global_normals.push_back(temp_normais[normalIndex - 1].y);
                    global_normals.push_back(temp_normais[normalIndex - 1].z);
                }
            } else if (prefix == "mtllib") {
                iss >> mtlFilePath;
            }
        }
    }
    file.close();
    verticesToDraw = global_vertices.size() / 3;
    std::cout << "OBJ file loaded: " << path << ". Total vertices for drawing: " << verticesToDraw << std::endl;
}