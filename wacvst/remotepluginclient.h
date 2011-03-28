/* -*- c-basic-offset: 4 -*- */

/*
  dssi-vst: a DSSI plugin wrapper for VST effects and instruments
  Copyright 2004-2007 Chris Cannam

	This file is part of wacvst.

    wacvst is free software: you can redistribute it and/or modify
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

#ifndef REMOTE_PLUGIN_CLIENT_H
#define REMOTE_PLUGIN_CLIENT_H

#include "remoteplugin.h"
#include "aeffectx.h"
#include <pthread.h>
#include <string>
#include <vector>
#include <sys/shm.h>

// Any of the methods in this file, including constructors, should be
// considered capable of throwing RemotePluginClosedException.  Do not
// call anything here without catching it.

class RemotePluginClient
{
public:
	RemotePluginClient(audioMasterCallback theMaster, AEffect *);
    virtual ~RemotePluginClient();

    std::string  getFileIdentifiers();

    int        getVersion();
    std::string  getName();
    std::string  getMaker();

    void         setBufferSize(int);
    void         setSampleRate(float);

    void         reset();
    void         terminate();
    
    int          getInputCount();
    int          getOutputCount();
	
	int			 getFlags();
	int			 getInitialDelay();
	int			 getUniqueID();
	
	int			 getChunk(void **ptr, int bank_prog);
 	int			 setChunk(void *ptr, int sz, int bank_prog);
	int			 canBeAutomated(int param);
	int			 getProgram();
	
    int          getParameterCount();
    std::string  getParameterName(int);
    void         setParameter(int, float);
    float        getParameter(int);
    float        getParameterDefault(int);
    void         getParameters(int, int, float *);

    int          getProgramCount();
    int		     getProgramName(int, std::string &s);
    void         setCurrentProgram(int);
	int			 processVstEvents(VstEvents *);
    bool         hasMIDIInput();

    // Must be three bytes per event
    void         sendMIDIData(unsigned char *data, int *frameoffsets, int events);

    // Either inputs or outputs may be NULL if (and only if) there are none
    void         process(float **inputs, float **outputs, int sampleFrames);

    void         setDebugLevel(RemotePluginDebugLevel);
    bool         warn(std::string);

    void         showGUI(std::string guiData);
    void         hideGUI();

	int			getEffInt(int opcode);
	void		getEffString(int opcode, int index, char *ptr, int len);
	void		effVoidOp(int opcode);
	void		effCloseOp();
	void		effMainsChanged(int s);

	int m_AudioMasterRequestFd;
	int m_AudioMasterResponseFd;
	audioMasterCallback m_audioMaster;
	AEffect *m_effect;

protected:
    RemotePluginClient();

    void         cleanup();
    void         syncStartup();

private:
    RemotePluginClient(const RemotePluginClient &); // not provided
    RemotePluginClient &operator=(const RemotePluginClient &); // not provided

    int m_controlRequestFd;
    int m_controlResponseFd;
    int m_processFd;
    int m_processResponseFd;
    int m_shmFd;

    char *m_controlRequestFileName;
    char *m_controlResponseFileName;
    char *m_processFileName;
    char *m_processResponseFileName;
    char *m_AudioMasterRequestFileName;
    char *m_AudioMasterResponseFileName;
	pthread_t m_audioMasterThread;
    char *m_shmFileName;

    char *m_shm;
    size_t m_shmSize;

    int m_bufferSize;
    int m_numInputs;
    int m_numOutputs;
    void sizeShm();
};


#endif

