// -*- c-basic-offset: 4 -*-

/*
  dssi-vst: a DSSI plugin wrapper for VST effects and instruments
  Copyright 2004-2007 Chris Cannam

    This program is free software: you can redistribute it and/or modify
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

#include "remotepluginserver.h"

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/poll.h>
#include <fcntl.h>
#include <unistd.h>

#include <time.h>
#include <iostream>

#include "rdwrops.h"

/* todo: needs to be a class member and protected */
bool m_inProcess;

RemotePluginServer::RemotePluginServer(std::string fileIdentifiers) :
    m_controlRequestFd(-1),
    m_controlResponseFd(-1),
    m_bufferSize(-1),
    m_numInputs(-1),
    m_numOutputs(-1),
    m_processFd(-1),
    m_processResponseFd(-1),
    m_shmFd(-1),
    m_controlRequestFileName(0),
    m_controlResponseFileName(0),
    m_processFileName(0),
    m_processResponseFileName(0),
    m_shmFileName(0),
	m_shm(0),
    m_shmSize(0),
    m_inputs(0),
    m_outputs(0)
{
    m_inProcess = false;
    char tmpFileBase[60];
    sprintf(tmpFileBase, "/tmp/rplugin_crq_%s",
	    fileIdentifiers.substr(0, 6).c_str());
    m_controlRequestFileName = strdup(tmpFileBase);

    if ((m_controlRequestFd = open(m_controlRequestFileName, O_RDWR)) < 0) {
	cleanup();
	throw((std::string)"Failed to open FIFO");
    }
    
    sprintf(tmpFileBase, "/tmp/rplugin_crs_%s",
	    fileIdentifiers.substr(6, 6).c_str());
    m_controlResponseFileName = strdup(tmpFileBase);

    if ((m_controlResponseFd = open(m_controlResponseFileName, O_WRONLY)) < 0) {
	cleanup();
	throw((std::string)"Failed to open FIFO");
    }
    
    sprintf(tmpFileBase, "/tmp/rplugin_prc_%s",
	    fileIdentifiers.substr(12, 6).c_str());
    m_processFileName = strdup(tmpFileBase);

    if ((m_processFd = open(m_processFileName, O_RDONLY)) < 0) {
	cleanup();
	throw((std::string)"Failed to open FIFO");
    }
    
    sprintf(tmpFileBase, "/tmp/rplugin_prr_%s",
	    fileIdentifiers.substr(18, 6).c_str());
    m_processResponseFileName = strdup(tmpFileBase);
    printf("%s\n", m_processResponseFileName);
    if ((m_processResponseFd = open(m_processResponseFileName, O_WRONLY)) < 0) {
	cleanup();
	throw((std::string)"Failed to open process response FIFO");
    }
    
    sprintf(tmpFileBase, "/tmp/rplugin_ars_%s",
	    fileIdentifiers.substr(36, 6).c_str());
    m_AudioMasterResponseFileName = strdup(tmpFileBase);

    if ((m_AudioMasterResponseFd = open(m_AudioMasterResponseFileName, O_RDWR)) < 0) {
	cleanup();
	throw((std::string)"Failed to open audio masterresponse FIFO");
    }

    sprintf(tmpFileBase, "/tmp/rplugin_arq_%s",
	    fileIdentifiers.substr(30, 6).c_str());
    m_AudioMasterRequestFileName = strdup(tmpFileBase);
#ifdef MASTER_THREAD
    if ((m_AudioMasterRequestFd = open(m_AudioMasterRequestFileName, O_WRONLY)) < 0) {
		cleanup();
		throw((std::string)"Failed to open FIFO");
    }
#else
    m_AudioMasterRequestFd = m_processResponseFd;
#endif
    
    sprintf(tmpFileBase, "/tmp/rplugin_shm_%s",
	    fileIdentifiers.substr(24, 6).c_str());
    m_shmFileName = strdup(tmpFileBase);

    bool b = false;

    if ((m_shmFd = open(m_shmFileName, O_RDWR)) < 0) {
	tryWrite(m_controlResponseFd, &b, sizeof(bool));
	cleanup();
	throw((std::string)"Failed to open shared memory file");
    }

    b = true;
    tryWrite(m_controlResponseFd, &b, sizeof(bool));
}

RemotePluginServer::~RemotePluginServer()
{
    cleanup();
}

void
RemotePluginServer::cleanup()
{
    if (m_shm) {
	munmap(m_shm, m_shmSize);
	m_shm = 0;
    }
    if (m_controlRequestFd >= 0) {
	close(m_controlRequestFd);
	m_controlRequestFd = -1;
    }
    if (m_controlResponseFd >= 0) {
	close(m_controlResponseFd);
	m_controlResponseFd = -1;
    }
    if (m_processFd >= 0) {
	close(m_processFd);
	m_processFd = -1;
    }
    if (m_shmFd >= 0) {
	close(m_shmFd);
	m_shmFd = -1;
    }
    if (m_controlRequestFileName) {
	free(m_controlRequestFileName);
	m_controlRequestFileName = 0;
    }
    if (m_controlResponseFileName) {
	free(m_controlResponseFileName);
	m_controlResponseFileName = 0;
    }
    if (m_processFileName) {
	free(m_processFileName);
	m_processFileName = 0;
    }
    if (m_shmFileName) {
	free(m_shmFileName);
	m_shmFileName = 0;
    }
    
    delete m_inputs;
    m_inputs = 0;

    delete m_outputs;
    m_outputs = 0;
}

void
RemotePluginServer::sizeShm()
{
    if (m_numInputs < 0 || m_numOutputs < 0 || m_bufferSize < 0) return;

    delete m_inputs;
    delete m_outputs;
    m_inputs = 0;
    m_outputs = 0;

//    size_t sz = (m_numInputs + m_numOutputs) * m_bufferSize * sizeof(float);
	size_t sz = FIXED_SHM_SIZE;
	
    if (m_shm) {
//	m_shm = (char *)mremap(m_shm, m_shmSize, sz, MREMAP_MAYMOVE);
    } else {
	m_shm = (char *)mmap(0, sz, PROT_READ | PROT_WRITE, MAP_SHARED, m_shmFd, 0);
    }
    if (!m_shm) {
	std::cerr << "RemotePluginServer::sizeShm: ERROR: mmap or mremap for failed for " << sz
		  << " bytes from fd " << m_shmFd << "!" << std::endl;
	m_shmSize = 0;
    } else {
	m_shmSize = sz;
	if (m_numInputs > 0) {
	    m_inputs = new float*[m_numInputs];
	}
	if (m_numOutputs > 0) {
	    m_outputs = new float*[m_numOutputs];
	}
	std::cerr << "sized shm to " << sz << ", " << m_numInputs << " inputs and " << m_numOutputs << " outputs" << std::endl;
    }
}    

void
RemotePluginServer::dispatch(int timeout)
{
    struct pollfd pfd[2];
    
    pfd[0].fd = m_controlRequestFd;
    pfd[1].fd = m_processFd;
    pfd[0].events = pfd[1].events = POLLIN | POLLPRI | POLLERR | POLLHUP | POLLNVAL;

    if (poll(pfd, 2, timeout) < 0) {
	throw RemotePluginClosedException();
    }
    
    if ((pfd[0].revents & POLLIN) || (pfd[0].revents & POLLPRI)) {
	dispatchControl();
    } else if (pfd[1].revents) {
	throw RemotePluginClosedException();
    }
    
    if ((pfd[1].revents & POLLIN) || (pfd[1].revents & POLLPRI)) {
	dispatchProcess();
    } else if (pfd[1].revents) {
	throw RemotePluginClosedException();
    }
}

void
RemotePluginServer::dispatchControl(int timeout)
{
#if 0
    struct pollfd pfd;
    printf("in dispatchcontrol event\n");    
    pfd.fd = m_controlRequestFd;
    pfd.events = POLLIN | POLLPRI | POLLERR | POLLHUP | POLLNVAL;

    if (poll(&pfd, 1, timeout) < 0) {
	throw RemotePluginClosedException();
    }
    printf("dispatchin control event\n");    
    if ((pfd.revents & POLLIN) || (pfd.revents & POLLPRI)) {
	dispatchControlEvents();
    } else if (pfd.revents) {
	throw RemotePluginClosedException();
    }
#else
    int n;
    fd_set rfds, ofds;
    timeval timeo = {0,timeout * 1000};
    FD_ZERO(&rfds);
    FD_SET(m_controlRequestFd, &rfds);
    FD_ZERO(&ofds);
    if ((n = select(m_controlRequestFd+1, &rfds, &ofds, &ofds, &timeo)) == -1) {
	throw RemotePluginClosedException();
    }
    if (n == 1) {
	//	printf("got a control select\n");
	dispatchControlEvents();
    }
#endif
}

void
RemotePluginServer::dispatchProcess(int timeout)
{
	// just block in dispatchProcessEvents
	dispatchProcessEvents();
}

void
RemotePluginServer::dispatchProcessEvents()
{    
    RemotePluginOpcode opcode = RemotePluginNoOpcode;
   
    //    tryRead(m_processFd, &opcode, sizeof(RemotePluginOpcode));
    if (read(m_processFd, &opcode, sizeof(RemotePluginOpcode)) != 
	sizeof(RemotePluginOpcode)) {
	std::cerr << "ERROR: RemotePluginServer: couldn't read opcode" << std::endl;
	throw RemotePluginClosedException();
	return;
    }

    //    std::cerr << "read process opcode: " << opcode << std::endl;

    switch (opcode) {

    case RemotePluginProcess:
    {
	// todo: lock out the control thread
	m_inProcess = true;
	int sampleFrames = readInt(m_processFd);
	if (m_bufferSize < 0) {
	    std::cerr << "ERROR: RemotePluginServer: buffer size must be set before process" << std::endl;
	    return;
	}
	if (m_numInputs < 0) {
	    std::cerr << "ERROR: RemotePluginServer: input count must be tested before process" << std::endl;
	    return;
	}
	if (m_numOutputs < 0) {
	    std::cerr << "ERROR: RemotePluginServer: output count must be tested before process" << std::endl;
	    return;
	}
	if (!m_shm) {
	    sizeShm();
	    if (!m_shm) {
		std::cerr << "ERROR: RemotePluginServer: no shared memory region available" << std::endl;
		return;
	    }
	}

	//	std::cerr << "server process: entering" << std::endl;

	size_t blocksz = m_bufferSize * sizeof(float);

	for (int i = 0; i < m_numInputs; ++i) {
	    m_inputs[i] = (float *)(m_shm + i * blocksz);
	}
	for (int i = 0; i < m_numOutputs; ++i) {
	    m_outputs[i] = (float *)(m_shm + (i + m_numInputs) * blocksz);
	}

       	process(m_inputs, m_outputs, sampleFrames);
#if 0
	for (int i = 0; i < m_numOutputs; ++i) {
	    memcpy(m_outputs[i], m_inputs[i], sampleFrames*sizeof(float));
	}
#endif
	m_inProcess = false;
	//	printf("writing process response\n");
	writeInt(m_processResponseFd, PluginProcessComplete);

	//       	std::cerr << "server process: written" << std::endl;
	break;
    }
	
    case RemotePluginSetParameter:
    {
		int pn(readInt(m_processFd));
		setParameter(pn, readFloat(m_processFd));
		writeInt(m_controlResponseFd, 1);
		break;
	}

    case RemotePluginSetCurrentProgram:
	setCurrentProgram(readInt(m_processFd));
	break;

    case RemotePluginSendMIDIData:
    {
	int events = 0;
	int *frameoffsets = 0;
	unsigned char *data = readMIDIData(m_processFd, &frameoffsets, events);
	if (events && data && frameoffsets) {
//    std::cerr << "RemotePluginServer::sendMIDIData(" << events << ")" << std::endl;

	    sendMIDIData(data, frameoffsets, events);
	}
	break;
    }

    case RemotePluginSetBufferSize:
    {
		int newSize = readInt(m_processFd);
		setBufferSize(newSize);
		m_bufferSize = newSize;
		writeInt(m_controlResponseFd, 1);
		break;
    }

    case RemotePluginSetSampleRate:
		setSampleRate(readFloat(m_processFd));
		break;
    
    case RemotePluginReset:
		reset();
		break;

    default:
	std::cerr << "WARNING: RemotePluginServer::dispatchProcessEvents: unexpected opcode "
		  << opcode << std::endl;
    }
    //    std::cerr << "dispatched process event\n";
}

void
RemotePluginServer::dispatchControlEvents()
{    
    RemotePluginOpcode opcode = RemotePluginNoOpcode;
    static float *parameterBuffer = 0;

    tryRead(m_controlRequestFd, &opcode, sizeof(RemotePluginOpcode));
    //    std::cerr << "control opcoded " << opcode << std::endl;

    switch (opcode) {

    case RemotePluginGetVersion:
	writeInt(m_controlResponseFd, getVersion());
	break;

    case RemotePluginGetName:
	writeString(m_controlResponseFd, getName());
	break;

    case RemotePluginGetMaker:
	writeString(m_controlResponseFd, getMaker());
	break;
    
    case RemotePluginTerminate:
	terminate();
	break;
    
    case RemotePluginGetInputCount:
	m_numInputs = getInputCount();
	writeInt(m_controlResponseFd, m_numInputs);
	break;

    case RemotePluginGetOutputCount:
	m_numOutputs = getOutputCount();
	writeInt(m_controlResponseFd, m_numOutputs);
	break;

	case RemotePluginGetFlags:
		m_flags = getFlags();
		writeInt(m_controlResponseFd, m_flags);
		break;

	case RemotePluginGetUniqueID:
		writeInt(m_controlResponseFd, getUniqueID());
		break;
	case RemotePluginGetInitialDelay:
		writeInt(m_controlResponseFd, getInitialDelay());
		break;
		
    case RemotePluginGetParameterCount:
	writeInt(m_controlResponseFd, getParameterCount());
	break;
	
    case RemotePluginGetParameterName:
	writeString(m_controlResponseFd, getParameterName(readInt(m_controlRequestFd)));
	break;
    
    case RemotePluginGetParameter:
	writeFloat(m_controlResponseFd, getParameter(readInt(m_controlRequestFd)));
	break;
    
    case RemotePluginGetParameterDefault:
	writeFloat(m_controlResponseFd, getParameterDefault(readInt(m_controlRequestFd)));
	break;

    case RemotePluginGetParameters:
    {
	if (!parameterBuffer) {
	    parameterBuffer = new float[getParameterCount()];
	}
 	int p0 = readInt(m_controlRequestFd);
	int pn = readInt(m_controlRequestFd);
	getParameters(p0, pn, parameterBuffer);
	tryWrite(m_controlResponseFd, parameterBuffer, (pn - p0 + 1) * sizeof(float));
	break;
    }

    case RemotePluginHasMIDIInput:
    {
	bool m = hasMIDIInput();
	tryWrite(m_controlResponseFd, &m, sizeof(bool));
	break;
    }
       
    case RemotePluginGetProgramCount:
	writeInt(m_controlResponseFd, getProgramCount());
	break;

    case RemotePluginGetProgramName:
	{
		char name[256];
		int r = getProgramName(readInt(m_controlRequestFd), name);
		writeString(m_controlResponseFd, name);
		writeInt(m_controlResponseFd, r);
		break;
	}
	
    case RemotePluginIsReady:
    {
	if (!m_shm) sizeShm();
	bool b(isReady());
	std::cerr << "isReady: returning " << b << std::endl;
	tryWrite(m_controlResponseFd, &b, sizeof(bool));
    }

    case RemotePluginSetDebugLevel:
    {
	RemotePluginDebugLevel newLevel = m_debugLevel;
	tryRead(m_controlRequestFd, &newLevel, sizeof(RemotePluginDebugLevel));
	setDebugLevel(newLevel);
	m_debugLevel = newLevel;
	break;
    }

    case RemotePluginWarn:
    {
	bool b = warn(readString(m_controlRequestFd));
	tryWrite(m_controlResponseFd, &b, sizeof(bool));
	break;
    }

    case RemotePluginShowGUI:
    {
	bool b = true;
	showGUI(readString(m_controlRequestFd));
	tryWrite(m_controlResponseFd, &b, sizeof(bool));
	break;
    }

    case RemotePluginHideGUI:
    {
	bool b = true;
	hideGUI();
	tryWrite(m_controlResponseFd, &b, sizeof(bool));
	break;
    }

    case RemotePluginNoOpcode:
	break;

    case RemotePluginGetEffInt:
    {
	int opcode = readInt(m_controlRequestFd);
	writeInt(m_controlResponseFd, getEffInt(opcode));
	break;
    }

    case RemotePluginDoVoid:
    {
	int opcode = readInt(m_controlRequestFd);
	effDoVoid(opcode);
	bool b = true;
	tryWrite(m_controlResponseFd, &b, sizeof(bool));
	break;
    }

    case RemotePluginGetEffString:
    {
	int opcode = readInt(m_controlRequestFd);
	int idx = readInt(m_controlRequestFd);
	writeString(m_controlResponseFd, getEffString(opcode, idx));
	break;
    }
	
    case RemoteMainsChanged:
    {
	int v = readInt(m_controlRequestFd);
	//std::cerr << "Mains changing " << v << std::endl;
	eff_mainsChanged(v);
	writeInt(m_controlResponseFd, 1);
	break;
    }

	case RemotePluginProcessEvents:
	{
		writeInt(m_controlResponseFd, processVstEvents());
		break;
	}
	
	case RemotePluginGetChunk:
	{
		getChunk();
		break;
	}	
 
	case RemotePluginSetChunk:
	{
		setChunk();
		break;
	}	

	case RemotePluginCanBeAutomated:
	{
		canBeAutomated();
		break;
	}	

	case RemotePluginGetProgram:
	{
		getProgram();
		break;
	}	

	case RemotePluginClose:
	{
		cleanup();
		std::cerr << "Server shutting down" << std::endl;
		exit(0);
	}	

	default:
	std::cerr << "WARNING: RemotePluginServer::dispatchControlEvents: unexpected opcode "
		  << opcode << std::endl;
    }
    //    std::cerr << "done dispatching control\n";
}
