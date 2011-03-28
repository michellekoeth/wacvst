/*
 *  wacvst.cp
 *  wacvst
 *
 *  Copyright 2009 retroware. All rights reserved.

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
    along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 */

#include "wacvst.h"
#include "wacvstPriv.h"


extern "C" {

#define VST_EXPORT	__attribute__ ((visibility ("default")))
//static AEffect theEffect;
static struct ERect fakeRect = {0, 0, 10, 10}; 
CFDictionaryRef get_props(CFBundleRef bundle);

/* adding a new opcode:
  define new opcode type in remoteplugin.h

  define/implement new method in remotepluginclient.h/remotepluginclient.cpp
    write opcode
	marshal arguments
	wait for response (do we need to??)
	marshal any response
	
	add a new case to handle opcode in remotepluginserver.cpp
	  if non-trivial:
	    add a virtual method definition to remotepluginserver.h and dssi-server.cpp
		provide implementation in dssi-server.cpp
   
    dssi-serever.cpp:
	   unmarshal args (could also happen in remotepluginserver.cpp)
	   m_plugin->....
	   write result back
*/
      
VstIntPtr dispatcher(AEffect* effect, VstInt32 opcode, VstInt32 index, VstIntPtr value, void* ptr, float opt) {
	RemotePluginClient *plugin = (RemotePluginClient *) effect->user;
	VstIntPtr v = 0;
//	printf("opcode %d\n", opcode);			
	switch (opcode)
	{
/*
		case effOpen: implemented
		case effClose: implemented
		case effSetProgram:	implemented
		case effGetProgram:	not implemented (not working)
		case effSetProgramName: not implmented
		case effGetProgramName: implmented	
		case effGetParamLabel: implemented		getParameterLabel (index, (char*)ptr);				break;
		case effGetParamDisplay: implemented	getParameterDisplay (index, (char*)ptr);			break;
		case effGetParamName: implemented	getParameterName (index, (char*)ptr);				break;

		case effSetSampleRate: implemented - check	setSampleRate (opt);								break;
		case effSetBlockSize: implmented	setBlockSize ((VstInt32)value);						break;
		case effMainsChanged: not implmented		if (!value) suspend (); else resume ();				break;

		//---Editor------------
		case effEditGetRect: implmented		if (editor) v = editor->getRect ((ERect**)ptr) ? 1 : 0;	break;
		case effEditOpen: implemented			if (editor) v = editor->open (ptr) ? 1 : 0;			break;
		case effEditClose: implemented		if (editor) editor->close ();						break;		
		case effEditIdle: implemented			if (editor) editor->idle ();						break;
		
		//---Persistence-------
		case effGetChunk: implemented			v = getChunk ((void**)ptr, index ? true : false);	break;
		case effSetChunk: implemented			v = setChunk (ptr, (VstInt32)value, index ? true : false);	break;

		effGetProgramNameIndexed: implemented

		3 - effGetProgram: implemented
		23 - effGetChunk: implemented
		24 - effSetChunk: implemented
		
	    12 - effMainsChanged
		26 - effCanBeAutomated
		33 - effGetInputProperties
		34 - effGetOutputProperties
		61 - effSetEditKnobMode
		62 - effGetCurrentMidiProgram
		66 - effGetMidiKeyName
		67 - effBeginSetProgram (on load)
		68 - effEndSetProgram (on load)
	    75 - effBeginLoadBank (on load)
		78 - effGetNumMidiInputChannels
		79 - effGetNumMidiOutputChannels
		
		
*/
		case effClose:
			plugin->effCloseOp();
			delete plugin;
			break;
		case effEditGetRect:
			*((struct ERect **)ptr) = &fakeRect;
			v = 1;
			break;
		case effEditIdle:
			plugin->effVoidOp(effEditIdle);
			break;
		case effStartProcess:
			plugin->effVoidOp(effStartProcess);
			break;
		case effStopProcess:
			plugin->effVoidOp(effStopProcess);
			v = 1;
			break;
		case effGetVendorString:
			strncpy((char *) ptr, plugin->getMaker().c_str(), kVstMaxVendorStrLen);
			((char *) ptr)[kVstMaxVendorStrLen-1] = 0;
			v = 1;
			break;
		case effGetEffectName:
			strncpy((char *) ptr, plugin->getName().c_str(), kVstMaxEffectNameLen);
			((char *) ptr)[kVstMaxEffectNameLen-1] = 0;
			v = 1;
			break;
		case effGetProgram:
			v = plugin->getProgram();
			break;
		case effGetParamName:
			strncpy((char *) ptr, plugin->getParameterName(index).c_str(), kVstMaxParamStrLen);
			((char *) ptr)[kVstMaxParamStrLen] = 0;
			v = 1;
			break;
		case effGetParamLabel:
			plugin->getEffString(effGetParamLabel, index, (char *) ptr, kVstMaxParamStrLen);
			v = 1;
			break;
		case effGetParamDisplay:
			plugin->getEffString(effGetParamDisplay, index, (char *) ptr, kVstMaxParamStrLen);
			// printf("value = %f display = %s\n", plugin->getParameter(index), ptr);
			v = 1;
			break;
		case effGetProgramNameIndexed:
		case effGetProgramName:
		{
			// getProgramName() handles both the indexed and non-indexed cases
			std::string s;
			int r;
			if (opcode == effGetProgramName) {
				r = plugin->getProgramName(-1, s);
			} else {
				r = plugin->getProgramName(index, s);
			}
			if (!r) {
				*((char *) ptr) = 0;
			}
			else {
				strncpy((char *) ptr, s.c_str(), kVstMaxProgNameLen);
				((char *) ptr)[kVstMaxProgNameLen-1] = 0;
			}
			v = r;
			break;
		}
		case effSetSampleRate:
			// todo: samplerate is actually a float in vst
			plugin->setSampleRate(opt);
			break;
		case effSetBlockSize:
			plugin->setBufferSize ((VstInt32)value);	
			break;
		case effMainsChanged:
			plugin->effMainsChanged ((VstInt32)value);	
			v = 1;
			break;
		case effGetVstVersion:
			v = kVstVersion;
			break;
		case effGetPlugCategory:
			v = plugin->getEffInt(effGetPlugCategory);
			break;
		case effSetProgram:
			if (value < plugin->getProgramCount()) plugin->setCurrentProgram ((VstInt32)value);
			break;
		case effGetChunk:
 			v = plugin->getChunk((void **) ptr, index);
			break;
		case effSetChunk:
 			v = plugin->setChunk(ptr, value, index);
			break;
		case effEditOpen:
			plugin->showGUI("");
			break;
		case effEditClose:
			plugin->hideGUI();
			break;
		case effCanDo:
			printf("saying we can do %s\n", (char *) ptr);
			v = 1;
			break;
		case effProcessEvents:
			v = plugin->processVstEvents((VstEvents *) ptr);
			break;
		default:
			printf("unsupported dispatcher opcode %d\n", opcode);
			break;
	}
//	printf("done with opcode\n");
	return v;
}

void processDouble(AEffect* effect, double** inputs, double** outputs, VstInt32 sampleFrames)
{
//	printf("in wrap process double\n");
	return;
}

void process(AEffect* effect, float** inputs, float** outputs, VstInt32 sampleFrames)
{
	RemotePluginClient *plugin = (RemotePluginClient *) effect->user;
	// todo: implement a process that takes a sampleframes parameter
	plugin->process(inputs, outputs, sampleFrames);
	return;
}

void setParameter(AEffect* effect, VstInt32 index, float parameter)
{
	RemotePluginClient *plugin = (RemotePluginClient *) effect->user;	
	plugin->setParameter(index, parameter);
	return;
}

float getParameter(AEffect* effect, VstInt32 index)
{
//	printf("in wrap get parameter\n");
	RemotePluginClient *plugin = (RemotePluginClient *) effect->user;
	
	return plugin->getParameter(index);
}

void initEffect(AEffect *eff, RemotePluginClient *plugin)
{
	memset(eff, 0x1, sizeof(AEffect));
	eff->magic = kEffectMagic;
	eff->dispatcher = dispatcher;
	eff->setParameter = setParameter;
	eff->getParameter = getParameter;
	eff->numInputs = plugin->getInputCount();
	eff->numOutputs = plugin->getOutputCount();
	eff->numPrograms = plugin->getProgramCount();
	eff->numParams = plugin->getParameterCount();
	eff->flags = plugin->getFlags();
//	eff->flags = effFlagsHasEditor | effFlagsIsSynth;
//	eff->flags = effFlagsHasEditor;
	eff->resvd1 = 0;
	eff->resvd2 = 0;

	eff->object = (void *) 0;
	eff->user = plugin;

	// todo: get these from client
	eff->initialDelay = plugin->getInitialDelay();
	eff->uniqueID = plugin->getUniqueID();
	eff->version = plugin->getVersion();

	eff->processReplacing = process;
	eff->processDoubleReplacing = processDouble;
}

void dumpit();

static const char *i_am = "com.retroware.wacvst";

VST_EXPORT AEffect* VSTPluginMain (audioMasterCallback audioMaster)
{
	CFStringRef cf_i_am = CFStringCreateWithCString(0, i_am, 0);
	CFBundleRef whoAmI = CFBundleGetBundleWithIdentifier(cf_i_am);
	CFDictionaryRef wacvst_props = get_props(whoAmI);
	if (wacvst_props == NULL) {
		return NULL;
	}
	CFStringRef pluginName =  (CFStringRef) CFDictionaryGetValue(wacvst_props, CFSTR("plugin-name"));
	if (pluginName == NULL) {
		return NULL;
	}

 	CFURLRef fullPath = CFBundleCopyResourceURL(whoAmI, pluginName, CFSTR("dll"), NULL);
	if (!fullPath) {
		return 0;
	}

	// Get VST Version of the Host
	if (!audioMaster (0, audioMasterVersion, 0, 0, 0, 0))
		return 0;  // old version

	RemotePluginClient *plugin;
	AEffect	*theEffect = (AEffect *) malloc(sizeof(AEffect));
	try {
		plugin = new RemoteVSTClient(whoAmI, audioMaster, theEffect, false);
		initEffect(theEffect, plugin);
		printf("plugin created\n");
#if 0
		struct VstTimeInfo *timeInfo;
		timeInfo = (struct VstTimeInfo *) audioMaster(0, audioMasterGetTime, 0, 514, 0, 0);
#endif
	} catch (std::string e) {
		return 0;
	}

	return theEffect;
}

CFDictionaryRef get_props(CFBundleRef bundle)
{
	SInt32 errorCode;
	CFDataRef data;
    CFPropertyListRef propertyList;
	CFURLRef configUrl = CFBundleCopyResourceURL(bundle, CFSTR("config"), CFSTR("plist"), NULL);
	if (configUrl == 0) {
		return NULL;
	}
	CFURLCreateDataAndPropertiesFromResource(kCFAllocatorDefault, configUrl, &data, NULL, NULL, &errorCode);
	propertyList = CFPropertyListCreateFromXMLData( kCFAllocatorDefault, data, kCFPropertyListImmutable, NULL);
	return (CFDictionaryRef) propertyList;
}

void dumpit() {
printf("effProcessEvents %d\n",effProcessEvents);
printf("effCanBeAutomated %d\n",effCanBeAutomated);
printf("effString2Parameter %d\n", effString2Parameter);
//printf("effGetNumProgramCategories %d\n", effGetNumProgramCategories);
printf("effGetProgramNameIndexed %d\n", effGetProgramNameIndexed);
//printf("effCopyProgram %d\n", effCopyProgram);
//printf("effConnectInput %d\n", effConnectInput);
//printf("effConnectOutput %d\n", effConnectOutput);
printf("effGetInputProperties %d\n", effGetInputProperties);
printf("effGetOutputProperties %d\n", effGetOutputProperties);
printf("effGetPlugCategory %d\n", effGetPlugCategory);
//printf("effGetCurrentPosition %d\n", effGetCurrentPosition);
//printf("effGetDestinationBuffer %d\n", effGetDestinationBuffer);
printf("effOfflineNotify %d\n", effOfflineNotify);
printf("effOfflinePrepare %d\n", effOfflinePrepare);
printf("effOfflineRun %d\n", effOfflineRun);
printf("effProcessVarIo %d\n", effProcessVarIo);
printf("effSetSpeakerArrangement %d\n", effSetSpeakerArrangement);
//printf("effSetBlockSizeAndSampleRate %d\n", effSetBlockSizeAndSampleRate);
printf("effSetBypass %d\n", effSetBypass);
printf("effGetEffectName %d\n", effGetEffectName);
//printf("effGetErrorText %d\n", effGetErrorText);
printf("effGetVendorString %d\n", effGetVendorString);
printf("effGetProductString %d\n", effGetProductString);
printf("effGetVendorVersion %d\n", effGetVendorVersion);
printf("effVendorSpecific %d\n", effVendorSpecific);
printf("effCanDo %d\n", effCanDo);
printf("effGetTailSize %d\n", effGetTailSize);
//printf("effIdle %d\n", effIdle);
//printf("effGetIcon %d\n", effGetIcon);
//printf("effSetViewPosition %d\n", effSetViewPosition);
printf("effGetParameterProperties %d\n", effGetParameterProperties);
//printf("effKeysRequired %d\n", effKeysRequired);
printf("effGetVstVersion %d\n", effGetVstVersion);
printf("effEditKeyDown %d\n", effEditKeyDown);
printf("effEditKeyUp %d\n", effEditKeyUp);
printf("effSetEditKnobMode %d\n", effSetEditKnobMode);
printf("effGetMidiProgramName %d\n", effGetMidiProgramName);
printf("effGetCurrentMidiProgram %d\n", effGetCurrentMidiProgram);
printf("effGetMidiProgramCategory %d\n", effGetMidiProgramCategory);
printf("effHasMidiProgramsChanged %d\n", effHasMidiProgramsChanged);
printf("effGetMidiKeyName %d\n", effGetMidiKeyName);
printf("effBeginSetProgram %d\n", effBeginSetProgram);
printf("effEndSetProgram %d\n", effEndSetProgram);
printf("effGetSpeakerArrangement %d\n", effGetSpeakerArrangement);
printf("effShellGetNextPlugin %d\n", effShellGetNextPlugin);
printf("effStartProcess %d\n", effStartProcess);
printf("effStopProcess %d\n", effStopProcess);
printf("effSetTotalSampleToProcess %d\n", effSetTotalSampleToProcess);
printf("effSetPanLaw %d\n", effSetPanLaw);
printf("effBeginLoadBank %d\n", effBeginLoadBank);
printf("effBeginLoadProgram %d\n", effBeginLoadProgram);
printf("effSetProcessPrecision %d\n", effSetProcessPrecision);
printf("effGetNumMidiInputChannels %d\n", effGetNumMidiInputChannels);
printf("effGetNumMidiOutputChannels %d\n", effGetNumMidiOutputChannels);
}

}
