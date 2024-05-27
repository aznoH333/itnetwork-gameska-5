/*******************************************************************************************
*
*   raylib [core] example - Basic window
*
*   Welcome to raylib!
*
*   To test examples, just press F6 and execute raylib_compile_execute script
*   Note that compiled executable is placed in the same folder as .c file
*
*   You can find all basic examples on C:\raylib\raylib\examples folder or
*   raylib official webpage: www.raylib.com
*
*   Enjoy using raylib. :)
*
*   Example originally created with raylib 1.0, last time updated with raylib 1.0
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2013-2024 Ramon Santamaria (@raysan5)
*
********************************************************************************************/
#include "gframework.c"
#include "raylib.h"
#include <stdlib.h>
#include <stdio.h>

//------------------------------------------------------------------------------------
// asteroid predec
//------------------------------------------------------------------------------------
#define ASTEROID_SMALL 0
#define ASTEROID_MEDIUM 1
#define ASTEROID_LARGE 2

struct Asteroid{
    float x;
    float y;
    float direction;
    float speed;
    int size;
    int lifeTime;
    bool exists;

};
typedef struct Asteroid Asteroid;
#define MAX_ASTEROIDS 300
struct AsteroidCollection{
    Asteroid asteroids[MAX_ASTEROIDS];
    int nextAsteroidIndex;
    int targetX;
    int targetY;
};
typedef struct AsteroidCollection AsteroidCollection;

bool areAsteroidsAlive(AsteroidCollection*);

struct Rocket{
    float x;
    float y;
    float direction;
    float speed;
    bool exists;
    int lifeTime;
};
typedef struct Rocket Rocket;

//------------------------------------------------------------------------------------
// game state
//------------------------------------------------------------------------------------
#define STATE_BUILD 0
#define STATE_ATTACK 1
#define STATE_GAME_OVER 2

struct GameState{
    int scrapCount;
    int state;
    int gameTimer;
    int waveTimer;
    bool giveReward;
    int wave;
    int difficulity;
    int rubberBandDifficulityModifier;
};
typedef struct GameState GameState;

GameState initGameState(){
    GameState out;

    out.state = STATE_BUILD;
    out.scrapCount = 300;
    out.gameTimer = 0;
    out.waveTimer = 0;
    out.giveReward = false;
    out.difficulity = 1;
    out.wave = 0;
    out.rubberBandDifficulityModifier = 0;


    return out;
}

void updateGameState(GameState* state, AsteroidCollection* collection){

    state->gameTimer++;


    if (state->state == STATE_ATTACK){

        state->waveTimer--;

        if (state->waveTimer <= 0 && areAsteroidsAlive(collection) == false){

            state->state = STATE_BUILD;
            state->giveReward = true;

        }

    }
}

void activateWave(GameState* state){

    state->waveTimer = min(60 + state->rubberBandDifficulityModifier + (state->wave * 30), 3000);
    state->difficulity = 1 + floor(state->wave / 10.0f);
    state->state = STATE_ATTACK;
    state->wave++;

}




//------------------------------------------------------------------------------------
// particles
//------------------------------------------------------------------------------------
#define PARTICLE_POW 0
#define PARTICLE_SCRAP 1
struct Particle{
    int x;
    int y;
    int type;
    bool destroy;
    int lifeTime;
    struct Particle* next;
};
typedef struct Particle Particle;

void pushParticle(Particle* element, Particle** particles){

    element->next = *particles;
    *particles = element;


}

void initParticle(int x, int y, int type, Particle** particles){
    Particle* p = malloc(sizeof(Particle));
    p->x = x;
    p->y = y;
    p->destroy = false;
    p->type = type;
    p->next = 0;

    switch (type){
        case PARTICLE_POW: p->lifeTime = 20; break;
        case PARTICLE_SCRAP: p->lifeTime = 45; break;

    }

    pushParticle(p, particles);
}

void updateParticles(Particle** particles){
    Particle* iter = *particles;
    Particle* prev = 0;


    while (iter != 0){

        // draw
        switch (iter->type){
            case PARTICLE_POW:

                draw(((iter->lifeTime / 45.0f) * 3.0f) + 11, iter->x, iter->y);
                break;
            case PARTICLE_SCRAP:
                draw(9, iter->x, iter->y);
                break;

        }


        // destroy
        iter->lifeTime--;
        if (iter->lifeTime <= 0){
            iter->destroy = true;
        }

        // controll
        if (iter->destroy){
            if (prev == 0){
                *particles = iter->next;
            }else {
                prev->next = iter->next;
            }
            Particle* temp = iter;
            iter = iter->next;
            free(temp);


        }else {
            // progress
            prev = iter;
            iter = iter->next;

        }

    }

}

//------------------------------------------------------------------------------------
// rockets
//------------------------------------------------------------------------------------
#define MAX_ROCKETS 30
Rocket rockets[MAX_ROCKETS];
int nextRocketIndex = 0;

void initRocket(float x, float y, float rotation){
    Rocket r;
    r.x = x;
    r.y = y;
    r.direction = rotation;
    r.speed = 4.5f;
    r.exists = true;
    r.lifeTime = 200;

    int failsafe = MAX_ROCKETS;
    while(rockets[nextRocketIndex].exists && failsafe-- > 0){
        nextRocketIndex++;
        nextRocketIndex %= MAX_ROCKETS;
    }
    rockets[nextRocketIndex] = r;
}

void initRockets(){
    for (int i = 0; i < MAX_ROCKETS; i++){
        rockets[i].exists = false;
    }
}

void updateRockets(Particle** particles, GameState* state){
    for (int i = 0; i < MAX_ROCKETS; i++){

        Rocket* r = &rockets[i];


        if (r->exists){
            r->x += sin(r->direction) * r->speed;
            r->y += cos(r->direction) * r->speed;

            drawR(10, r->x, r->y, -r->direction * RAD2DEG + 90);
            r->lifeTime--;

            if (r->lifeTime <= 0){
                r->exists = false;

            }
            if (state->gameTimer % 4 == 0){
                initParticle(r->x, r->y, PARTICLE_POW, particles);
            }
        }
    }

}



//------------------------------------------------------------------------------------
// scrap
//------------------------------------------------------------------------------------
int scrapCount = 0;

void addScrap(int x, int y, int ammount, GameState* gameState, Particle** particles){
    gameState->scrapCount += ammount;
    initParticle(x, y, PARTICLE_SCRAP, particles);
}




//------------------------------------------------------------------------------------
// tile
//------------------------------------------------------------------------------------
struct StationTile{
    int health;
    int maxHealth;
    int type;
    bool isPowered;
    int x;
    int y;
    int stationX;
    int stationY;
    bool exists;
    int cooldown;
};
typedef struct StationTile StationTile;

#define STATION_WALL 0
#define STATION_GENERATOR 1
#define STATION_TURRET 2
#define STATION_FORGE 3
#define STATION_CORE 4
#define STATION_SPRITE_START 1
int STATION_TILE_HEALTH_LOOKUP[] = {50, 20, 40, 30, 100};
int STATION_TILE_COST_LOOKUP[] = {20, 30, 40, 40};


void drawStationTile(StationTile* tile, GameState* state){
    draw(STATION_SPRITE_START + tile->type, tile->x, tile->y);


    // draw damage
    if (tile->health < tile->maxHealth >> 1){
        draw(7, tile->x, tile->y);
    }else if(tile->health < tile->maxHealth){
        draw(6, tile->x, tile->y);
    }

    // unpowered status
    if (tile->isPowered == false){
        Color c = WHITE;
        c.a = (unsigned char)lerp(40, WHITE.r, (sin(state->gameTimer / 25.0f) * 0.5f + 0.5f));

        drawC(17, tile->x, tile->y, c);
    }


}

void roundEndTileUpdate(StationTile* tile, GameState* gameState, Particle** particles){
    // repair rile
    tile->health = tile->maxHealth;

    if (!tile->isPowered){
        return;
    }

    switch (tile->type){
        case STATION_FORGE:
            addScrap(tile->x, tile->y, 20, gameState, particles);
            break;
        case STATION_CORE:
            addScrap(tile->x, tile->y, 60, gameState, particles);
    }
}

void setTilePoweredStatus(StationTile* tile, bool isNextToGenerator){
    switch (tile->type){
        case STATION_FORGE:
        case STATION_TURRET:
            tile->isPowered = isNextToGenerator;
            break;
        default:
            tile->isPowered = true;
            break;
    }
}

bool isTileTypeGenerator(int type){
    switch (type){
        case STATION_GENERATOR:
        case STATION_CORE:
            return true;
        default:
            return false;
    }
}

StationTile initStationTile(int type, int x, int y, int stationX, int stationY){
    StationTile out;
    out.x = x;
    out.y = y;
    out.stationX = stationX;
    out.stationY = stationY;
    out.isPowered = true;
    out.maxHealth = STATION_TILE_HEALTH_LOOKUP[type];
    out.health = out.maxHealth;
    out.type = type;
    out.exists = true;
    out.cooldown = 0;

    return out;
}

StationTile initEmptyTile(){
    StationTile out;
    out.x = 0;
    out.y = 0;
    out.stationX = 0;
    out.stationY = 0;
    out.isPowered = true;
    out.maxHealth = 0;
    out.health = 0;
    out.type = 0;
    out.exists = false;
    out.cooldown = 0;


    return out;

}

void damageTile(StationTile* tile, int damage){
    tile->health -= damage;
    if (tile->health < 0){
        tile->exists = false;
        tile->stationX = -999;
        tile->stationY = -999;
    }
}

//------------------------------------------------------------------------------------
// Station
//------------------------------------------------------------------------------------
#define MAX_STATION_TILES 120
struct Station{
    StationTile tiles[MAX_STATION_TILES];
    int nextTileIndex;
    int cursorX;
    int cursorY;
    int x;
    int y;
};
typedef struct Station Station;

bool canCursorMoveTo(Station* station, int newX, int newY){
    for (int i = 0; i < MAX_STATION_TILES; i++){
        StationTile* tile = &station->tiles[i];
        if (tile->exists && abs(tile->stationX - newX) <= 1 && abs(tile->stationY - newY) <= 1){
            return true;
        }
    }
    return false;
}


StationTile* getTile(Station* station, int tileX, int tileY){

    for (int i = 0; i < MAX_STATION_TILES; i++){

        StationTile* tile = &station->tiles[i];

        if (tile->stationX == tileX && tile->stationY == tileY){
            return tile;
        }
    }
    return 0;
}


bool canBuildTile(Station* station){
    StationTile* tile = getTile(station, station->cursorX, station->cursorY);

    if (tile == 0 || tile->exists == false){
        return true;
    }
    return false;
}

void updateStationPoweredStatus(Station* station){
    for (int i = 0; i < MAX_STATION_TILES; i++){

        StationTile* tile = &station->tiles[i];

        if (!tile->exists){
            continue;
        }

        bool isNextToGenerator = false;
        for (int x = tile->stationX - 1; x <= tile->stationX + 1; x++){
            for (int y = tile->stationY - 1; y <= tile->stationY + 1; y++){
                StationTile* tile = getTile(station, x, y);

                if (tile != 0 && tile->exists && isTileTypeGenerator(tile->type)){
                    isNextToGenerator = true;
                    goto exitLoop;
                }
            }
        }
        exitLoop:

        setTilePoweredStatus(tile, isNextToGenerator);
    }
}


void addTile(Station* station, int type, int x, int y){

    StationTile tile = initStationTile(type, station->x + (x * 32), station->y + (y * 32), x, y);

    int failsafe = MAX_STATION_TILES;
    while(station->tiles[station->nextTileIndex].exists == true && failsafe-- > 0){
        station->nextTileIndex++;
        station->nextTileIndex %= MAX_STATION_TILES;
    }
    station->tiles[station->nextTileIndex] = tile;
    updateStationPoweredStatus(station);
}

Station initStation(int x, int y){
    Station out;
    out.nextTileIndex = 0;
    out.cursorX = 0;
    out.cursorY = 0;
    out.x = x;
    out.y = y;

    // init empty tiles
    for (int i = 0; i < MAX_STATION_TILES; i++){
        out.tiles[i] = initEmptyTile();
    }


    addTile(&out, STATION_CORE, 0, 0);




    return out;
}

StationTile* collidesWithStation(Station* station, int x, int y, int w, int h){
    for (int i = 0; i < MAX_STATION_TILES; i++){

        StationTile* tile = &station->tiles[i];

        if (tile->exists && checkBoxCollisions(x, y, w, h, tile->x, tile->y, 32, 32)){
            return tile;
        }

    }
    return 0;
}

Asteroid* findClosestAsteroid(AsteroidCollection*, float x, float y);

void updateStation(Station* station, GameState* state, Particle** particles, AsteroidCollection* asteroids){


    // draw tiles
    for (int i = 0; i < MAX_STATION_TILES; i++){
        if (station->tiles[i].exists){
            drawStationTile(&station->tiles[i], state);
        }
    }


    if (state->state == STATE_BUILD){
        // give reward
        if (state->giveReward){
            state->giveReward = false;

            for (int i = 0; i < MAX_STATION_TILES; i++){

                StationTile* tile = &station->tiles[i];

                if (tile->exists){


                    roundEndTileUpdate(tile, state, particles);

                }

            }
            // assign rubberBandDifficulityModifier
            int currentRB = state->scrapCount - 100;
            if (currentRB > 0 && currentRB > state->rubberBandDifficulityModifier){
                state->rubberBandDifficulityModifier = currentRB;
            }

            // reset cursor
            if (!canCursorMoveTo(station, station->cursorX, station->cursorY)){
                station->cursorX = 0;
                station->cursorY = 0;
            }

            // check game over
            StationTile* coreTile = getTile(station, 0, 0);
            if (coreTile == 0 || coreTile->exists == false){
                state->state = STATE_GAME_OVER;
            }
        }


        // cursor
        draw(18, station->x + (station->cursorX * 32), station->y + (station->cursorY * 32));

        if (IsKeyPressed(KEY_W) && canCursorMoveTo(station, station->cursorX, station->cursorY - 1)){
            station->cursorY -= 1;
        }

        if (IsKeyPressed(KEY_S) && canCursorMoveTo(station, station->cursorX, station->cursorY + 1)){
            station->cursorY += 1;
        }

        if (IsKeyPressed(KEY_A) && canCursorMoveTo(station, station->cursorX - 1, station->cursorY)){
            station->cursorX -= 1;
        }

        if (IsKeyPressed(KEY_D) && canCursorMoveTo(station, station->cursorX + 1, station->cursorY)){
            station->cursorX += 1;
        }

        // building
        if (canBuildTile(station)){
            for (int i = 0; i <= 3; i++){
                if (IsKeyPressed(KEY_ONE + i) && state->scrapCount >= STATION_TILE_COST_LOOKUP[i]){
                    addTile(station, i, station->cursorX, station->cursorY);
                    state->scrapCount -= STATION_TILE_COST_LOOKUP[i];
                }
            }
        }


        // start wave
        if (IsKeyPressed(KEY_SPACE)){
            activateWave(state);
        }
    }





    else if (state->state == STATE_ATTACK){
        // shoot
        for (int i = 0; i < MAX_STATION_TILES; i++){
            StationTile* tile = &station->tiles[i];
            if (tile->exists && tile->isPowered && tile->type == STATION_TURRET){

                tile->cooldown--;

                if (tile->cooldown <= 0){
                    Asteroid* a = findClosestAsteroid(asteroids, tile->x, tile->y);

                    if (a != 0){

                        initRocket(tile->x, tile->y, atan2(a->x - tile->x, a->y - tile->y));
                        tile->cooldown = 100;


                    }

                }

            }
        }
    }

}

//------------------------------------------------------------------------------------
// Asteroids
//------------------------------------------------------------------------------------
Asteroid* findClosestAsteroid(AsteroidCollection* collection, float x, float y){

    Asteroid* out = 0;
    float dist = 200.0f;
    for (int i = 0; i < MAX_ASTEROIDS; i++){
        Asteroid* a = &collection->asteroids[i];

        if (a->exists && a->size > 0){

            float p = pythagoras(x, y, a->x, a->y);

            if (p < dist){
                out = a;
            }
        }
    }
    return out;

}

#define SMALL_ASTEROID_LIFETIME 400
void initAsteroid(AsteroidCollection* collection, float x, float y, int size, float direction, float speed){
    if (size < 0){
        return;
    }

    Asteroid a;
    a.x = x;
    a.y = y;
    a.size = size;
    a.direction = direction;
    a.speed = speed;
    a.lifeTime = SMALL_ASTEROID_LIFETIME * (size + 1);
    a.exists = true;

    int failsafe = MAX_ASTEROIDS;
    while(collection->asteroids[collection->nextAsteroidIndex].exists && failsafe-- > 0){
        collection->nextAsteroidIndex++;
        collection->nextAsteroidIndex %= MAX_ASTEROIDS;
    }

    collection->asteroids[collection->nextAsteroidIndex] = a;
}

void destroyAsteroid(Asteroid* this, AsteroidCollection* collection, Particle** particles){
    this->exists = false;
    initParticle(this->x + (sin(this->direction) * 16), this->y + (cos(this->direction) * 16), PARTICLE_POW, particles);
    for (int i = GetRandomValue(2, 3); i > 0; i--){
        float direction = GetRandomValue(0, 360) * DEG2RAD;
        initAsteroid(collection, this->x, this->y, this->size - 1, direction, this->speed * 1.1f);
    }

}

AsteroidCollection initAsteroidCollection(int targetX, int targetY){
    AsteroidCollection collection;
    collection.nextAsteroidIndex = 0;
    collection.targetX = targetX;
    collection.targetY = targetY;

    for (int i = 0; i < MAX_ASTEROIDS; i++){
        collection.asteroids[i].exists = false;
    }
    return collection;
}

bool areAsteroidsAlive(AsteroidCollection* collection){
    for (int i = 0; i < MAX_ASTEROIDS; i++){
        if (collection->asteroids[i].exists){
            return true;
        }
    }
    return false;
}

#define ASTEROID_SPRITE_START 14
#define ASTEROID_SPAWN_DISTANCE 356
void updateAsteroids(AsteroidCollection* collection, GameState* state, Station* station, Particle** particles){

    // spawn asteroids
    if (state->state == STATE_ATTACK){

        if (state->gameTimer % 10 == 0 && state->waveTimer > 0){

            for (int i = GetRandomValue(1, state->difficulity); i > 0;i--){

                float direction = GetRandomValue(0, 360) * DEG2RAD;


                float spawnX = collection->targetX + (sin(direction + PI) * ASTEROID_SPAWN_DISTANCE);
                float spawnY = collection->targetY + (cos(direction + PI) * ASTEROID_SPAWN_DISTANCE);

                float speed = 1.0f + (GetRandomValue(0, 4) * 0.2f);
                int size = GetRandomValue(ASTEROID_SMALL, ASTEROID_LARGE);

                initAsteroid(collection, spawnX, spawnY, size, direction, speed);
            }
        }
    }
    // update asteroids
    for (int i = 0; i < MAX_ASTEROIDS; i++){
        Asteroid* asteroid = &collection->asteroids[i];


        if (!asteroid->exists){
            continue;
        }

        asteroid->lifeTime--;

        // move
        asteroid->x += sin(asteroid->direction) * asteroid->speed;
        asteroid->y += cos(asteroid->direction) * asteroid->speed;

        // draw
        drawR(ASTEROID_SPRITE_START + asteroid->size, asteroid->x, asteroid->y, asteroid->direction * RAD2DEG);


        if (asteroid->lifeTime == 0){
            destroyAsteroid(asteroid, collection, particles);
        }else if (asteroid->lifeTime < (SMALL_ASTEROID_LIFETIME * (asteroid->size + 1)) - 300
            && !checkBoxCollisions(asteroid->x, asteroid->y, 32, 32, 0, 0, 640, 420)) // check if is on screen
        {
            asteroid->exists = false;

        }

        // collisions with rockets
        if (asteroid->size > 0){
            for (int j = 0; j < MAX_ROCKETS; j++){
                Rocket* r = &rockets[j];

                if (r->exists && checkBoxCollisions(r->x, r->y, 32, 32, asteroid->x, asteroid->y, 32, 32)){
                    destroyAsteroid(asteroid, collection, particles);
                    r->exists = false;
                    screenShake(2);
                }

            }
        }

        // collisions with tiles
        StationTile* tile = collidesWithStation(station, asteroid->x, asteroid->y, 32, 32);
        if (tile != 0){
            screenShake(0.5f);
            destroyAsteroid(asteroid, collection, particles);
            damageTile(tile, asteroid->speed * asteroid->size * 10.0f);
            updateStationPoweredStatus(station);
        }
    }
}

//------------------------------------------------------------------------------------
// HUD
//------------------------------------------------------------------------------------
const char* TILE_NAME_LOOKUP[] = {"wall", "pwr", "gun", "forge"};
#define DISPLAY_COUNTER_SIZE 10
void drawHud(GameState* state){

    char display[DISPLAY_COUNTER_SIZE];

    draw(8, 20, 20);
    sprintf(display, "%06i", state->scrapCount);
    drawFancyText(display, 52, 28, 20, WHITE);


    sprintf(display, "%06i", state->wave);
    drawFancyText("wave", 20, 52, 20, WHITE);
    drawFancyText(display, 70, 52, 20, WHITE);


    if (state->state == STATE_BUILD){

        for (int i = 0; i < 4; i++){

            draw(1 + i, 246 + i * 48, 10);
            char display[20];
            sprintf(display, "%i : %s", i + 1, TILE_NAME_LOOKUP[i]);
            drawFancyText(display, 246 + i * 48, 42, 1, WHITE);
            sprintf(display, "%i$", STATION_TILE_COST_LOOKUP[i]);
            drawFancyText(display, 246 + i * 48, 62, 1, WHITE);


        }

    }else if (state->state == STATE_GAME_OVER){
        drawFancyText("GAME OVER", 200, 100, 30, WHITE);
        drawFancyText("Press r", 240, 200, 20, WHITE);
    }

}


//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    initFramework();

    GameState state = initGameState();
    Station station = initStation(304, 164);
    AsteroidCollection asteroids = initAsteroidCollection(304, 164);
    Particle* particles = 0;
    initRockets();
    // Main game loop
    while (!WindowShouldClose())
    {
        
        fDrawBegin();
            ClearBackground(BLACK);
            updateStation(&station, &state, &particles, &asteroids);
            updateGameState(&state, &asteroids);
            updateAsteroids(&asteroids, &state, &station, &particles);
            updateParticles(&particles);
            updateRockets(&particles, &state);
            drawHud(&state);


            if (state.state == STATE_GAME_OVER && IsKeyPressed(KEY_R)){
                state = initGameState();
                station = initStation(304, 164);
                asteroids = initAsteroidCollection(304, 164);
                initRockets();

            }
        fDrawEnd();
        
    }

	disposeFramework();
    

    return 0;
}
