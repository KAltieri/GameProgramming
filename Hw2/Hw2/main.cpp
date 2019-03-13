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
GLuint LoadTexture(const char *filePath);
float collision(float x1, float w1, float x2, float w2);

class Entity
{
public:
    Entity(float x, float y, float width, float height, float speed = 0, float xDirect = 1, float yDirect = 1)
    : x(x), y(y), width(width), height(height), speed(speed), xDirect(xDirect), yDirect(yDirect) {}
    float x;
    float y;
    float width;
    float height;
    float speed;
    float xDirect;
    float yDirect;
    glm::mat4 matrix;
    
    void draw(ShaderProgram& program, float* vertex)
    {
        glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertex);
        glEnableVertexAttribArray(program.positionAttribute);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glDisableVertexAttribArray(program.positionAttribute);
    }
};

float* vertexCreator(Entity& enter)
{
    float* texVertices = new float[12]{-enter.width/2, -enter.height/2,
                            enter.width/2, -enter.height/2,
                            enter.width/2, enter.height/2,
                            -enter.width/2, -enter.height/2,
                            enter.width/2, enter.height/2,
                            -enter.width/2, enter.height/2};

    return texVertices;
};

int main(int argc, char *argv[])
{
    float screenHeight = 640;
    float screenWidth = 360;
    SDL_Init(SDL_INIT_VIDEO);
    displayWindow = SDL_CreateWindow("My Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, screenHeight, screenWidth, SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
    SDL_GL_MakeCurrent(displayWindow, context);
    
    ShaderProgram untexturedProgram;
    ShaderProgram texturedProgram;
    untexturedProgram.Load(RESOURCE_FOLDER"vertex.glsl", RESOURCE_FOLDER"fragment.glsl");
    texturedProgram.Load(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");
    
    GLuint ballImg = LoadTexture(RESOURCE_FOLDER"ball.png");
    glClearColor(0.65f, 1.0f, 0.55f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    Entity topBorder(0.0f, 0.9f, 3.0f, 0.15f);
    topBorder.matrix = glm::mat4(1.0f);
    topBorder.matrix = glm::translate(topBorder.matrix, glm::vec3(topBorder.x, topBorder.y, 0.0f));
    untexturedProgram.SetModelMatrix(topBorder.matrix);
    
    Entity botBorder(0.0f, -0.9f, 3.0f, 0.15f);
    botBorder.matrix = glm::mat4(1.0f);
    botBorder.matrix = glm::translate(botBorder.matrix, glm::vec3(botBorder.x, botBorder.y, 0.0f));
    untexturedProgram.SetModelMatrix(botBorder.matrix);
    
    Entity midBorder(0.0f, 0.0f, 0.15f, 1.85f);
    midBorder.matrix = glm::mat4(1.0f);
    untexturedProgram.SetModelMatrix(midBorder.matrix);
    
    Entity rightPaddle(topBorder.width/2 - 0.01f, 0.0f, 0.05f, 0.5f);
    rightPaddle.matrix = glm::mat4(1.0f);
    rightPaddle.matrix = glm::translate(rightPaddle.matrix, glm::vec3(rightPaddle.x, rightPaddle.y, 0.0f));
    untexturedProgram.SetModelMatrix(rightPaddle.matrix);
    
    Entity leftPaddle(-topBorder.width/2 + 0.01f, 0.0f, 0.05f, 0.5f);
    leftPaddle.matrix = glm::mat4(1.0f);
    leftPaddle.matrix = glm::translate(leftPaddle.matrix, glm::vec3(leftPaddle.x, leftPaddle.y, 0.0f));
    untexturedProgram.SetModelMatrix(leftPaddle.matrix);
    
    Entity ball(0.0f, 0.0f, 0.1f, 0.1f, 1.0f);
    ball.matrix = glm::mat4(1.0f);
    ball.matrix = glm::scale(ball.matrix, glm::vec3(ball.width, ball.height, 0.0f));
    texturedProgram.SetModelMatrix(ball.matrix);
    
    glm::mat4 viewMatrix = glm::mat4(1.0f);
    glm::mat4 projectionMatrix = glm::mat4(1.0f);
    
    float aspectRatio = screenHeight / screenWidth;
    float projectionHeight = 1.0f;
    float projectionWidth = projectionHeight * aspectRatio;
    float projectionDepth = 1.0f;
    projectionMatrix = glm::ortho(-projectionWidth, projectionWidth, -projectionHeight, projectionHeight,
                                            -projectionDepth, projectionDepth);
   
    untexturedProgram.SetProjectionMatrix(projectionMatrix);
    untexturedProgram.SetViewMatrix(viewMatrix);

    texturedProgram.SetProjectionMatrix(projectionMatrix);
    texturedProgram.SetViewMatrix(viewMatrix);
    
    float lastFrameTicks = 0.0f;
    untexturedProgram.SetColor(1.0f, 1.0f, 1.0f, 1.0f);
    
    float* midBorderVertices = vertexCreator(midBorder);
    float* borderVertices = vertexCreator(topBorder);
    
    float* lPV = vertexCreator(leftPaddle);
    float* rPV = vertexCreator(rightPaddle);
    
    float* ballV = vertexCreator(ball);
    float texCoords[] = {0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0};

    
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
        
        const Uint8 *keys = SDL_GetKeyboardState(NULL);
        
        glClear(GL_COLOR_BUFFER_BIT);
        
        float ticks = (float)SDL_GetTicks()/1000.0f;
        float elapsed = ticks - lastFrameTicks;
        lastFrameTicks = ticks;
        
        glUseProgram(untexturedProgram.programID);

        untexturedProgram.SetColor(1.0f, 1.0f, 1.0f, 0.5f);
        
        untexturedProgram.SetModelMatrix(midBorder.matrix);
        midBorder.draw(untexturedProgram, midBorderVertices);
        
        untexturedProgram.SetColor(1.0f, 1.0f, 1.0f, 1.0f);
        
        untexturedProgram.SetModelMatrix(topBorder.matrix);
        topBorder.draw(untexturedProgram, borderVertices);
        
        untexturedProgram.SetModelMatrix(botBorder.matrix);
        botBorder.draw(untexturedProgram, borderVertices);

        rightPaddle.matrix = glm::mat4(1.0f);
        if(collision(topBorder.y, topBorder.height, rightPaddle.y, rightPaddle.height) > 0.0005f)
        {
            if(keys[SDL_SCANCODE_UP])
            {
                rightPaddle.y += elapsed * 1.0f;
            }
        }
        if(collision(botBorder.y, botBorder.height, rightPaddle.y, rightPaddle.height) > 0.0005f)
        {
            if(keys[SDL_SCANCODE_DOWN])
            {
                rightPaddle.y -= elapsed * 1.0f;
            }
        }
        rightPaddle.matrix = glm::translate(rightPaddle.matrix, glm::vec3(rightPaddle.x, rightPaddle.y, 0.0f));
        untexturedProgram.SetModelMatrix(rightPaddle.matrix);
        rightPaddle.draw(untexturedProgram, rPV);

        if(collision(topBorder.y, topBorder.height, leftPaddle.y, leftPaddle.height) > 0.0005f)
        {
            if(keys[SDL_SCANCODE_W])
            {
                leftPaddle.y += elapsed * 1.0f;
            }
        }
        if(collision(botBorder.y, botBorder.height, leftPaddle.y, leftPaddle.height) > 0.0005f)
        {
            if(keys[SDL_SCANCODE_S])
            {
                leftPaddle.y += elapsed * -1.0f;
            }
        }
        leftPaddle.matrix = glm::mat4(1.0f);
        leftPaddle.matrix = glm::translate(leftPaddle.matrix, glm::vec3(leftPaddle.x, leftPaddle.y, 0.0f));
        untexturedProgram.SetModelMatrix(leftPaddle.matrix);
        leftPaddle.draw(untexturedProgram, lPV);

        glUseProgram(texturedProgram.programID);

        glBindTexture(GL_TEXTURE_2D, ballImg);
        
        ball.matrix = glm::mat4(1.0f);
        if(collision(topBorder.y, topBorder.height, ball.y, ball.height) < 0.00005f)
        {
            ball.yDirect = -1.0f;
        }
        if(collision(botBorder.y, botBorder.height, ball.y, ball.height) < 0.00005f)
        {
            ball.yDirect = 1.0f;
        }
        if(collision(leftPaddle.y, leftPaddle.height, ball.y, ball.height) < 0.00005f &&
           collision(leftPaddle.x, leftPaddle.width, ball.x, ball.width) < 0.00005f)
        {
            ball.xDirect = 1.0f;
        }
        if(collision(rightPaddle.y, rightPaddle.height, ball.y, ball.height) < 0.00005f &&
           collision(rightPaddle.x, rightPaddle.width, ball.x, ball.width) < 0.00005f)
        {
            ball.xDirect = -1.0f;
        }
        if((ball.x + ball.width) > projectionWidth)
        {
            ball.xDirect = -1.0f;
            ball.x = 0.0f;
            ball.y = 0.0f;
        }
        if((ball.x + ball.width) < -projectionWidth)
        {
            ball.xDirect = 1.0f;
            ball.x = 0.0f;
            ball.y = 0.0f;
        }
        ball.x += elapsed * ball.speed * ball.xDirect;
        ball.y += elapsed * ball.speed * ball.yDirect;
        ball.matrix = glm::translate(ball.matrix, glm::vec3(ball.x, ball.y, 0.0f));
        texturedProgram.SetModelMatrix(ball.matrix);
        glVertexAttribPointer(texturedProgram.positionAttribute, 2, GL_FLOAT, false, 0, ballV);
        glEnableVertexAttribArray(texturedProgram.positionAttribute);
        glVertexAttribPointer(texturedProgram.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
        glEnableVertexAttribArray(texturedProgram.texCoordAttribute);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        
        glDisableVertexAttribArray(untexturedProgram.positionAttribute);
        glDisableVertexAttribArray(texturedProgram.positionAttribute);
        glDisableVertexAttribArray(texturedProgram.texCoordAttribute);
        
        SDL_GL_SwapWindow(displayWindow);
    }
    
    SDL_Quit();
    return 0;
}

float collision (float xy1, float wh1, float xy2, float wh2)
{
    return fabsf(xy1 - xy2) - ((wh1 + wh2) / 2);
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

