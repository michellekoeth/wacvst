// -*- c-basic-offset: 4 -*-

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

#include "remotevstclient.h"
#include "aeffectx.h"

#include <sys/un.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/poll.h>
#include <fcntl.h>
#include <iostream>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <pthread.h>

#include "rdwrops.h"
#include "paths.h"


RemoteVSTClient::RemoteVSTClient(CFBundleRef bundle, audioMasterCallback theMaster, AEffect *theEffect, bool showGUI) :
    RemotePluginClient(theMaster, theEffect)
{
	m_bundle = bundle;
	m_audioMaster = theMaster;
	pid_t child;

	SInt32 errorCode;
	CFDataRef data;
	CFURLRef configUrl = CFBundleCopyResourceURL(bundle, CFSTR("config"), CFSTR("plist"), NULL);
	if (configUrl == 0) {
		return;
	}
	CFURLCreateDataAndPropertiesFromResource(kCFAllocatorDefault, configUrl, &data, NULL, NULL, &errorCode);
	m_props = (CFDictionaryRef) CFPropertyListCreateFromXMLData( kCFAllocatorDefault, data, kCFPropertyListImmutable, NULL);

	std::string dllName(CFStringGetCStringPtr(get_path_prop(CFSTR("plugin-name")), NULL));
    std::string arg = dllName + "," + getFileIdentifiers();
    if (showGUI) arg = "-g " + arg;
	const char *argStr = arg.c_str();

	CFURLRef server_url = CFBundleCopyResourceURL(m_bundle, CFSTR("dssi-vst-server.exe"), CFSTR("so"), NULL);
	if (server_url != NULL) {
		CFStringRef server_path = CFURLCopyFileSystemPath(server_url, kCFURLPOSIXPathStyle);
		if (server_path != NULL) {
			const char *c_server_path = CFStringGetCStringPtr(server_path, NULL);
			CFStringRef wine_path =  (CFStringRef) CFDictionaryGetValue(m_props, CFSTR("wine-path"));
			if (wine_path != NULL) {
				const char *c_wine_path = CFStringGetCStringPtr(wine_path, NULL);	
				std::cerr << "RemoteVSTClient: executing " << c_server_path << "\n";
				// if no dispaly environment guess...
				// setenv("DISPLAY", ":0", 0);
				setenv("DYLD_FALLBACK_LIBRARY_PATH", "/usr/X11/lib:/usr/lib", 0);
				if ((child = fork()) < 0) {
					cleanup();
					throw((std::string)"Fork failed");
				} else if (child == 0) { // child process
					if (execlp(c_wine_path, "wine", c_server_path, argStr, NULL)) {
						perror("Exec failed");
						exit(1);
					}
				}
				syncStartup();
				return;
			}
		}
	}
	cleanup();
	throw((std::string)"Failed to find dssi-vst-server executable");
	return;
 }

RemoteVSTClient::~RemoteVSTClient()
{
    for (int i = 0; i < 3; ++i) {
	if (waitpid(-1, NULL, WNOHANG)) break;
	sleep(1);
    }
}

CFStringRef RemoteVSTClient::get_path_prop(CFStringRef path_name)
{
	CFStringRef path =  (CFStringRef) CFDictionaryGetValue(m_props, path_name);
	if (path == NULL) {
		return NULL;
	}
	CFURLRef fullPath = CFBundleCopyResourceURL(m_bundle, path, CFSTR("dll"), NULL);
	if (!fullPath) {
		return 0;
	}

	return CFURLCopyFileSystemPath(fullPath, kCFURLPOSIXPathStyle);
}


