#pragma once
#include <string>
#include <map>
#include <glad/glad.h>

struct Character {
  unsigned int TextureID;
  int SizeX, SizeY;
  int BearingX, BearingY;
  unsigned int Advance;
  float uMin, vMin;
  float uMax, vMax;
};

class Font {
  public:
    Font(const std::string& fontPath, unsigned int fontSize);
    ~Font();

    const Character& GetCharacter(char c) const;
    unsigned int GetTextureID() const { return textureID; }

  private:
    unsigned int textureID;
    std::map<char, Character> characters;
    void Load(const std::string& path, unsigned int size);
};

