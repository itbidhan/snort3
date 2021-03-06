//--------------------------------------------------------------------------
// Copyright (C) 2015-2016 Cisco and/or its affiliates. All rights reserved.
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License Version 2 as published
// by the Free Software Foundation.  You may not use, modify or distribute
// this program under any other version of the GNU General Public License.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//--------------------------------------------------------------------------

// side_channel_module.h author Ed Borgoyn <eborgoyn@cisco.com>

#ifndef SIDE_CHANNEL_MODULE_H
#define SIDE_CHANNEL_MODULE_H

#include <vector>

#include "framework/connector.h"
#include "framework/module.h"
#include "main/thread.h"
#include "side_channel.h"

#define SIDECHANNEL_NAME "side_channel"
#define SIDECHANNEL_HELP "implement the side-channel asynchronous messaging subsystem"

struct SideChannelConfig
{
    PortBitSet* ports = nullptr;
    Connector::Direction direction = Connector::CONN_UNDEFINED;
    SCConnectors connectors;
};

extern THREAD_LOCAL SimpleStats sc_stats;
extern THREAD_LOCAL ProfileStats sc_perf_stats;

class SideChannelModule : public Module
{
public:
    SideChannelModule();
    ~SideChannelModule();

    bool set(const char*, Value&, SnortConfig*) override;
    bool begin(const char*, int, SnortConfig*) override;
    bool end(const char*, int, SnortConfig*) override;

    const PegInfo* get_pegs() const override;
    PegCount* get_counts() const override;

    ProfileStats* get_profile() const override;

#ifdef BUILD_SC_SHELL
    const Command* get_commands() const override;
#endif

private:
    SideChannelConfig* config = nullptr;
};

#endif

