// Copyright (c) the JPEG XL Project Authors. All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include "tools/speed_stats.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <string>

namespace jpegxl {
namespace tools {

void SpeedStats::NotifyElapsed(double elapsed_seconds) {
  if (elapsed_seconds > 0.0) {
    elapsed_.push_back(elapsed_seconds);
  }
}

bool SpeedStats::GetSummary(SpeedStats::Summary* s) {
  if (elapsed_.empty()) return false;

  s->min = *std::min_element(elapsed_.begin(), elapsed_.end());
  s->max = *std::max_element(elapsed_.begin(), elapsed_.end());

  // Single rep
  if (elapsed_.size() == 1) {
    s->central_tendency = elapsed_[0];
    s->variability = 0.0;
    s->type = "";
    return true;
  }

  // Two: skip first (noisier)
  if (elapsed_.size() == 2) {
    s->central_tendency = elapsed_[1];
    s->variability = 0.0;
    s->type = "second: ";
    return true;
  }

  // Prefer geomean unless numerically unreliable (too many reps)
  if (pow(elapsed_[0], elapsed_.size()) < 1E100) {
    double product = 1.0;
    for (size_t i = 1; i < elapsed_.size(); ++i) {
      product *= elapsed_[i];
    }

    s->central_tendency = pow(product, 1.0 / (elapsed_.size() - 1));
    s->variability = 0.0;
    s->type = "geomean: ";
    if (std::isnormal(s->central_tendency)) return true;
  }

  // Else: median
  std::sort(elapsed_.begin(), elapsed_.end());
  s->central_tendency = elapsed_[elapsed_.size() / 2];
  double stdev = 0;
  for (double t : elapsed_) {
    double diff = t - s->central_tendency;
    stdev += diff * diff;
  }
  s->variability = sqrt(stdev);
  s->type = "median: ";
  return true;
}

namespace {

std::string SummaryStat(double value, const char* unit,
                        const SpeedStats::Summary& s,
                        const std::string& prefix) {
  if (value == 0.) return "";

  char stat_str[100] = {'\0'};
  const double value_tendency = value / s.central_tendency;
  // Note flipped order: higher elapsed = lower mpps.
  const double value_min = value / s.max;
  const double value_max = value / s.min;

  char variability[20] = {'\0'};
  if (s.variability != 0.0) {
    const double stdev = value / s.variability;
    snprintf(variability, sizeof(variability), " (stdev %.3f)", stdev);
  }

  char range[20] = {'\0'};
  if (s.min != s.max) {
    snprintf(range, sizeof(range), " [%.3f, %.3f]", value_min, value_max);
  }

  snprintf(stat_str, sizeof(stat_str), "%s%.3f %s/s%s%s", s.type,
           value_tendency, unit, range, variability);
  return prefix + stat_str;
}

}  // namespace

bool SpeedStats::Print(size_t worker_threads) {
  Summary s;
  if (!GetSummary(&s)) {
    return false;
  }
  std::string mps_stats = SummaryStat(xsize_ * ysize_ * 1e-6, "MP", s, ", ");
  std::string mbs_stats = SummaryStat(file_size_ * 1e-6, "MB", s, ", ");
  size_t reps = elapsed_.size();
  std::string reps_str;
  if (reps > 1) {
    reps_str = ", " + std::to_string(reps) + " reps";
  }

  fprintf(stderr, "%d x %d%s%s%s, %d threads.\n", static_cast<int>(xsize_),
          static_cast<int>(ysize_), mps_stats.c_str(), mbs_stats.c_str(),
          reps_str.c_str(), static_cast<int>(worker_threads));
  return true;
}

}  // namespace tools
}  // namespace jpegxl
