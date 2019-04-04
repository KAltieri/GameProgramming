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
#include <vector>

#ifdef _WINDOWS
#define RESOURCE_FOLDER ""
#else
#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

SDL_Window* displayWindow;
GLuint LoadTexture(const char *filePath, int near);
float lerp(float v0, float v1, float t);
void DrawText(ShaderProgram &program, int fontTexture, std::string text, float size, float spacing, glm::vec3 position);

class SheetSprite {
public:
    SheetSprite() {}
    SheetSprite(unsigned int textureID, float u, float v, float width, float height, float size)
    : textureID(textureID), u(u), v(v), width(width), height(height), size(size) {}
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
        float aspect = width / height;
        float vertices[] = {
            -0.5f * size * aspect, -0.5f * size,
            0.5f * size * aspect, 0.5f * size,
            -0.5f * size * aspect, 0.5f * size,
            0.5f * size * aspect, 0.5f * size,
            -0.5f * size * aspect, -0.5f * size,
            0.5f * size * aspect, -0.5f * size};
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
    Entity() {}
    Entity(glm::vec3 position, glm::vec3 velocity, glm::vec3 size, glm::vec3 friction, float rotation, SheetSprite sprite)
    : position(position), velocity(velocity), size(size), friction(friction), sprite(sprite)
    {
        matrix = glm::mat4(1.0f);
        rotation = rotation * (3.1415926585 / 180.0f);
        matrix = glm::scale(matrix, size);
        matrix = glm::rotate(matrix, rotation, glm::vec3(0.0f, 0.0f, 1.0f));
        matrix = glm::translate(matrix, position);
        matrix = glm::mat4(1.0f);
        isEnabled = true;
    }
    
    void Update(float elapsed)
    {
        matrix = glm::mat4(1.0f);
        velocity.x = lerp(velocity.x, 0.0f, friction.x * elapsed);
        velocity.y = lerp(velocity.y, 0.0f, friction.y * elapsed);
        position.x += velocity.x * elapsed;
        position.y += velocity.y * elapsed;
        matrix = glm::translate(matrix, position);
    }
    
    void Draw(ShaderProgram &program)
    {
        program.SetModelMatrix(matrix);
        sprite.DrawSprite(program);
    }
    
    bool collision(Entity otherEnt)
    {
        return (collisionX(otherEnt) < 0.0001f && collisionY(otherEnt) < 0.0001f);
    }
    
    float collisionX (Entity otherEnt)
    {
        return fabsf(position.x - otherEnt.position.x) - ((size.x + otherEnt.size.x) / 2);
    }
    float collisionY (Entity otherEnt)
    {
        return fabsf(position.y - otherEnt.position.y) - ((size.y + otherEnt.size.y) / 2);
    }
    
    glm::mat4 matrix;
    
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec3 size;
    glm::vec3 friction;
    
    float rotation;
    
    SheetSprite sprite;
    
    bool isEnabled;
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
    
    GLuint fontSheet = LoadTexture(RESOURCE_FOLDER"pixel_font.png", 1);
    GLuint spriteSheet = LoadTexture(RESOURCE_FOLDER"Space shooter assets (300 assets)/Spritesheet/sheet.png", 0);
    
    SheetSprite temp = SheetSprite(spriteSheet, 224.0f/1024.0f, 832.0f/1024.0f, 99.0f/1024.0f, 75.0f/1024.0f, 0.1f);
    Entity ship(glm::vec3(0.0f, -0.85f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.1f, 0.1f, 0.0f), glm::vec3(10.0f, 1.0f, 0.0f), 0.0f, temp);
    program.SetModelMatrix(ship.matrix);
    
#define MAX_BULLETS 10
    int bulletIndex = 0;
    float bulletSize = 0.05f;
    Entity bullets[MAX_BULLETS];
    SheetSprite bulletSprite = SheetSprite(spriteSheet, 856.0f/1024.0f, 602.0f/1024.0f, 9.0f/1024.0f, 37.0f/1024.0f, bulletSize);
    for(int i=0; i < MAX_BULLETS; i++)
    {
        bullets[i] = Entity(glm::vec3(0.0f, -20.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(bulletSize, bulletSize, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), 0.0f, bulletSprite);
        program.SetModelMatrix(bullets[i].matrix);
    }

#define MAX_ENEMY1 10
    float enemy1Size = 0.05f;
    SheetSprite enemy1Sprite = SheetSprite(spriteSheet, 346.0f/1024.0f, 150.0f/1024.0f, 97.0f/1024.0f, 84.0f/1024.0f, enemy1Size);
    Entity enemy1[MAX_ENEMY1];
    std::vector<glm::vec3> origPositions1;
    for(int i = 0; i < MAX_ENEMY1; i++)
    {
        origPositions1.push_back(glm::vec3(-0.45f + (i*0.1f), 0.0f, 0.0f));
        enemy1[i] = Entity(glm::vec3(-0.45f + (i*0.1f), 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(enemy1Size, enemy1Size, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), 0.0f, enemy1Sprite);
        program.SetModelMatrix(enemy1[i].matrix);
    }
    
#define MAX_ENEMY2 10
    float enemy2Size = 0.05f;
    SheetSprite enemy2Sprite = SheetSprite(spriteSheet, 222.0f/1024.0f, 0.0f/1024.0f, 103.0f/1024.0f, 84.0f/1024.0f, enemy2Size);
    Entity enemy2[MAX_ENEMY2];
    std::vector<glm::vec3> origPositions2;
    for(int i = 0; i < MAX_ENEMY2; i++)
    {
        origPositions2.push_back(glm::vec3(-0.45f + (i*0.1f), 0.2f, 0.0f));
        enemy2[i] = Entity(glm::vec3(-0.45f + (i*0.1f), 0.2f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(enemy2Size, enemy2Size, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), 0.0f, enemy2Sprite);
        program.SetModelMatrix(enemy2[i].matrix);
    }
    
#define MAX_ENEMY3 10
    float enemy3Size = 0.05f;
    SheetSprite enemy3Sprite = SheetSprite(spriteSheet, 133.0f/1024.0f, 412.0f/1024.0f, 104.0f/1024.0f, 84.0f/1024.0f, enemy3Size);
    Entity enemy3[MAX_ENEMY3];
    std::vector<glm::vec3> origPositions3;
    for(int i = 0; i < MAX_ENEMY3; i++)
    {
        origPositions3.push_back(glm::vec3(-0.45f + (i*0.1f), 0.4f, 0.0f));
        enemy3[i] = Entity(glm::vec3(-0.45f + (i*0.1f), 0.4f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(enemy3Size, enemy3Size, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), 0.0f, enemy3Sprite);
        program.SetModelMatrix(enemy3[i].matrix);
    }
    
    glm::mat4 viewMatrix = glm::mat4(1.0f);
    glm::mat4 projectionMatrix = glm::mat4(1.0f);
    
    float aspectRatio = screenHeight / screenWidth;
    float projectionHeight = 1.0f;
    float projectionWidth = projectionHeight * aspectRatio;
    float projectionDepth = 1.0f;
    projectionMatrix = glm::ortho(-projectionWidth, projectionWidth, -projectionHeight, projectionHeight,
                                  -projectionDepth, projectionDepth);
    
    program.SetProjectionMatrix(projectionMatrix);
    program.SetViewMatrix(viewMatrix);
    
    float lastFrameTicks = 0.0f;
    float time = 0;
    float enemyTime = 0;
    float moveValue = 0.0125f;
    float moveDirection = 1;
    float noMore = 0;
    bool moveDown = false;
    
    int gameMode = 0; // 0 = Main Menu, 1 = Game Level
    
#ifdef _WINDOWS
    glewInit();
#endif
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glUseProgram(program.programID);

    SDL_Event event;
    bool done = false;
    while (!done) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
                done = true;
            }
        }
        float ticks = (float)SDL_GetTicks()/1000.0f;
        float elapsed = ticks - lastFrameTicks;
        lastFrameTicks = ticks;
        
        const Uint8 *keys = SDL_GetKeyboardState(NULL);

        glClear(GL_COLOR_BUFFER_BIT);
        switch(gameMode)
        {
            case 0:
                DrawText(program, fontSheet, "Space Invaders", 0.075f, 0.0009f, glm::vec3(-0.5f, 0.3f, 0.0f));
                DrawText(program, fontSheet, "Press Enter", 0.06, 0.0009f, glm::vec3(-0.3, 0.1, 0.0));
                DrawText(program, fontSheet, "To Start", 0.06, 0.0009f, glm::vec3(-0.2, -0.1, 0.0));
                if(keys[SDL_SCANCODE_RETURN])
                {
                    gameMode = 1;
                    for(int loop = 0; loop < MAX_ENEMY1; loop++)
                    {
                        enemy1[loop].isEnabled = true;
                        enemy1[loop].position = origPositions1[loop];
                    }
                    for(int loop = 0; loop < MAX_ENEMY2; loop++)
                    {
                        enemy2[loop].isEnabled = true;
                        enemy2[loop].position = origPositions2[loop];
                    }
                    for(int loop = 0; loop < MAX_ENEMY3; loop++)
                    {
                        enemy3[loop].isEnabled = true;
                        enemy3[loop].position = origPositions3[loop];
                    }
                }
            break;
            case 1:
                noMore = 0;
                ship.Draw(program);
                for(Entity& draw : bullets)
                {
                    draw.Draw(program);
                }
                for(Entity& draw : enemy1)
                {
                    if(draw.isEnabled)
                    {
                        draw.Draw(program);
                        noMore++;
                    }
                }
                for(Entity& draw : enemy2)
                {
                    if(draw.isEnabled)
                    {
                        draw.Draw(program);
                        noMore++;
                    }
                }
                for(Entity& draw : enemy3)
                {
                    if(draw.isEnabled)
                    {
                        draw.Draw(program);
                        noMore++;
                    }
                }
                if(noMore == 0)
                {
                    gameMode = 0;
                    break;
                }
                
                ship.Update(elapsed);
                for(Entity& bullet : bullets)
                {
                    bullet.Update(elapsed);
                    for(Entity& update : enemy1)
                    {
                        update.Update(elapsed);
                        if(update.collision(bullet) && update.isEnabled)
                        {
                            bullet.position = glm::vec3(0.0f, -20.0f, 0.0f);
                            bullet.velocity = glm::vec3(0.0f, 0.0f, 0.0f);
                            update.isEnabled = false;
                        }
                        if((update.position.x + update.size.x) > projectionWidth && update.isEnabled)
                        {
                            moveDirection = -1;
                            moveDown = true;
                        }
                        if((update.position.x - update.size.x) < -projectionWidth && update.isEnabled)
                        {
                            moveDirection = 1;
                            moveDown = true;
                        }
                        if((update.collision(ship) && update.isEnabled) || update.position.y < -0.95f)
                        {
                            gameMode = 0;
                            break;
                        }
                    }
                    for(Entity& update : enemy2)
                    {
                        update.Update(elapsed);
                        if(update.collision(bullet) && update.isEnabled)
                        {
                            bullet.position = glm::vec3(0.0f, -20.0f, 0.0f);
                            bullet.velocity = glm::vec3(0.0f, 0.0f, 0.0f);
                            update.isEnabled = false;
                        }
                        if((update.position.x + update.size.x) > projectionWidth && update.isEnabled)
                        {
                            moveDirection = -1;
                            moveDown = true;
                        }
                        if((update.position.x - update.size.x) < -projectionWidth && update.isEnabled)
                        {
                            moveDirection = 1;
                            moveDown = true;
                        }
                        if((update.collision(ship) && update.isEnabled) || update.position.y < -0.95f)
                        {
                            gameMode = 0;
                            break;
                        }
                    }
                    for(Entity& update : enemy3)
                    {
                        update.Update(elapsed);
                        if(update.collision(bullet) && update.isEnabled)
                        {
                            bullet.position = glm::vec3(0.0f, -20.0f, 0.0f);
                            bullet.velocity = glm::vec3(0.0f, 0.0f, 0.0f);
                            update.isEnabled = false;
                        }
                        if((update.position.x + update.size.x) > projectionWidth && update.isEnabled)
                        {
                            moveDirection = -1;
                            moveDown = true;
                        }
                        if((update.position.x - update.size.x) < -projectionWidth && update.isEnabled)
                        {
                            moveDirection = 1;
                            moveDown = true;
                        }
                        if((update.collision(ship) && update.isEnabled) || update.position.y < -0.95f)
                        {
                            gameMode = 0;
                            break;
                        }
                    }
                }
                
                if(keys[SDL_SCANCODE_RIGHT])
                {
                    ship.velocity.x = 0.85f;
                }
                if(keys[SDL_SCANCODE_LEFT])
                {
                    ship.velocity.x = -0.85f;
                }
                if((ship.position.x + ship.size.x/2) > projectionWidth)
                {
                    ship.position.x = projectionWidth - 0.1f;
                }
                if((ship.position.x - ship.size.x/2) < -projectionWidth)
                {
                    ship.position.x = -projectionWidth + 0.1f;
                }
                if(keys[SDL_SCANCODE_SPACE] && time > 0.5f)
                {
                    bullets[bulletIndex].matrix = glm::mat4(1.0f);
                    bullets[bulletIndex].position = ship.position;
                    bullets[bulletIndex].velocity = glm::vec3(0.0f, 2.0f, 0.0f);
                    bulletIndex++;
                    if(bulletIndex >= MAX_BULLETS)
                    {
                        bulletIndex = 0;
                    }
                    time = 0;
                }
                time += elapsed;
                
                if(enemyTime > 0.75f)
                {
                    for(Entity& move : enemy1)
                    {
                        move.position.x += moveValue * moveDirection;
                        if(moveDown)
                        {
                            move.position.y -= moveValue * 4;
                        }
                    }
                    for(Entity& move : enemy2)
                    {
                        move.position.x += moveValue * moveDirection;
                        if(moveDown)
                        {
                            move.position.y -= moveValue * 4;
                        }

                    }
                    for(Entity& move : enemy3)
                    {
                        move.position.x += moveValue * moveDirection;
                        if(moveDown)
                        {
                            move.position.y -= moveValue * 4;
                        }
                    }
                    enemyTime = 0;
                    moveDown = false;
                }
                enemyTime += elapsed;
                
                break;
        }
        
        
        SDL_GL_SwapWindow(displayWindow);
    }
    
    SDL_Quit();
    return 0;
}

float lerp(float v0, float v1, float t)
{
    return (1.0f-t)*v0 + t*v1;
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


void DrawText(ShaderProgram &program, int fontTexture, std::string text, float size, float spacing, glm::vec3 position) {
    glm::mat4 textMatrix = glm::mat4(1.0f);
    textMatrix = glm::translate(textMatrix, position);
    program.SetModelMatrix(textMatrix);
    float character_size = 1.0/16.0f;
    std::vector<float> vertexData;
    std::vector<float> texCoordData;
    for(int i=0; i < text.size(); i++) {
        int spriteIndex = (int)text[i];
        float texture_x = (float)(spriteIndex % 16) / 16.0f;
        float texture_y = (float)(spriteIndex / 16) / 16.0f;
        vertexData.insert(vertexData.end(), {
            ((size+spacing) * i) + (-0.5f * size), 0.5f * size,
            ((size+spacing) * i) + (-0.5f * size), -0.5f * size,
            ((size+spacing) * i) + (0.5f * size), 0.5f * size,
            ((size+spacing) * i) + (0.5f * size), -0.5f * size,
            ((size+spacing) * i) + (0.5f * size), 0.5f * size,
            ((size+spacing) * i) + (-0.5f * size), -0.5f * size,
        });
        texCoordData.insert(texCoordData.end(), {
            texture_x, texture_y,
            texture_x, texture_y + character_size,
            texture_x + character_size, texture_y,
            texture_x + character_size, texture_y + character_size,
            texture_x + character_size, texture_y,
            texture_x, texture_y + character_size,
        }); }
    glBindTexture(GL_TEXTURE_2D, fontTexture);
    
    program.SetModelMatrix(textMatrix);
    glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertexData.data());
    glEnableVertexAttribArray(program.positionAttribute);
    glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoordData.data());
    glEnableVertexAttribArray(program.texCoordAttribute);
    glDrawArrays(GL_TRIANGLES, 0, (int)text.size()*6);
    glDisableVertexAttribArray(program.positionAttribute);
    glDisableVertexAttribArray(program.texCoordAttribute);
}
