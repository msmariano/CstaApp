#ifndef CstaTypesH
#define CstaTypesH
#include "AsnNode.h"
#include <semaphore.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <stdlib.h>
#include <unistd.h>
using namespace std;




#define CSTA_MAX_SIZE 32768

typedef struct
{
	sem_t sema;
	int val;
	string str;
}HANDLE;

class Sync
{
public:
	Sync()
	{
		key = 0x20;
		semid = semget(key, 1, 0666 | IPC_CREAT);
	    operation[0].sem_op  = -1;
	    operation[0].sem_num = 0;
		operation[0].sem_flg = SEM_UNDO;
		timeout=false;
		unlocknormal=false;
		bLock=false;
		fTime = 0;
		bStop = false;
	}
	void Restart()
	{
		key = 0x20;
		semid = semget(key, 1, 0666 | IPC_CREAT);
		operation[0].sem_op  = -1;
		operation[0].sem_num = 0;
		operation[0].sem_flg = SEM_UNDO;
		timeout=false;
		unlocknormal=false;
		bLock=false;
		fTime = 0;
		bStop = false;

	}
	static void * pth_wait(void * arg)
	{
		Sync * pSync = (Sync*)arg;
		//cout << "Iniciando contagem!!!" << endl;
		for(int i=0;i<pSync->GetTime();i++)
		{
			sleep(1);
			if(pSync->bStop)
				break;
			//char car[10]="";
			//sprintf(car,"%d",i);
			//cout << car << endl;
		}
		if(!pSync->unlocknormal)
			pSync->timeout=true;
		pSync->unlock();
		pthread_exit(0);

	}
	~Sync()
	{
		semctl(semid, 0, IPC_RMID , 0);
	}
    int GetId()
    {
    	return semid;
    }
    void lock(bool bWait = false,int iTime =0)
    {
    	timeout = false;
    	bStop = false;
    	unlocknormal=false;
    	pthread_t tt;
    	fTime = iTime;
    	if(bWait)
    	{
    		pthread_create(&tt,0,pth_wait,this);
    	}
    	bLock=true;
    	semop(semid, operation, 1);
    	//if(bWait)
    	//	pthread_join(tt,NULL);
    }
    void unlock()
    {
    	if(bLock)
    	{
			operation[0].sem_op  = 1;
			operation[0].sem_num = 0;
			operation[0].sem_flg = SEM_UNDO;
			semop(semid, operation, 1);
			bStop = true;
			if(!timeout)
				unlocknormal=true;
			bLock=false;
    	}
    }
    int Get()
    {
    	return operation[0].sem_op + operation[0].sem_num;
    }
    int GetTime()
    {
    	return fTime;
    }
    bool GetTimeout()
    {
    	return timeout;
    }
    void SetCreater(string Arg)
    {
    	Creater = Arg;
    }
    string GetCreater()
    {
    	return Creater;
    }
    HANDLE h;
private:
	struct sembuf operation[1] ;
	int semid;
	int key;
	int fTime;
	bool timeout;
	bool unlocknormal;
	bool bLock;
	bool bStop;
	string Creater;

};


typedef enum
{
    etCallControl,
    etLogicalDevice,
    etPhysicalDevice,
    etDisconnect,
    etSnapshotDeviceData,
    etBeat
}TEventType;

typedef struct
{
    unsigned char msgData[CSTA_MAX_SIZE];
    int msgSize;
}TCSTAMessage;

typedef struct
{
    TEventType eventType;
    void * eventData;
}TEventPointer;

typedef struct {
        int reqType;
        int h;
        void (*callback)(int result);
}TRequest;

typedef enum {
        mtDevice,
        mtTrunk
}TMonitorType;

typedef struct
{
    string device;
    TMonitorType monitorType;
    int crossRefId;
}TCSTAMonitorItem;

typedef struct{
        string DialingNumber;
        int DeviceNumber;
} TDeviceID;

typedef string TAgentID;
typedef string TAgentPassword;

typedef struct {
        int CallID;
        TDeviceID DeviceID;
} TConnectionID;

typedef struct {
        TDeviceID DeviceID;
} TSubjectDeviceID;

typedef struct {
        TDeviceID DeviceID;
} TCallingDeviceID;

typedef struct {
        TDeviceID DeviceID;
} TCalledDeviceID;

typedef struct {
        TDeviceID DeviceID;
} TAssociatedCallingDeviceID;

typedef struct {
        TDeviceID DeviceID;
} TAssociatedCalledDeviceID;

typedef struct {
        TDeviceID DeviceID;
} TNetworkCallingDeviceID;

typedef struct {
        TDeviceID DeviceID;
} TNetworkCalledDeviceID;

typedef struct {
        TConnectionID newConnection;
        TConnectionID oldConnection;
        TDeviceID endPoint;
        TDeviceID associatedNID;
}TConnectionListItem;

typedef struct
{
         TConnectionListItem Connections[100];
         int Count;
}TConnectionList;

typedef enum {
        asAgentNotReady         = 0,
        asAgentNull             = 1,
        asAgentReady            = 2,
        asAgentBusy             = 3,
        asAgentWorkingAfterCall = 4
} TAgentState;

typedef enum {
        reqAgentLoggedOn         = 0,
        reqAgentLoggedOff        = 1,
        reqAgentNotReady         = 2,
        reqAgentReady            = 3,
        reqAgentWorkingAfterCall = 4
} TReqAgentState;
        

typedef enum {
        csNull            = 0,
        csInitiated       = 1,
        csAlerting        = 2,
        csConnected       = 3,
        csHold            = 4,
        csQueued          = 5,
        csFail            = 6,
        csConferenced     =10
} TLocalConnectionState;

typedef enum {
        ecsACDBusy					= 57,
	ecsACDForward					= 58,
	ecsACDSaturated					= 59,
	ecsActiveParticipation			        = 1,
	ecsAlertTimeExpired				= 60,
	ecsAlternate					= 2,
	ecsAutoWork					= 61,
	ecsBlocked					= 35,
	ecsBusy			        		= 3,
	ecsCallBack					= 4,
	ecsCallCancelled				= 5,
	ecsCallForward					= 9,
	ecsCallForwardImmediate			        = 6,
	ecsCallForwardBusy				= 7,
	ecsCallForwardNoAnswer			        = 8,
	ecsCallNotAnswered				= 10,
	ecsCallPickup					= 11,
	ecsCampOn					= 12,
	ecsCampOnTrunks					= 62,
	ecsCharacterCountReached		        = 36,
	ecsConference					= 63,
	ecsConsultation					= 37,
	ecsDestDetected					= 64,
	ecsDestNotObtainable			        = 13,
	ecsDestOutOfOrder				= 65,
	ecsDistributed					= 38,
	ecsDistributionDelay			        = 66,
	ecsDoNotDisturb					= 14,
	ecsDTMFDigitDetected			        = 39,
	ecsDurationExceeded				= 40,
	ecsEndOfMessageDetected			        = 41,
	ecsEnteringDistribution			        = 42,
	ecsForcedPause					= 43,
	ecsForcedTransition				= 67,
	ecsIncompatibleDestination		        = 15,
	ecsIntrude					= 68,
	ecsInvalidAccountCode			        = 16,
	ecsInvalidNumberFormat			        = 69,
	ecsJoinCall					= 70,
	ecsKeyOperation					= 17,
	ecsKeyOperationInUse			        = 71,
	ecsLockout					= 18,
	ecsMaintenance					= 19,
	ecsMakeCall					= 44,
	ecsMakePredictiveCall		        	= 72,
	ecsMessageDurationExceeded		        = 73,
	ecsMessageSizeExceeded			        = 45,
	ecsMultipleAlerting				= 74,
	ecsMultipleQueuing				= 75,
	ecsNetworkCongestion			        = 20,
	ecsNetworkDialling				= 76,
	ecsNetworkNotObtainable			        = 21,
	ecsNetworkOutOfOrder			        = 77,
	ecsNetworkSignal				= 46,
	ecsNewCall					= 22,
	ecsNextMessage					= 47,
	ecsNoAvailableAgents			        = 23,
	ecsNormal					= 78,
	ecsNormalClearing				= 48,
	ecsNoSpeechDetected				= 49,
	ecsNotAvaliableBearerService	                = 79,
	ecsNotSupportedBearerService	                = 80,
	ecsNumberChanged				= 50,
	ecsNumberUnallocated			        = 81,
	ecsOverflow					= 26,
	ecsOverride					= 24,
	ecsPark			        		= 25,
	ecsQueueCleared					= 82,
	ecsRecall					= 27,
	ecsRedirected					= 28,
	ecsRemainsInQueue				= 83,
	ecsReorderTone					= 29,
	ecsReserved					= 84,
	ecsResourcesNotAvailable		        = 30,
	ecsSelectedTrunkBusy			        = 85,
	ecsSilentParticipation			        = 31,
	ecsSingleStepConference			        = 51,
	ecsSingleStepTransfer			        = 52,
	ecsSpeechDetected				= 53,
	ecsSuspend					= 86,
	ecsSwitchingFunctionTerminated	                = 54,
	ecsTerminationCharacterReceived	                = 55,
	ecsTimeout					= 56,
	ecsTransfer					= 32,
	ecsTrunksBusy					= 33,
	ecsUnauthorisedBearerService	                = 87
} TEventCause;

typedef enum
{
  evOnHook=255,
  evOffHook=0
}THookEventType;

typedef enum {
        evBridged                       = 0,
        evCallCleared                   = 1,
        evConferenced                   = 2,
        evConnectionCleared             = 3,
        evDelivered                     = 4,
        evDigitsDialed                  = 5,
        evDiverted                      = 6,
        evEstablished                   = 7,
        evFailed                        = 8,
        evHeld                          = 9,
        evNetworkCapabilitiesChange     = 10,
        evNetworkReached                = 11,
        evOffered                       = 12,
        evOriginated                    = 13,
        evQueued                        = 14,
        evRetrieved                     = 15,
        evServiceInitiated              = 16,
        evTransferred                   = 17
} TCallControlEventType;

typedef enum {
        evButtonInformation             = 0,
        evButtonPress                   = 1,
        evDisplayUpdated                = 2,
        evHookswitch                    = 3,
        evLampMode                      = 4,
        evMessageWaiting                = 5,
        evMicrophoneGain                = 6,
        evMicrophoneMute                = 7,
        evRingerStatus                  = 8,
        evSpeakerMute                   = 9,
        evSpeakerVolume                 = 10
} TPhysicalDeviceEventType;

typedef enum {
        evAgentBusy                     = 0,
        evAgentLoggedOn                 = 1,
        evAgentLoggedOff                = 2,
        evAgentNotReady                 = 3,
        evAgentReady                    = 4,
        evAgentWorkingAfterCall         = 5,
        evAutoAnswer                    = 6,
        evAutoWorkMode                  = 7,
        evCallback                      = 8,
        evCallbackMessage               = 9,
        evCallerIDStatus                = 10,
        evDoNotDisturb                  = 11,
        evForwarding                    = 12,
        evRouteingMode                  = 13,
} TLogicalDeviceEventType;

typedef struct {
        TSubjectDeviceID AgentDevice;
        TAgentID AgentID;
        TDeviceID ACDGroup;
}TAgentBusy;

typedef struct {
        TSubjectDeviceID AgentDevice;
        TAgentID AgentID;
        TDeviceID ACDGroup;
}TAgentReady;

typedef struct {
        TSubjectDeviceID AgentDevice;
        TAgentID AgentID;
        TDeviceID ACDGroup;
}TAgentNotReady;

typedef struct {
        TSubjectDeviceID AgentDevice;
        TAgentID AgentID;
        TDeviceID ACDGroup;
        TAgentPassword AgentPassword;
}TAgentLoggedOn;

typedef struct {
        TSubjectDeviceID AgentDevice;
        TAgentID AgentID;
        TDeviceID ACDGroup;
        TAgentPassword AgentPassword;
}TAgentLoggedOff;

typedef struct {
        TSubjectDeviceID AgentDevice;
        TAgentID AgentID;
        TDeviceID ACDGroup;
        enum{
                notReady = 0,
                ready = 1,
                null = 2}
        PendingAgentState;
}TAgentWorkingAfterCall;

typedef struct{
        TSubjectDeviceID Device;
        int Button;
}TButtonPress;

typedef struct{
        TConnectionID PrimaryOldCall;
        TConnectionID SecondaryOldCall;
        TSubjectDeviceID ConferencingDevice;
        TSubjectDeviceID AddedParty;
        TConnectionList ConferenceConnections;
        TLocalConnectionState LocalConnectionInfo;
        TEventCause Cause;
}TConferenced;

typedef struct{
        TConnectionID DroppedConnection;
        TSubjectDeviceID ReleasingDevice;
        TLocalConnectionState LocalConnectionInfo;
        TEventCause Cause;
}TConnectionCleared;

typedef struct{
        TConnectionID Connection;
        TSubjectDeviceID AlertingDevice;
        TCallingDeviceID CallingDevice;
        TCalledDeviceID CalledDevice;
        TConnectionID OriginatingNIDConnection;
        TLocalConnectionState LocalConnectionInfo;
        TEventCause Cause;
        TNetworkCallingDeviceID NetworkCallingDeviceID;
        TNetworkCalledDeviceID NetworkCalledDeviceID;
        TAssociatedCallingDeviceID AssociatedCallingDeviceID;
        TAssociatedCalledDeviceID AssociatedCalledDeviceID;
}TDelivered;

typedef struct{
        TConnectionID Connection;
        TSubjectDeviceID DivertingDevice;
        TSubjectDeviceID NewDestination;
        TCallingDeviceID CallingDevice;
        TCalledDeviceID CalledDevice;
        TLocalConnectionState LocalConnectionInfo;
        TEventCause Cause;
        TNetworkCallingDeviceID NetworkCallingDeviceID;
        TNetworkCalledDeviceID NetworkCalledDeviceID;
        TAssociatedCallingDeviceID AssociatedCallingDeviceID;
        TAssociatedCalledDeviceID AssociatedCalledDeviceID;
}TDiverted;

typedef struct{
        TConnectionID EstablishedConnection;
        TSubjectDeviceID AnsweringDevice;
        TCallingDeviceID CallingDevice;
        TCalledDeviceID CalledDevice;
        TConnectionID OriginatingNIDConnection;
        TLocalConnectionState LocalConnectionInfo;
        TEventCause Cause;
        TNetworkCallingDeviceID NetworkCallingDeviceID;
        TNetworkCalledDeviceID NetworkCalledDeviceID;
        TAssociatedCallingDeviceID AssociatedCallingDeviceID;
        TAssociatedCalledDeviceID AssociatedCalledDeviceID;
}TEstablished;

typedef struct{
        TConnectionID FailedConnection;
        TSubjectDeviceID FailingDevice;
        TCallingDeviceID CallingDevice;
        TCalledDeviceID CalledDevice;
        TConnectionID OriginatingNIDConnection;
        TLocalConnectionState LocalConnectionInfo;
        TEventCause Cause;
        TNetworkCallingDeviceID NetworkCallingDeviceID;
        TNetworkCalledDeviceID NetworkCalledDeviceID;
        TAssociatedCallingDeviceID AssociatedCallingDeviceID;
        TAssociatedCalledDeviceID AssociatedCalledDeviceID;
}TFailed;

typedef struct{
        TConnectionID HeldConnection;
        TSubjectDeviceID HoldingDevice;
        TLocalConnectionState LocalConnectionInfo;
        TEventCause Cause;
}THeld;

typedef struct{
        TConnectionID OutboundConnection;
        TSubjectDeviceID NetworkInterfaceUsed;
        TCallingDeviceID CallingDevice;
        TCalledDeviceID CalledDevice;
        TConnectionID OriginatingNIDConnection;
        TLocalConnectionState LocalConnectionInfo;
        TEventCause Cause;
        TNetworkCallingDeviceID NetworkCallingDeviceID;
        TNetworkCalledDeviceID NetworkCalledDeviceID;
        TAssociatedCallingDeviceID AssociatedCallingDeviceID;
}TNetworkReached;

typedef struct{
        TConnectionID OriginatedConnection;
        TSubjectDeviceID CallingDevice;
        TCalledDeviceID CalledDevice;
        TLocalConnectionState LocalConnectionInfo;
        TEventCause Cause;
        TNetworkCallingDeviceID NetworkCallingDeviceID;
        TNetworkCalledDeviceID NetworkCalledDeviceID;
        TAssociatedCallingDeviceID AssociatedCallingDeviceID;
        TAssociatedCalledDeviceID AssociatedCalledDeviceID;
}TOriginated;

typedef struct{
        TConnectionID QueuedConnection;
        TSubjectDeviceID Queue;
        TCallingDeviceID CallingDevice;
        TCalledDeviceID CalledDevice;
        int NumberQueued;
        int CallsInFront;
        TLocalConnectionState LocalConnectionInfo;
        TEventCause Cause;
        TNetworkCallingDeviceID NetworkCallingDeviceID;
        TNetworkCalledDeviceID NetworkCalledDeviceID;
        TAssociatedCallingDeviceID AssociatedCallingDeviceID;
        TAssociatedCalledDeviceID AssociatedCalledDeviceID;
}TQueued;

typedef struct{
        TConnectionID RetrievedConnection;
        TSubjectDeviceID RetrievingDevice;
        TLocalConnectionState LocalConnectionInfo;
        TEventCause Cause;
}TRetrieved;

typedef struct{
        TConnectionID InitiatedConnection;
        TSubjectDeviceID InitiatingDevice;
        TLocalConnectionState LocalConnectionInfo;
        TEventCause Cause;
        TNetworkCallingDeviceID NetworkCallingDeviceID;
        TNetworkCalledDeviceID NetworkCalledDeviceID;
        TAssociatedCallingDeviceID AssociatedCallingDeviceID;
        TAssociatedCalledDeviceID AssociatedCalledDeviceID;
}TServiceInitiated;

typedef struct{
        TConnectionID PrimaryOldCall;
        TConnectionID SecondaryOldCall;
        TSubjectDeviceID TransferringDevice;
        TSubjectDeviceID TransferredToDevice;
        TConnectionList TransferredConnections;
        TLocalConnectionState LocalConnectionInfo;
        TEventCause Cause;
}TTransferred;

typedef struct{
        string Device;
        TMonitorType monitorType;
        int crossRefId;
        TConnectionCleared ConnectionCleared;
        TEstablished Established;
        TDelivered Delivered;
        TFailed Failed;
        THeld Held;
        TDiverted Diverted;
        TServiceInitiated ServiceInitiated;
        TOriginated Originated;
        TQueued Queued;
        TRetrieved Retrieved;
        TNetworkReached NetworkReached;
        TConferenced Conferenced;
        TTransferred Transferred;
        string CallControlEvText;
        string IvrID;
}TCallControlEvent;

typedef struct{
        string Device;
        TMonitorType monitorType;
        int crossRefId;
        TAgentBusy AgentBusy;
        TAgentLoggedOn AgentLoggedOn;
        TAgentLoggedOff AgentLoggedOff;
        TAgentNotReady AgentNotReady;
        TAgentReady AgentReady;
        TAgentWorkingAfterCall AgentWorkingAfterCall;
}TLogicalDeviceEvent;

typedef struct{
        string Device;
        int crossRefId;
        THookEventType hookType;
        TButtonPress ButtonPress;
}TPhysicalDeviceEvent;

class TResponseData
{
        private:
                bool CallbackSet;
        public:
                Sync h;
                HANDLE * handle;
                int InvokeId;
                int iHandle;
                int TimeOut; //milissegundos
                void * ptrData;
                HANDLE * GetHandle()
                {
                	return handle;
                }
                void SetHandle(string Arg);

                TResponseData();
                ~TResponseData();
};

typedef struct
{
    TConnectionID connectionIdentifier;
    TLocalConnectionState localCallState;
} TSnapshotDeviceResponseInfo;


typedef struct
{
         TSnapshotDeviceResponseInfo snapshotDeviceItem[1000];
         int Count;
}TSnapshotDeviceResult;

typedef struct
{
    TDeviceID device;
    TConnectionID connectionIdentifier;
    TLocalConnectionState localCallState;
} TSnapshotCallResponseInfo;

typedef struct
{
         TSnapshotCallResponseInfo snapshotCallItem[100];
         int Count;
}TSnapshotCallResult;

typedef struct
{
    TCallControlEvent Event;
    TCallControlEventType EventType;
}TCallControlEventData;
typedef struct
{
    TLogicalDeviceEvent Event;
    TLogicalDeviceEventType EventType;
}TLogicalDeviceEventData;
typedef struct
{
    TPhysicalDeviceEvent Event;
    TPhysicalDeviceEventType EventType;
}TPhysicalDeviceEventData;

#endif
