/********************************************************************************
 * Copyright (C) 2019-2022 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <odc/MiscUtils.h>
#include <odc/Version.h>

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <iostream>
#include <map>
#include <string>
#include <vector>

using namespace std;
namespace bpo = boost::program_options;
namespace bfs = boost::filesystem;
namespace bpt = boost::property_tree;
using namespace odc;
using namespace odc::core;

struct Resource
{
    Resource(const bpt::ptree& pt)
    {
        // Only valid tags are allowed.
        set<string> validTags{ "zone", "n" };
        for (const auto& [tagName, tree] : pt) {
            if (validTags.count(tagName) == 0) {
                stringstream ss;
                ss << "Failed to init from property tree. Unknown key " << quoted(tagName);
                throw runtime_error(ss.str());
            }
        }
        // throw if these tags are not present
        mZone = pt.get<string>("zone");
        // use defaults if these tags are not present
        mN = pt.get<int32_t>("n", -1);
    }

    string mZone;
    int32_t mN = -1;
};

struct Resources
{
    Resources(const string& res)
    {
        bpt::ptree pt;
        stringstream ss;
        ss << res;
        try {
            bpt::read_json(ss, pt);
        } catch (const exception& e) {
            stringstream ss2;
            ss2 << "Invalid resource JSON string provided: " << res;
            throw runtime_error(ss2.str());
        }

        size_t numArrayElements = pt.count("");
        if (numArrayElements == 0) {
            // single resource
            mResources.push_back(Resource(pt));
        } else {
            // array of resources
            const auto& resArray = pt.get_child("");
            for (const auto& [tagName, tree] : resArray) {
                mResources.push_back(Resource(tree));
            }
        }
    }

    vector<Resource> mResources;
};

struct ZoneConfig
{
    size_t numSlots;
    string slurmCfgPath;
    string envCfgPath;
};

map<string, ZoneConfig> getZoneConfig(const vector<string>& zonesStr)
{
    map<string, ZoneConfig> result;

    for (const auto& z : zonesStr) {
        vector<string> zoneCfg;
        boost::algorithm::split(zoneCfg, z, boost::algorithm::is_any_of(":"));
        if (zoneCfg.size() != 4) {
            throw runtime_error(odc::core::toString(
                "Provided zones configuration has incorrect format. Expected <name>:<numSlots>:<slurmCfgPath>:<envCfgPath>. Received: ",
            z));
        }
        result.emplace(zoneCfg.at(0), ZoneConfig{ stoul(zoneCfg.at(1)), zoneCfg.at(2), zoneCfg.at(3) });
    }

    return result;
}

int main(int argc, char** argv)
{
    try {
        string resources;
        string partitionID;
        vector<string> zonesStr;

        string defaultLogDir{ smart_path(string("$HOME/.ODC/log")) };

        bpo::options_description opts("odc-rp-epn-slurm options");
        opts.add_options()
            ("id", bpo::value<string>(&partitionID)->default_value(""), "Partition ID")
            ("res", bpo::value<string>(&resources), "Resource description in JSON format. E.g. {\"zone\":\"online\",\"n\":1}")
            ("logdir", bpo::value<string>(), "[DEPRECATED] Does nothing")
            ("severity", bpo::value<string>(), "[DEPRECATED] Does nothing")
            ("infologger", bpo::bool_switch()->default_value(false), "[DEPRECATED] Does nothing")
            ("zones", bpo::value<vector<string>>(&zonesStr)->multitoken()->composing(), "Zones in <name>:<numSlots>:<slurmCfgPath>:<envCfgPath> format")
            ("version,v", "Print version")
            ("help,h", "Help message");

        bpo::variables_map vm;
        bpo::store(bpo::command_line_parser(argc, argv).options(opts).run(), vm);
        bpo::notify(vm);

        if (vm.count("help")) {
            cout << opts;
            return EXIT_SUCCESS;
        }

        if (vm.count("version")) {
            cout << ODC_VERSION << endl;
            return EXIT_SUCCESS;
        }

        Resources res(resources);
        map<string, ZoneConfig> zones{getZoneConfig(zonesStr)};

        for (const auto& r : res.mResources) {
            stringstream ss;
            if (zones.find(r.mZone) == zones.end()) {
                cerr << "Zone not found: " << r.mZone;
                return EXIT_FAILURE;
            }
            const auto& zone = zones.at(r.mZone);
            ss << "<submit>"
               << "<rms>slurm</rms>";
            if (!zone.slurmCfgPath.empty()) {
                ss << "<configFile>" << zone.slurmCfgPath << "</configFile>";
            }
            if (!zone.envCfgPath.empty()) {
                ss << "<envFile>" << zone.envCfgPath << "</envFile>";
            }
            ss << "<agents>" << r.mN << "</agents>" // number of agents (assuming it is equals to number of nodes)
               << "<zone>" << r.mZone << "</zone>" // zone
               << "<slots>" << zone.numSlots << "</slots>" // number of slots per agent
               << "</submit>";

            cout << ss.str() << endl;
        }
    } catch (exception& e) {
        cerr << e.what();
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
