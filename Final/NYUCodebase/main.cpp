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
#include <SDL_mixer.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include "SatCollision.h"
#include <utility>
#ifdef _WINDOWS
#define RESOURCE_FOLDER ""
#else
#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

SDL_Window* displayWindow;

GLuint LoadTexture(const char *filePath, int near);
void DrawText(ShaderProgram &program, int fontTexture, std::string text, float size, float spacing, glm::vec3 position);
const std::vector<std::pair<float, float>> floatPairs(std::vector<glm::vec4> vertices);

enum GameState { START_SCREEN, MAIN_GAME_SCREEN, END_GAME_SCREEN};
enum EntityType { PLAYER, BULLET, ASTEROID };

GameState gameMode = START_SCREEN;

float lerp(float v0, float v1, float t)
{
    return (1.0f-t)*v0 + t*v1;
}

float genRandom(float low, float high)
{
    return low + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX/(high-low)));
}

//use an untextured program as well
class Particle
{
public:
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec3 velocityDeviation;
    float lifetime;
    
    glm::vec4 startColor;
    glm::vec4 endColor;
};

class ParticleEmitter
{
public:
    ParticleEmitter(unsigned int particleAmount) {}
    ParticleEmitter(){}
    ~ParticleEmitter(){}
    void Update(float elapsed)
    {
        
    }
    void Render(ShaderProgram& program)
    {
        std::vector<float> vertices;
        for(int i=0; i < particles.size(); i++) {
            vertices.push_back(particles[i].position.x);
            vertices.push_back(particles[i].position.y);
        }
        std::vector<float> particleColors;
        for(int i=0; i < particles.size(); i++) {
            float relativeLifetime = (particles[i].lifetime/maxLifetime);
            particleColors.push_back(lerp(particles[i].startColor.r, particles[i].endColor.r, relativeLifetime));
            particleColors.push_back(lerp(particles[i].startColor.g, particles[i].endColor.g, relativeLifetime));
            particleColors.push_back(lerp(particles[i].startColor.b, particles[i].endColor.b, relativeLifetime));
            particleColors.push_back(lerp(particles[i].startColor.a, particles[i].endColor.a, relativeLifetime));
        }
        GLuint colorAttribute = glGetAttribLocation(program.programID, "color");
        glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices.data());
        glEnableVertexAttribArray(program.positionAttribute);
        glVertexAttribPointer(colorAttribute, 4, GL_FLOAT, false, 0, particleColors.data());
        glEnableVertexAttribArray(colorAttribute);
        glDrawArrays(GL_POINTS, 0, particles.size());
    }
    
    glm::vec3 position;
    glm::vec3 gravity;
    float maxLifetime;
    
    std::vector<Particle> particles;
    
};

class SheetSprite {
public:
    SheetSprite() {}
    SheetSprite(unsigned int textureID, float u, float v, float width, float height, float size)
    : textureID(textureID), u(u), v(v), width(width), height(height), size(size)
    {
        aspect = width / height;
        vertices = {
            -0.5f * size * aspect, -0.5f * size,
            0.5f * size * aspect, 0.5f * size,
            -0.5f * size * aspect, 0.5f * size,
            0.5f * size * aspect, 0.5f * size,
            -0.5f * size * aspect, -0.5f * size,
            0.5f * size * aspect, -0.5f * size};
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
        glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices.data());
        glEnableVertexAttribArray(program.positionAttribute);
        glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
        glEnableVertexAttribArray(program.texCoordAttribute);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glDisableVertexAttribArray(program.positionAttribute);
        glDisableVertexAttribArray(program.texCoordAttribute);
    }
    std::vector<float> vertices;
    float aspect;
    float size;
    unsigned int textureID;
    float u;
    float v;
    float width;
    float height;
};

class Entity{
public:
    Entity(){}
    Entity(glm::vec3 position, glm::vec3 size, float rotation, glm::vec3 friction, EntityType type) : position(position), size(size), rotation(rotation), friction(friction), type(type)
    {
        matrix = glm::mat4(1.0f);
        matrix = glm::scale(matrix, size);
        matrix = glm::rotate(matrix, rotation, glm::vec3(0.0f, 0.0f, 1.0f));
        matrix = glm::translate(matrix, position);
        glm::vec3 defaultSet = glm::vec3(0.0f, 0.0f, 0.0f);
        velocity = defaultSet;
        acceleration = defaultSet;
    }
    Entity(glm::vec3 position, glm::vec3 size, float rotation, glm::vec3 friction, std::vector<SDL_Scancode> keys, EntityType type) : position(position), size(size), rotation(rotation), friction(friction), type(type)
    {
        matrix = glm::mat4(1.0f);
        matrix = glm::translate(matrix, position);
        matrix = glm::scale(matrix, size);
        matrix = glm::rotate(matrix, rotation, glm::vec3(0.0f, 0.0f, 1.0f));
        matrix = glm::mat4(1.0f);
        for(SDL_Scancode add : keys)
        {
            sCodes.push_back(add);
        }
        moveSpeed = 1.25f;
        rotationSpeed = moveSpeed*5;
        glm::vec3 defaultSet = glm::vec3(0.0f, 0.0f, 0.0f);
        velocity = defaultSet;
        acceleration = defaultSet;
    }
    void setEdgeSet()
    {
        edgeSet.push_back(glm::vec4(sprite.vertices[0], sprite.vertices[1], 1.0f, 1.0f));
        edgeSet.push_back(glm::vec4(sprite.vertices[10], sprite.vertices[11], 1.0f, 1.0f));
        edgeSet.push_back(glm::vec4(sprite.vertices[2], sprite.vertices[3], 1.0f, 1.0f));
        edgeSet.push_back(glm::vec4(sprite.vertices[4], sprite.vertices[5], 1.0f, 1.0f));
    }
    void transformEdgeSet()
    {
        for(glm::vec4& transform : edgeSet)
        {
            transform = matrix * transform;
        }
    }
    void Update(float elapsed)
    {
        //process collisions
        matrix = glm::mat4(1.0f);
        velocity.x = lerp(velocity.x, 0.0f, friction.x * elapsed);
        velocity.y = lerp(velocity.y, 0.0f, friction.y * elapsed);
        velocity.x += acceleration.x * elapsed;
        velocity.y += acceleration.y * elapsed;
        position.x += velocity.x * elapsed;
        position.y += velocity.y * elapsed;
        position.z = 0.0f;
        if(type == PLAYER)
        {
            time += elapsed;
            if(time >= 1.0f)
            {
                canShoot = true;
                time = 0.0f;
            }
            if(position.x - size.x/2 < -1.8f)
            {
                position.x = 1.75f;
            }
            if(position.x + size.x/2 > 1.8f)
            {
                position.x = -1.75f;
            }
            if(position.y - size.y/2 < -1.01f)
            {
                position.y = 0.95f;
            }
            if(position.y + size.y/2 > 1.01f)
            {
                position.y = -0.95f;
            }
        }
        if(type == BULLET)
        {
            if(position.x < -1.85f || position.x > 1.85f || position.y < -1.1f || position.y > 1.1f)
            {
                velocity = glm::vec3(0.0f, 0.0f, 0.0f);
                position = glm::vec3(0.0f, -20.0f, 0.0f);
            }
        }
        rotateAmount = rotateAmount * (3.1415926585 / 180.0f);
        rotation += rotateAmount;
        matrix = glm::translate(matrix, position);
        matrix = glm::rotate(matrix, rotation, glm::vec3(0.0f, 0.0f, 1.0f));
    }
    void Process(const Uint8* keys)
    {
        if(keys[sCodes[0]]) // move up
        {
            velocity = glm::vec3(cos(rotation+glm::pi<float>()/2)*moveSpeed, sin(rotation+glm::pi<float>()/2)*moveSpeed, 0.0f);
        }
        if(keys[sCodes[1]]) // move down
        {
            velocity = glm::vec3(cos(rotation+glm::pi<float>()/2)*-moveSpeed, sin(rotation+glm::pi<float>()/2)*-moveSpeed, 0.0f);
        }
        if(keys[sCodes[2]]) // rotate left
        {
            rotateAmount += rotationSpeed;
        }
        if(keys[sCodes[3]]) // rotate right
        {
            rotateAmount -= rotationSpeed;
        }
        if(rotation > 360)
        {
            rotation = 0;
        }
        if(keys[sCodes[4]]) // shoot
        {
            if(canShoot)
            {
                Mix_PlayChannel( -1, shootSound, 0);
                canShoot = false;
                shoot = true;
            }
        }
    }
    void Render(ShaderProgram& program)
    {
        program.SetModelMatrix(matrix);
        sprite.DrawSprite(program);
    }
    glm::mat4 matrix;
    glm::vec3 position;
    glm::vec3 size;
    float rotation;
    float rotateAmount;
    float moveSpeed;
    float rotationSpeed;
    
    glm::vec3 velocity;
    glm::vec3 acceleration;
    glm::vec3 friction;
    
    std::vector<glm::vec4> edgeSet;
    
    int health;
    float time;
    bool canShoot;
    bool shoot;
    
    std::vector<SDL_Scancode> sCodes;
    
    EntityType type;
    
    SheetSprite sprite;
    
    Mix_Chunk* shootSound;
    Mix_Chunk* hitSound;
    Mix_Chunk* deathSound;
    
private:
};

class Asteroid
{
public:
    Asteroid(glm::vec3 position, glm::vec3 size, float rotation, std::vector<float> indices) : position(position), size(size), rotation(rotation)
    {
        matrix = glm::mat4(1.0f);
        matrix = glm::scale(matrix, size);
        matrix = glm::rotate(matrix, rotation, glm::vec3(0.0f, 0.0f, 1.0f));
        matrix = glm::translate(matrix, position);
        glm::vec3 defaultSet = glm::vec3(0.0f, 0.0f, 0.0f);
        velocity = defaultSet;
        for(float addIndex : indices)
        {
            index.push_back(addIndex);
        }
        for(int mod = 0; mod < index.size(); mod+=2)
        {
            index[mod] *= size.x;
            index[mod+1] *= size.y;
        }
    }
    void Render(ShaderProgram& program)
    {
        program.SetModelMatrix(matrix);
        glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, index.data());
        glEnableVertexAttribArray(program.positionAttribute);
        glDrawArrays(GL_LINE_LOOP, 0, (int)index.size()/2);
        glDisableVertexAttribArray(program.positionAttribute);
    }
    void Update(float elapsed)
    {
        matrix = glm::mat4(1.0f);
        position.x += velocity.x * elapsed;
        position.y += velocity.y * elapsed;
        position.z = 0.0f;

        if(position.x < -1.8f)
        {
            position.x = 1.75f;
        }
        if(position.x > 1.8f)
        {
            position.x = -1.75f;
        }
        if(position.y < -1.01f)
        {
            position.y = 0.9f;
        }
        if(position.y > 1.01f)
        {
            position.y = -0.9f;
        }
        matrix = glm::translate(matrix, position);
        matrix = glm::rotate(matrix, rotation, glm::vec3(0.0f, 0.0f, 1.0f));
    }
    void setEdgeSet()
    {
        for(int add = 0; add < index.size(); add += 2)
        {
            edgeSet.push_back(glm::vec4(index[add], index[add+1], 1.0f, 1.0f));
        }
    }
    void transformEdgeSet()
    {
        for(glm::vec4& transform : edgeSet)
        {
            transform = matrix * transform;
        }
    }
    std::vector<glm::vec4> edgeSet;
    std::vector<float> index;
    glm::mat4 matrix;
    glm::vec3 position;
    glm::vec3 size;
    float rotation;
    float rotateAmount;
    float moveSpeed;
    float rotationSpeed;
    
    glm::vec3 velocity;
    
    int health;
};

class Play
{
public:
    Play(unsigned int texture)
    {
        InitialSetUp(texture);
    }
    Entity player1;
    Entity player2;
    int player1Score;
    int player2Score;
    std::vector<Asteroid> asteroids;
    int max_bullets;
    int bulletIndex;
    std::vector<Entity> bullets;
    bool p2Enabled;
    bool reset;
    void InitialSetUp(unsigned int texture)
    {
        //add lifetime to bullets?
        player1 = Entity(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.132f, 0.1f, 0.0f), 0.0f, glm::vec3(1.0f, 1.0f, 1.0f), {SDL_SCANCODE_W, SDL_SCANCODE_S, SDL_SCANCODE_A, SDL_SCANCODE_D, SDL_SCANCODE_F}, PLAYER); // pos, size, rotation, up, down, left, right, rLeft, rRight
        player2 = Entity(glm::vec3(0.25f, 0.0f, 0.0f), glm::vec3(0.132f, 0.1f, 0.0f), 0.0f, glm::vec3(1.0f, 1.0f, 1.0f), {SDL_SCANCODE_UP, SDL_SCANCODE_DOWN, SDL_SCANCODE_LEFT, SDL_SCANCODE_RIGHT, SDL_SCANCODE_SPACE}, PLAYER);
        player1.sprite = SheetSprite(texture, 0.0f/1024.0f, 941.0f/1024.0f, 112.0f/1024.0f, 75.0f/1024.0f, 0.1f);
        player1.setEdgeSet();
        player1.shootSound = Mix_LoadWAV(RESOURCE_FOLDER"shoot2.wav");
        player2.sprite = SheetSprite(texture, 112.0f/1024.0f, 791.0f/1024.0f, 112.0f/1024.0f, 75.0f/1024.0f, 0.1f);
        player1.friction = glm::vec3(1.0f, 1.0f, 1.0f);
        player2.friction = glm::vec3(1.0f, 1.0f, 1.0f);
        if(p2Enabled)
        {
            player1.position = glm::vec3(-0.25f, 0.0f, 0.0f);
        }
        max_bullets = 20;
        bulletIndex = 0;
        bullets = std::vector<Entity>(max_bullets);
        SheetSprite bulletSprite = SheetSprite(texture, 856.0f/1024.0f, 602.0f/1024.0f, 9.0f/1024.0f, 37.0f/1024.0f, 0.1f);
        for(int i=0; i < max_bullets; i++)
        {
            bullets[i] = Entity(glm::vec3(0.0f, -20.0f, 0.0f), glm::vec3(0.1f, 0.1f, 0.0f), 0.0f, glm::vec3(0.5f, 0.5f, 0.0f), BULLET);
            bullets[i].sprite = bulletSprite;
        }
        for(int i = 0; i < 1; i++)
        {
            asteroids.push_back(Asteroid(glm::vec3(genRandom(-1,1), genRandom(-1,1), 0.0f), glm::vec3(genRandom(0.5, 1), genRandom(0.5,1), 0.5f), 0.0f, {
                -0.5f, -0.5f,
                0.5f, -0.5f,
                0.5f, 0.5f,
                -0.5f, 0.5f
            }));
            asteroids[i].position.x -= 1.0f;
//            asteroids[i].velocity = glm::vec3(genRandom(0,1.5), genRandom(0, 1.5), 0.0f);
            asteroids[i].velocity = glm::vec3(0.0f, 0.0f, 0.0f);
            asteroids[i].rotation = genRandom(0, 360);
            asteroids[i].setEdgeSet();
        }
        player1Score = 0;
        player2Score = 0;
    }
    void Reset()
    {
        player1.position = glm::vec3(0.0f, 0.0f, 0.0f);
        if(p2Enabled)
        {
            player2.position = glm::vec3(0.25f, 0.0f, 0.0f);
            player1.position = glm::vec3(-0.25f, 0.0f, 0.0f);
        }
        //delete old asteroids, put in new asteroids
        for(int i = 0; i < 1; i++)
        {
            asteroids.push_back(Asteroid(glm::vec3(genRandom(-1,1), genRandom(-1,1), 0.0f), glm::vec3(genRandom(0.5, 1), genRandom(0.5,1), 0.5f), 0.0f, {
                -0.5f, -0.5f,
                0.5f, -0.5f,
                0.5f, 0.5f,
                -0.5f, 0.5f
            }));
            asteroids[i].position.x -= 1.0f;
            asteroids[i].velocity = glm::vec3(genRandom(0,1.5), genRandom(0, 1.5), 0.0f);
            asteroids[i].setEdgeSet();
        }
    }
    void Render(ShaderProgram& program, ShaderProgram& untextProgram)
    {
        player1.Render(program);
        if(p2Enabled){
            player2.Render(program);
        }
        glUseProgram(untextProgram.programID);
        for(Asteroid& asteroid : asteroids)
        {
            asteroid.Render(untextProgram);
        }
        glUseProgram(program.programID);
        for(Entity& bullet : bullets)
        {
            bullet.Render(program);
        }
    }
    void Update(float elapsed)
    {
        //collisions and health updates
        for(Asteroid& check : asteroids)
        {
            std::pair<float,float> penetration;
            check.transformEdgeSet();
            player1.transformEdgeSet();
//            std::cout << "-----------------------------------------------\nPlayer\n" << std::endl;
//            for(int i = 0; i < player1.edgeSet.size(); i++)
//            {
//                std::cout << player1.edgeSet[i].x << " " << player1.edgeSet[i].y << std::endl;
//            }
//            std::cout << "-----------------------------------------------\nAsteroid\n" << std::endl;
//            for(int i = 0; i < check.edgeSet.size(); i++)
//            {
//                std::cout << check.edgeSet[i].x << " " << check.edgeSet[i].y << std::endl;
//            }
            const std::vector<std::pair<float, float>> asTest = floatPairs(check.edgeSet);
            const std::vector<std::pair<float, float>> pTest = floatPairs(player1.edgeSet);
//            std::cout << "-----------------------------------------------\nAsteroid\n" << std::endl;
//            for(int i = 0; i < asTest.size(); i++)
//            {
//                std::cout << asTest[i].first << " " << asTest[i].second << std::endl;
//            }
//            std::cout << "-----------------------------------------------\nPlayer\n" << std::endl;
//            for(int i = 0; i < pTest.size(); i++)
//            {
//                std::cout << pTest[i].first << " " << pTest[i].second << std::endl;
//            }
            std::cout << std::boolalpha << CheckSATCollision(asTest, pTest, penetration) << std::endl;
//            std::cout << "-----------------------------------------------\nPenetration\n" << std::endl;
//            std::cout << penetration.first << " " << penetration.second << std::endl;
//            std::cout << "-----------------------------------------------" << std::endl;
//
        }
        player1.Update(elapsed);
        if(p2Enabled){
            player2.Update(elapsed);
        }
        for(Asteroid& asteroid : asteroids)
        {
            asteroid.Update(elapsed);
        }
        for(Entity& bullet : bullets)
        {
            bullet.Update(elapsed);
        }
    }
    void shoot(Entity& entity)
    {
        entity.shoot = false;
        bullets[bulletIndex].position = entity.position;
        bullets[bulletIndex].velocity = glm::vec3(cos(entity.rotation+glm::pi<float>()/2)*2.5f, sin(entity.rotation+glm::pi<float>()/2)*2.5f, 0.0f);
        bullets[bulletIndex].rotation = entity.rotation;
        bulletIndex++;
        if(bulletIndex >= max_bullets)
        {
            bulletIndex = 0;
        }
        entity.shoot = false;
    }
    void ProcessInput(const Uint8* keys)
    {
        player1.Process(keys);
        if(player1.shoot)
        {
            shoot(player1);
        }
        if(p2Enabled)
        {
            player2.Process(keys);
            if(player2.shoot)
            {
                shoot(player2);
            }
        }
    }
private:
};

class Menu
{
public:
    Menu()
    {
        wait = 0.5f;
    }
    float wait;
    void DrawRectangle(ShaderProgram& program, std::vector<float> index, glm::vec3(position))
    {
        glm::mat4 matrix = glm::mat4(1.0f);
        program.SetModelMatrix(matrix);
        glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, index.data());
        glEnableVertexAttribArray(program.positionAttribute);
        glDrawArrays(GL_LINE_LOOP, 0, (int)index.size()/2);
        glDisableVertexAttribArray(program.positionAttribute);
    }
    void MainMenuRender(ShaderProgram& program, int fontSheet)
    {
        DrawText(program, fontSheet, "Asteroids", 0.25f, 0.0005f, glm::vec3(-1.0f, 0.7f, 0.0f));
        DrawText(program, fontSheet, "1 Player", 0.15f, 0.00005f, glm::vec3(-1.5f, -0.75f, 0.0f));
        DrawText(program, fontSheet, "2 Player", 0.15f, 0.00005f, glm::vec3(0.25f, -0.75f, 0.0f));
    }
    void EndMenuRender(ShaderProgram& program, int fontSheet, Play& game)
    {
        DrawText(program, fontSheet, "Game Over", 0.25f, 0.0005f, glm::vec3(-1.0f, 0.7f, 0.0f));
        if(game.p2Enabled)
        {
            DrawText(program, fontSheet, "Score: ", 0.15, 0.0005f, glm::vec3(-0.8f, 0.5f, 0.0f));
            DrawText(program, fontSheet, "Player1: ", 0.15f, 0.0005f, glm::vec3(-0.8f, 0.3f, 0.0f));
            DrawText(program, fontSheet, std::to_string(game.player1Score), 0.2f, 0.0005f, glm::vec3(0.4f, 0.3f, 0.0f));
            DrawText(program, fontSheet, "Player2: ", 0.15f, 0.0005f, glm::vec3(-0.8f, 0.1f, 0.0f));
            DrawText(program, fontSheet, std::to_string(game.player2Score), 0.2f, 0.0005f, glm::vec3(0.4f, 0.1f, 0.0f));

        }
        else
        {
            DrawText(program, fontSheet, "Score: ", 0.15, 0.0005f, glm::vec3(-0.6f, 0.3f, 0.0f));
            DrawText(program, fontSheet, std::to_string(game.player1Score), 0.2f, 0.0005f, glm::vec3(0.3f, 0.3f, 0.0f));
        }
        DrawText(program, fontSheet, "Replay", 0.15f, 0.00005f, glm::vec3(-0.5f, -0.45f, 0.0f));
        DrawText(program, fontSheet, "Return to Start", 0.15f, 0.00005f, glm::vec3(-1.0f, -0.75f, 0.0f));
    }
    void MenuUpdate(float elapsed)
    {
        wait += elapsed;
    }
    void MainMenuProcess(Play& game)
    {
        int x, y;
        if(SDL_GetMouseState(&x, &y) && SDL_BUTTON(SDL_BUTTON_LEFT) && wait > 0.5f)
        {
            wait = 0;
            float unitX = (((float)x / 640.0f) * 3.554f ) - 1.777f;
            float unitY = (((float)(360-y) / 360.0f) * 2.0f ) - 1.0f;
            if(unitX < 0.0f)
            {
                game.p2Enabled = false;
                gameMode = MAIN_GAME_SCREEN;
            }
            else
            {
                game.p2Enabled = true;
                gameMode = MAIN_GAME_SCREEN;
            }
        }
    }
    void EndMenuProcess(Play& game)
    {
        int x, y;
        if(SDL_GetMouseState(&x, &y) && SDL_BUTTON(SDL_BUTTON_LEFT) && wait > 0.5f)
        {
            wait = 0;
            float unitX = (((float)x / 640.0f) * 3.554f ) - 1.777f;
            float unitY = (((float)(360-y) / 360.0f) * 2.0f ) - 1.0f;
            if(unitY > -0.45f)
            {
                game.Reset();
                gameMode = MAIN_GAME_SCREEN;
                unitX = 0.0f;
                unitY = 0.0f;
            }
            else
            {
                gameMode = START_SCREEN;
                unitX = 0.0f;
                unitY = 0.0f;
            }
        }
    }
private:
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
    ShaderProgram programU;
    
    program.Load(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");
    programU.Load(RESOURCE_FOLDER"vertex.glsl", RESOURCE_FOLDER"fragment.glsl");
    
    glm::mat4 projectionMatrix = glm::mat4(1.0f);
    
    float aspectRatio = screenWidth / screenHeight;
    float projectionHeight = 1.0f;
    float projectionWidth = projectionHeight * aspectRatio;
#define SCREENWIDTH projectionWidth
    float projectionDepth = 1.0f;
    projectionMatrix = glm::ortho(-projectionWidth, projectionWidth, -projectionHeight, projectionHeight,
                                  -projectionDepth, projectionDepth);
    program.SetProjectionMatrix(projectionMatrix);
    programU.SetProjectionMatrix(projectionMatrix);
    
    glm::mat4 viewMatrix = glm::mat4(1.0f);
    program.SetViewMatrix(viewMatrix);
    programU.SetViewMatrix(viewMatrix);
    
    Mix_OpenAudio( 44100, MIX_DEFAULT_FORMAT, 2, 4096 );
    Mix_Music* music;
    music = Mix_LoadMUS(RESOURCE_FOLDER"bensound-deepblue.mp3");
    Mix_PlayMusic(music, -1);
    
    GLuint fontSheet = LoadTexture(RESOURCE_FOLDER"font1.png", 1);
    GLuint spriteSheet = LoadTexture(RESOURCE_FOLDER"sheet.png", 1);
    
    srand(time(NULL));

#ifdef _WINDOWS
    glewInit();
#endif
    
    const Uint8 *keys = SDL_GetKeyboardState(NULL);

    Menu menus;
    Play game(spriteSheet);
    float lastFrameTicks = 0.0f;
    
    // 60 FPS (1.0f/60.0f) (update sixty times a second)
#define FIXED_TIMESTEP 0.0166666f
#define MAX_TIMESTEPS 6
    float accumulator = 0.0f;

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
            if(keys[SDL_SCANCODE_M])
            {
                gameMode = START_SCREEN;
            }
            if(keys[SDL_SCANCODE_Q])
            {
                exit(0);
            }
            switch(gameMode)
            {
                case START_SCREEN:
                    menus.MenuUpdate(FIXED_TIMESTEP);
                    break;
                case MAIN_GAME_SCREEN:
                    game.Update(FIXED_TIMESTEP);
                    break;
                case END_GAME_SCREEN:
                    menus.MenuUpdate(FIXED_TIMESTEP);
                    break;
            }
            elapsed -= FIXED_TIMESTEP;
        }
        //process, update, render
        accumulator = elapsed;
        //UPDATE
        
        if(keys[SDL_SCANCODE_M])
        {
            gameMode = START_SCREEN;
        }
        if(keys[SDL_SCANCODE_Q])
        {
            exit(0);
        }
        if(keys[SDL_SCANCODE_0])
        {
            gameMode = END_GAME_SCREEN;
        }
        switch(gameMode)
        {
            case START_SCREEN:
                menus.MenuUpdate(elapsed);
                menus.MainMenuProcess(game);
                menus.MainMenuRender(program, fontSheet);
                break;
            case MAIN_GAME_SCREEN:
                game.Update(elapsed);
                game.ProcessInput(keys);
                game.Render(program, programU);
                break;
            case END_GAME_SCREEN:
                menus.MenuUpdate(elapsed);
                menus.EndMenuProcess(game);
                menus.EndMenuRender(program, fontSheet, game);
                break;
        }

        // PUT STUFF HERE
        
        SDL_GL_SwapWindow(displayWindow);
        
        //PUT STUFF HERE
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

const std::vector<std::pair<float, float>> floatPairs(std::vector<glm::vec4> vertices)
{
    std::vector<std::pair<float, float>> pairs;
    for(int pair = 0; pair < vertices.size(); pair++)
    {
//        std::cout << vertices[pair].x << " " << vertices[pair].y << std::endl;
        std::pair<float, float> temp = { vertices[pair].x, vertices[pair].y};
        pairs.push_back(temp);
    }
//    for(int pair = 0; pair < pairs.size(); pair++)
//    {
//        std::cout << pairs[pair].first << " " << pairs[pair].second << std::endl;
//    }
    return pairs;
}
