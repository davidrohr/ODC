/********************************************************************************
 * Copyright (C) 2014-2021 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <fairmq/Device.h>
#include <fairmq/runDevice.h>

#include <cstdint> // uint64_t

namespace bpo = boost::program_options;

struct Sampler : fair::mq::Device
{
    void InitTask() override { fIterations = fConfig->GetValue<uint64_t>("iterations"); }

    bool ConditionalRun() override
    {
        fair::mq::MessagePtr msg(NewSimpleMessage("Data"));

        // LOG(info) << "Sending \"Data\"";

        if (Send(msg, "data1") < 0) {
            return false;
        }

        if (fIterations > 0) {
            ++fCounter;
            if (fCounter >= fIterations) {
                LOG(info) << "Sent " << fCounter << " messages. Finished.";
                return false;
            }
        }

        return true;
    }

  private:
    uint64_t fIterations = 0;
    uint64_t fCounter = 0;
};

void addCustomOptions(bpo::options_description& options)
{
    options.add_options()("iterations,i", bpo::value<uint64_t>()->default_value(0), "Maximum number of iterations of Run/ConditionalRun/OnData (0 - infinite)");
}

std::unique_ptr<fair::mq::Device> getDevice(fair::mq::ProgOptions& /*config*/) { return std::make_unique<Sampler>(); }
