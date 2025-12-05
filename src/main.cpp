#include <SDL2/SDL.h>
#include <SDL2/SDL_error.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_video.h>
#include <iostream>

extern "C" {
  #include <lua.h>
  #include <lualib.h>
  #include <lauxlib.h>
}


int lua_drawNode(lua_State* L) {

  // check first argument is a lua table and push it on 1
  luaL_checktype(L, 1, LUA_TTABLE);

  // get parent x and y passed in function push them on 2, 3
  int parentX = luaL_optinteger(L, 2, 0);
  int parentY = luaL_optinteger(L, 3, 0);

  // upvalue renderer
  SDL_Renderer* renderer = (SDL_Renderer*)lua_touserdata(L, lua_upvalueindex(1));

  // read x field from 1
  lua_getfield(L, 1, "x");
  int x = luaL_optinteger(L, -1, 0);
  lua_pop(L, 1);

  //read y field from 1
  lua_getfield(L, 1, "y");
  int y = luaL_optinteger(L, -1, 0);
  lua_pop(L, 1);

  //read w field from 1
  lua_getfield(L, 1, "w");
  int w = luaL_optinteger(L, -1, 0);
  lua_pop(L, 1);

  // read h from field 1
  lua_getfield(L, 1, "h");
  int h = luaL_optinteger(L, -1, 0);
  lua_pop(L, 1);

  // read color from 1
  int r=255,g=255,b=255;
  lua_getfield(L, 1, "color");
  if (lua_istable(L, -1)) {
    lua_rawgeti(L, -1, 1); r = luaL_optinteger(L, -1, 255); lua_pop(L, 1);
    lua_rawgeti(L, -1, 2); g = luaL_optinteger(L, -1, 255); lua_pop(L, 1);
    lua_rawgeti(L, -1, 3); b = luaL_optinteger(L, -1, 255); lua_pop(L, 1);
  }
  lua_pop(L, 1);

  // compute absolute position
  int absX = parentX + x;
  int absY = parentY + y;

  // draw this node (as a rect)
  SDL_SetRenderDrawColor(renderer, r, g, b, 255);
  SDL_Rect rect = { absX, absY, w, h };
  SDL_RenderFillRect(renderer, &rect);

  // recursively draw children
  lua_getfield(L, 1, "children");
  if (lua_istable(L, -1)) {
    lua_pushnil(L);
    while (lua_next(L, -2) != 0) {
      lua_pushvalue(L, -1);
      lua_pushinteger(L, absX);
      lua_pushinteger(L, absY);

      lua_getglobal(L, "drawNode");
      lua_insert(L, -4);

      if (lua_pcall(L, 3, 0, 0) != LUA_OK) {
        std::cout << "Lua Error: " << lua_tostring(L, -1) << std::endl;
        lua_pop(L, 1);
      }
      lua_pop(L, 1);
    }
  }

  lua_pop(L, 1);
  return 0;
}

int main() {
  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    std::cout << "SDL Init Failed: " << SDL_GetError() << std::endl;
    return 1;
  }

  SDL_Window* window = SDL_CreateWindow(
    "Iceberg window",
    SDL_WINDOWPOS_CENTERED,
    SDL_WINDOWPOS_CENTERED,
    800,
    600,
    SDL_WINDOW_SHOWN
  );

  if (!window) {
    std::cout << "Window Creation Failed" << SDL_GetError() << std::endl;
    SDL_Quit();
    return 1;
  }

  SDL_Renderer* renderer = SDL_CreateRenderer(
    window,
    -1,
    SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
  );

  if (!renderer) {
    std::cout << "Renderer Creation Failed: " << SDL_GetError() << std::endl;
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }

  // initializing lua
  lua_State* L = luaL_newstate();
  luaL_openlibs(L);
  
  lua_pushlightuserdata(L, renderer);
  lua_pushcclosure(L, lua_drawNode, 1);
  lua_setglobal(L, "drawNode");

  if (luaL_dofile(L, "../lua/app.lua") != LUA_OK) {
    std::cout << "Lua Error: " << lua_tostring(L, -1) << std::endl;
  }

  

  bool running = true;
  SDL_Event event;

  while (running) {
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        running = false;
      }
    }

    SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
    SDL_RenderClear(renderer);

    lua_getglobal(L, "render");
    if (lua_isfunction(L, -1)) {
      if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
        std::cout << "Lua Error: " << lua_tostring(L, -1) << std::endl;
      }
    } else {
      lua_pop(L, 1);
    }

    SDL_RenderPresent(renderer);
  }

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  lua_close(L);
  return 0;
}

