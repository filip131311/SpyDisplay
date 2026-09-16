// Host-side unit test for the pure helpers in include/timeutil.h and the
// tsumego table/selection in include/tsumego.h.
// Build & run:  c++ -std=c++11 -Iinclude test/host_test.cpp -o /tmp/t && /tmp/t
#include <cstdio>
#include <cstdlib>
#include "timeutil.h"
#include "tsumego.h"

static int fails = 0;
#define CHECK(cond)                                                   \
  do {                                                                \
    if (!(cond)) {                                                    \
      std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);     \
      ++fails;                                                        \
    }                                                                 \
  } while (0)

int main() {
  const long TZ = 7200;  // CEST
  // Known anchors
  CHECK(daysFromCivil(1970, 1, 1) == 0);
  CHECK(daysFromCivil(2000, 3, 1) == 11017);
  CHECK(daysFromCivil(2026, 10, 1) == 20727);

  // 2026-08-21 10:00 UTC = 1787392800 ... compute from daysFromCivil instead
  time_t aug21_10utc = (time_t)daysFromCivil(2026, 8, 21) * 86400 + 10 * 3600;
  CHECK(daysUntil(aug21_10utc, TZ, 2026, 10, 1) == 41);

  // Local midnight boundary: 2026-09-30 21:59:59 UTC is 23:59:59 CEST -> 1 day
  time_t sep30_2159 = (time_t)daysFromCivil(2026, 9, 30) * 86400 + 21 * 3600 + 59 * 60 + 59;
  CHECK(daysUntil(sep30_2159, TZ, 2026, 10, 1) == 1);
  // one second later it is Oct 1st locally -> 0
  CHECK(daysUntil(sep30_2159 + 1, TZ, 2026, 10, 1) == 0);
  // the day after -> -1
  CHECK(daysUntil(sep30_2159 + 1 + 86400, TZ, 2026, 10, 1) == -1);

  // secondsToLocalMidnight
  CHECK(secondsToLocalMidnight(sep30_2159, TZ) == 1);
  CHECK(secondsToLocalMidnight(sep30_2159 + 1, TZ) == 86400);
  CHECK(secondsToLocalMidnight(aug21_10utc, TZ) == 12 * 3600);

  // wrapIndex cycles both ways
  CHECK(wrapIndex(2, 2) == 0);
  CHECK(wrapIndex(-1, 2) == 1);
  CHECK(wrapIndex(1, 2) == 1);

  // --- tsumego table -------------------------------------------------------
  CHECK(TSUMEGO_COUNT == 900);
  CHECK(TSUMEGO_OFFSET[0] == 0);
  CHECK(TSUMEGO_OFFSET[TSUMEGO_COUNT] == sizeof(TSUMEGO_DATA));
  {
    // every book number 1..900 appears exactly once, in a shuffled order
    static int seen[TSUMEGO_COUNT + 1] = {0};
    bool identity = true;
    for (int i = 0; i < TSUMEGO_COUNT; ++i) {
      int n = TSUMEGO_NUMBER[i];
      CHECK(n >= 1 && n <= TSUMEGO_COUNT);
      if (n >= 1 && n <= TSUMEGO_COUNT) ++seen[n];
      if (n != i + 1) identity = false;
    }
    for (int n = 1; n <= TSUMEGO_COUNT; ++n) CHECK(seen[n] == 1);
    CHECK(!identity);
  }
  for (int i = 0; i < TSUMEGO_COUNT; ++i) {
    Tsumego p = tsumegoAt(i);
    int n = tsumegoStoneCount(p);
    CHECK(n >= 8 && n <= 60);
    CHECK(TSUMEGO_OFFSET[i] + 2 + n == TSUMEGO_OFFSET[i + 1]);
    for (int k = 0; k < n; ++k) {
      TsumegoStone s = tsumegoStone(p, k);
      CHECK(s.col < 19 && s.row < 19);
      for (int j = 0; j < k; ++j) {  // no two stones on one point
        TsumegoStone t = tsumegoStone(p, j);
        CHECK(!(s.col == t.col && s.row == t.row));
      }
    }
    TsumegoView v = tsumegoView(p);
    CHECK(v.cols >= 4 && v.cols <= 19 && v.rows >= 4 && v.rows <= 19);
  }
  // Book problem 1: B eb fb bc cc dc be / W da ab bb cb db -> max col 5, max row 4
  for (int i = 0; i < TSUMEGO_COUNT; ++i) {
    if (TSUMEGO_NUMBER[i] != 1) continue;
    Tsumego p = tsumegoAt(i);
    CHECK(p.nBlack == 6 && p.nWhite == 5);
    CHECK(tsumegoStone(p, 0).col == 4 && tsumegoStone(p, 0).row == 1);  // eb
    CHECK(tsumegoStone(p, 6).col == 3 && tsumegoStone(p, 6).row == 0);  // da
    CHECK(tsumegoStoneIsBlack(p, 5) && !tsumegoStoneIsBlack(p, 6));
    CHECK(tsumegoOccupied(p, 1, 4) && !tsumegoOccupied(p, 0, 0));
    TsumegoView v = tsumegoView(p);
    CHECK(v.cols == 8 && v.rows == 7);
  }

  // one entry per day, cycling through the whole table without repeats
  CHECK(tsumegoIndexForDay(0) == 0);
  CHECK(tsumegoIndexForDay(899) == 899);
  CHECK(tsumegoIndexForDay(900) == 0);
  CHECK(tsumegoIndexForDay(-1) == 899);
  CHECK(tsumegoIndexForDay(daysFromCivil(2026, 9, 16)) != tsumegoIndexForDay(daysFromCivil(2026, 9, 17)));
  {
    long day = daysFromCivil(2026, 9, 16);
    static int hits[TSUMEGO_COUNT] = {0};
    for (int d = 0; d < TSUMEGO_COUNT; ++d) ++hits[tsumegoIndexForDay(day + d)];
    for (int i = 0; i < TSUMEGO_COUNT; ++i) CHECK(hits[i] == 1);
  }

  if (fails) { std::printf("%d failure(s)\n", fails); return 1; }
  std::printf("all host tests passed\n");
  return 0;
}
