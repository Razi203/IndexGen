#pragma once
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <numeric>
#include <string>
#include <vector>

// -------------------- Reusable precompute --------------------
struct PeqIndex {
  int m = 0;                   // pattern length
  int B = 0;                   // number of 64-bit blocks
  std::vector<uint64_t> table; // flat: 256 * B

  PeqIndex() = default;
  explicit PeqIndex(const std::string &pattern) { build(pattern); }

  void build(const std::string &pattern) {
    m = (int)pattern.size();
    B = (m + 63) / 64;
    table.assign(256ull * (size_t)std::max(1, B), 0ULL);
    for (int i = 0; i < m; ++i) {
      unsigned c = (unsigned char)pattern[i];
      table[c * (size_t)B + (i >> 6)] |= (1ULL << (i & 63));
    }
  }
  inline const uint64_t *blocks(unsigned char c) const {
    return &table[(size_t)c * B];
  }
};

// -------------------- Pattern handle (API-level struct) --------------------
struct PatternHandle {
  std::string pattern;
  PeqIndex peq;
  int m = 0;                 // == peq.m
  int B = 0;                 // == peq.B
  uint64_t lastMask = ~0ULL; // mask for last block (exact Myers)
  uint64_t highestBit = 0;   // 1ULL << ((m-1) & 63)
  int highestShift = 0;      // ((m-1) & 63) — for branchless bit extraction

  PatternHandle() = default;
  explicit PatternHandle(const std::string &p) { build(p); }

  void build(const std::string &p) {
    pattern = p;
    peq.build(p);
    m = peq.m;
    B = peq.B;
    if (m == 0) {
      lastMask = ~0ULL;
      highestBit = 0;
      highestShift = 0;
      return;
    }
    const size_t last = (size_t)B - 1;
    const size_t r = (size_t)m - last * 64;
    lastMask = (r == 64) ? ~0ULL : ((1ULL << r) - 1ULL);
    highestShift = ((m - 1) & 63);
    highestBit = (1ULL << highestShift);
  }
};

// -------------------- Exact Myers single word (m <= 64) --------------------
inline int MyersSingleWord(const PatternHandle &H, const std::string &text) {
  const size_t m = (size_t)H.m;
  if (m == 0)
    return (int)text.size();
  assert(H.B == 1 && "MyersSingleWord expects m <= 64");

  uint64_t PV = ~0ULL, MV = 0ULL;
  int score = (int)m;

  for (unsigned char tc : text) {
    uint64_t Eq = H.peq.blocks(tc)[0];

    uint64_t X = Eq | MV;
    uint64_t D0 = ((((X & PV) + PV) ^ PV) | X);
    uint64_t HN = PV & D0;
    uint64_t HP = MV | ~(PV | D0);

    uint64_t X2 = (HP << 1) | 1ULL;
    MV = X2 & D0;
    PV = (HN << 1) | ~(X2 | D0);

    // branchless top-bit update
    score +=
        int((HP >> H.highestShift) & 1U) - int((HN >> H.highestShift) & 1U);
  }
  return score;
}

// -------------------- Exact Myers multi word (any m) --------------------
inline int MyersMultiWord(const PatternHandle &H, const std::string &text) {
  const size_t m = (size_t)H.m;
  if (!m)
    return (int)text.size();
  const size_t B = (size_t)H.B;
  const size_t last = B - 1;

  std::vector<uint64_t> PV(B, ~0ULL), MV(B, 0ULL);
  PV[last] &= H.lastMask;

  int score = (int)m;

  for (unsigned char c : text) {
    const uint64_t *EqBase = H.peq.blocks(c);
    uint64_t lastHP = 0, lastHN = 0;
    unsigned char carry = 0, c1 = 1, c2 = 0;

    for (size_t b = 0; b < B; ++b) {
      const uint64_t Eq = EqBase[b];
      const uint64_t X = Eq | MV[b];

      // portable add-with-carry: u = (X & PV) + PV + carry
      uint64_t tmp = (X & PV[b]);
      uint64_t sum = tmp + PV[b];
      bool cA = (sum < tmp);
      uint64_t u = sum + (uint64_t)carry;
      bool cB = (u < sum);
      carry = (unsigned char)(cA || cB);

      const uint64_t D0 = (u ^ PV[b]) | X;
      const uint64_t HN = PV[b] & D0;
      const uint64_t HP = MV[b] | ~(PV[b] | D0);
      lastHP = HP;
      lastHN = HN;

      const uint64_t X2 = (HP << 1) | c1;
      c1 = (unsigned char)(HP >> 63);
      const uint64_t HNs = (HN << 1) | c2;
      c2 = (unsigned char)(HN >> 63);

      MV[b] = X2 & D0;
      PV[b] = HNs | ~(X2 | D0);
    }
    PV[last] &= H.lastMask;
    MV[last] &= H.lastMask;

    // branchless top-bit update
    score += int((lastHP >> H.highestShift) & 1U) -
             int((lastHN >> H.highestShift) & 1U);
  }
  return score;
}

// -------------------- Compose Eq for Hyyrö/GKR band from PeqIndex
// --------------------
inline uint64_t ComposeEqBand(const PeqIndex &P, unsigned char tch, int /*m*/,
                              int j, int c) {
  // MSB (bit 63) corresponds to unclamped i_top = j + c
  const int i_top = j + c;
  const int bTop = i_top >> 6; // may be outside [0..B-1]
  const int off = i_top & 63;  // 0..63
  const int B = P.B;
  const uint64_t *base = P.blocks(tch);

  uint64_t Eq = 0ULL;
  if ((unsigned)bTop < (unsigned)B) {
    uint64_t topMask =
        base[bTop] & ((off == 63) ? ~0ULL : ((1ULL << (off + 1)) - 1ULL));
    Eq |= (topMask << (63 - off));
  }
  if (off < 63) {
    const int need = 63 - off;
    const int bPrev = bTop - 1;
    if ((unsigned)bPrev < (unsigned)B) {
      uint64_t prevMask = base[bPrev] & (~0ULL << (64 - need));
      Eq |= (prevMask >> (off + 1));
    }
  }
  return Eq;
}

// -------------------- GKR/Hyyrö single-word band (lv ≤ 64) with early-exit
// -------------------- Gropl, Klau, Reinert and Hyrro Algorithm for banded edit
// distance using bit-parllelism + Ukkonen's band
inline int GKRHyyroSingleWordBand(const std::string &text,
                                  const PatternHandle &H, int k) {
  using Bit = uint64_t;

  const int m = H.m;
  const int n = (int)text.size();

  if (m == 0)
    return (n <= k ? n : k + 1);
  if (n == 0)
    return (m <= k ? m : k + 1);

  // length-diff feasibility
  if (k < abs(n - m))
    return k + 1;

  // exact fallback at/above min(m,n)
  if (k >= std::min(m, n)) {
    int d = (m <= 64) ? MyersSingleWord(H, text) : MyersMultiWord(H, text);
    return (d <= k) ? d : (k + 1);
  }

  // band params; if band > 64 fallback to exact computation
  const int c = (k - n + m) / 2;
  const int lv = std::min(m, ((k - n + m) / 2) + ((k + n - m) / 2) + 1);
  assert(lv > 0);
  if (lv > 64 || lv == m) {
    int d = (H.m <= 64) ? MyersSingleWord(H, text) : MyersMultiWord(H, text);
    return (d <= k) ? d : (k + 1);
  }

  // VP = 1^(c+1) 0^(64-(c+1))  (top c+1 bits set)
  Bit VP = ~Bit(0);
  VP >>= (64 - (c + 1));
  VP <<= (64 - (c + 1));

  Bit VN = 0;
  int score = c;

  // late-phase boundary and moving index for late phase
  const int late_start_j = (m - c); // enter late when j >= late_start_j
  int s = 62;                       // HP/HN bit index during late phase

  // constant tail of late columns if we're still in early phase
  const int late_tail_full = std::max(0, n - late_start_j);

  for (int j = 0; j < n; ++j) {
    Bit Eq = ComposeEqBand(H.peq, (unsigned char)text[j], m, j, c);

    Bit X = Eq | VN;
    Bit D0 = (((X & VP) + VP) ^ VP) | X;

    Bit HN = VP & D0;
    Bit HP = VN | ~(D0 | VP);

    X = (D0 >> 1);
    VN = X & HP;
    VP = HN | ~(X | HP);

    if (j < late_start_j) {
      // early phase: score += 1 - msb(D0)
      score += 1 - int((D0 >> 63) & 1ULL);
    } else {
      // late phase: adjust at moving index s, then decrement s
      score += int((HP >> s) & 1ULL);
      score -= int((HN >> s) & 1ULL);
      --s;
    }

    // ------- EARLY EXIT -------
    // Remaining columns from next step:
    // - if still in early phase: remaining_late = n - late_start_j
    // - otherwise: all remaining columns are late = n - (j + 1)
    int remLate = (j + 1 < late_start_j) ? late_tail_full : (n - (j + 1));
    if (score - remLate > k)
      return k + 1;
  }

  return (score <= k ? score : k + 1);
}

// // Reference edit distance
// inline int FastEditDistance(const std::string &source, const std::string
// &target)
// {
//     if (source.size() > target.size())
//     {
//         return FastEditDistance(target, source);
//     }

//     const int min_size = source.size(), max_size = target.size();
//     std::vector<int> lev_dist(min_size + 1);

//     for (int i = 0; i <= min_size; ++i)
//     {
//         lev_dist[i] = i;
//     }

//     for (int j = 1; j <= max_size; ++j)
//     {
//         int previous_diagonal = lev_dist[0], previous_diagonal_save;
//         ++lev_dist[0];

//         for (int i = 1; i <= min_size; ++i)
//         {
//             previous_diagonal_save = lev_dist[i];
//             if (source[i - 1] == target[j - 1])
//             {
//                 lev_dist[i] = previous_diagonal;
//             }
//             else
//             {
//                 lev_dist[i] = std::min(std::min(lev_dist[i - 1],
//                 lev_dist[i]), previous_diagonal) + 1;
//             }
//             previous_diagonal = previous_diagonal_save;
//         }
//     }

//     return lev_dist[min_size];
// }

// -------------------- Convenience wrappers --------------------
inline PatternHandle MakePattern(const std::string &p) {
  return PatternHandle(p);
}

// Thresholded/banded API (primary), reuses handle. Use for repeated
// computations with same pattern.
inline int EditDistanceBanded(const std::string &text, const PatternHandle &H,
                              int k) {
  return GKRHyyroSingleWordBand(text, H, k);
}

// One-shot helpers (build handle on the fly)
inline int EditDistanceBanded(const std::string &a, const std::string &b,
                              int k) {
  const std::string *pat = &a, *txt = &b;
  if (a.size() > b.size()) {
    pat = &b;
    txt = &a;
  }
  PatternHandle H(*pat);
  return GKRHyyroSingleWordBand(*txt, H, k);
}
// Exact distance using same handle (no banding). Use for repeated computations
// with same pattern.
inline int EditDistanceExact(const std::string &text, const PatternHandle &H) {
  return (H.m <= 64) ? MyersSingleWord(H, text) : MyersMultiWord(H, text);
}

inline int EditDistanceExact(const std::string &a, const std::string &b) {
  const std::string *pat = &a, *txt = &b;
  if (a.size() > b.size()) {
    pat = &b;
    txt = &a;
  }
  PatternHandle H(*pat);
  return (H.m <= 64) ? MyersSingleWord(H, *txt) : MyersMultiWord(H, *txt);
}

inline bool EditDistanceExactAtLeast(const std::string &text,
                                     const PatternHandle &H, int minED) {
  int ed = EditDistanceExact(text, H);
  return ed >= minED;
}

inline bool EditDistanceBandedAtLeast(const std::string &text,
                                      const PatternHandle &H, int minED) {
  int ed = EditDistanceBanded(text, H, minED - 1); // compute up to minED - 1
  return ed >= minED;
}