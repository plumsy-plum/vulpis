#pragma once
#include "../ui/ui.h"

namespace Layout {
  void measure(Node* n, bool isRoot = false, int windowWidth = 800, int windowHeight = 600);
  void compute(Node* n, int x, int y);
}
