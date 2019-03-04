#ifdef _WINDOWS
#include <GL/glew.h>
#endif
#include <SDL.h>
#define GL_GLEXT_PROTOTYPES 1
#include <SDL_opengl.h>
#include <SDL_image.h>
#include "ShaderProgram.h"
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#ifdef _WINDOWS
#define RESOURCE_FOLDER ""
#else
#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

SDL_Window* displayWindow;
GLuint LoadTexture(const char *filepath);

int main(int argc, char *argv[])
{
    SDL_Init(SDL_INIT_VIDEO);
    displayWindow = SDL_CreateWindow("My Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 720, 360, SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
    SDL_GL_MakeCurrent(displayWindow, context);
    
    ShaderProgram untexturedProgram;
    ShaderProgram texturedProgram;
    untexturedProgram.Load(RESOURCE_FOLDER"vertex.glsl", RESOURCE_FOLDER"fragment.glsl");
    texturedProgram.Load(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");

    GLuint dog = LoadTexture(RESOURCE_FOLDER"dog.png");
    GLuint sun = LoadTexture(RESOURCE_FOLDER"sun.png");
    GLuint airplane = LoadTexture(RESOURCE_FOLDER"airplane.png");
    
    glClearColor(0.89f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    //untextured
    //Grass
    glm::mat4 mM1 = glm::mat4(1.0f);
    mM1 = glm::translate(mM1, glm::vec3(0.0f, -1.0f, 0.0f));
    
    //House
    glm::mat4 mM2 = glm::mat4(1.0f);
    mM2 = glm::translate(mM2, glm::vec3(0.0f, -0.3f, 0.0f));
    
    //Roof
    glm::mat4 mM3 = glm::mat4(1.0f);
    mM3 = glm::translate(mM3, glm::vec3(0.0f, 0.1f, 0.0f));
    
    glm::mat4 untexturedviewMatrix = glm::mat4(1.0f);
    glm::mat4 untexturedprojectionMatrix = glm::mat4(1.0f);

    //textured
    //Sun
    glm::mat4 mM4 = glm::mat4(1.0f);
    mM4 = glm::translate(mM4, glm::vec3(-2.0f, 1.0f, 0.0f));
    
    //Dog
    glm::mat4 mM5 = glm::mat4(1.0f);
    mM5 = glm::translate(mM5, glm::vec3(-1.5f, -0.5f, 0.0f));
    mM5 = glm::scale(mM5, glm::vec3(0.8f, 0.8f, 0.0f));
    
    //Airplane
    glm::mat4 mM6 = glm::mat4(1.0f);
    mM6 = glm::translate(mM6, glm::vec3(1.25f, 0.5f, 0.0f));
    mM6 = glm::scale(mM6, glm::vec3(1.5f, 1.25f, 0.0f));
    mM6 = glm::rotate(mM6, 10.0f * (3.1415926f / 180.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    
    glm::mat4 texturedviewMatrix = glm::mat4(1.0f);
    glm::mat4 texturedprojectionMatrix = glm::mat4(1.0f);

    float aspectRatio = 720.0f / 360.0f;
    float projectionHeight = 1.0f;
    float projectionWidth = projectionHeight * aspectRatio;
    float projectionDepth = 1.0f;
    untexturedprojectionMatrix = glm::ortho(-projectionWidth, projectionWidth, -projectionHeight, projectionHeight,
                                            -projectionDepth, projectionDepth);
    texturedprojectionMatrix = glm::ortho(-projectionWidth, projectionWidth, -projectionHeight, projectionHeight,
                                          -projectionDepth, projectionDepth);
    
    untexturedProgram.SetModelMatrix(mM1);
    untexturedProgram.SetModelMatrix(mM2);
    untexturedProgram.SetModelMatrix(mM3);

    untexturedProgram.SetProjectionMatrix(untexturedviewMatrix);
    untexturedProgram.SetViewMatrix(untexturedviewMatrix);
    glUseProgram(untexturedProgram.programID);
    
    texturedProgram.SetModelMatrix(mM4);
    texturedProgram.SetModelMatrix(mM5);
    texturedProgram.SetModelMatrix(mM6);

    texturedProgram.SetProjectionMatrix(texturedprojectionMatrix);
    texturedProgram.SetViewMatrix(texturedviewMatrix);
    glUseProgram(texturedProgram.programID);

    float lastFrameTicks = 0.0f;

#ifdef _WINDOWS
    glewInit();
#endif

    SDL_Event event;
    bool done = false;
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    while (!done) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
                done = true;
            }
        }

        glClear(GL_COLOR_BUFFER_BIT);
        
        float ticks = (float)SDL_GetTicks()/1000.0f;
        float elapsed = ticks - lastFrameTicks;
        lastFrameTicks = ticks;

        //Grass - Green Rectangle
        untexturedProgram.SetColor(0.2f, 0.8f, 0.4f, 1.0f);
        float vertices1[] = {-2.0, -0.25, 2.0, -0.25, 2.0, 0.25, -2.0, -0.25, 2.0, 0.25, -2.0, 0.25};
        glVertexAttribPointer(untexturedProgram.positionAttribute, 2, GL_FLOAT, false, 0, vertices1);
        untexturedProgram.SetModelMatrix(mM1);
        glEnableVertexAttribArray(untexturedProgram.positionAttribute);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        
        //House Base - Gray Square
        untexturedProgram.SetColor(0.9f, 0.9f, 0.9f, 1.0f);
        float vertices2[] = {-0.25, -0.5, 0.25, -0.5, 0.25, 0.5, -0.25, -0.5, 0.25, 0.5, -0.25, 0.5};
        glVertexAttribPointer(untexturedProgram.positionAttribute, 2, GL_FLOAT, false, 0, vertices2);
        untexturedProgram.SetModelMatrix(mM2);
        glEnableVertexAttribArray(untexturedProgram.positionAttribute);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        
        //House Roof - Blue Square
        untexturedProgram.SetColor(0.0f, 0.0f, 1.0f, 1.0f);
        float vertices3[] = {-0.3, -0.1, 0.3, -0.1, 0.3, 0.1, -0.3, -0.1, 0.3, 0.1, -0.3, 0.1};
        glVertexAttribPointer(untexturedProgram.positionAttribute, 2, GL_FLOAT, false, 0, vertices3);
        untexturedProgram.SetModelMatrix(mM3);
        glEnableVertexAttribArray(untexturedProgram.positionAttribute);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        
        //Sun
        glBindTexture(GL_TEXTURE_2D, sun);
        float textvertices[] = {-0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5};
        glVertexAttribPointer(texturedProgram.positionAttribute, 2, GL_FLOAT, false, 0, textvertices);
        glEnableVertexAttribArray(texturedProgram.positionAttribute);
        
        float texCoords[] = {0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0};
        glVertexAttribPointer(texturedProgram.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
        glEnableVertexAttribArray(texturedProgram.texCoordAttribute);
        float angle = 0.01f * (3.1415926f / 180.0f) + elapsed;
        mM4 = glm::rotate(mM4, angle, glm::vec3(0.0f, 0.0f, 1.0f));
        texturedProgram.SetModelMatrix(mM4);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        
        //Dog
        glBindTexture(GL_TEXTURE_2D, dog);
        glVertexAttribPointer(texturedProgram.positionAttribute, 2, GL_FLOAT, false, 0, textvertices);
        glEnableVertexAttribArray(texturedProgram.positionAttribute);
        
        glVertexAttribPointer(texturedProgram.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
        glEnableVertexAttribArray(texturedProgram.texCoordAttribute);
        texturedProgram.SetModelMatrix(mM5);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        
        //Dog
        glBindTexture(GL_TEXTURE_2D, airplane);
        glVertexAttribPointer(texturedProgram.positionAttribute, 2, GL_FLOAT, false, 0, textvertices);
        glEnableVertexAttribArray(texturedProgram.positionAttribute);
        
        glVertexAttribPointer(texturedProgram.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
        glEnableVertexAttribArray(texturedProgram.texCoordAttribute);
        texturedProgram.SetModelMatrix(mM6);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        
        glDisableVertexAttribArray(untexturedProgram.positionAttribute);
        glDisableVertexAttribArray(texturedProgram.positionAttribute);
        glDisableVertexAttribArray(texturedProgram.texCoordAttribute);
        
        SDL_GL_SwapWindow(displayWindow);
    }
    
    SDL_Quit();
    return 0;
}

GLuint LoadTexture(const char *filePath) {
    int w,h,comp;
    unsigned char* image = stbi_load(filePath, &w, &h, &comp, STBI_rgb_alpha);
    if(image == NULL) {
        std::cout << "Unable to load image. Make sure the path is correct\n";
        assert(false);
    }
    GLuint retTexture;
    glGenTextures(1, &retTexture);
    glBindTexture(GL_TEXTURE_2D, retTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    stbi_image_free(image);
    return retTexture;
}
