#ifdef _WINDOWS
#include <GL/glew.h>
#endif
#include <SDL.h>
#define GL_GLEXT_PROTOTYPES 1
#include <SDL_opengl.h>
#include <SDL_image.h>
#include "ShaderProgram.h"
#include "glm/mat4x4.hpp"
#include "FlareMap.h"
#include "glm/gtc/matrix_transform.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <vector>

#ifdef _WINDOWS
#define RESOURCE_FOLDER ""
#else
#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

#define LEVEL_HEIGHT map.mapHeight
#define LEVEL_WIDTH map.mapWidth
#define TILE_SIZE 0.1f
#define SPRITE_COUNT_X 16
#define SPRITE_COUNT_Y 8

SDL_Window* displayWindow;
GLuint LoadTexture(const char *filePath, int near);
void drawMap(ShaderProgram& program, FlareMap& map, unsigned int fontsheet);
float lerp(float v0, float v1, float t) { return (1.0-t)*v0 + t*v1; }
void worldToTileCoordinates(float worldX, float worldY, int *gridX, int *gridY)
{
    *gridX = (int)(worldX / TILE_SIZE);
    *gridY = (int)(worldY / -TILE_SIZE);
}
void tileToWorldCoordinates(int gridX, int gridY, float *worldX, float *worldY)
{
    *worldX = (float)(gridX * TILE_SIZE);
    *worldY = (float)(gridY * -TILE_SIZE);
}

class SheetSprite {
public:
    SheetSprite() {}
    SheetSprite(unsigned int textureID, int spriteNumber, float size) : textureID(textureID), size(size)
    {
        u = (float)(((int)spriteNumber) % SPRITE_COUNT_X) / (float) SPRITE_COUNT_X;
        v = (float)(((int)spriteNumber) / SPRITE_COUNT_X) / (float) SPRITE_COUNT_Y;
        width = 1.0f/(float)SPRITE_COUNT_X;
        height = 1.0f/(float)SPRITE_COUNT_Y;
    }
    void DrawSprite(ShaderProgram &program)
    {
        glBindTexture(GL_TEXTURE_2D, textureID);
        GLfloat texCoords[] = {
            u, v+height,
            u+width, v,
            u, v,
            u+width, v,
            u, v+height,
            u+width, v+height
        };
        float vertices[] = {
//            -0.5f * size * aspect, -0.5f * size,
//            0.5f * size * aspect, 0.5f * size,
//            -0.5f * size * aspect, 0.5f * size,
//            0.5f * size * aspect, 0.5f * size,
//            -0.5f * size * aspect, -0.5f * size,
//            0.5f * size * aspect, -0.5f * size
            -0.5f * size, -0.5f * size,
            0.5f * size, 0.5f * size,
            -0.5f * size, 0.5f * size,
            0.5f * size, 0.5f * size,
            -0.5f * size, -0.5f * size,
            0.5f * size, -0.5f * size
            
            
        };
        glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
        glEnableVertexAttribArray(program.positionAttribute);
        glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
        glEnableVertexAttribArray(program.texCoordAttribute);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glDisableVertexAttribArray(program.positionAttribute);
        glDisableVertexAttribArray(program.texCoordAttribute);
    }
    float size;
    unsigned int textureID;
    float u;
    float v;
    float width;
    float height;
};

class Entity
{
public:
    Entity(){};
    Entity(float x, float y){
        matrix = glm::mat4(1.0f);
        size = glm::vec3(TILE_SIZE, TILE_SIZE, 0.0f);
        matrix = glm::scale(matrix, size);
        position = glm::vec3(x, y, 0.0f);
        matrix = glm::translate(matrix, position);
        isStatic = false;
        acceleration = glm::vec3(0.0f, -0.5f, 0.0f);
        friction = glm::vec3(1.0f, 0.0f, 0.0f);
        velocity = glm::vec3(0.0f, 0.0f, 0.0f);
    }
    
    SheetSprite sprite;
    glm::mat4 matrix;
    glm::vec3 position;
    glm::vec3 size;
    glm::vec3 velocity;
    glm::vec3 acceleration;
    glm::vec3 friction;
    
    bool colTop;
    bool colBot;
    bool colLeft;
    bool colRight;
    
    bool isStatic;
    
    void Render(ShaderProgram& program)
    {
        program.SetModelMatrix(matrix);
        sprite.DrawSprite(program);
    }
    void Update(float elapsed, FlareMap& map)
    {
        matrix = glm::mat4(1.0f);
        velocity.x = lerp(velocity.x, 0.0f, friction.x * elapsed);
        velocity.y = lerp(velocity.y, 0.0f, friction.y * elapsed);
        velocity.y += acceleration.y * elapsed;
        position.x += velocity.x * elapsed;
        position.y += velocity.y * elapsed;
        int* findX = new int (0);
        int* findY = new int (0);
        worldToTileCoordinates(position.x, position.y, findX, findY);
        
        //4 checks: [*findY][*findX-1], [*findY][*findX+1], [*findY-1][*findX], [*findY+1][findX]
        worldToTileCoordinates(position.x, position.y, findX, findY);
        collisionChecks(*findY, *findX, map);
        matrix = glm::translate(matrix, position);
    }
    bool Collides(Entity& entity)
    {
        return false;
    }
    void collisionChecks(int height, int width, FlareMap& map)
    {
        if(height > 14)
        {
            height = 14;
            position.y += TILE_SIZE;
        }
        if(width > 30)
        {
            width = 30;
            position.x -= TILE_SIZE;
        }
//        if(height < 1)
//        {
//            height = 1;
//            position.y -= TILE_SIZE;
//        }
        if(width < 1)
        {
            width = 1;
            position.x += TILE_SIZE;
        }
        if(map.mapData[height+1][width] != 0)
        {
            yBotCollisionCheck(height+1, width);
        }
        if(map.mapData[height][width-1] != 0)
        {
            xLCollisionCheck(height, width-1);
        }
        if(map.mapData[height][width+1] != 0)
        {
            xRCollisionCheck(height, width+1);
        }
        if(map.mapData[height-1][width] != 0)
        {
            yTopCollisionCheck(height-1, width);
        }
    }
    void xLCollisionCheck(int height, int width)
    {
        float* tempX = new float(0);
        float* tempY = new float(0);
        tileToWorldCoordinates(width, height, tempX, tempY);
        if(position.x - size.x/2 < *tempX + TILE_SIZE/2)
        {
            position.x += (*tempX + TILE_SIZE/2 - position.x - size.x/2) + 0.05f;
        }
    }
    void xRCollisionCheck(int height, int width)
    {
        float* tempX = new float(0);
        float* tempY = new float(0);
        tileToWorldCoordinates(width, height, tempX, tempY);
        if(position.x + size.x/2 > *tempX + TILE_SIZE/2)
        {
            position.x -= (position.x + size.x/2 - *tempX - TILE_SIZE/2) + 0.05f;
        }
    }
    void yBotCollisionCheck(int height, int width)
    {
        float* tempX = new float(0);
        float* tempY = new float(0);
        tileToWorldCoordinates(width, height, tempX, tempY);
        if(position.y - size.y/2 < *tempY+TILE_SIZE/2)
        {
            colBot = true;
            position.y += (*tempY + TILE_SIZE/2 - position.y - size.y/2) + 0.05f;
        }
    }
    void yTopCollisionCheck(int height, int width)
    {
        float* tempX = new float(0);
        float* tempY = new float(0);
        tileToWorldCoordinates(width, height, tempX, tempY);
        if(position.y + size.y/2 > *tempY - TILE_SIZE/2)
        {
            position.y -= (position.y + size.y/2 - *tempY - TILE_SIZE/2) + 0.05f;
        }
    }
    void ProcessInput(const Uint8* keys)
    {
        if(keys[SDL_SCANCODE_LEFT])
        {
            velocity.x -= TILE_SIZE;
        }
        if(keys[SDL_SCANCODE_RIGHT])
        {
            velocity.x += TILE_SIZE;
        }
        if(keys[SDL_SCANCODE_SPACE])
        {
            if(colBot)
            {
                std::cout << "hi" << std::endl;
                velocity.y += 0.05f;
                colBot = false;
            }
        }
    }
};

int main(int argc, char *argv[])
{
    float screenWidth = 640;
    float screenHeight = 360;
    SDL_Init(SDL_INIT_VIDEO);
    displayWindow = SDL_CreateWindow("My Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, screenWidth, screenHeight, SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
    SDL_GL_MakeCurrent(displayWindow, context);
    
    ShaderProgram program;
    program.Load(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");
    
    FlareMap map;
    
    GLuint tileSheet = LoadTexture(RESOURCE_FOLDER"sprites.png", 1);
    map.Load(RESOURCE_FOLDER"TileMapTest.txt");
    
    float* tempX = new float(0);
    float* tempY = new float(0);
    tileToWorldCoordinates(map.entities[0].x, map.entities[0].y, tempX, tempY);
    Entity enemy2(*tempX, *tempY+TILE_SIZE);
    enemy2.sprite = SheetSprite(tileSheet, 81, TILE_SIZE);
    program.SetModelMatrix(enemy2.matrix);
    tileToWorldCoordinates(map.entities[1].x, map.entities[1].y, tempX, tempY);
    Entity enemy1(*tempX, *tempY+TILE_SIZE);
    enemy1.sprite = SheetSprite(tileSheet, 81, TILE_SIZE);
    tileToWorldCoordinates(map.entities[2].x, map.entities[2].y, tempX, tempY);
    program.SetModelMatrix(enemy1.matrix);
    Entity player(*tempX, *tempY+TILE_SIZE);
    player.sprite = SheetSprite(tileSheet, 98, TILE_SIZE);
    program.SetModelMatrix(player.matrix);
    
    glm::mat4 projectionMatrix = glm::mat4(1.0f);
    
    float aspectRatio = screenWidth / screenHeight;
    float projectionHeight = 1.0f;
    float projectionWidth = projectionHeight * aspectRatio;
    float projectionDepth = 1.0f;
    projectionMatrix = glm::ortho(-projectionWidth, projectionWidth, -projectionHeight, projectionHeight,
                                  -projectionDepth, projectionDepth);
    program.SetProjectionMatrix(projectionMatrix);
    
    glm::mat4 viewMatrix = glm::mat4(1.0f);
    viewMatrix = glm::translate(viewMatrix, glm::vec3(player.position.x, -player.position.y, 0.0f));
    program.SetViewMatrix(viewMatrix);
    
    glClearColor(0.0f, 0.86f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    float lastFrameTicks = 0.0f;
    
    // 60 FPS (1.0f/60.0f) (update sixty times a second)
#define FIXED_TIMESTEP 0.0166666f
#define MAX_TIMESTEPS 6
    float accumulator = 0.0f;
    
#ifdef _WINDOWS
    glewInit();
#endif

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    SDL_Event event;
    bool done = false;
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
        elapsed += accumulator;
        if(elapsed < FIXED_TIMESTEP)
        {
            accumulator = elapsed;
            continue;
        }
        while(elapsed >= FIXED_TIMESTEP)
        {
            player.Update(FIXED_TIMESTEP, map);
            enemy1.Update(FIXED_TIMESTEP, map);
            enemy2.Update(FIXED_TIMESTEP, map);
            elapsed -= FIXED_TIMESTEP;
        }
        accumulator = elapsed;
        player.Update(accumulator, map);
        enemy1.Update(accumulator, map);
        enemy2.Update(accumulator, map);
        
        player.ProcessInput(keys);

        viewMatrix = glm::mat4(1.0f);
        viewMatrix = glm::translate(viewMatrix, glm::vec3(-player.position.x, -player.position.y, 0.0f));
        program.SetViewMatrix(viewMatrix);
        
        drawMap(program, map, tileSheet);
        player.Render(program);
        enemy1.Render(program);
        enemy2.Render(program);
        
        SDL_GL_SwapWindow(displayWindow);
    }
    
    SDL_Quit();
    return 0;
}

GLuint LoadTexture(const char *filePath, int near) {
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
    if(near)
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }
    else
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    stbi_image_free(image);
    return retTexture;
}

void drawMap(ShaderProgram& program, FlareMap& map, unsigned int mapSheet)
{
    glm::mat4 mapMatrix = glm::mat4(1.0f);
    float spriteWidth = 1.0f/(float)SPRITE_COUNT_X;
    float spriteHeight = 1.0f/(float)SPRITE_COUNT_Y;
    std::vector<float> vertexData;
    std::vector<float> texCoordData;
    for(int y=0; y < LEVEL_HEIGHT; y++) {
        for(int x=0; x < LEVEL_WIDTH; x++) {
            
            if(map.mapData[y][x] != 0) {
            
            float u = (float)(((int)map.mapData[y][x]) % SPRITE_COUNT_X) / (float) SPRITE_COUNT_X;
            float v = (float)(((int)map.mapData[y][x]) / SPRITE_COUNT_X) / (float) SPRITE_COUNT_Y;
            
            vertexData.insert(vertexData.end(), {
                TILE_SIZE * x, -TILE_SIZE * y,
                TILE_SIZE * x, (-TILE_SIZE * y)-TILE_SIZE,
                (TILE_SIZE * x)+TILE_SIZE, (-TILE_SIZE * y)-TILE_SIZE,
                TILE_SIZE * x, -TILE_SIZE * y,
                (TILE_SIZE * x)+TILE_SIZE, (-TILE_SIZE * y)-TILE_SIZE,
                (TILE_SIZE * x)+TILE_SIZE, -TILE_SIZE * y
            });
            texCoordData.insert(texCoordData.end(), {
                u, v,
                u, v+spriteHeight,
                u+spriteWidth, v+spriteHeight,
                
                u, v,
                u+spriteWidth, v+spriteHeight,
                u+spriteWidth, v
            });
            }
        }
    }
    glBindTexture(GL_TEXTURE_2D, mapSheet);

    program.SetModelMatrix(mapMatrix);
    glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertexData.data());
    glEnableVertexAttribArray(program.positionAttribute);
    glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoordData.data());
    glEnableVertexAttribArray(program.texCoordAttribute);
    glDrawArrays(GL_TRIANGLES, 0, vertexData.size()/2);
    glDisableVertexAttribArray(program.positionAttribute);
    glDisableVertexAttribArray(program.texCoordAttribute);
}
