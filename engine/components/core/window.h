#pragma once

#include <SDL2/SDL.h>

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

//window management functions
SDL_Window* createWindow(const char* title, int width, int height, bool maximized = false);
void destroyWindow(SDL_Window* window);

//lua bindings
int l_setWindowSize(lua_State* L);
void registerWindowFunctions(lua_State* L, SDL_Window* window);

