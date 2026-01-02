#include "ui.h"
#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_render.h>
#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <lauxlib.h>
#include <string>
#include <lua.h>
#include "../color/color.h"

bool operator!=(const Length& a, const Length& b) {
  return a.value != b.value || a.type != b.type;
}

bool operator!=(const SDL_Color& a, const SDL_Color b) {
  return a.a != b.a || a.r != b.r || a.g != b.g || a.b != b.b;
}

template <typename T> 
void update(T& field, const T& newVal, bool& dirtyFlag) {
  if (field != newVal) {
    field = newVal;
    dirtyFlag = true;
  }
}

Align parseAlign(std::string s) {
    if (s == "center") return Align::Center;
    if (s == "end") return Align::End;
    if (s == "stretch") return Align::Stretch;
    return Align::Start;
}

Justify parseJustify(std::string s) {
    if (s == "center") return Justify::Center;
    if (s == "end") return Justify::End;
    if (s == "space-around") return Justify::SpaceAround;
    if (s == "space-between") return Justify::SpaceBetween;
    if (s == "space-evenly") return Justify::SpaceEvenly;
    return Justify::Start;
}

Length getLength(lua_State* L, const char* key) {
    Length len;
    lua_getfield(L, -1, key);

    if (lua_isnumber(L, -1)) {
        len.value = (float)lua_tonumber(L, -1);
        len.type = PIXEL;
    }
    else if (lua_isstring(L, -1)) {
        std::string s = lua_tostring(L, -1);
        if (!s.empty() && s.back() == '%') {
            try {
                float val = std::stof(s.substr(0, s.size() - 1));
                len = Length::Percent(val);
            } catch (...) {
                len = Length(0);
            }
        } else {
             len = Length(0);
        }
    }
    
    lua_pop(L, 1);
    return len;
}

void updateCallback(lua_State* L, int tableIdx, const char* key, int& ref) {
  lua_getfield(L, tableIdx, key);
  if (lua_isfunction(L, -1)) {
    if (ref != -2) {
      luaL_unref(L, LUA_REGISTRYINDEX, ref);
    }
    ref = luaL_ref(L, LUA_REGISTRYINDEX);
  } else {
    lua_pop(L, 1);
  }
}

Node* buildNode(lua_State* L, int idx) {
    luaL_checktype(L, idx, LUA_TTABLE);

    Node* n = new Node();

    lua_getfield(L, idx, "type");
    if (lua_isstring(L, -1))
        n->type = lua_tostring(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, idx, "style");
    bool hasStyle = lua_istable(L, -1);

    auto getInt = [&](const char* key, int defaultVal) {
        int val = defaultVal;
        if (hasStyle) {
            lua_getfield(L, -1, key);
            if (lua_isnumber(L, -1))
                val = lua_tointeger(L, -1);
            lua_pop(L, 1);
        }
        return val;
    };

    auto getFloat = [&](const char* key, float defaultVal) {
      float val = defaultVal;
      if (hasStyle) {
        lua_getfield(L, -1, key);
        if (lua_isnumber(L, -1)) val = (float)lua_tonumber(L, -1);
        lua_pop(L, 1);
      }
      return val;
    };

    auto getString = [&](const char* key, std::string defaultVal) {
      std::string val = defaultVal;
      if (hasStyle) {
        lua_getfield(L, -1, key);
        if (lua_isstring(L, -1)) val = lua_tostring(L, -1);
        lua_pop(L, 1);
      }
      return val;
    };

    if (hasStyle) {
        n->widthStyle = getLength(L, "w");
        n->heightStyle = getLength(L, "h");
    }

    n->spacing = getInt("gap", getInt("spacing", 0));

    int p = getInt("padding", 0);
    n->padding       = p;
    n->paddingTop    = getInt("paddingTop", p);
    n->paddingBottom = getInt("paddingBottom", p);
    n->paddingLeft   = getInt("paddingLeft", p);
    n->paddingRight  = getInt("paddingRight", p);

    int m = getInt("margin", 0);
    n->margin       = m;
    n->marginTop    = getInt("marginTop", m);
    n->marginBottom = getInt("marginBottom", m);
    n->marginLeft   = getInt("marginLeft", m);
    n->marginRight  = getInt("marginRight", m);

    n->minHeight = getInt("minHeight", 0);
    n->maxHeight = getInt("maxHeight", 99999);
    n->minWidth = getInt("minWidth", 0);
    n->maxWidth = getInt("maxWidth", 99999);

    n->flexGrow = getFloat("flexGrow", 0.0f);
    n->alignItems = parseAlign(getString("alignItems", "start"));
    n->justifyContent = parseJustify(getString("justifyContent", "start"));

    if (hasStyle) {
        lua_getfield(L, -1, "BGColor");
        if (lua_isstring(L, -1)) {
          const char* hex = lua_tostring(L, -1);
          n->color = parseHexColor(hex);
          n->hasBackground = true;
        }
        else if (lua_istable(L, -1)) {
            lua_rawgeti(L, -1, 1); n->color.r = luaL_optinteger(L, -1, 255); lua_pop(L, 1);
            lua_rawgeti(L, -1, 2); n->color.g = luaL_optinteger(L, -1, 255); lua_pop(L, 1);
            lua_rawgeti(L, -1, 3); n->color.b = luaL_optinteger(L, -1, 255); lua_pop(L, 1);
            lua_rawgeti(L, -1, 4); n->color.a = luaL_optinteger(L, -1, 255); lua_pop(L, 1);

            n->hasBackground = true;
        }
        lua_pop(L, 1);
    }

    lua_pop(L, 1);

    lua_getfield(L, idx, "onClick");
    if (lua_isfunction(L, -1)) {
        n->onClickRef = luaL_ref(L, LUA_REGISTRYINDEX);
    } else {
        lua_pop(L, 1);
    }

    updateCallback(L, idx, "onClick", n->onClickRef);
    lua_getfield(L, idx, "children");
    if (lua_istable(L, -1)) {
        lua_pushnil(L);
        while (lua_next(L, -2) != 0) {
            Node* child = buildNode(L, lua_gettop(L));
            child->parent = n;
            n->children.push_back(child);
            lua_pop(L, 1);
        }
    }
    lua_pop(L, 1);

    return n;
}

void resolveStyles(Node* n, int parentW, int parentH) {
    if (!n) return;

    if (n->widthStyle.value != 0) {
        n->w = n->widthStyle.resolve((float)parentW);
    }

    if (n->heightStyle.value != 0) {
        n->h = n->heightStyle.resolve((float)parentH);
    }

    int contentW = (int)n->w - (n->paddingLeft + n->paddingRight);
    int contentH = (int)n->h - (n->paddingTop + n->paddingBottom);
    
    if (contentW < 0) contentW = 0;
    if (contentH < 0) contentH = 0;

    for (Node* c : n->children) {
        resolveStyles(c, contentW, contentH);
    }
}

void measure(Node* n) {
  if (n->type == "vstack") {
    int totalH = 0;
    int maxW = 0;

    for (Node* c : n->children) {
      measure(c);

      int childH = (int)c->h + c->marginBottom + c->marginTop;
      int childW = (int)c->w + c->marginLeft + c->marginRight;

      totalH += childH + n->spacing;
      maxW = std::max(maxW, childW);
    }

    if (!n->children.empty()) {
      totalH -= n->spacing;
    }

    if (n->w == 0) n->w = maxW + n->paddingLeft + n->paddingRight;
    if (n->h == 0) n->h = totalH + n->paddingTop + n->paddingBottom;
  }
  else if (n->type == "hstack") {
    int totalW = 0;
    int maxH = 0;

    for (Node* c : n->children) {
      measure(c);

      int childH = (int)c->h + c->marginTop + c->marginBottom;
      int childW = (int)c->w + c->marginLeft + c->marginRight;

      totalW += childW + n->spacing;
      maxH = std::max(maxH, childH);
    }

    if (!n->children.empty()) {
      totalW -= n->spacing;
    }

    if (n->w == 0) n->w = totalW + n->paddingRight + n->paddingLeft;
    if (n->h == 0) n->h = maxH + n->paddingTop + n->paddingBottom;
  }
}

void layout(Node* n, int x, int y) {
  n->x = (float)x;
  n->y = (float)y;

  if (n->type == "vstack") {
    int cursor = y + n->paddingTop;

    for (Node* c : n->children) {
      int cx = x + n->paddingLeft + c->marginLeft;
      int cy = cursor + c->marginTop;

      layout(c, cx, cy);
      cursor += (int)c->h + n->spacing + c->marginTop + c->marginBottom;
    }
  }
  else if (n->type == "hstack") {
    int cursor = x + n->paddingLeft; 

    for (Node* c : n->children) {
      int cx = cursor + c->marginLeft;
      int cy = y + n->paddingTop + c->marginTop;

      layout(c, cx, cy);
      cursor += (int)c->w + n->spacing + c->marginRight + c->marginLeft;
    }
  }
}

void renderNode(SDL_Renderer* r, Node* n) {

  SDL_Rect nodeBox = {
    (int)n->x,
    (int)n->y,
    (int)n->w,
    (int)n->h,
  };

  if (n->hasBackground) {
    SDL_SetRenderDrawColor(r, n->color.r, n->color.g, n->color.b, n->color.a);
    SDL_RenderFillRect(r, &nodeBox);
  }

  SDL_Rect oldClip;
  SDL_RenderGetClipRect(r, &oldClip);

  if (!SDL_RenderIsClipEnabled(r)) {
    SDL_RenderGetViewport(r, &oldClip);
  }

  SDL_Rect newClip;
  bool isVisible = SDL_IntersectRect(&oldClip, &nodeBox, &newClip);

  if (isVisible) {
    SDL_RenderSetClipRect(r, &newClip);

    for (Node* c : n->children) {
      renderNode(r, c);
    }
  }

  SDL_RenderSetClipRect(r, &oldClip);
  
}

void freeTree(Node* n) {
  for (Node* c : n->children)
    freeTree(c);
  delete n;
}

int getIntProp(lua_State* L, const char* key, int defaultVal) {
  // assumes stlye in already on top of lua stack
  int val = defaultVal;
  lua_getfield(L, -1, key);
  if (lua_isnumber(L, -1)) {
    val = lua_tointeger(L, -1);
  }
  lua_pop(L, 1);
  return val;
}

float getFloatProp(lua_State* L, const char* key, float defaultVal) {
  // assumes stlye in already on top of lua stack
  float val = defaultVal;
  lua_getfield(L, -1, key);
  if (lua_isnumber(L, -1)) {
    val = (float)lua_tonumber(L, -1);
  }
  lua_pop(L, 1);
  return val;
}

std::string getStringProp(lua_State* L, const char* key, std::string defaultVal) {
  // assumes stlye in already on top of lua stack
  std::string val = defaultVal;
  lua_getfield(L, -1, key);
  if (lua_isstring(L, -1)) {
    val = lua_tostring(L, -1);
  }
  lua_pop(L, 1);
  return val;
}


void patchNode(lua_State* L, Node* n, int idx) {
  lua_getfield(L, idx, "style");
  if (!lua_istable(L, -1)) {
    lua_pop(L, 1);
    return;
  }

  bool layoutChanged = false;
  bool paintChanged = false;

  // comparing % w and h 
  update(n->widthStyle, getLength(L, "w"), layoutChanged);
  update(n->heightStyle, getLength(L, "h"), layoutChanged);
  
  // min max height width
  update(n->minWidth, getFloatProp(L, "minWidth", 0.0f), layoutChanged);
  update(n->maxWidth,  getFloatProp(L, "maxWidth", 99999.0f),  layoutChanged);
  update(n->minHeight, getFloatProp(L, "minHeight", 0.0f),     layoutChanged);
  update(n->maxHeight, getFloatProp(L, "maxHeight", 99999.0f), layoutChanged);

  // flexbox - alignItems - justifyContent
  update(n->flexGrow, getFloatProp(L, "flexGrow", 0.0f), layoutChanged);
  update(n->alignItems, parseAlign(getStringProp(L, "alignItems", "start")), layoutChanged);
  update(n->justifyContent, parseJustify(getStringProp(L, "justifyContent", "start")), layoutChanged);

  // we have support for both gap and spacing
  int gapVal = getIntProp(L, "gap", getIntProp(L, "spacing", 0));
  update(n->spacing, gapVal, layoutChanged);

  // Box Model (margin / padding)
  auto applyBoxModel = [&](const char* base, const char* t, const char* b, const char* l, const char* r, int& vBase, int& vT, int& vB, int& vL, int& vR) {
    int val = getIntProp(L, base, 0);
    update(vBase, val, layoutChanged);
    update(vT, getIntProp(L, t, val), layoutChanged);
    update(vB, getIntProp(L, b, val), layoutChanged);
    update(vL, getIntProp(L, l, val), layoutChanged);
    update(vR, getIntProp(L, r, val), layoutChanged);
  };

  applyBoxModel("padding", "paddingTop", "paddingBottom", "paddingLeft", "paddingRight", n->padding, n->paddingTop, n->paddingBottom, n->paddingLeft, n->paddingRight);
  applyBoxModel("margin", "marginTop", "marginBottom", "marginLeft", "marginRight", n->margin, n->marginTop, n->marginBottom, n->marginLeft, n->marginRight);

  lua_getfield(L, -1, "BGColor");
  if (lua_isstring(L, -1)) {
    const char* hex = lua_tostring(L, -1);
    SDL_Color newCol = parseHexColor(hex);
    update(n->color, newCol, paintChanged);

    if (!n->hasBackground) {
      n->hasBackground = true;
      paintChanged = true;
    }
  } else if (lua_isnil(L, -1)) {
    if (n->hasBackground) {
      n->hasBackground = false;
      paintChanged = true;
    }
  }
  lua_pop(L, 1);

  if (layoutChanged) {
    n->makeLayoutDirty();
  } else if (paintChanged) {
    n->makePaintDirty();
  }

  lua_pop(L, 1);
  updateCallback(L, idx, "onClick", n->onClickRef);
}

void reconcile(lua_State *L, Node *current, int idx) {
  if (!current) return;

  patchNode(L, current, idx);

  lua_getfield(L, idx, "children");
  if (lua_istable(L, -1)) {
    size_t luaChildCount = lua_rawlen(L, -1);

    if (luaChildCount != current->children.size()) {

      //clear old tree
      for (Node* c : current->children) freeTree(c);
      current->children.clear();

      // build new tree
      for (size_t i = 0; i < luaChildCount; ++i) {
        lua_rawgeti(L, -1, i + 1);
        Node* child = buildNode(L, lua_gettop(L));
        child->parent = current;
        current->children.push_back(child);
        lua_pop(L, 1);
      }

      // Recursive reconcile
      current->makeLayoutDirty();
    } else {
      for (size_t i = 0; i < luaChildCount; ++i) {
        lua_rawgeti(L, -1, i + 1);
        reconcile(L, current->children[i], lua_gettop(L));
        lua_pop(L, 1);
      }
    }
  }
  lua_pop(L, 1);
}





