#pragma once

#include <string>
#include <vector>
#include <SDL2/SDL.h>

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

enum class Justify {
  Start,
  End,
  Center,
  SpaceBetween,
  SpaceAround,
  SpaceEvenly
};

enum class Align {
  Start,
  Center,
  End,
  Stretch
};

struct Node {
  std::string type;
  std::vector<Node*> children;

  int x = 0, y = 0;
  int w = 0, h = 0;

  int spacing = 0;
  int margin = 0;
  int marginTop = 0, marginBottom = 0, marginLeft = 0, marginRight = 0;
  int padding = 0;
  int paddingTop = 0, paddingBottom = 0, paddingLeft = 0, paddingRight = 0;

  int minWidth = 0, maxWidth = 99999;
  int minHeight = 0, maxHeight = 99999;

  float flexGrow = 0.0f;
  Align alignItems = Align::Start;
  Justify justifyContent = Justify::Start;

  SDL_Color color = {0,0,0,0};
  bool hasBackground = false;
};

Node* buildNode(lua_State* L, int idx);
void renderNode(SDL_Renderer* r, Node* n);
void freeTree(Node* n);

