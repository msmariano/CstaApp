#ifndef CallListH
#define CallListH

#include <vector>
#include "AsnNode.h"
#include "CstaTypes.h"
#include <semaphore.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <stdlib.h>
#include <time.h>

using namespace std;


typedef struct{
        int callId;
        TLocalConnectionState localConnectionInfo;
        string connectedDevice;
        string connectedName;
        string calledDevice;
        string strSrc;
        string strDest;
        string associatedDevice;
        string associatedOp;
        bool source;
        bool connectSet;
        time_t tsConnect;
        time_t tsCreation;
        string CustoIn;
        string CustoPP;
        string Conexao;
        string PulsoIn;
        string PulsoAd;
        string localidade;
        string group;
        time_t queueTime;
        bool queueSet;
        bool tarifacaoSet;
        bool bDiverted;
        string Device;
}TCallItem;

typedef struct
{
    TCallItem Connections[100];
    int Count;
}TCalls;

class TMonitorItem
{
public:
        int crossRefId;
        TMonitorType monitorType;
        string device;
        TAgentState AgentState;
        time_t lastLogicalEvent;
        string AgentGroup;
        string AgentID;

        vector<TCallItem*> CallList;

        void Clear()
        {
            for(unsigned ind = 0;ind < CallList.size();ind++)
            {
                    TCallItem * tmpCall;
                    tmpCall = (TCallItem *)CallList[ind];
                    delete tmpCall;
            }
        }
        TMonitorItem(){

            lastLogicalEvent = time(NULL);
            AgentState=asAgentNull;
            crossRefId = -1;
            monitorType = mtDevice;
        }
        ~TMonitorItem(){
            for(unsigned ind = 0;ind < CallList.size();ind++)
            {
                TCallItem * tmpCall;
                tmpCall = (TCallItem *)CallList[ind];
                delete tmpCall;
            }
       }
};


class TCallList
{
   protected:
        void SetAssociatedDevice(int crossRefIdentifier, int callId, string associatedDevice);
        void SetConnectedDevice(int crossRefIdentifier, int callId, string connectedDevice);
        void SetCalledDevice(int crossRefIdentifier, int callId, string calledDevice);
        void SetCallSource(int crossRefIdentifier, int callId);
        void SetCallSrc(int crossRefIdentifier, int callId, string strSrc);
        void SetCallDest(int crossRefIdentifier, int callId, string strDest);
        void ClearInitiated(int crossRefIdentifier);
        void ClearDelivered(int crossRefIdentifier, int currentCallId);
        void ClearEstablished(int crossRefIdentifier, int currentCallId);
        void AddCall(int crossRefIdentifier, int callId, TLocalConnectionState localConnectionInfo);
        void UpdateCall(int crossRefIdentifier, int callId, TLocalConnectionState localConnectionInfo);
        void AddConferencedCall(int crossRefIdentifier, int callId, int oldCallId, TLocalConnectionState localConnectionInfo, string connectedId);


        void SetGroup(int callId, string group);
        void SetQueueTime(int callId, time_t);
        void DelCall(int crossRefIdentifier, int callId, string releasingDevice);
        void SetLogicalDeviceState(int crossRefIdentifier, TAgentState AgentState, string AgentGroup, string AgentID);
        void SetConnectionState(int crossRefIdentifier, int callId, TLocalConnectionState localConnectionInfo);


        TCallItem * SetServiceInitiated(TCallControlEvent * Event, int crossRefId);
        TCallItem * SetConferenced(TCallControlEvent * Event, int crossRefId);
        TCallItem * SetConnectionCleared(TCallControlEvent * Event, int crossRefId);
        TCallItem * SetDelivered(TCallControlEvent * Event, int crossRefId);
        TCallItem * SetDiverted(TCallControlEvent * Event, int crossRefId);
        TCallItem * SetEstablished(TCallControlEvent * Event, int crossRefId);
        TCallItem * SetFailed(TCallControlEvent * Event, int crossRefId);
        TCallItem * SetHeld(TCallControlEvent * Event, int crossRefId);
        TCallItem * SetOriginated(TCallControlEvent * Event, int crossRefId);
        TCallItem * SetQueued(TCallControlEvent * Event, int crossRefId);
        TCallItem * SetRetrieved(TCallControlEvent * Event, int crossRefId);
        TCallItem * SetTransferred(TCallControlEvent * Event, int crossRefId);

        vector<TMonitorItem*>  MonitorList;

        sem_t sema;
        sem_t semaGetMonitor;

        bool hasEndCallCbk;
        void * ptrEndCallArg;
        void (* cbkEndCall)(TCallItem * call, void * arg);

        TCallItem * GetCall(int crossRefId,int callId);
        TCallItem * GetCall(string device,int callId);
   public:
        string LCIText(TLocalConnectionState lcs);
        string IntToStr(int Arg);
        void RegisterEndCallHandler(void (*)(TCallItem *, void *), void *);
        TCallItem GetCallItem(string device, int callId);
        void Clear();
        void ClearCallsFromDevice(int crossRefId);
        void ClearCallsFromDevice(string device);
        TCallItem * UpdateCalls(TCallControlEventType eventType, TCallControlEvent * Event, int crossRefIdentifier);
        TCallItem * UpdateCalls(TLogicalDeviceEventType eventType, TLogicalDeviceEvent * Event, int crossRefIdentifier);

        void AddMonitorItem(string device, int crossRefId, TMonitorType monitorType);

        string GetDeviceByMonitorId(int crossRefIdentifier);
        int GetMonitorIdByDevice(string device);
        string GetDeviceByAgent(string agent);
        time_t GetLastLogicalEvent(string device);
        bool MonitorExists(string device);
        bool MonitorExists(int crossRef);

        bool IsDeviceInCall(string device);
        void DeleteMonitorItem(string device);
        void SetLogicalDeviceState(string device, TAgentState agentState, string agentId);
        string GetCallSource(int crossRefIdentifier, int callId);
        string GetCallDestination(int crossRefIdentifier, int callId);

        TCalls GetCallsFromDevice(string device);
        TCalls GetCallsFromAgent(string agent);

        string GetSourceByDevice(string device);
        string GetDestinationByDevice(string device);
        string GetCalledByDevice(string device);
        string GetAgentIDByDevice(string device);
        TAgentState GetAgentStateByDevice(string device);
        TAgentState GetAgentStateByAgentId(string agent);
        string GetACDGroupByDevice(string device);
        string logCallDevice(string device);
        bool GetMonitorItemByAgent(string agent, TMonitorItem ** MonitorItem);
        bool GetMonitorItem(int crossRefId, TMonitorItem ** MonitorItem);
        bool GetMonitorItem(string device, TMonitorItem ** MonitorItem);
        string logCalls();

        void Assign(int crossRefId, TSnapshotDeviceResult * deviceState);
        TCallItem * Assign(int crossRefId, TSnapshotCallResult * deviceCalls);

        TCallList();
        ~TCallList();
        void DelCall(int crossRefIdentifier, int callId);
        void DelCall(string, int callId);

        int DeviceToCrossRef(string device);
};

#endif
