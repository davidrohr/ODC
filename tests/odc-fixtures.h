/********************************************************************************
 * Copyright (C) 2019-2021 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef __ODC__odc_core_lib_fixtures
#define __ODC__odc_core_lib_fixtures

#include <odc/Logger.h>
#include <odc/MiscUtils.h>
#include <odc/Semaphore.h>
#include <odc/TopologyDefs.h>

#include <dds/Tools.h>
#include <dds/Topology.h>

#include <boost/asio/io_context.hpp>
#include <boost/filesystem.hpp>
#include <boost/test/unit_test_log.hpp>

#include <map>
#include <memory>
#include <string>
#include <thread>
#include <unordered_set>

struct AsyncOpFixture
{
    boost::asio::io_context mIoContext;
};

struct TopologyFixture
{
    TopologyFixture(std::string topoXMLPath)
        : mDDSTopo(std::move(topoXMLPath))
    {
        using namespace dds::tools_api;
        using namespace odc::core;

        mDDSSession.create();

        Logger::Config logConfig;
        logConfig.mSeverity = ESeverity::debug;
        std::stringstream ss;
        ss << mDDSSession.getSessionID();
        const boost::filesystem::path p{ boost::filesystem::temp_directory_path() / ss.str() };
        logConfig.mLogDir = p.string();

        try {
            Logger::instance().init(logConfig);
        } catch (std::exception& e) {
            std::cerr << "Can't initialize log: " << e.what() << std::endl;
        }

        SSubmitRequestData submitInfo;
        submitInfo.m_rms = "localhost";
        submitInfo.m_instances = 1;
        submitInfo.m_slots = mSlots;
        submitInfo.m_config = "";

        SharedSemaphore blocker;
        auto submitRequest = SSubmitRequest::makeRequest(submitInfo);
        submitRequest->setMessageCallback([](const SMessageResponseData& message) { BOOST_TEST_MESSAGE(message.m_msg); });
        submitRequest->setDoneCallback([blocker]() mutable { blocker.Signal(); });
        mDDSSession.sendRequest<SSubmitRequest>(submitRequest);
        blocker.Wait();

        std::size_t idleSlotsCount(0);
        int interval(8);
        while (idleSlotsCount < mSlots) {
            std::this_thread::sleep_for(std::chrono::milliseconds(interval));
            interval = std::min(256, interval * 2);
            SAgentCountRequest::response_t res;
            mDDSSession.syncSendRequest<SAgentCountRequest>(SAgentCountRequest::request_t(), res);
            idleSlotsCount = res.m_idleSlotsCount;
        }

        STopologyRequestData topologyInfo;
        topologyInfo.m_updateType = STopologyRequestData::EUpdateType::ACTIVATE;
        topologyInfo.m_topologyFile = mDDSTopo.getFilepath();

        auto topologyRequest = STopologyRequest::makeRequest(topologyInfo);
        topologyRequest->setMessageCallback([](const SMessageResponseData& message) { BOOST_TEST_MESSAGE(message.m_msg); });
        topologyRequest->setDoneCallback([blocker]() mutable { blocker.Signal(); });
        mDDSSession.sendRequest<STopologyRequest>(topologyRequest);
        blocker.Wait();

        std::size_t execSlotsCount(0);
        interval = 8;
        while (execSlotsCount < mSlots) {
            std::this_thread::sleep_for(std::chrono::milliseconds(interval));
            interval = std::min(256, interval * 2);
            SAgentCountRequest::response_t res;
            mDDSSession.syncSendRequest<SAgentCountRequest>(SAgentCountRequest::request_t(), res);
            execSlotsCount = res.m_executingSlotsCount;
        }
    }

    ~TopologyFixture()
    {
        if (mDDSSession.IsRunning()) {
            mDDSSession.shutdown();
        }
    }

    static constexpr int mSlots = 6;
    dds::tools_api::CSession mDDSSession;
    dds::topology_api::CTopology mDDSTopo;
    boost::asio::io_context mIoContext;
    std::unordered_set<uint64_t> mExpendableTasks = {};
    std::map<std::string, odc::core::CollectionInfo> mCollectionInfo = {};
    std::atomic<uint64_t> mLastRunNr = 0;
};

#endif
