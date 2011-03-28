/* -*- c-basic-offset: 4 -*- */

/*
  dssi-vst: a DSSI plugin wrapper for VST effects and instruments
  Copyright 2004-2007 Chris Cannam
*/

#ifndef REMOTE_PLUGIN_H
#define REMOTE_PLUGIN_H

//#define MASTER_THREAD

#define FIXED_SHM_SIZE  (4 * 65536 * sizeof(float))
static const float RemotePluginVersion = 0.986;

enum RemotePluginDebugLevel {
    RemotePluginDebugNone,
    RemotePluginDebugSetup,
    RemotePluginDebugEvents,
    RemotePluginDebugData
};

enum RemotePluginOpcode {

    RemotePluginGetVersion = 0,
    RemotePluginGetName,
    RemotePluginGetMaker,

    RemotePluginSetBufferSize = 100,
    RemotePluginSetSampleRate,
    RemotePluginReset,
    RemotePluginTerminate,

    RemotePluginGetInputCount = 200,
    RemotePluginGetOutputCount,

    RemotePluginGetParameterCount = 300,
    RemotePluginGetParameterName,
    RemotePluginSetParameter,
    RemotePluginGetParameter,
    RemotePluginGetParameterDefault,
    RemotePluginGetParameters,

    RemotePluginGetProgramCount = 350,
    RemotePluginGetProgramName,
    RemotePluginSetCurrentProgram,

    RemotePluginHasMIDIInput = 400,
    RemotePluginSendMIDIData,

    RemotePluginProcess = 500,
    RemotePluginIsReady,

    RemotePluginSetDebugLevel = 600,
    RemotePluginWarn,

    RemotePluginShowGUI = 700,
    RemotePluginHideGUI,

    RemotePluginGetEffInt = 800,
    RemotePluginGetEffString,
    RemotePluginDoVoid,
	
	// vst specific opcodes
	RemotePluginGetFlags,
	RemotePluginProcessEvents,
	RemotePluginGetChunk,
	RemotePluginSetChunk,
	RemotePluginCanBeAutomated,
	RemotePluginGetProgram,
	RemotePluginClose,
	RemoteMainsChanged,
	RemotePluginGetUniqueID,
	RemotePluginGetInitialDelay,
	
    RemotePluginNoOpcode = 9999
};

enum PluginHostOpcodes {
	PluginProcessComplete = 1000
};

class RemotePluginClosedException { };

#endif
