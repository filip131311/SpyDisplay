// Tsumego-of-the-day helpers: which problem to show and how to draw it.
// Pure C++ with no Arduino dependencies so the selection can be unit-tested
// and the board rendering previewed on the host (test/preview_tsumego.cpp).
#pragma once

#include <stdint.h>

#include "tsumego_data.h"

struct Tsumego {
  uint16_t number;        // problem number in the book
  uint8_t nBlack, nWhite;
  const uint8_t* stones;  // nBlack black then nWhite white, (row << 4) | col
};

struct TsumegoStone {
  uint8_t col, row;  // 0 = left / top edge
};

static inline Tsumego tsumegoAt(int idx) {
  Tsumego p;
  const uint8_t* chunk = TSUMEGO_DATA + TSUMEGO_OFFSET[idx];
  p.number = TSUMEGO_NUMBER[idx];
  p.nBlack = chunk[0];
  p.nWhite = chunk[1];
  p.stones = chunk + 2;
  return p;
}

static inline int tsumegoStoneCount(const Tsumego& p) {
  return p.nBlack + p.nWhite;
}

static inline TsumegoStone tsumegoStone(const Tsumego& p, int k) {
  TsumegoStone s;
  s.col = (uint8_t)(p.stones[k] & 15);
  s.row = (uint8_t)(p.stones[k] >> 4);
  return s;
}

static inline bool tsumegoStoneIsBlack(const Tsumego& p, int k) {
  return k < p.nBlack;
}

// Entry shown on a given local day (days since the epoch): walks the
// pre-shuffled table, so every entry appears once per TSUMEGO_COUNT days.
static inline int tsumegoIndexForDay(long day) {
  long i = day % TSUMEGO_COUNT;
  if (i < 0) i += TSUMEGO_COUNT;
  return (int)i;
}

// Visible part of the board: the top-left corner, two lines beyond the
// furthest stone in each direction (clamped to the 19x19 board).
struct TsumegoView {
  int cols, rows;
};

static inline TsumegoView tsumegoView(const Tsumego& p) {
  int maxCol = 0, maxRow = 0;
  for (int k = 0; k < tsumegoStoneCount(p); ++k) {
    TsumegoStone s = tsumegoStone(p, k);
    if (s.col > maxCol) maxCol = s.col;
    if (s.row > maxRow) maxRow = s.row;
  }
  TsumegoView v;
  v.cols = maxCol + 3 < 19 ? maxCol + 3 : 19;
  v.rows = maxRow + 3 < 19 ? maxRow + 3 : 19;
  return v;
}

static inline int tsumegoMin(int a, int b) { return a < b ? a : b; }

static inline bool tsumegoOccupied(const Tsumego& p, int col, int row) {
  for (int k = 0; k < tsumegoStoneCount(p); ++k) {
    TsumegoStone s = tsumegoStone(p, k);
    if (s.col == col && s.row == row) return true;
  }
  return false;
}

// Draws the cropped corner centred in the rectangle (x, y, w, h). Uses only
// fillRect / fillCircle / drawCircle so G can be a LovyanGFX canvas or the
// host stub. Colours are passed through untouched (use TFT_BLACK/TFT_WHITE
// on the device). The pitch (distance between lines) is capped at maxPitch.
template <class G>
static void drawTsumegoBoard(G& g, const Tsumego& p, int x, int y, int w,
                             int h, int black, int white, int maxPitch = 64) {
  TsumegoView v = tsumegoView(p);
  int pitch = tsumegoMin(tsumegoMin(w / v.cols, h / v.rows), maxPitch);
  if (pitch < 8) pitch = 8;

  // Board area: half a pitch of margin outside the top/left edge lines (so
  // edge stones are not clipped); on the cut sides the lines run half a
  // pitch past the last intersection to show the board continues.
  const int bw = v.cols * pitch, bh = v.rows * pitch;
  const int x0 = x + (w - bw) / 2, y0 = y + (h - bh) / 2;
  const int half = pitch / 2;
  const int thin = pitch >= 40 ? 2 : 1, thick = thin * 2;
  const bool rightEdge = (v.cols == 19), bottomEdge = (v.rows == 19);
  const int xEnd = rightEdge ? x0 + half + (v.cols - 1) * pitch : x0 + bw;
  const int yEnd = bottomEdge ? y0 + half + (v.rows - 1) * pitch : y0 + bh;

  auto ix = [&](int col) { return x0 + half + col * pitch; };
  auto iy = [&](int row) { return y0 + half + row * pitch; };

  for (int r = 0; r < v.rows; ++r) {
    bool edge = (r == 0) || (bottomEdge && r == v.rows - 1);
    int t = edge ? thick : thin;
    g.fillRect(ix(0) - t / 2, iy(r) - t / 2, xEnd - ix(0) + t / 2, t, black);
  }
  for (int c = 0; c < v.cols; ++c) {
    bool edge = (c == 0) || (rightEdge && c == v.cols - 1);
    int t = edge ? thick : thin;
    g.fillRect(ix(c) - t / 2, iy(0) - t / 2, t, yEnd - iy(0) + t / 2, black);
  }

  // Star points (hoshi) that fall inside the crop and are not covered
  static const int hoshi[] = {3, 9, 15};
  for (int a = 0; a < 3; ++a)
    for (int b = 0; b < 3; ++b) {
      int c = hoshi[a], r = hoshi[b];
      if (c < v.cols && r < v.rows && !tsumegoOccupied(p, c, r))
        g.fillCircle(ix(c), iy(r), pitch / 12 + 1, black);
    }

  const int rStone = (pitch * 47) / 100;
  for (int k = 0; k < tsumegoStoneCount(p); ++k) {
    TsumegoStone s = tsumegoStone(p, k);
    int cx = ix(s.col), cy = iy(s.row);
    if (tsumegoStoneIsBlack(p, k)) {
      g.fillCircle(cx, cy, rStone, black);
    } else {
      g.fillCircle(cx, cy, rStone, white);
      for (int t = 0; t < thin; ++t) g.drawCircle(cx, cy, rStone - t, black);
    }
  }
}
