/********************************************************************************
 * Copyright (C) 2019-2022 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <odc/CliControlService.h>
#include <odc/Topology.h>
// STD
#include <sstream>

using namespace odc;
using namespace odc::core;
using namespace odc::cli;
using namespace std;

CCliControlService::CCliControlService()
    : m_service(make_shared<ControlService>())
{
}

void CCliControlService::setTimeout(const std::chrono::seconds& _timeout)
{
    m_service->setTimeout(_timeout);
}

void CCliControlService::registerResourcePlugins(const CPluginManager::PluginMap_t& _pluginMap)
{
    m_service->registerResourcePlugins(_pluginMap);
}

void CCliControlService::registerRequestTriggers(const CPluginManager::PluginMap_t& _triggerMap)
{
    m_service->registerRequestTriggers(_triggerMap);
}

void CCliControlService::restore(const std::string& _restoreId)
{
    m_service->restore(_restoreId);
}

std::string CCliControlService::requestInitialize(const CommonParams& _common, const SInitializeParams& _params)
{
    return generalReply(m_service->execInitialize(_common, _params));
}

std::string CCliControlService::requestSubmit(const CommonParams& _common, const SSubmitParams& _params)
{
    return generalReply(m_service->execSubmit(_common, _params));
}

std::string CCliControlService::requestActivate(const CommonParams& _common, const SActivateParams& _params)
{
    return generalReply(m_service->execActivate(_common, _params));
}

std::string CCliControlService::requestRun(const CommonParams& _common,
                                           const SInitializeParams& _initializeParams,
                                           const SSubmitParams& _submitParams,
                                           const SActivateParams& _activateParams)
{
    return generalReply(m_service->execRun(_common, _initializeParams, _submitParams, _activateParams));
}

std::string CCliControlService::requestUpscale(const CommonParams& _common, const SUpdateParams& _params)
{
    return generalReply(m_service->execUpdate(_common, _params));
}

std::string CCliControlService::requestDownscale(const CommonParams& _common, const SUpdateParams& _params)
{
    return generalReply(m_service->execUpdate(_common, _params));
}

std::string CCliControlService::requestGetState(const CommonParams& _common, const SDeviceParams& _params)
{
    return generalReply(m_service->execGetState(_common, _params));
}

std::string CCliControlService::requestSetProperties(const CommonParams& _common, const SSetPropertiesParams& _params)
{
    return generalReply(m_service->execSetProperties(_common, _params));
}

std::string CCliControlService::requestConfigure(const CommonParams& _common, const SDeviceParams& _params)
{
    return generalReply(m_service->execConfigure(_common, _params));
}

std::string CCliControlService::requestStart(const CommonParams& _common, const SDeviceParams& _params)
{
    return generalReply(m_service->execStart(_common, _params));
}

std::string CCliControlService::requestStop(const CommonParams& _common, const SDeviceParams& _params)
{
    return generalReply(m_service->execStop(_common, _params));
}

std::string CCliControlService::requestReset(const CommonParams& _common, const SDeviceParams& _params)
{
    return generalReply(m_service->execReset(_common, _params));
}

std::string CCliControlService::requestTerminate(const CommonParams& _common, const SDeviceParams& _params)
{
    return generalReply(m_service->execTerminate(_common, _params));
}

std::string CCliControlService::requestShutdown(const CommonParams& _common)
{
    return generalReply(m_service->execShutdown(_common));
}

std::string CCliControlService::requestStatus(const SStatusParams& _params)
{
    return statusReply(m_service->execStatus(_params));
}

string CCliControlService::generalReply(const RequestResult& result)
{
    stringstream ss;

    if (result.m_statusCode == StatusCode::ok)
    {
        ss << "  Status code: SUCCESS\n  Message: " << result.m_msg << endl;
    }
    else
    {
        ss << "  Status code: ERROR\n  Error code: " << result.m_error.m_code.value()
           << "\n  Error message: " << result.m_error.m_code.message() << " (" << result.m_error.m_details << ")"
           << endl;
    }

    ss << "  Aggregated state: " << result.m_aggregatedState << endl;
    ss << "  Partition ID: " << result.m_partitionID << endl;
    ss << "  Run Nr: " << result.m_runNr << endl;
    ss << "  Session ID: " << result.m_sessionID << endl;

    if (result.mFullState != nullptr)
    {
        ss << endl << "  Devices: " << endl;
        for (const auto& state : *(result.mFullState))
        {
            ss << "    { id: " << state.m_status.taskId << "; path: " << state.m_path
               << "; state: " << state.m_status.state << " }" << endl;
        }
        ss << endl;
    }

    ss << "  Execution time: " << result.m_execTime << " msec" << endl;

    return ss.str();
}

std::string CCliControlService::statusReply(const StatusRequestResult& result)
{
    stringstream ss;
    if (result.m_statusCode == StatusCode::ok)
    {
        ss << "  Status code: SUCCESS\n  Message: " << result.m_msg << endl;
    }
    else
    {
        ss << "  Status code: ERROR\n  Error code: " << result.m_error.m_code.value()
           << "\n  Error message: " << result.m_error.m_code.message() << " (" << result.m_error.m_details << ")"
           << endl;
    }
    ss << "  Partitions: " << endl;
    for (const auto& p : result.m_partitions)
    {
        ss << "    { partition ID: " << p.m_partitionID << "; session ID: " << p.m_sessionID
           << "; status: " << ((p.m_sessionStatus == DDSSessionStatus::running) ? "RUNNING" : "STOPPED")
           << "; state: " << GetAggregatedTopologyStateName(p.m_aggregatedState) << " }" << endl;
    }
    ss << "  Execution time: " << result.m_execTime << " msec" << endl;
    return ss.str();
}
