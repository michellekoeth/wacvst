/* -*- c-basic-offset: 4 -*- */

/*
  dssi-vst: a DSSI plugin wrapper for VST effects and instruments
  Copyright 2004-2007 Chris Cannam

	This file is part of wacvst.

    wacvst free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#ifndef REMOTE_VST_CLIENT_H
#define REMOTE_VST_CLIENT_H

#include "remotepluginclient.h"
#include "aeffectx.h"

class RemoteVSTClient : public RemotePluginClient
{
public:
    // may throw a string exception
    RemoteVSTClient(CFBundleRef bundle, audioMasterCallback theMaster, AEffect *, bool showGUI = false);

    virtual ~RemoteVSTClient();
	audioMasterCallback m_audioMaster;

    struct PluginRecord {
	std::string dllName;
	std::string pluginName;
	std::string vendorName;
	bool isSynth;
	bool hasGUI;
	int inputs;
	int outputs;
	int parameters;
	std::vector<std::string> parameterNames;
	std::vector<float> parameterDefaults;
	int programs;
	std::vector<std::string> programNames;
    };

//    static void queryPlugins(std::vector<PluginRecord> &plugins);

protected:
//    static bool addFromFd(int fd, PluginRecord &rec);

private:
	CFStringRef get_path_prop(CFStringRef);
	CFDictionaryRef m_props;
	CFBundleRef m_bundle;
	
    RemoteVSTClient(const RemoteVSTClient &); // not provided
    RemoteVSTClient &operator=(const RemoteVSTClient &); // not provided
};    

#endif
