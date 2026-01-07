#include "opengl_renderer.h"
#include "commands.h"
#include <cstddef>
#include <iostream>
#include <vector>

const char* vertexSource = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec4 aColor;

out vec4 fColor;
uniform mat4 projection;

void main() {
  gl_Position = projection * vec4(aPos, 0.0, 1.0);
  fColor = aColor;
}
)";

const char* fragmentSource = R"(
  #version 330 core
  out vec4 FragColor;
  in vec4 fColor;

  void main() {
    FragColor = fColor;
  }
)";

OpenGLRenderer::OpenGLRenderer(SDL_Window* win) : window(win) {
  context = SDL_GL_CreateContext(window);
  if (!context) {
    std::cerr << "Failed to create OpenGl context" << SDL_GetError() << std::endl;
  }

  if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
    std::cerr << "Failed to initialize GLAD" << std::endl;
  }

  std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;

  SDL_GL_SetSwapInterval(1);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  initShaders();
  initBuffers();
}

OpenGLRenderer::~OpenGLRenderer() {
  glDeleteVertexArrays(1, &vao);
  glDeleteBuffers(1, &vbo);
  glDeleteProgram(shaderProgram);
  SDL_GL_DeleteContext(context);
}

void OpenGLRenderer::initShaders() {
  // Compile Vertex Shader
  GLuint vs = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vs, 1, &vertexSource, NULL);
  glCompileShader(vs);

  // Compile Fragment Shader
  GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fs, 1, &fragmentSource, NULL);
  glCompileShader(fs);

  shaderProgram = glCreateProgram();
  glAttachShader(shaderProgram, vs);
  glAttachShader(shaderProgram, fs);
  glLinkProgram(shaderProgram);

  glDeleteShader(fs);
  glDeleteShader(vs);
}

void OpenGLRenderer::initBuffers() {
  glGenVertexArrays(1, &vao);
  glGenBuffers(1, &vbo);

  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE ,sizeof(Vertex), (void*)offsetof(Vertex, x));

  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex), (void*)offsetof(Vertex, color));

  glBindVertexArray(0);
}

void OpenGLRenderer::beginFrame() {
  SDL_GetWindowSize(window, &winWidth, &winHeight);
  int drawableW, drawableH;
  SDL_GL_GetDrawableSize(window, &drawableW, &drawableH);
  glViewport(0, 0, drawableW, drawableH);

  glClearColor(0.12f, 0.12f, 0.12f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  glUseProgram(shaderProgram);

  float L = 0.0f, R = (float)winWidth;
  float B = (float)winHeight, T = 0.0f;

  float ortho[16] = {
    2.0f/(R-L),   0.0f,         0.0f,  0.0f,
    0.0f,         2.0f/(T-B),   0.0f,  0.0f,
    0.0f,         0.0f,        -1.0f,  0.0f,
    -(R+L)/(R-L), -(T+B)/(T-B), 0.0f,  1.0f
  };

  GLint projLoc = glGetUniformLocation(shaderProgram, "projection");
  glUniformMatrix4fv(projLoc, 1, GL_FALSE, ortho);

  vertices.clear();
}

void OpenGLRenderer::endFrame() {
  flush();
  SDL_GL_SwapWindow(window);
}

void OpenGLRenderer::flush() {
  if (vertices.empty()) return;
  
  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);

  glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_DYNAMIC_DRAW);

  glDrawArrays(GL_TRIANGLES, 0, vertices.size());

  glBindVertexArray(0);
  vertices.clear();
}

void OpenGLRenderer::submit(const RenderCommandList& list) {
  for (const auto& cmd : list.commands) {
    if (std::holds_alternative<DrawRectCommand>(cmd)) {
      const auto& data = std::get<DrawRectCommand>(cmd);
      float x = data.rect.x;
      float y = data.rect.y;
      float w = data.rect.w;
      float h = data.rect.h;
      Color c = data.color;

      // triangle 1
      vertices.push_back({x, y, c});
      vertices.push_back({x + w, y, c});
      vertices.push_back({x + w, y + h, c});

      // triangle 2
      vertices.push_back({x, y, c});
      vertices.push_back({x, y + h, c});
      vertices.push_back({x + w, y + h, c});
    }

    else if (std::holds_alternative<PushClipCommand>(cmd)) {
      flush();
      const auto& data = std::get<PushClipCommand>(cmd);

      int drawW, drawH;
      SDL_GL_GetDrawableSize(window, &drawW, &drawH);

      float scaleX = (float)drawW / winWidth;
      float scaleY = (float)drawH / winHeight;

      int scissorX = (float)(data.rect.x * scaleX);
      int scissorY = (float)(drawH - (data.rect.y + data.rect.h) * scaleY);
      int scissorW = (int)(data.rect.w * scaleX);
      int scissorH = (int)(data.rect.h * scaleY);

      glEnable(GL_SCISSOR_TEST);
      glScissor(scissorX, scissorY, scissorW, scissorH);
    }

    else if (std::holds_alternative<PopClipCommand>(cmd)) {
      flush();
      glDisable(GL_SCISSOR_TEST);
    }
  }
}
