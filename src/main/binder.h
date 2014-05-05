/*
** Copyright (C) 2014 Cisco and/or its affiliates. All rights reserved.
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License Version 2 as
** published by the Free Software Foundation.  You may not use, modify or
** distribute this program under any other version of the GNU General
** Public License.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
// binder.cc author Russ Combs <rucombs@cisco.com>

#ifndef BINDER_H
#define BINDER_H

#include <string>

#include "framework/bits.h"

enum BindRole
{
    BR_EITHER,
    BR_CLIENT,
    BR_SERVER
};

enum BindAction
{
    BA_INSPECT,
    BA_ALLOW,
    BA_BLOCK
};

struct Binding
{
    // when
    std::string id;
    VlanList vlans;
    std::string nets;
    ByteList protos;
    PortList ports;
    BindRole role;

    // use
    std::string type;
    std::string name;

    // action
    BindAction action;

    Binding()
    { role = BR_EITHER; action = BA_INSPECT; };
};

#endif
