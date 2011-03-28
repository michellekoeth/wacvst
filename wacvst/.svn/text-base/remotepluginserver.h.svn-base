/* -*- c-basic-offset: 4 -*- */

/*
  dssi-vst: a DSSI plugin wrapper for VST effects and instruments
  Copyright 2004-2007 Chris Cannam
*/

#ifndef REMOTE_PLUGIN_SERVER_H
#define REMOTE_PLUGIN_SERVER_H

#include "remoteplugin.h"
#include <string>

extern int m_AudioMasterRequestFd;
extern int m_AudioMasterResponseFd;
class RemotePluginServer
{
public:
    virtual ~RemotePluginServer();
    
    virtual bool         isReady() = 0;

    virtual std::string  getName() = 0;
    virtual std::string  getMaker() = 0;

    virtual void         setBufferSize(int) = 0;
    virtual void         setSampleRate(float) = 0;

    virtual void         reset() = 0;
    virtual void         terminate() = 0;
    
    virtual int          getInputCount() = 0;
    virtual int          getOutputCount() = 0;
	virtual int			 getFlags() { return 0; }
	virtual int			 processVstEvents() = 0;
	virtual void		 getChunk() = 0;
	virtual void		 setChunk() = 0;
	virtual void	     canBeAutomated() = 0;
	virtual void	     getProgram() = 0;

	virtual int			 getInitialDelay() = 0;
	virtual int			 getUniqueID() = 0;
	virtual int			 getVersion() = 0;

	virtual int          getParameterCount()                  { return 0; }
    virtual std::string  getParameterName(int)                { return ""; }
    virtual void         setParameter(int, float)             { return; }
    virtual float        getParameter(int)                    { return 0.0f; }
    virtual float        getParameterDefault(int)             { return 0.0f; }
    virtual void         getParameters(int p0, int pn, float *v) {
	for (int i = p0; i <= pn; ++i) v[i - p0] = 0.0f;
    }

    virtual int          getProgramCount()                    { return 0; }
    virtual int			 getProgramName(int, char *name)      { *name = 0; return 0; }
    virtual void         setCurrentProgram(int)               { return; }

    virtual bool         hasMIDIInput()                       { return false; }
    virtual void         sendMIDIData(unsigned char *data,
				      int *frameOffsets,
				      int events)             { return; }

    virtual int          getEffInt(int opcode) { return 0; }
    virtual std::string  getEffString(int opcode, int index) { return ""; }
    virtual void         effDoVoid(int opcode) {return;}

    virtual void         process(float **inputs, float **outputs, int sampleFrames) = 0;

    virtual void         setDebugLevel(RemotePluginDebugLevel) { return; } 
    virtual bool         warn(std::string) = 0;

    virtual void         showGUI(std::string guiData) { } 
    virtual void         hideGUI() { }
    virtual void         eff_mainsChanged(int v) = 0;
	
    void dispatch(int timeout = -1); // may throw RemotePluginClosedException
    void dispatchControl(int timeout = -1); // may throw RemotePluginClosedException
    void dispatchProcess(int timeout = -1); // may throw RemotePluginClosedException

protected:
    RemotePluginServer(std::string fileIdentifiers);
    int m_controlRequestFd;
    int m_controlResponseFd;

    void cleanup();

private:
    RemotePluginServer(const RemotePluginServer &); // not provided
    RemotePluginServer &operator=(const RemotePluginServer &); // not provided

    void dispatchControlEvents();
    void dispatchProcessEvents();

    int m_bufferSize;
    int m_numInputs;
    int m_numOutputs;
	int m_flags;

    /*    int m_AudioMasterRequestFd;
	  int m_AudioMasterResponseFd; */
    int m_processFd;
    int m_processResponseFd;
    int m_shmFd;

    char *m_controlRequestFileName;
    char *m_controlResponseFileName;
    char *m_AudioMasterRequestFileName;
    char *m_AudioMasterResponseFileName;
    char *m_processFileName;
    char *m_processResponseFileName;
    char *m_shmFileName;

    char *m_shm;
    size_t m_shmSize;

    float **m_inputs;
    float **m_outputs;

    RemotePluginDebugLevel m_debugLevel;

    void sizeShm();
};

#endif
