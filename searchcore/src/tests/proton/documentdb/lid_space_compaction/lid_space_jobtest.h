// Copyright Verizon Media. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#include "lid_space_common.h"
#include <vespa/searchcore/proton/server/blockable_maintenance_job.h>
#include <vespa/persistence/spi/bucketexecutor.h>
#include <vespa/vespalib/gtest/gtest.h>

struct JobTestBase : public ::testing::TestWithParam<bool> {
    std::unique_ptr<storage::spi::BucketExecutor> _bucketExecutor;
    std::unique_ptr<MyHandler> _handler;
    MyStorer _storer;
    MyFrozenBucketHandler _frozenHandler;
    test::DiskMemUsageNotifier _diskMemUsageNotifier;
    test::ClusterStateHandler _clusterStateHandler;
    std::unique_ptr<BlockableMaintenanceJob> _job;
    JobTestBase();
    ~JobTestBase() override;
    void init(uint32_t allowedLidBloat,
              double allowedLidBloatFactor,
              double resourceLimitFactor,
              vespalib::duration interval,
              bool nodeRetired,
              uint32_t maxOutstandingMoveOps);
    JobTestBase &addStats(uint32_t docIdLimit,
                          const LidVector &usedLids,
                          const LidPairVector &usedFreePairs);
    JobTestBase &addMultiStats(uint32_t docIdLimit,
                              const std::vector<LidVector> &usedLidsVector,
                              const LidPairVector &usedFreePairs);
    JobTestBase &addStats(uint32_t docIdLimit,
                          uint32_t numDocs,
                          uint32_t lowestFreeLid,
                          uint32_t highestUsedLid);
    bool run() const;
    JobTestBase &endScan();
    JobTestBase &compact();
    void notifyNodeRetired(bool nodeRetired);
    void assertJobContext(uint32_t moveToLid,
                          uint32_t moveFromLid,
                          uint32_t handleMoveCnt,
                          uint32_t wantedLidLimit,
                          uint32_t compactStoreCnt) const;
    void assertNoWorkDone() const;
    JobTestBase &setupOneDocumentToCompact();
    void assertOneDocumentCompacted();
    JobTestBase &setupThreeDocumentsToCompact();
    void sync() const;
    bool useBucketDB() const { return GetParam(); }
};

struct JobTest : public JobTestBase {
    std::unique_ptr<IMaintenanceJobRunner> _jobRunner;

    JobTest();
    ~JobTest();
    void init(uint32_t allowedLidBloat,
              double allowedLidBloatFactor,
              double resourceLimitFactor = RESOURCE_LIMIT_FACTOR,
              vespalib::duration interval = JOB_DELAY,
              bool nodeRetired = false,
              uint32_t maxOutstandingMoveOps = MAX_OUTSTANDING_MOVE_OPS);
    void init_with_interval(vespalib::duration interval);
    void init_with_node_retired(bool retired);
};

class JobDisabledByRemoveOpsTest : public JobTest {
public:
    JobDisabledByRemoveOpsTest();
    ~JobDisabledByRemoveOpsTest();

    void job_is_disabled_while_remove_ops_are_ongoing(bool remove_batch);
    void job_becomes_disabled_if_remove_ops_starts(bool remove_batch);
    void job_is_re_enabled_when_remove_ops_are_no_longer_ongoing(bool remove_batch);
};

class MyCountJobRunner;

struct MaxOutstandingJobTest : public JobTest {
    std::unique_ptr<MyCountJobRunner> runner;
    MaxOutstandingJobTest();
    ~MaxOutstandingJobTest();
    void init(uint32_t maxOutstandingMoveOps);
    void assertRunToBlocked();
    void assertRunToNotBlocked();
    void unblockJob(uint32_t expRunnerCnt);
};
