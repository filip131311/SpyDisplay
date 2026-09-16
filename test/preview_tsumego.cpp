// Host-side preview of the tsumego board rendering: draws one problem into
// a 960x540 grayscale image and writes it as a binary PGM to stdout.
// Build & run:
//   c++ -std=c++11 -Iinclude test/preview_tsumego.cpp -o /tmp/p
//   /tmp/p 233 > /tmp/p.pgm          # problem #233 from the book
//   /tmp/p --day 20712 > /tmp/p.pgm  # the entry shown on that local day
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "tsumego.h"

// Minimal stand-in for LovyanGFX with the primitives drawTsumegoBoard() uses.
// Colours are RGB565 like the TFT_* macros on the device.
struct HostCanvas {
  int w, h;
  std::vector<unsigned char> px;
  HostCanvas(int w_, int h_) : w(w_), h(h_), px(w_ * h_, 255) {}

  static unsigned char gray(int c565) {
    int r = (c565 >> 11) & 31, g = (c565 >> 5) & 63, b = c565 & 31;
    return (unsigned char)((r * 255 / 31 + g * 255 / 63 + b * 255 / 31) / 3);
  }
  void set(int x, int y, int c) {
    if (x >= 0 && y >= 0 && x < w && y < h) px[y * w + x] = gray(c);
  }
  void fillRect(int x, int y, int rw, int rh, int c) {
    for (int j = y; j < y + rh; ++j)
      for (int i = x; i < x + rw; ++i) set(i, j, c);
  }
  void fillCircle(int cx, int cy, int r, int c) {
    for (int j = -r; j <= r; ++j)
      for (int i = -r; i <= r; ++i)
        if (i * i + j * j <= r * r) set(cx + i, cy + j, c);
  }
  void drawCircle(int cx, int cy, int r, int c) {
    for (int j = -r; j <= r; ++j)
      for (int i = -r; i <= r; ++i) {
        int d = i * i + j * j;
        if (d <= r * r && d > (r - 1) * (r - 1)) set(cx + i, cy + j, c);
      }
  }
};

int main(int argc, char** argv) {
  int idx = 0;
  if (argc == 3 && std::strcmp(argv[1], "--day") == 0) {
    idx = tsumegoIndexForDay(std::atol(argv[2]));
  } else if (argc == 2) {
    int number = std::atoi(argv[1]);
    idx = -1;
    for (int i = 0; i < TSUMEGO_COUNT; ++i)
      if (TSUMEGO_NUMBER[i] == number) idx = i;
    if (idx < 0) {
      std::fprintf(stderr, "no problem #%d\n", number);
      return 1;
    }
  } else {
    std::fprintf(stderr, "usage: %s <problem-number> | --day <days-since-epoch>\n", argv[0]);
    return 2;
  }

  Tsumego p = tsumegoAt(idx);
  TsumegoView v = tsumegoView(p);
  std::fprintf(stderr, "entry %d: problem #%d, %d black, %d white, view %dx%d\n",
               idx, p.number, p.nBlack, p.nWhite, v.cols, v.rows);

  // Same layout as tsumegoRender() in src/main.cpp: header above, footer below
  HostCanvas g(960, 540);
  const int TFT_BLACK = 0x0000, TFT_WHITE = 0xFFFF;
  g.fillRect(0, 0, 960, 540, TFT_WHITE);
  drawTsumegoBoard(g, p, 40, 110, 960 - 80, 540 - 110 - 75, TFT_BLACK, TFT_WHITE);

  std::printf("P5\n%d %d\n255\n", g.w, g.h);
  std::fwrite(g.px.data(), 1, g.px.size(), stdout);
  return 0;
}
