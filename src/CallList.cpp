#include "../include/CallList.h"
#include <iostream>
#include <string>



#define SEPARATOR "<BR>"
#define WAIT_OBJECT_0 1
#define INFINITE -1

using namespace std;

//#define SEPARATOR ""

//void gera_log(string logText);

string trim(string str)
{
    return "";
}

string SomenteNumeros(string device)
{
        string tmpDevice;
        bool bFirstZero;

        bFirstZero = true;
        for(unsigned i = 1; i <= device.size();i++)
        {
                if(device[i] >= '0' && device[i] <= '9')
                {
                        if(device[i] == '0')
                        {
                                if(!bFirstZero)
                                        tmpDevice += device[i];
                        }else
                        {
                                tmpDevice += device[i];
                                bFirstZero = false;
                        }
                }

                if(device[i] == '>')
                    break;
        }

        if(tmpDevice.substr(1, 3) == "800")
                tmpDevice = string("0")+tmpDevice;

        return trim(tmpDevice);
}

string TCallList::LCIText(TLocalConnectionState lcs)
{
        switch(lcs)
        {
                case csNull:
                        return "Null";
                case csInitiated:
                        return "Initiated";
                case csAlerting:
                        return "Alerting";
                case csConnected:
                        return "Connected";
                case csHold:
                        return "Hold";
                case csQueued:
                        return "Queued";
                case csFail:
                        return "Fail";
                default:
                        return "(undefined)";
        }
}

string logObject(TAgentState AgentState, int nivel)
{
        string tempTab;
        string logText;
        string stateText;
        for(int t = 0;t<nivel;t++)
        {
                tempTab +="\t";
        }

        switch(AgentState)
        {
                case asAgentNotReady:
                        stateText = "AgentNotReady";
                        break;
                case asAgentNull:
                        stateText = "AgentNull";
                        break;
                case asAgentReady:
                        stateText = "AgentReady";
                        break;
                case asAgentBusy:
                        stateText = "AgentBusy";
                        break;
                case asAgentWorkingAfterCall:
                        stateText = "AgentWorkingAfterCall";
                        break;
        }

        return stateText;
}


string TCallList::IntToStr(int Arg)
{
	return "";
}

void TCallList::ClearCallsFromDevice(int crossRefId)
{
    TMonitorItem * tmpMonitorItem;

    sem_wait(&sema);
    {
        try
        {
            for(unsigned i =0;i < MonitorList.size();i++)
            {
                    tmpMonitorItem = (TMonitorItem *)MonitorList[i];
                    if(tmpMonitorItem->crossRefId == crossRefId)
                    {
                        tmpMonitorItem->Clear();
                    }
            }
        }catch(...)
        {
        }

    }
    sem_post(&sema);
}
void TCallList::ClearCallsFromDevice(string device)
{
    TMonitorItem * tmpMonitorItem;
    sem_wait(&sema);
    {
        try
        {
            for(unsigned i =0;i < MonitorList.size();i++)
            {
                    tmpMonitorItem = (TMonitorItem *)MonitorList[i];
                    if(tmpMonitorItem->device == device)
                    {
                        tmpMonitorItem->Clear();
                    }
            }
        }catch(...)
        {
        }
    }
    sem_post(&sema);
}

void TCallList::Clear()
{
    TMonitorItem * tmpMonitorItem;

    sem_wait(&sema);
    {
        try
        {

            for(unsigned i =0;i < MonitorList.size();i++)
            {
                    tmpMonitorItem = (TMonitorItem *)MonitorList[i];
                    delete tmpMonitorItem;
            }
            MonitorList.erase(MonitorList.begin(),MonitorList.end());
        }
        catch(...)
        {
        }

    }
    sem_post(&sema);
}

TCalls TCallList::GetCallsFromAgent(string agent)
{
        TCalls Calls;
        Calls.Count = 0;

        TMonitorItem * tmpMonitorItem;
        TCallItem * tmpCall;
        sem_wait(&sema);
        {
            try
            {
                if(GetMonitorItemByAgent(agent, &tmpMonitorItem))
                {
                        Calls.Count = tmpMonitorItem->CallList.size();
                        for(unsigned i = 0;i<tmpMonitorItem->CallList.size();i++)
                        {
                                tmpCall = (TCallItem * )tmpMonitorItem->CallList[i];
                                Calls.Connections[i] = *tmpCall;
                        }
                }
            }catch(...)
            {
            }
        }
        sem_post(&sema);
        return Calls;
}

TCalls TCallList::GetCallsFromDevice(string device)
{
        TCalls Calls;
        Calls.Count = 0;

        TMonitorItem * tmpMonitorItem;
        TCallItem * tmpCall;

        sem_wait(&sema);
        {
            try
            {

                if(GetMonitorItem(device, &tmpMonitorItem))
                {
                        Calls.Count = tmpMonitorItem->CallList.size();
                        for(unsigned i = 0;i<tmpMonitorItem->CallList.size();i++)
                        {
                                tmpCall = (TCallItem * )tmpMonitorItem->CallList[i];
                                Calls.Connections[i] = *tmpCall;
                        }
                }
            }catch(...)
            {
            }

        }
        sem_post(&sema);
        return Calls;
}

void TCallList::DeleteMonitorItem(string device)
{
        TMonitorItem * tmpMonitorItem;

        sem_wait(&sema);
        {
            try
            {
                for(unsigned i =0;i < MonitorList.size();i++)
                {
                        tmpMonitorItem = (TMonitorItem *)MonitorList[i];
                        if(tmpMonitorItem->device == device)
                        {
                                MonitorList.erase(MonitorList.begin()+i);
                                i--;
                                delete tmpMonitorItem;
                        }
                }
            }catch(...)
            {
            }
        }
        sem_post(&sema);
}


bool TCallList::MonitorExists(string device)
{
        TMonitorItem * tmpMonitorItem;
        bool mExists;

        mExists = false;

        sem_wait(&sema);
        {
            try
            {

                for(unsigned i =0;i < MonitorList.size();i++)
                {
                        tmpMonitorItem = (TMonitorItem *)MonitorList[i];
                        if(tmpMonitorItem->device == device)
                        {
                                mExists = true;
                        }
                }
            }catch(...)
            {
            }
        }
        sem_post(&sema);

        return mExists;
}

bool TCallList::MonitorExists(int crossRef)
{
        TMonitorItem * tmpMonitorItem;
        bool mExists;

        mExists = false;

        sem_wait(&sema);
        {
            try
            {

                for(unsigned i =0;i < MonitorList.size();i++)
                {
                        tmpMonitorItem = (TMonitorItem *)MonitorList[i];
                        if(tmpMonitorItem->crossRefId == crossRef)
                        {
                                mExists = true;
                        }
                }
            }catch(...)
            {
            }
        }
        sem_post(&sema);

        return mExists;
}

void TCallList::AddMonitorItem(string device, int crossRefId, TMonitorType monitorType)
{
    TMonitorItem * tmpMonitorItem;
    bool bFind = false;

    sem_wait(&sema);
    {
        try
        {

            for(unsigned  i = 0;i < MonitorList.size();i++)
            {
                tmpMonitorItem = (TMonitorItem *)MonitorList[i];

                if(tmpMonitorItem->crossRefId == crossRefId)
                {
                    tmpMonitorItem->device = device;
                    tmpMonitorItem->monitorType = monitorType;
                }
            }

            if(!bFind)
            {
                tmpMonitorItem = new TMonitorItem;
                tmpMonitorItem->crossRefId = crossRefId;
                tmpMonitorItem->device = device;
                tmpMonitorItem->monitorType = monitorType;
                tmpMonitorItem->AgentState = asAgentNull;

                MonitorList.push_back(tmpMonitorItem);
            }
        }catch(...)
        {
        }

    }
    sem_post(&sema);
}


bool TCallList::GetMonitorItemByAgent(string agent, TMonitorItem ** MonitorItem)
{
        TMonitorItem * tmpMonitorItem;
        bool result = false;

        sem_wait(&semaGetMonitor);
        {
            try
            {

                for(unsigned i =0;i < MonitorList.size();i++)
                {
                        tmpMonitorItem = (TMonitorItem *)MonitorList[i];
                        if(tmpMonitorItem->AgentID == agent)
                        {
                                *MonitorItem = tmpMonitorItem;
                                result = true;
                                break;
                        }
                }
            }catch(...)
            {
            }
        }
        sem_post(&semaGetMonitor);
        return result;
}

bool TCallList::GetMonitorItem(string device, TMonitorItem ** MonitorItem)
{
        TMonitorItem * tmpMonitorItem;
        bool result = false;
        
        sem_wait(&semaGetMonitor);
        {
            try
            {
                for(unsigned i =0;i < MonitorList.size();i++)
                {
                        tmpMonitorItem = (TMonitorItem *)MonitorList[i];
                        if(tmpMonitorItem->device == device)
                        {
                                *MonitorItem = tmpMonitorItem;
                                result = true;
                                break;
                        }
                }
            }catch(...)
            {
            }

        }
        sem_post(&semaGetMonitor);
        return result;
}

bool TCallList::GetMonitorItem(int crossRefId, TMonitorItem ** MonitorItem)
{
        TMonitorItem * tmpMonitorItem;
        bool result = false;
        sem_wait(&semaGetMonitor);
        {
            try
            {
                for(unsigned i =0;i < MonitorList.size();i++)
                {
                        tmpMonitorItem = (TMonitorItem *)MonitorList[i];
                        if(tmpMonitorItem->crossRefId == crossRefId)
                        {
                                *MonitorItem = tmpMonitorItem;
                                result = true;
                                break;
                        }
                }
            }catch(...)
            {
            }

        }
        sem_post(&semaGetMonitor);
        return result;
}


string TCallList::GetDeviceByMonitorId(int crossRefIdentifier)
{
        TMonitorItem * tmpMonitor;

        if(GetMonitorItem(crossRefIdentifier, &tmpMonitor))
        {
                return tmpMonitor->device;
        }
        return "";
}

int TCallList::GetMonitorIdByDevice(string device)
{
        TMonitorItem * tmpMonitor;

        if(GetMonitorItem(device, &tmpMonitor))
        {
                return tmpMonitor->crossRefId;
        }
        return -1;
}

void TCallList::SetLogicalDeviceState(string device, TAgentState agentState, string agentId)
{
        TMonitorItem * tmpMonitor;

        if(GetMonitorItem(device, &tmpMonitor))
        {
                tmpMonitor->AgentState = agentState;
                if(trim(agentId) != "")
                {
                    tmpMonitor->AgentID = agentId;
                    tmpMonitor->lastLogicalEvent = time(NULL);
                }
        }
}

void TCallList::SetLogicalDeviceState(int crossRefIdentifier, TAgentState AgentState, string AgentGroup, string AgentID)
{
        TMonitorItem * tmpMonitor;

        if(GetMonitorItem(crossRefIdentifier, &tmpMonitor))
        {
                tmpMonitor->AgentState = AgentState;
                if(trim(AgentGroup) != "")
                {
                    tmpMonitor->AgentGroup = AgentGroup;
                }
                if(trim(AgentID) != "")
                {
                    tmpMonitor->AgentID = AgentID;
                }
                tmpMonitor->lastLogicalEvent = time(NULL);
        }
}

bool TCallList::IsDeviceInCall(string device)
{
        bool inCall;
        TCallItem * tmpCall;
        TMonitorItem * tmpMonitor;

        inCall = false;

        sem_wait(&sema);
        {
            try
            {
                if(GetMonitorItem(device, &tmpMonitor))
                {
                        for(unsigned i = 0;i < tmpMonitor->CallList.size() && !inCall;i++)
                        {
                                tmpCall = (TCallItem *)tmpMonitor->CallList[i];
                                if(tmpCall->localConnectionInfo == csConnected)
                                {
                                        inCall = true;
                                }else if(tmpCall->localConnectionInfo == csHold)
                                {
                                        inCall = true;
                                }
                        }
                }
            }catch(...)
            {
            }

        }
        sem_post(&sema);

        return inCall;
}


string TCallList::GetSourceByDevice(string device)
{
        TMonitorItem * tmpMonitorItem;
        TCallItem * tmpCall;
        string result;
        sem_wait(&sema);
        {
            try
            {
                if(GetMonitorItem(device, &tmpMonitorItem))
                {
                        for(unsigned i = 0;i<tmpMonitorItem->CallList.size();i++)
                        {
                                tmpCall = (TCallItem * )tmpMonitorItem->CallList[i];
                                if(tmpMonitorItem->monitorType == mtDevice)
                                {
                                        if(tmpCall->source)
                                        {
                                                if(tmpCall->localConnectionInfo != csInitiated)
                                                        result = tmpMonitorItem->device;
                                        }
                                        else
                                                if(tmpCall->localConnectionInfo != csInitiated)
                                                        result = tmpCall->connectedDevice;
                                }else
                                {
                                        result = tmpCall->strSrc;
                                }

                        }
                }
            }catch(...)
            {
            }

        }
        sem_post(&sema);
        return result;
}

string TCallList::GetDestinationByDevice(string device)
{
        TMonitorItem * tmpMonitorItem;
        TCallItem * tmpCall;
        string result;

        sem_wait(&sema);
        {
            try
            {
                if(GetMonitorItem(device, &tmpMonitorItem))
                {
                        for(unsigned i = 0;i<tmpMonitorItem->CallList.size();i++)
                        {
                                tmpCall = (TCallItem * )tmpMonitorItem->CallList[i];
                                if(tmpMonitorItem->monitorType == mtDevice)
                                {
                                        if(tmpCall->source)
                                        {
                                                if(tmpCall->localConnectionInfo != csInitiated)
                                                        result = tmpCall->connectedDevice;
                                        }
                                        else
                                                if(tmpCall->localConnectionInfo != csInitiated)
                                                        result = tmpMonitorItem->device;
                                }else
                                {
                                        result = tmpCall->strDest;
                                }
                        }
                }
            }catch(...)
            {
            }

        }
        sem_post(&sema);
        return result;
}

string TCallList::GetCalledByDevice(string device)
{
        TMonitorItem * tmpMonitorItem;
        TCallItem * tmpCall;
        string result;

        sem_wait(&sema);
        {
            try
            {
                if(GetMonitorItem(device, &tmpMonitorItem))
                {
                        for(unsigned i = 0;i<tmpMonitorItem->CallList.size();i++)
                        {
                                tmpCall = (TCallItem * )tmpMonitorItem->CallList[i];
                                if(tmpCall->localConnectionInfo != csInitiated)
                                        result = tmpCall->calledDevice;
                        }
                }
            }catch(...)
            {
            }

        }
        sem_post(&sema);
        return result;
}

TAgentState TCallList::GetAgentStateByAgentId(string agent)
{
        TMonitorItem * tmpMonitorItem;
        TAgentState result = asAgentNull;
        sem_wait(&sema);
        {
            try
            {
                if(GetMonitorItemByAgent(agent, &tmpMonitorItem))
                {
                        result = tmpMonitorItem->AgentState;
                }
            }catch(...)
            {
            }

        }
        sem_post(&sema);
        return result;
}

TAgentState TCallList::GetAgentStateByDevice(string device)
{
        TMonitorItem * tmpMonitorItem;
        TAgentState result = asAgentNull;
        sem_wait(&sema);
        {
            try
            {
                if(GetMonitorItem(device, &tmpMonitorItem))
                {
                        result = tmpMonitorItem->AgentState;
                }
            }catch(...)
            {
            }

        }
        sem_post(&sema);

        return result;
}

string TCallList::GetACDGroupByDevice(string device)
{
        TMonitorItem * tmpMonitorItem;
        string result;
        sem_wait(&sema);
        {
            try
            {
                if(GetMonitorItem(device, &tmpMonitorItem))
                {
                        result = tmpMonitorItem->AgentGroup;
                }
            }catch(...)
            {
            }

        }
        sem_post(&sema);
        return result;
}

string TCallList::GetAgentIDByDevice(string device)
{
        TMonitorItem * tmpMonitorItem;
        string result;

        {
            try
            {
                if(GetMonitorItem(device, &tmpMonitorItem))
                {
                        result = tmpMonitorItem->AgentID;
                }
            }catch(...)
            {
            }

        }
        return result;
}

string TCallList::logCalls()
{
        string result;
        TMonitorItem * tmpMonitorItem;
         for(unsigned i =0;i < MonitorList.size();i++)
        {
                tmpMonitorItem = (TMonitorItem *)MonitorList[i];
                result += logCallDevice(tmpMonitorItem->device)+"\n";
        }

        return result;
}

string TCallList::logCallDevice(string device)
{
        TMonitorItem * tmpMonitorItem;
        string result;
        result = "CallList Device "+device+": \n"+SEPARATOR+"\r\n";
        if(GetMonitorItem(device, &tmpMonitorItem))
        {
                if(tmpMonitorItem->AgentState != asAgentNull)
                {
                        result += "\tAgente "+tmpMonitorItem->AgentID+" logado no grupo "+tmpMonitorItem->AgentGroup+" Estado: "+logObject(tmpMonitorItem->AgentState, 0)+SEPARATOR+"\r\n";
                }

                result += "\n";
                for(unsigned i = 0;i < tmpMonitorItem->CallList.size();i++)
                {
                        TCallItem * tmpCall;

                        tmpCall = (TCallItem *)tmpMonitorItem->CallList[i];
                        result += "\tCall - R:"+device+" -> C:"+IntToStr(tmpCall->callId)+" : "+LCIText(tmpCall->localConnectionInfo)+"\n\tSource: "+GetCallSource(tmpMonitorItem->crossRefId, tmpCall->callId)+" Destination: "+GetCallDestination(tmpMonitorItem->crossRefId, tmpCall->callId)+"\n\n"+SEPARATOR;
                }

                result += SEPARATOR;
        }
        return result;
}

void TCallList::SetCalledDevice(int crossRefIdentifier, int callId, string calledDevice)
{
        TMonitorItem * tmpMonitorItem;
        TCallItem * tmpCall;
        calledDevice = SomenteNumeros(calledDevice);
        if(GetMonitorItem(crossRefIdentifier, &tmpMonitorItem))
        {
                for(unsigned i = 0;i<tmpMonitorItem->CallList.size();i++)
                {
                        tmpCall = (TCallItem * )tmpMonitorItem->CallList[i];
                        if(tmpCall->callId == callId)
                        {
                                tmpCall->calledDevice = calledDevice;
                                break;
                        }
                }
        }
}


void TCallList::SetAssociatedDevice(int crossRefIdentifier, int callId, string associatedDevice)
{
        TMonitorItem * tmpMonitorItem;
        TCallItem * tmpCall;
        associatedDevice = SomenteNumeros(associatedDevice);
        if(GetMonitorItem(crossRefIdentifier, &tmpMonitorItem))
        {
                for(unsigned i = 0;i<tmpMonitorItem->CallList.size();i++)
                {
                        tmpCall = (TCallItem * )tmpMonitorItem->CallList[i];
                        if(tmpCall->callId == callId)
                        {
                                if(associatedDevice != "")
                                        tmpCall->associatedDevice = associatedDevice;
                                break;
                        }
                }
        }
}

void TCallList::SetConnectedDevice(int crossRefIdentifier, int callId, string connectedDevice)
{
        TMonitorItem * tmpMonitorItem;
        TCallItem * tmpCall;
        connectedDevice = SomenteNumeros(connectedDevice);
        if(GetMonitorItem(crossRefIdentifier, &tmpMonitorItem))
        {
                for(unsigned i = 0;i<tmpMonitorItem->CallList.size();i++)
                {
                        tmpCall = (TCallItem * )tmpMonitorItem->CallList[i];
                        if(tmpCall->callId == callId)
                        {
                                if(tmpMonitorItem->device != connectedDevice && connectedDevice != "")
                                        tmpCall->connectedDevice = connectedDevice;
                                break;
                        }
                }
        }
}

void TCallList::SetCallSrc(int crossRefIdentifier, int callId, string strSrc)
{
        TMonitorItem * tmpMonitorItem;
        TCallItem * tmpCall;
        strSrc = SomenteNumeros(strSrc);
        if(GetMonitorItem(crossRefIdentifier, &tmpMonitorItem))
        {
                for(unsigned i = 0;i<tmpMonitorItem->CallList.size();i++)
                {
                        tmpCall = (TCallItem * )tmpMonitorItem->CallList[i];
                        if(tmpCall->callId == callId)
                        {
                                tmpCall->strSrc = strSrc;
                                break;
                        }
                }
        }
}

void TCallList::SetCallDest(int crossRefIdentifier, int callId, string strDest)
{
        TMonitorItem * tmpMonitorItem;
        TCallItem * tmpCall;
        strDest = SomenteNumeros(strDest);
        if(GetMonitorItem(crossRefIdentifier, &tmpMonitorItem))
        {
                for(unsigned i = 0;i<tmpMonitorItem->CallList.size();i++)
                {
                        tmpCall = (TCallItem * )tmpMonitorItem->CallList[i];
                        if(tmpCall->callId == callId)
                        {
                                tmpCall->strDest = strDest;
                                break;
                        }
                }
        }
}



void TCallList::SetCallSource(int crossRefIdentifier, int callId)
{
        TMonitorItem * tmpMonitorItem;
        TCallItem * tmpCall;
        if(GetMonitorItem(crossRefIdentifier, &tmpMonitorItem))
        {
                for(unsigned i = 0;i<tmpMonitorItem->CallList.size();i++)
                {
                        tmpCall = (TCallItem * )tmpMonitorItem->CallList[i];
                        if(tmpCall->callId == callId)
                        {
                                tmpCall->source = true;
                                break;
                        }
                }
        }
}

string TCallList::GetCallDestination(int crossRefIdentifier, int callId)
{
        TMonitorItem * tmpMonitorItem;
        TCallItem * tmpCall;
        string result;

        sem_wait(&sema);
        {
                try
                {
                        if(GetMonitorItem(crossRefIdentifier, &tmpMonitorItem))
                        {
                                for(unsigned i = 0;i<tmpMonitorItem->CallList.size();i++)
                                {
                                        tmpCall = (TCallItem * )tmpMonitorItem->CallList[i];
                                        if(tmpCall->callId == callId)
                                        {
                                                if(tmpMonitorItem->monitorType == mtTrunk)
                                                {
                                                        result = tmpCall->strDest;
                                                }else if(tmpMonitorItem->monitorType == mtDevice)
                                                {
                                                        if(tmpCall->source)
                                                        {
                                                                if(tmpCall->localConnectionInfo != csInitiated)
                                                                        result = tmpCall->connectedDevice;
                                                        }
                                                        else
                                                                if(tmpCall->localConnectionInfo != csInitiated)
                                                                        result = tmpMonitorItem->device;
                                                }
                                        }
                                }
                        }
                }catch(...)
                {
                }

        }
        sem_post(&sema);

        return result;
}

string TCallList::GetCallSource(int crossRefIdentifier, int callId)
{
        TMonitorItem * tmpMonitorItem;
        TCallItem * tmpCall;
        string result;

        sem_wait(&sema);
        {
                try
                {

                        if(GetMonitorItem(crossRefIdentifier, &tmpMonitorItem))
                        {
                                for(unsigned i = 0;i<tmpMonitorItem->CallList.size();i++)
                                {
                                        tmpCall = (TCallItem * )tmpMonitorItem->CallList[i];
                                        if(tmpCall->callId == callId)
                                        {
                                                if(tmpMonitorItem->monitorType == mtTrunk)
                                                {
                                                        result = tmpCall->strSrc;
                                                }else if(tmpMonitorItem->monitorType == mtDevice)
                                                {
                                                        if(tmpCall->source)
                                                        {
                                                                if(tmpCall->localConnectionInfo != csInitiated)
                                                                        result = tmpMonitorItem->device;
                                                        }
                                                        else
                                                                if(tmpCall->localConnectionInfo != csInitiated)
                                                                        result = tmpCall->connectedDevice;
                                                }
                                        }
                                }
                        }
                }catch(...)
                {
                }

        }
        sem_post(&sema);
        return result;
}

void TCallList::SetConnectionState(int crossRefIdentifier, int callId, TLocalConnectionState localConnectionInfo)
{
        if(localConnectionInfo == csNull)
        {
                DelCall(crossRefIdentifier, callId);
        }else if(localConnectionInfo != csFail)
        {
                AddCall(crossRefIdentifier, callId, localConnectionInfo);
        }else
        {
                UpdateCall(crossRefIdentifier, callId, localConnectionInfo);
        }
}

void TCallList::DelCall(int crossRefIdentifier, int callId)
{
        TMonitorItem * tmpMonitor;
        if(GetMonitorItem(crossRefIdentifier, &tmpMonitor))
        {
                TCallItem * tmpCall;
                for(unsigned i = 0;i < tmpMonitor->CallList.size();i++)
                {
                        tmpCall = (TCallItem *)tmpMonitor->CallList[i];

                        if(tmpCall->callId == callId)
                        {
                                if(tmpCall->connectSet)
                                {
                                    tmpMonitor->lastLogicalEvent = time(NULL);
                                }
                                tmpMonitor->CallList.erase(tmpMonitor->CallList.begin()+i);
                                if(hasEndCallCbk)
                                {
                                    try
                                    {
                                        cbkEndCall(tmpCall, ptrEndCallArg);
                                    }
                                    catch(...)
                                    {
                                        //gera_log("CallList Falha OnEndCall ");
                                    }
                                }
                                delete tmpCall;
                                i--;
                        }
                }
        }
}

void TCallList::DelCall(string device, int callId)
{
        TMonitorItem * tmpMonitor;
        if(GetMonitorItem(device, &tmpMonitor))
        {
                TCallItem * tmpCall;
                for(unsigned i = 0;i < tmpMonitor->CallList.size();i++)
                {
                        tmpCall = (TCallItem *)tmpMonitor->CallList[i];

                        if(tmpCall->callId == callId)
                        {
                                if(hasEndCallCbk)
                                {
                                    try
                                    {
                                        cbkEndCall(tmpCall, ptrEndCallArg);
                                    }catch(...)
                                    {
                                        //gera_log("CallList Falha OnEndCall ");
                                    }
                                }
                                tmpMonitor->CallList.erase(tmpMonitor->CallList.begin()+i);
                                delete tmpCall;
                                i--;
                        }
                }
        }
}

void TCallList::DelCall(int crossRefIdentifier, int callId, string releasingDevice)
{
        TMonitorItem * tmpMonitor;
        if(GetMonitorItem(crossRefIdentifier, &tmpMonitor))
        {
                TCallItem * tmpCall;
                for(unsigned i = 0;i < tmpMonitor->CallList.size();i++)
                {
                        tmpCall = (TCallItem *)tmpMonitor->CallList[i];

                        if(tmpCall->callId == callId && tmpCall->connectedDevice == releasingDevice)
                        {
                                if(hasEndCallCbk)
                                {
                                    try
                                    {
                                        cbkEndCall(tmpCall, ptrEndCallArg);
                                    }
                                    catch(...)
                                    {
                                        //gera_log("CallList Falha OnEndCall ");
                                    }
                                }
                                tmpMonitor->CallList.erase(tmpMonitor->CallList.begin()+i);


                                delete tmpCall;
                                i--;
                        }
                }
        }
}

void TCallList::ClearInitiated(int crossRefIdentifier)
{
        TMonitorItem * tmpMonitor;

        if(GetMonitorItem(crossRefIdentifier, &tmpMonitor))
        {
                TCallItem * tmpCall;

                for(unsigned i = 0;i < tmpMonitor->CallList.size();i++)
                {
                        tmpCall = (TCallItem *)tmpMonitor->CallList[i];

                        if(tmpCall->localConnectionInfo == csInitiated)
                        {
                        		tmpMonitor->CallList.erase(tmpMonitor->CallList.begin()+i);
                                delete tmpCall;
                                i--;
                        }
                }

        }
}

void TCallList::ClearDelivered(int crossRefIdentifier, int currentCallId)
{
        TMonitorItem * tmpMonitor;

        if(GetMonitorItem(crossRefIdentifier, &tmpMonitor))
        {
                TCallItem * tmpCall;

                for(unsigned i = 0;i < tmpMonitor->CallList.size();i++)
                {
                        tmpCall = (TCallItem *)tmpMonitor->CallList[i];

                        if(tmpCall->localConnectionInfo == csAlerting && tmpCall->callId != currentCallId)
                        {
                        	tmpMonitor->CallList.erase(tmpMonitor->CallList.begin()+i);
                            delete tmpCall;
                            i--;
                        }
                }

        }
}

void TCallList::ClearEstablished(int crossRefIdentifier, int currentCallId)
{
        TMonitorItem * tmpMonitor;

        if(GetMonitorItem(crossRefIdentifier, &tmpMonitor))
        {
                TCallItem * tmpCall;

                for(unsigned i = 0;i < tmpMonitor->CallList.size();i++)
                {
                        tmpCall = (TCallItem *)tmpMonitor->CallList[i];

                        if(tmpCall->localConnectionInfo == csConnected && tmpCall->callId != currentCallId)
                        {
                        	tmpMonitor->CallList.erase(tmpMonitor->CallList.begin()+i);
                            delete tmpCall;
                            i--;
                        }
                }

        }
}

TCallItem * TCallList::GetCall(int crossRefId,int callId)
{
        TMonitorItem * tmpMonitor;
        TCallItem * ptrCall=NULL;

        ptrCall = NULL;
        if(GetMonitorItem(crossRefId, &tmpMonitor) && callId != 0)
        {
                TCallItem * tmpCall;

                for(unsigned i = 0;i < tmpMonitor->CallList.size();i++)
                {
                        tmpCall = (TCallItem *)tmpMonitor->CallList[i];

                        if(tmpCall->callId == callId)
                        {
                                ptrCall = tmpCall;
                        }
                }
        }

        return ptrCall;
}

TCallItem * TCallList::GetCall(string device,int callId)
{
        TMonitorItem * tmpMonitor;
        TCallItem * ptrCall;

        ptrCall = NULL;
        if(GetMonitorItem(device, &tmpMonitor) && callId != 0)
        {
                TCallItem * tmpCall;

                for(unsigned i = 0;i < tmpMonitor->CallList.size();i++)
                {
                        tmpCall = (TCallItem *)tmpMonitor->CallList[i];

                        if(tmpCall->callId == callId)
                        {
                                ptrCall = tmpCall;
                        }
                }
        }

        return ptrCall;
}

TCallItem TCallList::GetCallItem(string device, int callId)
{
    TMonitorItem * tmpMonitor;
    TCallItem result;



    if(GetMonitorItem(device, &tmpMonitor) && callId != 0)
    {
            TCallItem * tmpCall;

            for(unsigned i = 0;i < tmpMonitor->CallList.size();i++)
            {
                    tmpCall = (TCallItem *)tmpMonitor->CallList[i];

                    if(tmpCall->callId == callId)
                    {
                            result = *tmpCall;
                    }
            }
    }

    return result;
}

void TCallList::AddCall(int crossRefIdentifier, int callId, TLocalConnectionState localConnectionInfo)
{
        TMonitorItem * tmpMonitor;
        bool callExists;

        callExists = false;

        if(GetMonitorItem(crossRefIdentifier, &tmpMonitor) && callId != 0)
        {
                TCallItem * tmpCall;

                for(unsigned i = 0;i < tmpMonitor->CallList.size();i++)
                {
                        tmpCall = (TCallItem *)tmpMonitor->CallList[i];

                        if(tmpCall->callId == callId)
                        {
                                tmpCall->localConnectionInfo = localConnectionInfo;
                                if(tmpCall->localConnectionInfo == csConnected)
                                {
                                        if(!tmpCall->connectSet)
                                        {
                                                tmpCall->connectSet = true;
                                                tmpCall->tsConnect = time(NULL);
                                        }
                                        tmpMonitor->lastLogicalEvent = time(NULL);
                                }
                                callExists = true;
                        }
                }

                if(localConnectionInfo == csConnected)
                {
                    ClearEstablished(crossRefIdentifier, callId);
                }

                if(!callExists)
                {
                        tmpCall = new TCallItem;
                        tmpCall->callId = callId;
                        tmpCall->localConnectionInfo = localConnectionInfo;
                        tmpCall->source = false;
                        tmpCall->tarifacaoSet = false;
                        tmpCall->bDiverted = false;
                        tmpCall->queueSet = false;
                        tmpCall->Device = tmpMonitor->device;
                        tmpCall->tsCreation = time(NULL);
                        if(tmpCall->localConnectionInfo == csConnected)
                        {
                                tmpCall->connectSet = true;
                                tmpCall->tsConnect = time(NULL);
                        }
                        else
                                tmpCall->connectSet = false;
                        tmpMonitor->CallList.push_back(tmpCall);
                }
        }
}
void TCallList::UpdateCall(int crossRefIdentifier, int callId, TLocalConnectionState localConnectionInfo)
{
        TMonitorItem * tmpMonitor;
        if(GetMonitorItem(crossRefIdentifier, &tmpMonitor) && callId != 0)
        {
                TCallItem * tmpCall;

                for(unsigned i = 0;i < tmpMonitor->CallList.size();i++)
                {
                        tmpCall = (TCallItem *)tmpMonitor->CallList[i];

                        if(tmpCall->callId == callId)
                        {
                                tmpCall->localConnectionInfo = localConnectionInfo;
                        }
                }
        }
}

void TCallList::AddConferencedCall(int crossRefIdentifier, int callId, int oldCallId, TLocalConnectionState localConnectionInfo, string connectedId)
{
        TMonitorItem * tmpMonitor;
        bool bAddCall;

        if(trim(connectedId).size() > 1)
        {

                bAddCall = true;

                if(GetMonitorItem(crossRefIdentifier, &tmpMonitor) && callId != 0)
                {

                        if(connectedId != tmpMonitor->device)
                        {
                                TCallItem * tmpCall;

                                for(unsigned i = 0;i < tmpMonitor->CallList.size();i++)
                                {
                                        tmpCall = (TCallItem *)tmpMonitor->CallList[i];
                                        if(tmpCall->localConnectionInfo != csConferenced)
                                        {
                                                if(tmpCall->callId == callId)
                                                {
                                                        tmpCall->localConnectionInfo = localConnectionInfo;
                                                        tmpCall->connectedDevice =  connectedId;
                                                        if(!tmpCall->connectSet)
                                                        {
                                                                tmpCall->connectSet = true;
                                                                tmpCall->tsConnect = time(NULL);
                                                        }
                                                        bAddCall = false;
                                                }else if(tmpCall->callId == oldCallId)
                                                {
                                                        tmpCall->callId = callId;
                                                        tmpCall->localConnectionInfo = localConnectionInfo;
                                                        if(tmpCall->localConnectionInfo == csConnected || tmpCall->localConnectionInfo == csConferenced)
                                                        {
                                                                if(!tmpCall->connectSet)
                                                                {
                                                                        tmpCall->connectSet = true;
                                                                        tmpCall->tsConnect = time(NULL);
                                                                }
                                                        }

                                                        bAddCall = false;
                                                }
                                        }else if(tmpCall->connectedDevice == trim(connectedId))
                                        {
                                                bAddCall = false;
                                        }
                                }

                                if(bAddCall)
                                {
                                        tmpCall = new TCallItem;
                                        tmpCall->callId = callId;
                                        tmpCall->localConnectionInfo = localConnectionInfo;
                                        tmpCall->source = false;
                                        tmpCall->tarifacaoSet = false;
                                        tmpCall->bDiverted = false;
                                        tmpCall->queueSet = false;
                                        tmpCall->Device = tmpMonitor->device;
                                        tmpCall->tsCreation = time(NULL);
                                        tmpCall->connectedDevice = connectedId;
                                        if(tmpCall->localConnectionInfo == csConnected || tmpCall->localConnectionInfo == csConferenced)
                                        {
                                                tmpCall->connectSet = true;
                                                tmpCall->tsConnect = time(NULL);
                                        }
                                        else
                                                tmpCall->connectSet = false;

                                        tmpMonitor->CallList.push_back(tmpCall);
                                }

                        }
                }
        }
} 

TCallItem * TCallList::UpdateCalls(TLogicalDeviceEventType eventType, TLogicalDeviceEvent * Event, int crossRefIdentifier)
{
        TCallItem * Call = NULL;

        sem_wait(&sema);
        {
            try
            {
                switch(eventType)
                {
                        case evAgentBusy:
                                SetLogicalDeviceState(crossRefIdentifier, asAgentBusy, Event->AgentBusy.ACDGroup.DialingNumber, Event->AgentBusy.AgentID);
                                break;
                        case evAgentLoggedOn:
                                SetLogicalDeviceState(crossRefIdentifier, asAgentReady, Event->AgentLoggedOn.ACDGroup.DialingNumber, Event->AgentLoggedOn.AgentID);
                                break;
                        case evAgentLoggedOff:
                                SetLogicalDeviceState(crossRefIdentifier, asAgentNull, Event->AgentLoggedOff.ACDGroup.DialingNumber, Event->AgentLoggedOff.AgentID);
                                break;
                        case evAgentNotReady:
                                SetLogicalDeviceState(crossRefIdentifier, asAgentNotReady, Event->AgentNotReady.ACDGroup.DialingNumber, Event->AgentNotReady.AgentID);
                                break;
                        case evAgentReady:
                                SetLogicalDeviceState(crossRefIdentifier, asAgentReady, Event->AgentReady.ACDGroup.DialingNumber, Event->AgentReady.AgentID);
                                break;
                        case evAgentWorkingAfterCall:
                                SetLogicalDeviceState(crossRefIdentifier, asAgentWorkingAfterCall, Event->AgentWorkingAfterCall.ACDGroup.DialingNumber, Event->AgentWorkingAfterCall.AgentID);
                                break;
                        case evRouteingMode:
                        case evForwarding:
                        case evDoNotDisturb:
                        case evCallerIDStatus:
                        case evCallbackMessage:
                        case evCallback:
                        case evAutoWorkMode:
                        case evAutoAnswer:
                                break;
                }
            }catch(...)
            {
            }

        }
        sem_post(&sema);
        return Call;
}

TCallItem *  TCallList::UpdateCalls(TCallControlEventType eventType, TCallControlEvent * Event, int crossRefIdentifier)
{
        TCallItem * Call = NULL;
        
        sem_wait(&sema);
        {
            try
            {
                switch(eventType)
                {
                		case evOffered:
                		case evNetworkCapabilitiesChange:
                		case evDigitsDialed:
                		case evCallCleared:
                		case evBridged:
                				break;
                        case evConferenced:
                                Call = SetConferenced(Event, crossRefIdentifier);
                                break;
                        case evConnectionCleared:
                                Call = SetConnectionCleared(Event, crossRefIdentifier);
                                break;
                        case evDelivered:
                                Call = SetDelivered(Event, crossRefIdentifier);
                                break;
                        case evDiverted:
                                Call = SetDiverted(Event, crossRefIdentifier);
                                break;
                        case evEstablished:
                                Call = SetEstablished(Event, crossRefIdentifier);
                                break;
                        case evFailed:
                                Call = SetFailed(Event, crossRefIdentifier);
                                break;
                        case evHeld:
                                Call = SetHeld(Event, crossRefIdentifier);
                                break;
                        case evNetworkReached:
                                SetConnectionState(crossRefIdentifier, Event->NetworkReached.OutboundConnection.CallID, Event->NetworkReached.LocalConnectionInfo);
                                break;
                        case evOriginated:
                                Call = SetOriginated(Event, crossRefIdentifier);
                                break;
                        case evQueued:
                                Call = SetQueued(Event, crossRefIdentifier);
                                break;
                        case evRetrieved:
                                Call = SetRetrieved(Event, crossRefIdentifier);
                                break;
                        case evServiceInitiated:
                                Call = SetServiceInitiated(Event, crossRefIdentifier);
                                break;
                        case evTransferred:
                                Call = SetTransferred(Event, crossRefIdentifier);
                                break;
                }
            }catch(...)
            {
            }

        }
        sem_post(&sema);

        return Call;
}

void TCallList::RegisterEndCallHandler(void (*callback)(TCallItem *, void *), void * arg)
{
    this->cbkEndCall = callback;
    ptrEndCallArg = arg;
    hasEndCallCbk = true;
}


TCallList::TCallList()
{
    hasEndCallCbk = false;
    sem_init(&sema, 0, 1);
    sem_init(&semaGetMonitor, 0, 1);
    cbkEndCall = NULL;
    ptrEndCallArg = NULL;
}

TCallList::~TCallList()
{
    TMonitorItem * ptrMonitor;
    sem_destroy(&sema);
    sem_destroy(&semaGetMonitor);
    Clear();
    for(unsigned i = 0; i < MonitorList.size();i++)
    {
        ptrMonitor = (TMonitorItem *)MonitorList[i];
        delete ptrMonitor;
    }

}



TCallItem * TCallList::SetServiceInitiated(TCallControlEvent * Event, int crossRefId)
{
    TCallItem * Call;
    Call = NULL;
    SetConnectionState(crossRefId, Event->ServiceInitiated.InitiatedConnection.CallID, Event->ServiceInitiated.LocalConnectionInfo);
    SetAssociatedDevice(crossRefId, Event->ServiceInitiated.InitiatedConnection.CallID, Event->ServiceInitiated.AssociatedCalledDeviceID.DeviceID.DialingNumber);
    SetAssociatedDevice(crossRefId, Event->ServiceInitiated.InitiatedConnection.CallID, Event->ServiceInitiated.AssociatedCallingDeviceID.DeviceID.DialingNumber);
    Call = GetCall(crossRefId, Event->ServiceInitiated.InitiatedConnection.CallID);
    return Call;
}

TCallItem * TCallList::SetConferenced(TCallControlEvent * Event, int crossRefId)
{
    TCallItem * Call;
    Call = NULL;
    if(Event->Conferenced.LocalConnectionInfo != csNull)
    {
            for(int i = 0;i<Event->Conferenced.ConferenceConnections.Count;i++)
            {
                    AddConferencedCall(
                            crossRefId,
                            Event->Conferenced.ConferenceConnections.Connections[i].newConnection.CallID,
                            Event->Conferenced.ConferenceConnections.Connections[i].oldConnection.CallID,
                            csConferenced,
                            Event->Conferenced.ConferenceConnections.Connections[i].newConnection.DeviceID.DialingNumber
                            );
                    AddConferencedCall(
                            crossRefId,
                            Event->Conferenced.ConferenceConnections.Connections[i].newConnection.CallID,
                            Event->Conferenced.ConferenceConnections.Connections[i].oldConnection.CallID,
                            csConferenced,
                            Event->Conferenced.ConferenceConnections.Connections[i].endPoint.DialingNumber
                            );
                    Call = GetCall(crossRefId, Event->Conferenced.ConferenceConnections.Connections[i].newConnection.CallID);
                    //if(Event->Conferenced.ConferenceConnections.Connections[i].newConnection.CallID != Event->Conferenced.ConferenceConnections.Connections[i].oldConnection.CallID)
                    //        DelCall(crossRefIdentifier, Event->Conferenced.ConferenceConnections.Connections[i].oldConnection.CallID);
            }
    }else
    {
            DelCall(crossRefId, Event->Conferenced.PrimaryOldCall.CallID);
            DelCall(crossRefId, Event->Conferenced.SecondaryOldCall.CallID);
    }

    ClearInitiated(crossRefId);
    return Call;
}

TCallItem * TCallList::SetConnectionCleared(TCallControlEvent * Event, int crossRefId)
{
    TCallItem * Call;
    Call = NULL;
    if(Event->ConnectionCleared.ReleasingDevice.DeviceID.DialingNumber == Event->Device)
            Event->ConnectionCleared.LocalConnectionInfo = csNull;
    SetConnectionState(crossRefId, Event->ConnectionCleared.DroppedConnection.CallID, Event->ConnectionCleared.LocalConnectionInfo);
    //DelCall(crossRefId, Event->ConnectionCleared.DroppedConnection.CallID, Event->ConnectionCleared.ReleasingDevice.DeviceID.DialingNumber);
    Call = GetCall(crossRefId, Event->ConnectionCleared.DroppedConnection.CallID);
    return Call;
}

TCallItem * TCallList::SetDelivered(TCallControlEvent * Event, int crossRefId)
{
    TCallItem * Call;
    Call = NULL;
    if(!Event->Delivered.Connection.CallID)
    {
        Event->Delivered.Connection.CallID = Event->Delivered.OriginatingNIDConnection.CallID;
    }
    SetConnectionState(crossRefId, Event->Delivered.Connection.CallID, csAlerting);
    SetCalledDevice(crossRefId, Event->Delivered.Connection.CallID, Event->Delivered.CalledDevice.DeviceID.DialingNumber);
    SetConnectedDevice(crossRefId, Event->Delivered.Connection.CallID, Event->Delivered.CalledDevice.DeviceID.DialingNumber);
    SetConnectedDevice(crossRefId, Event->Delivered.Connection.CallID, Event->Delivered.CallingDevice.DeviceID.DialingNumber);
    SetConnectedDevice(crossRefId, Event->Delivered.Connection.CallID, Event->Delivered.AlertingDevice.DeviceID.DialingNumber);
    SetAssociatedDevice(crossRefId, Event->Delivered.Connection.CallID, Event->Delivered.AssociatedCalledDeviceID.DeviceID.DialingNumber);
    SetAssociatedDevice(crossRefId, Event->Delivered.Connection.CallID, Event->Delivered.AssociatedCallingDeviceID.DeviceID.DialingNumber);
    SetCallSrc(crossRefId,Event->Delivered.Connection.CallID, Event->Delivered.CallingDevice.DeviceID.DialingNumber);
    SetCallDest(crossRefId,Event->Delivered.Connection.CallID, Event->Delivered.CalledDevice.DeviceID.DialingNumber);
    ClearDelivered(crossRefId, Event->Delivered.Connection.CallID);
    Call = GetCall(crossRefId, Event->Delivered.Connection.CallID);
    return Call;
}

TCallItem * TCallList::SetDiverted(TCallControlEvent * Event, int crossRefId)
{
    TCallItem * Call;
    Call = NULL;
    //if(Event->Diverted.DivertingDevice.DeviceID.DialingNumber == Event->Device)
//            Event->Diverted.LocalConnectionInfo = csNull;
    //else
    //        Event->Diverted.LocalConnectionInfo = csAlerting;

    //SetConnectedDevice(crossRefId, Event->Diverted.Connection.CallID, Event->Diverted.NewDestination.DeviceID.DialingNumber);
    Call = GetCall(crossRefId, Event->Diverted.Connection.CallID);
    if(Call)
    {
      Call->bDiverted = true;
      SetConnectionState(crossRefId, Event->Diverted.Connection.CallID, Event->Diverted.LocalConnectionInfo);
      SetAssociatedDevice(crossRefId, Event->Diverted.Connection.CallID, Event->Diverted.AssociatedCalledDeviceID.DeviceID.DialingNumber);
      SetAssociatedDevice(crossRefId, Event->Diverted.Connection.CallID, Event->Diverted.AssociatedCallingDeviceID.DeviceID.DialingNumber);
      Call = GetCall(crossRefId, Event->Diverted.Connection.CallID);
    }
    return Call;
}

TCallItem * TCallList::SetEstablished(TCallControlEvent * Event, int crossRefId)
{
    TCallItem * Call;
    Call = NULL;
    if(Event->Established.EstablishedConnection.CallID == 0)
            Event->Established.EstablishedConnection.CallID = Event->Established.OriginatingNIDConnection.CallID;
    SetConnectionState(crossRefId, Event->Established.EstablishedConnection.CallID, Event->Established.LocalConnectionInfo);
    SetCalledDevice(crossRefId, Event->Established.EstablishedConnection.CallID, Event->Established.CalledDevice.DeviceID.DialingNumber);
    SetConnectedDevice(crossRefId, Event->Established.EstablishedConnection.CallID, Event->Established.AnsweringDevice.DeviceID.DialingNumber);
    SetConnectedDevice(crossRefId, Event->Established.EstablishedConnection.CallID, Event->Established.CallingDevice.DeviceID.DialingNumber);
    SetAssociatedDevice(crossRefId, Event->Established.EstablishedConnection.CallID, Event->Established.AssociatedCalledDeviceID.DeviceID.DialingNumber);
    SetAssociatedDevice(crossRefId, Event->Established.EstablishedConnection.CallID, Event->Established.AssociatedCallingDeviceID.DeviceID.DialingNumber);

    //SetConnectedDevice(crossRefId, Event->Established.EstablishedConnection.CallID, "04891487607");
    //SetAssociatedDevice(crossRefId, Event->Established.EstablishedConnection.CallID, "7801");


    ClearInitiated(crossRefId);
    SetCallSrc(crossRefId,Event->Established.EstablishedConnection.CallID, Event->Established.CallingDevice.DeviceID.DialingNumber);
    SetCallDest(crossRefId,Event->Established.EstablishedConnection.CallID, Event->Established.AnsweringDevice.DeviceID.DialingNumber);
    Call = GetCall(crossRefId, Event->Established.EstablishedConnection.CallID);
    return Call;
}

TCallItem * TCallList::SetFailed(TCallControlEvent * Event, int crossRefId)
{
    TCallItem * Call;
    Call = NULL;
    SetConnectionState(crossRefId, Event->Failed.FailedConnection.CallID, /*Event->Failed.LocalConnectionInfo*/csFail);
    SetAssociatedDevice(crossRefId, Event->Failed.FailedConnection.CallID, Event->Failed.AssociatedCalledDeviceID.DeviceID.DialingNumber);
    SetAssociatedDevice(crossRefId, Event->Failed.FailedConnection.CallID, Event->Failed.AssociatedCallingDeviceID.DeviceID.DialingNumber);
    Call = GetCall(crossRefId, Event->Failed.FailedConnection.CallID);
    return Call;
}

TCallItem * TCallList::SetHeld(TCallControlEvent * Event, int crossRefId)
{
    TCallItem * Call;
    Call = NULL;
    SetConnectionState(crossRefId, Event->Held.HeldConnection.CallID, Event->Held.LocalConnectionInfo);
    Call = GetCall(crossRefId, Event->Held.HeldConnection.CallID);
    return Call;
}

TCallItem * TCallList::SetOriginated(TCallControlEvent * Event, int crossRefId)
{
    TCallItem * Call;
    Call = NULL;
    Call = GetCall(crossRefId, Event->Originated.OriginatedConnection.CallID);
    SetConnectionState(crossRefId, Event->Originated.OriginatedConnection.CallID, csAlerting);
    SetCalledDevice(crossRefId, Event->Originated.OriginatedConnection.CallID, Event->Originated.CalledDevice.DeviceID.DialingNumber);
    SetConnectedDevice(crossRefId, Event->Originated.OriginatedConnection.CallID, Event->Originated.CalledDevice.DeviceID.DialingNumber);
    SetAssociatedDevice(crossRefId, Event->Originated.OriginatedConnection.CallID, Event->Originated.AssociatedCalledDeviceID.DeviceID.DialingNumber);
    SetAssociatedDevice(crossRefId, Event->Originated.OriginatedConnection.CallID, Event->Originated.AssociatedCallingDeviceID.DeviceID.DialingNumber);
    SetCallSource(crossRefId,Event->Originated.OriginatedConnection.CallID);
    SetCallSrc(crossRefId,Event->Originated.OriginatedConnection.CallID, Event->Originated.CallingDevice.DeviceID.DialingNumber);
    SetCallDest(crossRefId,Event->Originated.OriginatedConnection.CallID, Event->Originated.CalledDevice.DeviceID.DialingNumber);
    return Call;
}

TCallItem * TCallList::SetQueued(TCallControlEvent * Event, int crossRefId)
{
    TCallItem * Call;
    Call = NULL;
    SetConnectionState(crossRefId, Event->Queued.QueuedConnection.CallID, Event->Queued.LocalConnectionInfo);
    SetConnectedDevice(crossRefId, Event->Queued.QueuedConnection.CallID, Event->Queued.CallingDevice.DeviceID.DialingNumber);
    SetConnectedDevice(crossRefId, Event->Queued.QueuedConnection.CallID, Event->Queued.CalledDevice.DeviceID.DialingNumber);
    SetAssociatedDevice(crossRefId, Event->Queued.QueuedConnection.CallID, Event->Queued.AssociatedCalledDeviceID.DeviceID.DialingNumber);
    SetAssociatedDevice(crossRefId, Event->Queued.QueuedConnection.CallID, Event->Queued.AssociatedCallingDeviceID.DeviceID.DialingNumber);
    Call = GetCall(crossRefId, Event->Queued.QueuedConnection.CallID);
    return Call;
}


TCallItem * TCallList::SetRetrieved(TCallControlEvent * Event, int crossRefId)
{
    TCallItem * Call;
    Call = NULL;
    SetConnectionState(crossRefId, Event->Retrieved.RetrievedConnection.CallID, Event->Retrieved.LocalConnectionInfo);
    ClearInitiated(crossRefId);
    Call = GetCall(crossRefId, Event->Retrieved.RetrievedConnection.CallID);
    return Call;
}

TCallItem * TCallList::SetTransferred(TCallControlEvent * Event, int crossRefId)
{
    TCallItem * Call;
    TCallItem * tmpCall;
    Call = NULL;
    if(Event->Transferred.LocalConnectionInfo != csNull)
    {
            for(int i = 0;i<Event->Transferred.TransferredConnections.Count;i++)
            {
                    AddCall(crossRefId, Event->Transferred.TransferredConnections.Connections[i].newConnection.CallID, Event->Transferred.LocalConnectionInfo);
                    SetConnectedDevice(crossRefId, Event->Transferred.TransferredConnections.Connections[i].newConnection.CallID, Event->Transferred.TransferredConnections.Connections[i].newConnection.DeviceID.DialingNumber);
                    SetConnectedDevice(crossRefId, Event->Transferred.TransferredConnections.Connections[i].newConnection.CallID, Event->Transferred.TransferredConnections.Connections[i].endPoint.DialingNumber);
                    if(Event->Transferred.TransferredConnections.Connections[i].newConnection.CallID != Event->Transferred.TransferredConnections.Connections[i].oldConnection.CallID)
                            DelCall(crossRefId, Event->Transferred.TransferredConnections.Connections[i].oldConnection.CallID);

                    Call = GetCall(crossRefId, Event->Transferred.TransferredConnections.Connections[i].newConnection.CallID);
                    if((tmpCall = GetCall(Event->Transferred.TransferringDevice.DeviceID.DialingNumber, Event->Transferred.TransferredConnections.Connections[i].newConnection.CallID)) != NULL)
                    {
                            if(Call != NULL)
                                    Call->source = tmpCall->source;
                    }else if((tmpCall = GetCall(Event->Transferred.TransferringDevice.DeviceID.DialingNumber, Event->Transferred.TransferredConnections.Connections[i].oldConnection.CallID)) != NULL)
                    {
                            if(Call != NULL)
                                    Call->source = tmpCall->source;
                    }
            }

    }else
    {
            DelCall(crossRefId, Event->Transferred.PrimaryOldCall.CallID);
            DelCall(crossRefId, Event->Transferred.SecondaryOldCall.CallID);
    }

    ClearInitiated(crossRefId);
    return Call;
}

void TCallList::Assign(int crossRefId, TSnapshotDeviceResult * deviceState)
{
    TMonitorItem * tmpMonitorItem;
    TCallItem * tmpCall;
    bool bExiste;
    sem_wait(&sema);
    {
        try
        {
            if(GetMonitorItem(crossRefId, &tmpMonitorItem))
            {
                    for(unsigned i = 0;i < tmpMonitorItem->CallList.size();i++)
                    {

                            tmpCall = (TCallItem *)tmpMonitorItem->CallList[i];
                            bExiste = false;
                            for(int j = 0;j < deviceState->Count;j++)
                            {
                                    if(deviceState->snapshotDeviceItem[j].connectionIdentifier.CallID == tmpCall->callId)
                                    {
                                        bExiste = true;
                                    }
                            }
                            if(!bExiste)
                            {
                            	tmpMonitorItem->CallList.erase(tmpMonitorItem->CallList.begin()+1);
                                delete tmpCall;
                            }
                    }

                    for(int i = 0;i < deviceState->Count;i++)
                    {
                            bExiste = false;
                            for(unsigned j = 0;j < tmpMonitorItem->CallList.size();j++)
                            {
                                    tmpCall = (TCallItem *)tmpMonitorItem->CallList[j];

                                    if(deviceState->snapshotDeviceItem[i].connectionIdentifier.CallID == tmpCall->callId)
                                    {
                                        bExiste = true;
                                    }
                            }
                            if(!bExiste)
                            {
                                tmpCall = new TCallItem;
                                tmpCall->callId = deviceState->snapshotDeviceItem[i].connectionIdentifier.CallID;
                                tmpCall->localConnectionInfo = deviceState->snapshotDeviceItem[i].localCallState;
                                tmpCall->source = false;
                                tmpCall->tarifacaoSet = false;
                                tmpCall->bDiverted = false;
                                tmpCall->queueSet = false;
                                tmpCall->Device = tmpMonitorItem->device;
                                tmpCall->tsCreation = time(NULL);
                                if(tmpCall->localConnectionInfo == csConnected)
                                {
                                        tmpCall->connectSet = true;
                                        tmpCall->tsConnect = time(NULL);
                                }
                                else
                                        tmpCall->connectSet = false;
                                tmpMonitorItem->CallList.push_back(tmpCall);
                            }
                    }
            }
        }catch(...)
        {
        }

    }
    sem_post(&sema);
}
TCallItem * TCallList::Assign(int crossRefId, TSnapshotCallResult * deviceCalls)
{
        TCallItem * Call;
        Call = NULL;
        sem_wait(&sema);
        {
            try
            {
                for(int i = 0;i < deviceCalls->Count;i++)
                {
                        if(deviceCalls->snapshotCallItem[i].device.DialingNumber == GetDeviceByMonitorId(crossRefId) && i == 0)
                                SetCallSource(crossRefId, deviceCalls->snapshotCallItem[i].connectionIdentifier.CallID);

                        SetConnectedDevice(crossRefId, deviceCalls->snapshotCallItem[i].connectionIdentifier.CallID, deviceCalls->snapshotCallItem[i].device.DialingNumber);

                        if(deviceCalls->snapshotCallItem[i].connectionIdentifier.CallID != 0)
                                Call = GetCall(crossRefId, deviceCalls->snapshotCallItem[i].connectionIdentifier.CallID);
                }
            }catch(...)
            {
            }

        }
        sem_post(&sema);

        return Call;
}


int TCallList::DeviceToCrossRef(string device)
{
        int result = 0;
        TMonitorItem * tmpMonitor;
        sem_wait(&sema);
        {
            try
            {
                if(GetMonitorItem(device, &tmpMonitor))
                {
                        result = tmpMonitor->crossRefId;
                }
            }catch(...)
            {
            }

        }
        sem_post(&sema);
        return result;
}

time_t TCallList::GetLastLogicalEvent(string device)
{
    time_t result;
    TMonitorItem * tmpMonitor;

    result = time(NULL);
    sem_wait(&sema);
    {
        try
        {
            if(GetMonitorItem(device, &tmpMonitor))
            {
                    result = tmpMonitor->lastLogicalEvent;
            }
        }catch(...)
        {
        }

    }
    sem_post(&sema);
    return result;
}

string TCallList::GetDeviceByAgent(string agent)
{
    string result;
    TMonitorItem * tmpMonitor;

    sem_wait(&sema);
    {
        try
        {
            if(GetMonitorItemByAgent(agent, &tmpMonitor))
            {
                    result = tmpMonitor->device;
            }
        }catch(...)
        {
        }

    }
    sem_post(&sema);
    return result;
}

void TCallList::SetGroup(int callId, string group)
{
    TCallItem * tmpCall;
    TMonitorItem * tmpMonitorItem;

    sem_wait(&sema);
    {
        try
        {
            for(unsigned i =0;i < MonitorList.size();i++)
            {
                    tmpMonitorItem = (TMonitorItem *)MonitorList[i];


                    for(unsigned i = 0;i < tmpMonitorItem->CallList.size();i++)
                    {
                            tmpCall = (TCallItem *)tmpMonitorItem->CallList[i];

                            if(tmpCall->callId == callId)
                            {
                                    tmpCall->group = group;
                            }
                    }
            }
        }catch(...)
        {
        }

    }
    sem_post(&sema);
}
void TCallList::SetQueueTime(int callId, time_t time)
{
    TCallItem * tmpCall;
    TMonitorItem * tmpMonitorItem;

    //sem_wait(&sema);
    {
        try
        {
            for(unsigned i =0;i < MonitorList.size();i++)
            {
                    tmpMonitorItem = (TMonitorItem *)MonitorList[i];


                    for(unsigned j = 0;j < tmpMonitorItem->CallList.size();j++)
                    {
                            tmpCall = (TCallItem *)tmpMonitorItem->CallList[j];

                            if(tmpCall->callId == callId)
                            {
                                    tmpCall->queueSet = true;
                                    tmpCall->queueTime = time;
                            }
                    }
            }
        }catch(...)
        {
        }
        //ReleaseSemaphore(sema,1,0);
    }
}
