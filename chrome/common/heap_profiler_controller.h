// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_COMMON_HEAP_PROFILER_CONTROLLER_H_
#define CHROME_COMMON_HEAP_PROFILER_CONTROLLER_H_

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/synchronization/atomic_flag.h"

namespace base {
class TaskRunner;
}  // namespace base

// HeapProfilerController controls collection of sampled heap allocation
// snapshots for the current process.
class HeapProfilerController {
 public:
  HeapProfilerController();
  ~HeapProfilerController();

  // Starts periodic heap snapshot collection.
  void Start();

  void SetTaskRunnerForTest(scoped_refptr<base::TaskRunner> task_runner) {
    task_runner_ = std::move(task_runner);
  }

  static bool IsReportingEnabled();

 private:
  using StoppedFlag = base::RefCountedData<base::AtomicFlag>;

  static void ScheduleNextSnapshot(scoped_refptr<base::TaskRunner> task_runner,
                                   scoped_refptr<StoppedFlag> stopped);
  static void TakeSnapshot(scoped_refptr<base::TaskRunner> task_runner,
                           scoped_refptr<StoppedFlag> stopped);
  static void RetrieveAndSendSnapshot();

  scoped_refptr<base::TaskRunner> task_runner_;
  scoped_refptr<StoppedFlag> stopped_;

  DISALLOW_COPY_AND_ASSIGN(HeapProfilerController);
};

#endif  // CHROME_COMMON_HEAP_PROFILER_CONTROLLER_H_
