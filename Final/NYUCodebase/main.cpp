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

enum GameState { START_SCREEN, INSTRUCTION_SCREEN, MAIN_GAME_SCREEN, END_GAME_SCREEN};
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

class Particle
{
public:
    Particle(glm::vec3 position, glm::vec3 velocity, float lifetime, glm::vec4 sColor, glm::vec4 eColor)
    : position(position), velocity(velocity), lifetime(lifetime), startColor(sColor), endColor(eColor)
    {
        position.x += velocity.x * lifetime * 2;
        position.y += velocity.y * lifetime * 2;
    }
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
    ParticleEmitter() {}
    ParticleEmitter(glm::vec3 position, float particleLife, unsigned int particleAmount) : position(position), maxLifetime(particleLife)
    {
        for(int add = 0; add < particleAmount; add++)
        {
            particles.push_back(Particle(glm::vec3(position), glm::vec3(genRandom(-1, 1), genRandom(-1, 1), 0.0f), genRandom(0, maxLifetime), glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), glm::vec4(0.0f, 0.0f, 0.0f, 0.0f)));
        }
        enable = true;
    }
    ParticleEmitter(glm::vec3 position, float emitterLife, float particleLife, unsigned int particleAmount, glm::vec4 startColor, glm::vec4 endColor) : position(position), emitterLife(emitterLife), maxLifetime(particleLife)
    {
        for(int add = 0; add < particleAmount; add++)
        {
            particles.push_back(Particle(glm::vec3(position), glm::vec3(genRandom(-1, 1), genRandom(-1, 1), 0.0f), genRandom(0, maxLifetime), startColor, endColor));
        }
        timer = 0.0f;
        enable = true;
    }
    void Update(float elapsed)
    {
        timer += elapsed;
        if(timer > emitterLife && emitterLife != -1.0f)
        {
            enable = false;
        }
        matrix = glm::mat4(1.0f);
        for(Particle& part : particles)
        {
            if(part.lifetime > maxLifetime)
            {
                part.lifetime = 0.0f;
                part.position.x = position.x;
                part.position.y = position.y;
            }
            part.position.x += part.velocity.x * elapsed;
            part.position.y += part.velocity.y * elapsed;
            part.lifetime += elapsed;
        }
    }
    void Render(ShaderProgram& program)
    {
        program.SetModelMatrix(matrix);
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
    
    bool enable;
    glm::vec3 position;
    glm::mat4 matrix;
    float maxLifetime;
    float emitterLife;
    float timer;
    std::pair<float, float> maxSpan;
    
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
        moveSpeed = 0.75f;
        rotationSpeed = moveSpeed*5;
        glm::vec3 defaultSet = glm::vec3(0.0f, 0.0f, 0.0f);
        velocity = defaultSet;
        acceleration = defaultSet;
        time = 0.0f;
        health = 5;
        rotateAmount = 0.0f;
    }
    void setEdgeSet()
    {
        edgeSet.push_back(glm::vec4(sprite.vertices[0], sprite.vertices[1], 1.0f, 1.0f));
        edgeSet.push_back(glm::vec4(sprite.vertices[10], sprite.vertices[11], 1.0f, 1.0f));
        edgeSet.push_back(glm::vec4(sprite.vertices[2], sprite.vertices[3], 1.0f, 1.0f));
        edgeSet.push_back(glm::vec4(sprite.vertices[4], sprite.vertices[5], 1.0f, 1.0f));
    }
    std::vector<glm::vec4> transformEdgeSet()
    {
        std::vector<glm::vec4> temp;
        for(glm::vec4& transform : edgeSet)
        {
            temp.push_back(matrix * transform);
        }
        return temp;
    }
    void Update(float elapsed)
    {
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
                shoot = true;
                time = 0.0f;
            }
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
    }
    void Render(ShaderProgram& program)
    {
        program.SetModelMatrix(matrix);
        sprite.DrawSprite(program);
    }
    void collisionUpdate()
    {
        if(type == PLAYER)
        {
            health--;
            Mix_PlayChannel(-1, hitSound, 0);
        }
        if(type == BULLET)
        {
            position = glm::vec3(0.0f, -20.0f, 0.0f);
            velocity = glm::vec3(0.0f, 0.0f, 0.0f);
        }
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
    bool shoot;
    
    std::vector<SDL_Scancode> sCodes;
    
    EntityType type;
    
    SheetSprite sprite;
    
    Mix_Chunk* shootSound;
    Mix_Chunk* hitSound;
    Mix_Chunk* deathSound;
    
private:
};

class Asteroid : public std::iterator<std::input_iterator_tag, Asteroid>
{
public:
    Asteroid(glm::vec3 position, glm::vec3 size, float rotation, glm::vec3 velocity, std::vector<float> indices) : position(position), size(size), rotation(rotation), velocity(velocity)
    {
        matrix = glm::mat4(1.0f);
        matrix = glm::scale(matrix, size);
        matrix = glm::rotate(matrix, rotation, glm::vec3(0.0f, 0.0f, 1.0f));
        matrix = glm::translate(matrix, position);
        for(float addIndex : indices)
        {
            index.push_back(addIndex);
        }
        for(int mod = 0; mod < index.size(); mod+=2)
        {
            index[mod] *= size.x;
            index[mod+1] *= size.y;
        }
        isEnable = true;
        setEdgeSet();
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
    std::vector<glm::vec4> transformEdgeSet()
    {
        std::vector<glm::vec4> temp;
        for(glm::vec4& transform : edgeSet)
        {
            temp.push_back(matrix * transform);
        }
        return temp;
    }
    void collisionUpdate(std::pair<float, float> penetration, int factor)
    {
        position.x += penetration.first * 0.5f * factor;
        position.y += penetration.second * 0.5f * factor;
        //penetration contains overlap - penetration.first * 0.5 for x, penetration.second *0.5 on y
        // and you move the other entity by negative penetration.first *0.5 and negative penetration.second *0.5
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
    
    bool isEnable;
    
    glm::vec3 velocity;
    
    int health;
};

bool operator==(Asteroid& left, Asteroid& right)
{
    if(left.index.size() == right.index.size())
    {
        for(int check = 0; check < left.index.size(); check++)
        {
            if(left.index[check] != right.index[check])
            {
                return false;
            }
        }
        if(left.matrix == right.matrix)
        {
            return true;
        }
    }
    return false;
}
bool operator!=(Asteroid& left, Asteroid& right)
{
    return !(left == right);
}

class Play
{
public:
    Play(unsigned int texture)
    {
        p2Enable = false;
        background = ParticleEmitter(glm::vec3(0.0f, 0.0f, 0.0f), -1, 5, 1000, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));
        screenShake = false;
        screenTime = 0.0f;
        player1 = Entity(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.132f, 0.1f, 0.0f), 0.0f, glm::vec3(1.0f, 1.0f, 1.0f), {SDL_SCANCODE_W, SDL_SCANCODE_S, SDL_SCANCODE_A, SDL_SCANCODE_D, SDL_SCANCODE_F}, PLAYER); // pos, size, rotation, up, down, rotateL, rotateR, Shoot
        player2 = Entity(glm::vec3(0.25f, 0.0f, 0.0f), glm::vec3(0.132f, 0.1f, 0.0f), 0.0f, glm::vec3(1.0f, 1.0f, 1.0f), {SDL_SCANCODE_UP, SDL_SCANCODE_DOWN, SDL_SCANCODE_LEFT, SDL_SCANCODE_RIGHT, SDL_SCANCODE_SPACE}, PLAYER);
        player1.sprite = SheetSprite(texture, 0.0f/1024.0f, 941.0f/1024.0f, 112.0f/1024.0f, 75.0f/1024.0f, 0.1f);
        player1.setEdgeSet();
        player1.shootSound = Mix_LoadWAV(RESOURCE_FOLDER"shoot2.wav");
        player1.hitSound = Mix_LoadWAV(RESOURCE_FOLDER"hit.wav");
        player1.deathSound = Mix_LoadWAV(RESOURCE_FOLDER"death.wav");
        player2.sprite = SheetSprite(texture, 112.0f/1024.0f, 791.0f/1024.0f, 112.0f/1024.0f, 75.0f/1024.0f, 0.1f);
        player2.setEdgeSet();
        player2.shootSound = Mix_LoadWAV(RESOURCE_FOLDER"shoot2.wav");
        player2.hitSound = Mix_LoadWAV(RESOURCE_FOLDER"hit.wav");
        player2.deathSound = Mix_LoadWAV(RESOURCE_FOLDER"death2.wav");
        max_bullets = 20;
        bulletIndex = 0;
        bullets = std::vector<Entity>(max_bullets);
        SheetSprite bulletSprite = SheetSprite(texture, 856.0f/1024.0f, 602.0f/1024.0f, 9.0f/1024.0f, 37.0f/1024.0f, 0.1f);
        for(int i=0; i < max_bullets; i++)
        {
            bullets[i] = Entity(glm::vec3(0.0f, -20.0f, 0.0f), glm::vec3(0.1f, 0.1f, 0.0f), 0.0f, glm::vec3(0.5f, 0.5f, 0.0f), BULLET);
            bullets[i].sprite = bulletSprite;
            bullets[i].setEdgeSet();
        }
        possibleIndices.push_back({ -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f});
        possibleIndices.push_back({ -0.4, -0.4f, 0.4f, -0.4f, 0.6f, 0.0f, 0.5f, 0.5f, -0.6f, 0.0f});
        possibleIndices.push_back({ -0.5, -0.5f, 0.5f, -0.5f, 0.0f, 0.5f});
        possibleIndices.push_back({ -0.4f, -0.4f, 0.4f, -0.4f, 0.8f, 0.0f, 0.4f, 0.4f, -0.4f, 0.4f, -0.8f, 0.0f });
        asteroidInitialization();
        player1Score = 0;
        player2Score = 0;
        timer = 0.0f;
    }
    Entity player1;
    Entity player2;
    int player1Score;
    int player2Score;
    std::vector<Asteroid> asteroids;
    int max_bullets;
    int bulletIndex;
    std::vector<Entity> bullets;
    bool p2Enable;
    bool screenShake;
    float screenTime;
    float timer;
    ParticleEmitter background;
    std::vector<ParticleEmitter> collisions;
    std::vector<std::vector<float>> possibleIndices;
    void player2Enable()
    {
        p2Enable = true;
        player2.health = 5;
        player1.position = glm::vec3(-0.25f, 0.0f, 0.0f);
//        std::cout << "Position: " << player2.position.x << ", " << player2.position.y << ", " << player2.position.z << std::endl;
//        std::cout << "Rotation: " << player2.rotation << ", Rotation Amount: " << player2.rotateAmount << std::endl;
//        std::cout << "Health: " << player2.health << ", Score: " << player2Score << std::endl;
//        std::cout << "Edge Set: " << player2.edgeSet[0].x << ", " << player2.edgeSet[0].y << ", " << player2.edgeSet[0].z << std::endl;
//        std::cout << "Edge Set: " << player2.edgeSet[1].x << ", " << player2.edgeSet[1].y << ", " << player2.edgeSet[1].z << std::endl;
//        std::cout << "Edge Set: " << player2.edgeSet[2].x << ", " << player2.edgeSet[2].y << ", " << player2.edgeSet[2].z << std::endl;
//        std::cout << "Edge Set: " << player2.edgeSet[3].x << ", " << player2.edgeSet[3].y << ", " << player2.edgeSet[3].z << std::endl;
//        std::cout << "Enabled: " << std::boolalpha << p2Enable << std::endl;
    }
    void asteroidInitialization()
    {
        asteroids.clear();
        for(int i = 0; i < 5; i++)
        {
            asteroidCreation();
        }
    }
    void asteroidCreation()
    {
        float posX = -1.77f;
        float posY = -1.0f;
        if(genRandom(0, 1) > 0.5f)
        {
            posX = genRandom(1.5f, 1.75f);
        }
        else
        {
            posX = genRandom(-1.75f, -1.5f);
        }
        if(genRandom(0, 1) > 0.5f)
        {
            posY = genRandom(0.8f, 0.95f);
        }
        else
        {
            posY = genRandom(-0.95f, -0.8f);
        }
        int index = (int) (genRandom(0, 3.9));
        asteroids.push_back(Asteroid(glm::vec3(posX, posY, 0.0f), glm::vec3(genRandom(0.1f, 0.6f), genRandom(0.1f, 1.0f), 0.5f),
                            genRandom(0, 360), glm::vec3(genRandom(-0.75f, 0.55f), genRandom(-0.55f, 0.75f), 0.0f), possibleIndices[index]));
    }
    void Reset()
    {
        player1.health = 5;
        player2.health = 5;
        player2.position = glm::vec3(0.25f, 0.0f, 0.0f);
        player2.velocity = glm::vec3(0.0f, 0.0f, 0.0f);
        player2.rotation = 0.0f;
        if(p2Enable)
        {
            player1.position = glm::vec3(-0.25f, 0.0f, 0.0f);
        }
        player1.position = glm::vec3(0.0f, 0.0f, 0.0f);
        player1.rotation = 0.0f;
        player1.velocity = glm::vec3(0.0f, 0.0f, 0.0f);
        asteroids.clear();
        for(int i = 0; i < 5; i++)
        {
            asteroidCreation();
        }
        player1Score = 0;
        player2Score = 0;
    }
    void Render(ShaderProgram& program, ShaderProgram& untextProgram, glm::mat4 viewMatrix)
    {
        float screenShakeIntensity = 1.0f;
        if(player1.health > 0)
        {
            screenShakeIntensity = 1/player1.health;
        }
        if(p2Enable && player1.health + player2.health > 0)
        {
            screenShakeIntensity = 1/(player1.health+player2.health);
        }
        if(screenShake)
        {
            viewMatrix = glm::mat4(1.0f);
            viewMatrix = glm::translate(viewMatrix, glm::vec3(cos(genRandom(0, 1)), sin(genRandom(0, 1))* screenShakeIntensity, 0.0f));
            program.SetViewMatrix(viewMatrix);
            untextProgram.SetViewMatrix(viewMatrix);
        }
        if(!screenShake)
        {
            viewMatrix = glm::mat4(1.0f);
            viewMatrix = glm::translate(viewMatrix, glm::vec3(0.0f, 0.0f, 0.0f));
            program.SetViewMatrix(viewMatrix);
            untextProgram.SetViewMatrix(viewMatrix);
        }
        glUseProgram(untextProgram.programID);
        background.Render(untextProgram);
        for(ParticleEmitter& emitters : collisions)
        {
            if(emitters.enable)
            {
                emitters.Render(program);
            }
        }
        for(Asteroid& asteroid : asteroids)
        {
            if(asteroid.isEnable)
            {
                asteroid.Render(untextProgram);
            }
        }
        glUseProgram(program.programID);
        player1.Render(program);
        if(p2Enable)
        {
            player2.Render(program);
        }
        for(Entity& bullet : bullets)
        {
            bullet.Render(program);
        }

    }
    void Update(float elapsed)
    {
        timer += elapsed;
        if(timer > genRandom(2, 4))
        {
            asteroidCreation();
            timer = 0.0f;
        }
        screenTime += elapsed;
        if(screenTime > 0.25f)
        {
            screenShake = false;
            screenTime = 0.0f;
        }
        background.Update(elapsed);
        for(ParticleEmitter& emitter : collisions)
        {
            if(emitter.enable)
            {
                emitter.Update(elapsed);
            }
        }
        for(Asteroid& check : asteroids)
        {
            if(!check.isEnable)
            {
                continue;
            }
            std::pair<float,float> penetration;
            if(CheckSATCollision(floatPairs(check.transformEdgeSet()), floatPairs(player1.transformEdgeSet()), penetration))
            {
                screenShake = true;
                player1.collisionUpdate();
                check.isEnable = false;
            }
            if(!check.isEnable)
            {
                continue;
            }
            if(p2Enable)
            {
                if(CheckSATCollision(floatPairs(check.transformEdgeSet()), floatPairs(player2.transformEdgeSet()), penetration))
                {
                    std::cout << penetration.first << " " << penetration.second << " " << timer << std::endl;
                    screenShake = true;
                    player2.collisionUpdate();
                    check.isEnable = false;
                }
            }
            if(!check.isEnable)
            {
                continue;
            }
            for(Entity& bullet : bullets)
            {
                if(bullet.position.y != -20.0f)
                {
                    if(CheckSATCollision(floatPairs(check.transformEdgeSet()), floatPairs(bullet.transformEdgeSet()), penetration))
                    {
                        check.isEnable = false;
                        player1Score += 10;
                        bullet.collisionUpdate();
                    }
                }
            }
            if(!check.isEnable)
            {
                continue;
            }
            for(Asteroid& asteroid : asteroids)
            {
                if(asteroid != check && asteroid.isEnable)
                {
                    if(CheckSATCollision(floatPairs(check.transformEdgeSet()), floatPairs(asteroid.transformEdgeSet()), penetration))
                    {
                        check.collisionUpdate(penetration, 1);
                        asteroid.collisionUpdate(penetration, -1);
                        float xPos = check.position.x - penetration.first*50*check.size.x;
                        float yPos = check.position.y - penetration.second*50*check.size.y;
                        collisions.push_back(ParticleEmitter(glm::vec3(xPos, yPos, 0.0f), 1.0f, 1.0f, 50, glm::vec4(1.0f, 0.5f, 0.0f, 1.0f), glm::vec4(1.0f, 0.0f, 0.0f, 1.0f)));
                    }
                }
            }
        }
        player1.Update(elapsed);
        if(player1.health <= 0)
        {
            Mix_PlayChannel(-1, player1.deathSound, 0);
            gameMode = END_GAME_SCREEN;
            Reset();
        }
        if(p2Enable)
        {
            player2.Update(elapsed);
            if(player2.health <= 0)
            {
                Mix_PlayChannel(-1, player2.deathSound, 0);
                gameMode = END_GAME_SCREEN;
                Reset();
            }
        }
        bool gameOverTest = true;
        for(Asteroid& asteroid : asteroids)
        {
            if(asteroid.isEnable)
            {
                gameOverTest = false;
                asteroid.Update(elapsed);
            }
        }
        if(gameOverTest)
        {
            gameMode = END_GAME_SCREEN;
            Reset();
        }
        for(Entity& bullet : bullets)
        {
            bullet.Update(elapsed);
        }
    }
    void shoot(Entity& entity)
    {
        if(entity.shoot)
        {
            Mix_PlayChannel(-1, entity.shootSound, 0);
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
    }
    void ProcessInput(const Uint8* keys)
    {
        player1.Process(keys);
        player2.Process(keys);
    }
private:
};

class Menu
{
public:
    void MainMenuRender(ShaderProgram& program, int fontSheet)
    {
        DrawText(program, fontSheet, "Asteroids", 0.25f, 0.0005f, glm::vec3(-1.0f, 0.7f, 0.0f));
        DrawText(program, fontSheet, "1 Player", 0.15f, 0.00005f, glm::vec3(-1.5f, -0.75f, 0.0f));
        DrawText(program, fontSheet, "2 Player", 0.15f, 0.00005f, glm::vec3(0.25f, -0.75f, 0.0f));
    }
    void InstructionsRender(ShaderProgram& program, int fontSheet, Play& game)
    {
        DrawText(program, fontSheet, "Instructions:", 0.25f, 0.0005f, glm::vec3(-1.5f, 0.7f, 0.0f));
        if(game.p2Enable)
        {
            DrawText(program, fontSheet, "Player 1:", 0.1f, 0.000000001f, glm::vec3(-1.7f, 0.5f, 0.0f));
            DrawText(program, fontSheet, "Move Forward:W", 0.1f, 0.000000001f, glm::vec3(-1.7f, 0.35f, 0.0f));
            DrawText(program, fontSheet, "Move Backward:S", 0.1f, 0.000000001f, glm::vec3(-1.7f, 0.2f, 0.0f));
            DrawText(program, fontSheet, "Rotate Left:A", 0.1f, 0.000000001f, glm::vec3(-1.7f, 0.05f, 0.0f));
            DrawText(program, fontSheet, "Rotate Right:D", 0.1f, 0.000000001f, glm::vec3(-1.7f, -0.1f, 0.0f));
            
            DrawText(program, fontSheet, "Player 2:", 0.1f, 0.000000001f, glm::vec3(-0.1f, 0.5f, 0.0f));
            DrawText(program, fontSheet, "Move Forward:Up", 0.1f, 0.000000001f, glm::vec3(-0.1f, 0.35f, 0.0f));
            DrawText(program, fontSheet, "Move Backward:Down", 0.1f, 0.000000001f, glm::vec3(-0.1f, 0.2f, 0.0f));
            DrawText(program, fontSheet, "Rotate Left:Left", 0.1f, 0.000000001f, glm::vec3(-0.1f, 0.05f, 0.0f));
            DrawText(program, fontSheet, "Rotate Right:Right", 0.1f, 0.000000001f, glm::vec3(-0.1f, -0.1f, 0.0f));
        }
        else
        {
            DrawText(program, fontSheet, "Move Forward:W", 0.15f, 0.000000001f, glm::vec3(-1.25f, 0.35f, 0.0f));
            DrawText(program, fontSheet, "Move Backward:S", 0.15f, 0.000000001f, glm::vec3(-1.25f, 0.2f, 0.0f));
            DrawText(program, fontSheet, "Rotate Left:A", 0.15f, 0.000000001f, glm::vec3(-1.25f, 0.05f, 0.0f));
            DrawText(program, fontSheet, "Rotate Right:D", 0.15f, 0.000000001f, glm::vec3(-1.25f, -0.1f, 0.0f));
        }
        DrawText(program, fontSheet, "Press Q to Quit", 0.15f, 0.00005f, glm::vec3(-1.25f, -0.35f, 0.0f));
        DrawText(program, fontSheet, "Continue to Game", 0.15f, 0.00005f, glm::vec3(-1.25f, -0.55f, 0.0f));
        DrawText(program, fontSheet, "Return to Main Menu", 0.15f, 0.00005f, glm::vec3(-1.25f, -0.75f, 0.0f));
        
    }
    void InstructionsProcess(float xPos, float yPos)
    {
        if(xPos > -1.25 && xPos < 1.25 && yPos <-0.55 && yPos > -0.75)
        {
            gameMode = MAIN_GAME_SCREEN;
        }
        if(xPos > -1.25 && xPos < 1.25 && yPos <-0.75)
        {
            gameMode = START_SCREEN;
        }
    }

    void EndMenuRender(ShaderProgram& program, int fontSheet, Play& game)
    {
        if(game.p2Enable)
        {
            if(game.player1.health > 0 && game.player2.health > 0)
            {
                DrawText(program, fontSheet, "You Survived!", 0.25f, 0.000005f, glm::vec3(-1.5f, 0.7f, 0.0f));
            }
            else
            {
                DrawText(program, fontSheet, "You Died!", 0.25f, 0.0005f, glm::vec3(-1.0f, 0.7f, 0.0f));
            }
            DrawText(program, fontSheet, "Score: ", 0.15, 0.0005f, glm::vec3(-0.8f, 0.5f, 0.0f));
            DrawText(program, fontSheet, "Player1: ", 0.15f, 0.0005f, glm::vec3(-0.8f, 0.3f, 0.0f));
            DrawText(program, fontSheet, std::to_string(game.player1Score), 0.2f, 0.0005f, glm::vec3(0.4f, 0.3f, 0.0f));
            DrawText(program, fontSheet, "Player2: ", 0.15f, 0.0005f, glm::vec3(-0.8f, 0.1f, 0.0f));
            DrawText(program, fontSheet, std::to_string(game.player2Score), 0.2f, 0.0005f, glm::vec3(0.4f, 0.1f, 0.0f));
        }
        else
        {
            if(game.player1.health > 0)
            {
                DrawText(program, fontSheet, "You Survived!", 0.25f, 0.000005f, glm::vec3(-1.5f, 0.7f, 0.0f));
            }
            else
            {
                DrawText(program, fontSheet, "You Died!", 0.25f, 0.000005f, glm::vec3(-1.0f, 0.7f, 0.0f));
            }
            DrawText(program, fontSheet, "Score: ", 0.15, 0.0005f, glm::vec3(-0.6f, 0.3f, 0.0f));
            DrawText(program, fontSheet, std::to_string(game.player1Score), 0.2f, 0.0005f, glm::vec3(0.3f, 0.3f, 0.0f));
        }
        DrawText(program, fontSheet, "Replay", 0.15f, 0.00005f, glm::vec3(-0.5f, -0.45f, 0.0f));
        DrawText(program, fontSheet, "Return to Start", 0.15f, 0.00005f, glm::vec3(-1.0f, -0.75f, 0.0f));
    }
    void MainMenuProcess(float xPos, float yPos, Play& game)
    {
        if(xPos > 0.0f && yPos < 0.0f)
        {
            game.player2Enable();
            gameMode = INSTRUCTION_SCREEN;
        }
        if(xPos < 0.0f && yPos < 0.0f)
        {
            gameMode = INSTRUCTION_SCREEN;
        }
    }
    void EndMenuProcess(float xPos, float yPos, Play& game)
    {
        if(yPos > -0.5f && yPos < 0.0f)
        {
            gameMode = MAIN_GAME_SCREEN;
        }
        if(yPos < -0.5f)
        {
            game.p2Enable = false;
            gameMode = START_SCREEN;
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
            else if(event.type == SDL_MOUSEBUTTONDOWN)
            {
                float unitX = (((float)event.motion.x / 640.0f) * 3.554f ) - 1.777f;
                float unitY = (((float)(360-event.motion.y) / 360.0f) * 2.0f ) - 1.0f;
                if(event.button.button == SDL_BUTTON_LEFT)
                {
                    switch (gameMode) {
                        case START_SCREEN:
                            menus.MainMenuProcess(unitX, unitY, game);
                            break;
                        case INSTRUCTION_SCREEN:
                            menus.InstructionsProcess(unitX, unitY);
                            break;
                        case MAIN_GAME_SCREEN: break;
                        case END_GAME_SCREEN:
                            menus.EndMenuProcess(unitX, unitY, game);
                            break;
                    }
                }
            }
            else if (event.type == SDL_KEYDOWN)
            {
                if(event.key.keysym.scancode == game.player1.sCodes[4])
                {
                    game.shoot(game.player1);
                }
                if(event.key.keysym.scancode == game.player2.sCodes[4])
                {
                    game.shoot(game.player2);
                }
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
                case START_SCREEN: break;
                case INSTRUCTION_SCREEN: break;
                case MAIN_GAME_SCREEN:
                    game.Update(FIXED_TIMESTEP);
                    break;
                case END_GAME_SCREEN: break;
            }
            elapsed -= FIXED_TIMESTEP;
        }
        accumulator = elapsed;
        
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
                menus.MainMenuRender(program, fontSheet);
                break;
            case INSTRUCTION_SCREEN:
                menus.InstructionsRender(program, fontSheet, game);
                break;
            case MAIN_GAME_SCREEN:
                game.ProcessInput(keys);
                game.Render(program, programU, viewMatrix);
                break;
            case END_GAME_SCREEN:
                menus.EndMenuRender(program, fontSheet, game);
                break;
        }
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
        std::pair<float, float> temp = { vertices[pair].x, vertices[pair].y};
        pairs.push_back(temp);
    }
    return pairs;
}
