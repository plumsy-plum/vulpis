#include <SDL2/SDL.h>
#include <SDL2/SDL_error.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_video.h>
#include <algorithm>
#include <iostream>
#include <vector>
#include <string>

extern "C" {
  #include <lua.h>
  #include <lualib.h>
  #include <lauxlib.h>
}

struct Node {
  std::string type;
  std::vector<Node*> children;

  int x = 0, y = 0;
  int w = 0, h = 0;

  int spacing = 0;
  int margin = 0;
  int padding = 0;

  SDL_Color color = {255, 255, 255, 255};
};

Node* buildNode(lua_State* L, int idx) {
  luaL_checktype(L, idx, LUA_TTABLE);

  Node* n = new Node();

  // type
  lua_getfield(L, idx, "type");
  if (lua_isstring(L, -1)) 
    n->type = lua_tostring(L, -1);
  lua_pop(L, 1);

  // spacing
  lua_getfield(L, idx, "spacing");
  n->spacing = luaL_optinteger(L, -1, 0);
  lua_pop(L, 1);

  // width
  lua_getfield(L, idx, "w");
  n->w = luaL_optinteger(L, -1, 0);
  lua_pop(L, 1);

  // height
  lua_getfield(L, idx, "h");
  n->h = luaL_optinteger(L, -1, 0);
  lua_pop(L, 1);

  // padding
  lua_getfield(L, idx, "padding");
  n->padding = luaL_optinteger(L, -1, 0);
  lua_pop(L, 1);

  // margin
  lua_getfield(L, idx, "margin");
  n->margin = luaL_optinteger(L, -1, 0);
  lua_pop(L, 1);

  // color
  lua_getfield(L, idx, "color");
  if (lua_istable(L, -1)) {
    lua_rawgeti(L, -1, 1); n->color.r = luaL_optinteger(L, -1, 255); lua_pop(L, 1);
    lua_rawgeti(L, -1, 2); n->color.g = luaL_optinteger(L, -1, 255); lua_pop(L, 1);
    lua_rawgeti(L, -1, 3); n->color.b = luaL_optinteger(L, -1, 255); lua_pop(L, 1);
    n->color.a = 255;
  }
  lua_pop(L, 1);

  // children
  lua_getfield(L, idx, "children");
  if (lua_istable(L, -1)) {
    lua_pushnil(L);
    while (lua_next(L, -2) != 0) {
      Node* child = buildNode(L, lua_gettop(L));
      n->children.push_back(child);
      lua_pop(L, 1);
    }
  }
  lua_pop(L, 1);

  return n;
}

void measure(Node* n) {

  if (n->type == "vstack") {
    int totalH = 0;
    int maxW = 0;

    for (Node* c : n->children) {
      measure(c);

      int childH = c->h + (2 * c->margin);
      int childW = c->w + (2 * c->margin);

      totalH += childH + n->spacing;
      maxW = std::max(maxW, childW);
    }

    if (!n->children.empty()) {
      totalH -= n->spacing; // remove last added spacing
    }

    n->w = maxW + (2 * n->padding);
    n->h = totalH + (2 * n->padding);
  }
  else if (n->type == "hstack") {
    int totalW = 0;
    int maxH = 0;

    for (Node* c : n->children) {
      measure(c);

      int childH = c->h + (2 * c->margin);
      int childW = c->w + (2 * c->margin);

      totalW += childW + n->spacing;
      maxH = std::max(maxH, childH);
    }

    if (!n->children.empty()) {
      totalW -= n->spacing;
    }

    n->w = totalW + (2 * n->padding);
    n->h = maxH + (2 * n->padding);
  }
}

void layout(Node* n, int x, int y) {
  n->x = x;
  n->y = y;

  if (n->type == "vstack") {
    int cursor = y + n->padding;

    for (Node* c : n->children) {
      int cx = x + n->padding + c->margin;
      int cy = cursor + c->margin;

      layout(c, cx, cy);
      cursor += c->h + n->spacing + (2 * c->margin);
    }
  }
  else if (n->type == "hstack") {
    int cursor = x + n->padding; 

    for (Node* c : n->children) {
      int cx = cursor + c->margin;
      int cy = y + n->padding + c->margin;

      layout(c, cx, cy);
      cursor += c->w + n->spacing + (2 * c->margin);
    }
  }
}

void renderNode(SDL_Renderer* r, Node* n) {
  if (n->type == "rect") {
    SDL_SetRenderDrawColor(r, n->color.r, n->color.g, n->color.b, n->color.a);
    SDL_Rect rect = { n->x, n->y, n->w, n->h };
    SDL_RenderFillRect(r, &rect);
  }

  for (Node* c : n->children) {
    renderNode(r, c);
  }
}

void freeTree(Node* n) {
  for (Node* c : n->children)
    freeTree(c);
  delete n;
}

int main() {
  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    std::cout << "SDL Init Failed: " << SDL_GetError() << std::endl;
    return 1;
  }

  SDL_Window* window = SDL_CreateWindow(
    "Vulpis window",
    SDL_WINDOWPOS_CENTERED,
    SDL_WINDOWPOS_CENTERED,
    800,
    600,
    SDL_WINDOW_SHOWN
  );

  if (!window) {
    std::cout << "Window Creation Failed: " << SDL_GetError() << std::endl;
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

  if (luaL_dofile(L, "../lua/app.lua") != LUA_OK) {
    std::cout << "Lua Error: " << lua_tostring(L, -1) << std::endl;
    lua_close(L);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }

  bool running = true;
  SDL_Event event;

  while (running) {
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        running = false;
      }
    }

    // Build UI tree from Lua
    lua_getglobal(L, "UI");
    if (!lua_istable(L, -1)) {
      lua_pop(L, 1);
      SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
      SDL_RenderClear(renderer);
      SDL_RenderPresent(renderer);
      continue;
    }

    Node* root = buildNode(L, -1);
    lua_pop(L, 1);

    // Layout
    measure(root);
    layout(root, 0, 0);

    // Render
    SDL_SetRenderDrawColor(renderer ,30,30,30,255);
    SDL_RenderClear(renderer);
    renderNode(renderer, root);
    SDL_RenderPresent(renderer);

    // Cleanup
    freeTree(root);
  }

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  lua_close(L);
  return 0;
}

