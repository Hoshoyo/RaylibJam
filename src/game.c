#include "game.h"

static Game game;

void game_init()
{
    LoadSave();

    game.logo = LoadTexture("res/HailstonesLogo_Storm.png");
    game.camera = (Camera){ { 5.0f, 2.0f, 5.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, 45.0f, CAMERA_PERSPECTIVE };
    
    Mesh mesh = GenMeshCube(2.0f, 1.0f, 2.0f);
    mesh.vertices[0] += 1.0f;
    
    game.cube = LoadModelFromMesh(mesh);
    UpdateMeshBuffer(game.cube.meshes[0], 0, mesh.vertices, mesh.vertexCount * 3 * sizeof(float), 0);
}

void game_update()
{
    if(IsKeyPressed('X'))
    {
        SaveGame("Save data");
    }
    // Update
    UpdateCamera(&game.camera, CAMERA_FIRST_PERSON);
}

void game_render()
{
    BeginDrawing();
    ClearBackground(DARKGRAY);
    {
        BeginMode3D(game.camera);
    
        DrawModel(game.cube, (Vector3){0,0,0}, 1.0f, WHITE);
        DrawGrid(10, 1.0);
    
        EndMode3D();
    }

    EndDrawing();
}