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

#include "remotepluginclient.h"

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <iostream>
#include <errno.h>

#include "rdwrops.h"

void *audioMasterThread(void *theArg)
{
	// currently we use the process thread to handle plugin->host requests
	RemotePluginClient *client = (RemotePluginClient *) theArg;
	if (client->m_AudioMasterRequestFd == 0) {
		printf("wtf....\n");
	}
	int opcode;
	while (1) {
//		printf("waiting %d\n", client->m_AudioMasterRequestFd);fflush(stdout);
		read(client->m_AudioMasterRequestFd, &opcode, 4);
//			printf("got audio master request %d\n", opcode);
		switch (opcode) {
		case audioMasterGetTime: 
		{
			struct VstTimeInfo *timeInfo;
			VstTimeInfo junk;
			int v;
			read(client->m_AudioMasterRequestFd, &v, 4);
			timeInfo = (struct VstTimeInfo *) client->m_audioMaster(NULL, audioMasterGetTime, 0, v, 0, 0);
			if (!timeInfo) {
				timeInfo = &junk;
			}
			write(client->m_AudioMasterResponseFd, timeInfo, sizeof(struct VstTimeInfo));
//		printf("audio master %x %x\n", client->m_audioMaster, timeInfo);
			break;
		}
		case audioMasterIdle:
		{
			int j = 0;
			printf("got idle request\n");
			client->m_audioMaster(client->m_effect, audioMasterIdle, 0, 0, 0, 0);
			printf("issuing responset\n");
			write(client->m_AudioMasterResponseFd, &j, 4);
			break;
		}
		default:
			printf("wtf %d\n", opcode);
			break;
		}
	}
	return 0;
}

RemotePluginClient::RemotePluginClient(audioMasterCallback theMaster, AEffect *theEffect) :
	m_audioMaster(theMaster),
	m_effect(theEffect),
    m_controlRequestFd(-1),
    m_controlResponseFd(-1),
    m_processFd(-1),
    m_shmFd(-1),
    m_controlRequestFileName(0),
    m_controlResponseFileName(0),
    m_processFileName(0),
    m_shmFileName(0),
    m_shm(0),
    m_shmSize(0),
    m_bufferSize(-1),
    m_numInputs(-1),
    m_numOutputs(-1)
{
    char tmpFileBase[60];
    sprintf(tmpFileBase, "/tmp/rplugin_crq_XXXXXX");
    if (mkstemp(tmpFileBase) < 0) {
	cleanup();
	throw((std::string)"Failed to obtain temporary filename");
    }
    m_controlRequestFileName = strdup(tmpFileBase);

    unlink(m_controlRequestFileName);
    if (mkfifo(m_controlRequestFileName, 0666)) { //!!! what permission is correct here?
		perror(m_controlRequestFileName);
		cleanup();
		throw((std::string)"Failed to create FIFO");
    }

    sprintf(tmpFileBase, "/tmp/rplugin_crs_XXXXXX");
    if (mkstemp(tmpFileBase) < 0) {
		cleanup();
		throw((std::string)"Failed to obtain temporary filename");
	}
    m_controlResponseFileName = strdup(tmpFileBase);

    unlink(m_controlResponseFileName);
    if (mkfifo(m_controlResponseFileName, 0666)) {
		perror(m_controlResponseFileName);
		cleanup();
		throw((std::string)"Failed to create FIFO");
    }

    sprintf(tmpFileBase, "/tmp/rplugin_prc_XXXXXX");
    if (mkstemp(tmpFileBase) < 0) {
		cleanup();
		throw((std::string)"Failed to obtain temporary filename");
	}
    m_processFileName = strdup(tmpFileBase);

    unlink(m_processFileName);
		if (mkfifo(m_processFileName, 0666)) {
		perror(m_processFileName);
		cleanup();
		throw((std::string)"Failed to create FIFO");
    }

    sprintf(tmpFileBase, "/tmp/rplugin_prr_XXXXXX");
		if (mkstemp(tmpFileBase) < 0) {
		cleanup();
		throw((std::string)"Failed to obtain temporary filename");
    }
    m_processResponseFileName = strdup(tmpFileBase);

    unlink(m_processResponseFileName);
    if (mkfifo(m_processResponseFileName, 0666)) {
		perror(m_processResponseFileName);
		cleanup();
		throw((std::string)"Failed to create FIFO");
    }

    sprintf(tmpFileBase, "/tmp/rplugin_arq_XXXXXX");
    if (mkstemp(tmpFileBase) < 0) {
		cleanup();
		throw((std::string)"Failed to obtain temporary filename");
    }
    m_AudioMasterRequestFileName = strdup(tmpFileBase);

    unlink(m_AudioMasterRequestFileName);
    if (mkfifo(m_AudioMasterRequestFileName, 0666)) { 
		perror(m_AudioMasterRequestFileName);
		cleanup();
		throw((std::string)"Failed to create FIFO");
    }

    sprintf(tmpFileBase, "/tmp/rplugin_ars_XXXXXX");
    if (mkstemp(tmpFileBase) < 0) {
		cleanup();
		throw((std::string)"Failed to obtain temporary filename");
    }
    m_AudioMasterResponseFileName = strdup(tmpFileBase);

    unlink(m_AudioMasterResponseFileName);
    if (mkfifo(m_AudioMasterResponseFileName, 0666)) {
		perror(m_AudioMasterResponseFileName);
		cleanup();
		throw((std::string)"Failed to create FIFO");
    }

    sprintf(tmpFileBase, "/tmp/rplugin_shm_XXXXXX");
    if (mkstemp(tmpFileBase) < 0) {
	cleanup();
	throw((std::string)"Failed to obtain temporary filename");
    }
    m_shmFileName = strdup(tmpFileBase);

    m_shmFd = open(m_shmFileName, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (m_shmFd < 0) {
		cleanup();
		throw((std::string)"Failed to open or create shared memory file");
    }
}

RemotePluginClient::~RemotePluginClient()
{
    cleanup();
}

void
RemotePluginClient::syncStartup()
{
    // The first (write) fd we open in a nonblocking call, with a
    // short retry loop so we can easily give up if the other end
    // doesn't appear to be responding.  We want a nonblocking FIFO
    // for this and the process fd anyway.

    bool connected = false;
    int timeout = 15;
    for (int attempt = 0; attempt < timeout; ++attempt) {

		if ((m_controlRequestFd =
			open(m_controlRequestFileName, O_WRONLY | O_NONBLOCK)) >= 0) {
			connected = true;
			break;
		} else if (errno != ENXIO) {
			// an actual error occurred
			break;
		}

		sleep(1);
    }

    if (!connected) {
		cleanup();
		throw((std::string)"Plugin server timed out on startup");
	}
    if ((m_controlResponseFd = open(m_controlResponseFileName, O_RDONLY)) < 0) {
		cleanup();
		throw((std::string)"Failed to open control FIFO");
    }

    connected = false;
    for (int attempt = 0; attempt < 6; ++attempt) {
		if ((m_processFd = open(m_processFileName, O_WRONLY | O_NONBLOCK)) >= 0) {
			connected = true;
			break;
		} else if (errno != ENXIO) {
			// an actual error occurred
			break;
		}
		sleep(1);
	}
    if (!connected) {
		cleanup();
		throw((std::string)"Failed to open process FIFO");
    }
    if ((m_processResponseFd = open(m_processResponseFileName, O_RDONLY)) < 0) {
		cleanup();
		throw((std::string)"Failed to open process FIFO");
    }

 	// for the audiomaster stuff, the client is the server
   connected = false;
    for (int attempt = 0; attempt < 6; ++attempt) {
		if ((m_AudioMasterResponseFd = open(m_AudioMasterResponseFileName, O_WRONLY | O_NONBLOCK)) >= 0) {
			connected = true;
			break;
		} else if (errno != ENXIO) {
			// an actual error occurred
			break;
		}
		sleep(1);
	}
    if (!connected) {
		cleanup();
		throw((std::string)"Failed to open audio master response FIFO");
    }
#ifdef MASTER_THREAD
    if ((m_AudioMasterRequestFd = open(m_AudioMasterRequestFileName, O_RDONLY)) < 0) {
		cleanup();
		throw((std::string)"Failed to open audio master request FIFO");
    }
#endif

    bool b = false;
    tryRead(m_controlResponseFd, &b, sizeof(bool));
	if (!b) {
		cleanup();
		throw((std::string)"Remote plugin did not start correctly");
    }
#ifdef MASTER_THREAD
	pthread_create(&m_audioMasterThread, NULL, audioMasterThread, this);
#endif
}

void
RemotePluginClient::cleanup()
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
	unlink(m_controlRequestFileName);
	free(m_controlRequestFileName);
	m_controlRequestFileName = 0;
    }
    if (m_controlResponseFileName) {
	unlink(m_controlResponseFileName);
	free(m_controlResponseFileName);
	m_controlResponseFileName = 0;
    }
    if (m_processFileName) {
	unlink(m_processFileName);
	free(m_processFileName);
	m_processFileName = 0;
    }
    if (m_shmFileName) {
	unlink(m_shmFileName);
	free(m_shmFileName);
	m_shmFileName = 0;
    }
}

std::string
RemotePluginClient::getFileIdentifiers()
{
    std::string id;
    id += m_controlRequestFileName + strlen(m_controlRequestFileName) - 6;
    id += m_controlResponseFileName + strlen(m_controlResponseFileName) - 6;
    id += m_processFileName + strlen(m_processFileName) - 6;
    id += m_processResponseFileName + strlen(m_processResponseFileName) - 6;
    id += m_shmFileName + strlen(m_shmFileName) - 6;
    id += m_AudioMasterRequestFileName + strlen(m_AudioMasterRequestFileName) - 6;
    id += m_AudioMasterResponseFileName + strlen(m_AudioMasterResponseFileName) - 6;
    std::cerr << "Returning file identifiers: " << id << std::endl;
    return id;
}

void
RemotePluginClient::sizeShm()
{
    if (m_numInputs < 0 || m_numOutputs < 0 || m_bufferSize < 0) return;
    size_t sz = (m_numInputs + m_numOutputs) * m_bufferSize * sizeof(float);
	if (sz > FIXED_SHM_SIZE) {
		cleanup();
		throw((std::string)"exceeded fixed shm size");
	}
		
	sz = FIXED_SHM_SIZE;
	
    ftruncate(m_shmFd, sz);

    if (m_shm) {
#ifdef RESIZE_SHM
		// this currently isn't working so we just use a big, fixed size buffer
		munmap(m_shm, m_shmSize);
		m_shm = (char *)mmap(0, sz, PROT_READ | PROT_WRITE, MAP_SHARED, m_shmFd, 0);
#endif
	} else {
		m_shm = (char *)mmap(0, sz, PROT_READ | PROT_WRITE, MAP_SHARED, m_shmFd, 0);
    }
    if (!m_shm) {
		std::cerr << "RemotePluginClient::sizeShm: ERROR: mmap or mremap failed for " << sz
			<< " bytes from fd " << m_shmFd << "!" << std::endl;
		m_shmSize = 0;
    } else {
		memset(m_shm, 0, sz);
		m_shmSize = sz;
		std::cerr << "client sized shm to " << sz << std::endl;
    }
}

int
RemotePluginClient::getVersion()
{
    writeOpcode(m_controlRequestFd, RemotePluginGetVersion);
    return readInt(m_controlResponseFd);
}

int
RemotePluginClient::getUniqueID()
{
    writeOpcode(m_controlRequestFd, RemotePluginGetUniqueID);
    return readInt(m_controlResponseFd);
}

int
RemotePluginClient::getInitialDelay()
{
    writeOpcode(m_controlRequestFd, RemotePluginGetInitialDelay);
    return readInt(m_controlResponseFd);
}

std::string
RemotePluginClient::getName()
{
    writeOpcode(m_controlRequestFd, RemotePluginGetName);
    return readString(m_controlResponseFd);
}

std::string
RemotePluginClient::getMaker()
{
    writeOpcode(m_controlRequestFd, RemotePluginGetMaker);
    return readString(m_controlResponseFd);
}

void
RemotePluginClient::setBufferSize(int s)
{
	int b;
	if (s == m_bufferSize)
		return;
    m_bufferSize = s;
    sizeShm();
    writeOpcode(m_processFd, RemotePluginSetBufferSize);
    writeInt(m_processFd, s);
	b = readInt(m_controlResponseFd);
	
}

void
RemotePluginClient::effMainsChanged(int s)
{
    writeOpcode(m_controlRequestFd, RemoteMainsChanged);
    writeInt(m_controlRequestFd, s);
	readInt(m_controlResponseFd);
}

void
RemotePluginClient::setSampleRate(float s)
{
    writeOpcode(m_processFd, RemotePluginSetSampleRate);
    writeFloat(m_processFd, s);
	readInt(m_controlResponseFd);
}

void
RemotePluginClient::reset()
{
    writeOpcode(m_processFd, RemotePluginReset);
    if (m_shmSize > 0) {
	memset(m_shm, 0, m_shmSize);
    }
}

void
RemotePluginClient::terminate()
{
    writeOpcode(m_controlRequestFd, RemotePluginTerminate);
}

int
RemotePluginClient::getEffInt(int opcode)
{
    writeOpcode(m_controlRequestFd, RemotePluginGetEffInt);
    writeInt(m_controlRequestFd, opcode);
	return readInt(m_controlResponseFd);
}
	
void
RemotePluginClient::getEffString(int opcode, int index, char *ptr, int len)
{
    writeOpcode(m_controlRequestFd, RemotePluginGetEffString);
	writeInt(m_controlRequestFd, opcode);
	writeInt(m_controlRequestFd, index);
    strncpy(ptr, readString(m_controlResponseFd).c_str(), len);
	ptr[len-1] = 0;
}

int
RemotePluginClient::getInputCount()
{
    writeOpcode(m_controlRequestFd, RemotePluginGetInputCount);
    m_numInputs = readInt(m_controlResponseFd);
    sizeShm();
    return m_numInputs;
}

int
RemotePluginClient::getOutputCount()
{
    writeOpcode(m_controlRequestFd, RemotePluginGetOutputCount);
    m_numOutputs = readInt(m_controlResponseFd);
    sizeShm();
    return m_numOutputs;
}

int
RemotePluginClient::getParameterCount()
{
    writeOpcode(m_controlRequestFd, RemotePluginGetParameterCount);
    return readInt(m_controlResponseFd);
}

std::string
RemotePluginClient::getParameterName(int p)
{
    writeOpcode(m_controlRequestFd, RemotePluginGetParameterName);
    writeInt(m_controlRequestFd, p);
    return readString(m_controlResponseFd);
}

void
RemotePluginClient::setParameter(int p, float v)
{
    writeOpcode(m_processFd, RemotePluginSetParameter);
    writeInt(m_processFd, p);
    writeFloat(m_processFd, v);
	// wait for a response
	readInt(m_controlResponseFd);
}

float
RemotePluginClient::getParameter(int p)
{
    writeOpcode(m_controlRequestFd, RemotePluginGetParameter);
    writeInt(m_controlRequestFd, p);
    return readFloat(m_controlResponseFd);
}

float
RemotePluginClient::getParameterDefault(int p)
{
    writeOpcode(m_controlRequestFd, RemotePluginGetParameterDefault);
    writeInt(m_controlRequestFd, p);
    return readFloat(m_controlResponseFd);
}

void
RemotePluginClient::getParameters(int p0, int pn, float *v)
{
    writeOpcode(m_controlRequestFd, RemotePluginGetParameters);
    writeInt(m_controlRequestFd, p0);
    writeInt(m_controlRequestFd, pn);
    tryRead(m_controlResponseFd, v, (pn - p0 + 1) * sizeof(float));
}

bool
RemotePluginClient::hasMIDIInput()
{
    writeOpcode(m_controlRequestFd, RemotePluginHasMIDIInput);
    bool b;
    tryRead(m_controlResponseFd, &b, sizeof(bool));
    return b;
}

int
RemotePluginClient::getProgramCount()
{
    writeOpcode(m_controlRequestFd, RemotePluginGetProgramCount);
    return readInt(m_controlResponseFd);
}

int
RemotePluginClient::getProgramName(int n, std::string &s)
{
    writeOpcode(m_controlRequestFd, RemotePluginGetProgramName);
    writeInt(m_controlRequestFd, n);
	s = readString(m_controlResponseFd);
	return readInt(m_controlResponseFd);
}    

void
RemotePluginClient::setCurrentProgram(int n)
{
    writeOpcode(m_processFd, RemotePluginSetCurrentProgram);
    writeInt(m_processFd, n);
}

void
RemotePluginClient::sendMIDIData(unsigned char *data, int *frameoffsets, int events)
{
    writeOpcode(m_processFd, RemotePluginSendMIDIData);
    writeInt(m_processFd, events);
    tryWrite(m_processFd, data, events * 3);

    if (!frameoffsets) {
	// This should not happen with a good client, but we'd better
	// cope as well as possible with the lazy ol' degenerates
	frameoffsets = (int *)alloca(events * sizeof(int));
	memset(frameoffsets, 0, events * sizeof(int));
    }

//    std::cerr << "RemotePluginClient::sendMIDIData(" << events << ")" << std::endl;

    tryWrite(m_processFd, frameoffsets, events * sizeof(int));
}

void
RemotePluginClient::process(float **inputs, float **outputs, int sampleFrames)
{
    if (m_bufferSize < 0) {
	std::cerr << "ERROR: RemotePluginClient::setBufferSize must be called before RemotePluginClient::process" << std::endl;
	return;
    }
    if (m_numInputs < 0) {
	std::cerr << "ERROR: RemotePluginClient::getInputCount must be called before RemotePluginClient::process" << std::endl;
	return;
    }
    if (m_numOutputs < 0) {
	std::cerr << "ERROR: RemotePluginClient::getOutputCount must be called before RemotePluginClient::process" << std::endl;
	return;
    }
    if (!m_shm) {
	std::cerr << "ERROR: RemotePluginClient::process: no shared memory region available" << std::endl;
	return;
    }

    size_t blocksz = m_bufferSize * sizeof(float);

    //!!! put counter in shm to indicate number of blocks processed?
    // (so we know if we've screwed up)

    for (int i = 0; i < m_numInputs; ++i) {
		memcpy(m_shm + i * blocksz, inputs[i], sampleFrames*sizeof(float));
    }

//    std::cout << "process: writing opcode " << RemotePluginProcess << std::endl;
    writeOpcode(m_processFd, RemotePluginProcess);
    writeInt(m_processFd, sampleFrames);
    int resp;
    while ((resp = readInt(m_processResponseFd)) != PluginProcessComplete) {
#ifndef MASTER_THREAD
		// this is currently a hack in which we use the process thread as opposed
		// the audiomasterthread to handle plugin to host requests
		// in remotepluginserver.cpp, we set AudioMasterRequestFd to processResponseFd
		// we suck on the processResponseFd until we get either a processComplete opcode
		// all other opcodes are assumed to plugin->host opcode
		switch (resp) {
		case audioMasterGetTime:
		{
			struct VstTimeInfo *timeInfo;
			VstTimeInfo junk;
			int val = readInt(m_processResponseFd);
			timeInfo = (struct VstTimeInfo *) m_audioMaster(NULL, audioMasterGetTime, 0, val, 0, 0);
			if (!timeInfo) {
				timeInfo = &junk;
			}
			write(m_AudioMasterResponseFd, timeInfo, sizeof(struct VstTimeInfo));
			break;
		}
		case audioMasterIdle:
		{
#if 0
			int j = 0;
			printf("got idle request\n");
			m_audioMaster(m_effect, audioMasterIdle, 0, 0, 0, 0);
			write(m_AudioMasterResponseFd, &j, 4);
#endif
			break;
		}
		default:
			break;
		}
#endif
	}

//    std::cout << "process: wrote opcode " << RemotePluginProcess << std::endl;

    for (int i = 0; i < m_numOutputs; ++i) {
		memcpy(outputs[i], m_shm + (i + m_numInputs) * blocksz, sampleFrames*sizeof(float));
    }
	

//   gettimeofday(&finish, 0);
//	std::cout << "process: time " << finish.tv_sec - start.tv_sec
//		  << " sec, " << finish.tv_usec - start.tv_usec << " usec"
//		  << std::endl;
    return;
}

void
RemotePluginClient::setDebugLevel(RemotePluginDebugLevel level)
{
    writeOpcode(m_controlRequestFd, RemotePluginSetDebugLevel);
    tryWrite(m_controlRequestFd, &level, sizeof(RemotePluginDebugLevel));
}

bool
RemotePluginClient::warn(std::string str)
{
    writeOpcode(m_controlRequestFd, RemotePluginWarn);
    writeString(m_controlRequestFd, str);
    bool b;
    tryRead(m_controlResponseFd, &b, sizeof(bool));
    return b;
}

void
RemotePluginClient::showGUI(std::string guiData)
{
    writeOpcode(m_controlRequestFd, RemotePluginShowGUI);
    writeString(m_controlRequestFd, guiData);
    bool b;
    tryRead(m_controlResponseFd, &b, sizeof(bool));
}    

void
RemotePluginClient::hideGUI()
{
    writeOpcode(m_controlRequestFd, RemotePluginHideGUI);
    bool b;
    tryRead(m_controlResponseFd, &b, sizeof(bool));
}

void
RemotePluginClient::effCloseOp()
{
    writeOpcode(m_controlRequestFd, RemotePluginClose);
	// we don't expect a response.....
}

void
RemotePluginClient::effVoidOp(int opcode)
{
    writeOpcode(m_controlRequestFd, RemotePluginDoVoid);
    writeInt(m_controlRequestFd, opcode);
    bool b;
    tryRead(m_controlResponseFd, &b, sizeof(bool));
}

int
RemotePluginClient::getFlags()
{
    writeOpcode(m_controlRequestFd, RemotePluginGetFlags);
    return readInt(m_controlResponseFd);
}

int RemotePluginClient::processVstEvents(VstEvents *evnts) 
{
    writeOpcode(m_controlRequestFd, RemotePluginProcessEvents);
	writeInt(m_controlRequestFd, evnts->numEvents);
	// for each event
	//   write size
	//   write data
	int i;
	for (i = 0; i < evnts->numEvents; i++) {
		unsigned int size = (2*sizeof(VstInt32)) + evnts->events[i]->byteSize;
		writeInt(m_controlRequestFd, size);
		tryWrite(m_controlRequestFd, evnts->events[i], size);
	}
	return readInt(m_controlResponseFd);
}

int RemotePluginClient::getChunk(void **ptr, int bank_prg)
{
	static void *chunk_ptr = 0;
	writeOpcode(m_controlRequestFd, RemotePluginGetChunk);
	writeInt(m_controlRequestFd, bank_prg);
	int sz = readInt(m_controlResponseFd);
	printf("chunk size %d\n", sz);
	if (chunk_ptr != 0) {
		free(chunk_ptr);
	}
	chunk_ptr = malloc(sz);
    tryRead(m_controlResponseFd, chunk_ptr, sz);
	*ptr = chunk_ptr;
	return sz;
}

int RemotePluginClient::setChunk(void *ptr, int sz, int bank_prg)
{
	writeOpcode(m_controlRequestFd, RemotePluginSetChunk);
	writeInt(m_controlRequestFd, sz);
	writeInt(m_controlRequestFd, bank_prg);
	tryWrite(m_controlRequestFd, ptr, sz);
	 
	return readInt(m_controlResponseFd);
}

int RemotePluginClient::canBeAutomated(int param)
{
	writeOpcode(m_controlRequestFd, RemotePluginCanBeAutomated);
	writeInt(m_controlRequestFd, param);
	return readInt(m_controlResponseFd);
}	

int RemotePluginClient::getProgram()
{
	writeOpcode(m_controlRequestFd, RemotePluginGetProgram);
	return readInt(m_controlResponseFd);
}


