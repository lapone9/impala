// Copyright (c) 2012 Cloudera, Inc. All rights reserved.

#include "runtime/exec-env.h"

#include <vector>

#include <boost/algorithm/string.hpp>

#include "common/logging.h"
#include "common/service-ids.h"
#include "runtime/client-cache.h"
#include "runtime/data-stream-mgr.h"
#include "runtime/disk-io-mgr.h"
#include "runtime/hbase-table-cache.h"
#include "runtime/hdfs-fs-cache.h"
#include "sparrow/simple-scheduler.h"
#include "sparrow/subscription-manager.h"
#include "util/metrics.h"
#include "util/webserver.h"
#include "util/default-path-handlers.h"
#include "gen-cpp/ImpalaInternalService.h"

using namespace std;
using namespace boost;
using sparrow::Scheduler;
using sparrow::SimpleScheduler;
using sparrow::SubscriptionManager;

DEFINE_string(backends, "", "comma-separated list of <host:port> pairs");
DEFINE_bool(use_statestore, false,
    "Use an external state store for membership information");
DEFINE_bool(enable_webserver, true, "If true, debug webserver is enabled");

namespace impala {

ExecEnv::ExecEnv()
  : stream_mgr_(new DataStreamMgr()),
    subscription_mgr_(new SubscriptionManager()),
    client_cache_(new BackendClientCache(0, 0)),
    fs_cache_(new HdfsFsCache()),
    htable_cache_(new HBaseTableCache()),
    disk_io_mgr_(new DiskIoMgr()),
    webserver_(new Webserver()),
    metrics_(new Metrics()),
    enable_webserver_(FLAGS_enable_webserver),
    tz_database_(TimezoneDatabase()) {
  // Initialize the scheduler either dynamically (with a statestore) or statically (with
  // configured backends)
  if (FLAGS_use_statestore) {
    scheduler_.reset(new SimpleScheduler(subscription_mgr_.get(), IMPALA_SERVICE_ID, 
        metrics_.get()));
  } else if (!FLAGS_backends.empty()) {
    vector<string> backends;
    vector<THostPort> addresses;
    split(backends, FLAGS_backends, is_any_of(","));
    for (int i = 0; i < backends.size(); ++i) {
      int pos = backends[i].find(':');
      if (pos == string::npos) {
        DCHECK(false) << "ignoring backend " << backends[i] << ": missing ':'";
        continue;
      }
      addresses.push_back(THostPort());
      THostPort& addr = addresses.back();
      addr.host = backends[i].substr(0, pos);
      addr.port = atoi(backends[i].substr(pos + 1).c_str());
    }
    scheduler_.reset(new SimpleScheduler(addresses, metrics_.get()));
  } else {
    // No scheduler required if query is not distributed.
    // TODO: Check that num_nodes is correctly set?
    scheduler_.reset(NULL);
  }
  Status status = disk_io_mgr_->Init();
  CHECK(status.ok());
}

ExecEnv::~ExecEnv() {
  if (scheduler_ != NULL) scheduler_->Close();
}

Status ExecEnv::StartServices() {
  LOG(INFO) << "Starting global services";
  // Start services in order to ensure that dependencies between them are met
  if (enable_webserver_) {
    RETURN_IF_ERROR(webserver_->Start());
    AddDefaultPathHandlers(webserver_.get());
  } else {
    LOG(INFO) << "Not starting webserver";
  }

  metrics_->Init(enable_webserver_ ? webserver_.get() : NULL);

  if (FLAGS_use_statestore) RETURN_IF_ERROR(subscription_mgr_->Start());  

  if (scheduler_ != NULL) RETURN_IF_ERROR(scheduler_->Init());


  return Status::OK;
}

}
