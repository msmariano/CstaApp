
#include <sys/socket.h>
#include "CstaTypes.h"
#include "AsnNode.h"
#include "CallList.h"
#include <time.h>
#include <vector>
#include <pthread.h>
#include <semaphore.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <stdlib.h>
#include <iostream>
#include <stdio.h>
#include <netinet/in.h>
#include <netdb.h>
#define _CCSTA

#define srcAsnData  0xa6


using namespace std;


string DateToStr(time_t t);
int DateToInt(time_t t);
string IntToStr(int iArg);
string TimeToStr(time_t t);
bool TimeMaiorADiffB(int diff,time_t A,time_t B);


class CAsn
{

public:
	CAsn()
	{
		next = NULL;
		prior = NULL;
		data = NULL;
		type = 0;
		size = 0;


	}
	~CAsn()
	{
		if(next != NULL)
		{
			delete next;
		}
		if(data != NULL)
		{
			delete data;
		}

	}
	void  AllocData()
	{
		data = new CAsn();
		next->prior = this;

	}
	CAsn * AllocNext()
	{
		next = new CAsn();
		next->prior = this;
		return next;
	}

public:
	CAsn * next;
	CAsn * prior;
	CAsn * data;
	int type;
	int size;
	string info;



};

class TEventDispatcher
{
private:
	vector<TEventPointer*> events;

	void * ptrCallControlArg;
	void * ptrLogicalDeviceArg;
	void * ptrPhysicalDeviceArg;
	void * ptrDisconnectArg;
	void * ptrBeatArg;
	void * ptrSnapshotDeviceDataArg;


	void (* cbkCallControlEvent)(TCallControlEventType EventType, TCallControlEvent * Event, void * arg);
	void (* cbkLogicalDeviceEvent)(TLogicalDeviceEventType EventType, TLogicalDeviceEvent * Event, void * arg);
	void (* cbkPhysicalDeviceEvent)(TPhysicalDeviceEventType EventType, TPhysicalDeviceEvent * Event, void * arg);
	void (* cbkSnapshotDeviceData)(TSnapshotDeviceResult * snapshotData, void * arg);
	void (* cbkDisconnect)(void * arg);
	void (* cbkBeat)(void * arg);

	static void * thDispatchEvent(void * arg);
	sem_t sema;
	pthread_t hThread;
public:
	bool controle;
	void RegisterCallControlHandler(void (*)(TCallControlEventType, TCallControlEvent *, void *), void *);
	void RegisterLogicalDeviceHandler(void (*)(TLogicalDeviceEventType, TLogicalDeviceEvent *, void *), void *);
	void RegisterPhysicalDeviceHandler(void (* callback)(TPhysicalDeviceEventType, TPhysicalDeviceEvent*, void *), void * arg);
	void RegisterSnapshotDeviceDataHandler(void (* callback)(TSnapshotDeviceResult *, void *), void * arg);
	void RegisterDisconnectHandler(void (*)(void *), void*);
	void RegisterBeatHandler(void (*)(void *), void*);

	void AddCallControlEvent(TCallControlEventType EventType, TCallControlEvent * Event);
	void AddLogicalDeviceEvent(TLogicalDeviceEventType EventType, TLogicalDeviceEvent * Event);
	void AddPhysicalDeviceEvent(TPhysicalDeviceEventType EventType, TPhysicalDeviceEvent * Event);
	void AddSnapshotDeviceDataEvent(TSnapshotDeviceResult * snapshotData);
	void AddDisconnectEvent();
	void AddBeatEvent();
	void Clear();

	TEventDispatcher();
	~TEventDispatcher();
};



class TCSTABuffer
{
private:
	unsigned char mBuffer[CSTA_MAX_SIZE*2];
	unsigned int mBufferSize;
	unsigned int mHeaderSize;
	vector<TCSTAMessage*> messageList;
	void loghex(unsigned char * data, int size, string description);
public:
	void Clear();
	void Add(unsigned char * pBuffer, int dSize);
	void SetHeaderSize(unsigned int headerSize);
	unsigned int GetMessage(unsigned char * pBuffer, int dMaxSize);
	TCSTABuffer(unsigned int headerSize);
	~TCSTABuffer();
	bool bGlogs;
	string sPathlogs;
};



class CCSTA
{
private:
	int hBufSize;
	bool bVLogs;
	bool bGlogs;
	string sPathlogs;
	int dBufSize;
	unsigned int PacketSize;
	unsigned int TotalHeaderSize;
	int handle_socket;
	int currentInvokeId;
	bool socket_active;
	char * HeaderBuffer;
	char * DataBuffer;
	Sync initStatus;
	pthread_t  hMainLoop;
	pthread_t  hReceiveLoop;
	pthread_t  hCheckStatus;
	sem_t semaBuffer;
	sem_t sema;
	TEventDispatcher * eventDispatcher;
	
	vector<TResponseData*> CstaRequests;
	vector<TCSTAMonitorItem*>  monitorList;
	
	string GetDeviceByMonitorId(int crossRefId);
	TMonitorType GetMonitorType(int crossRefId);
	void DeleteMonitorItem(string device);

	void AddMonitorItem(string device, int crossRefId, TMonitorType monitorType);

	/*---------DEVICE ID-----------*/
	void GetDeviceID(CAsnNode * node, TDeviceID * DeviceID);
	void GetSubjectDeviceID(CAsnNode * node, TSubjectDeviceID * SubjectDeviceID);
	void GetCallingDeviceID(CAsnNode * node, TCallingDeviceID * CallingDeviceID);
	void GetCalledDeviceID(CAsnNode * node, TCalledDeviceID * CalledDeviceID);
	void GetNetworkCallingDeviceID(CAsnNode * node, TNetworkCallingDeviceID * NetworkCallingDeviceID);
	void GetNetworkCalledDeviceID(CAsnNode * node, TNetworkCalledDeviceID * NetworkCalledDeviceID);
	void GetAssociatedCallingDeviceID(CAsnNode * node, TAssociatedCallingDeviceID * AssociatedCallingDeviceID);
	void GetAssociatedCalledDeviceID(CAsnNode * node, TAssociatedCalledDeviceID * AssociatedCalledDeviceID);
	/*---------DEVICE ID-----------*/

	/*---------RESULTS---------*/
	void GetSnapshotDeviceResult(CAsnNode * result, TSnapshotDeviceResult * data);
	void GetSnapshotCallResult(CAsnNode * result, TSnapshotCallResult * data);

	/*---------RESULTS---------*/

	/*---------CONNECTION ID-----------*/
	void GetConnectionID(CAsnNode * node, TConnectionID * ConnectionID);
	void GetConnectionListItem(CAsnNode * node, TConnectionListItem * ConnectionListItem);
	void GetConnectionList(CAsnNode * node, TConnectionList * ConnectionList);
	/*---------CONNECTION ID-----------*/

	/*---------EVENTS-----------*/
	void CallControlEvent(CAsnNode * node, int crossRefIdentifier);
	void PhysicalDeviceEvent(CAsnNode * node, int crossRefIdentifier);
	void LogicalDeviceEvent(CAsnNode * node, int crossRefIdentifier);
	void CstaEvent(CAsnNode * event);
	void CstaSnapshotDeviceData(CAsnNode * event);
	void AgentBusyEvent(CAsnNode * node, TAgentBusy * AgentBusy);
	void AgentLoggedOnEvent(CAsnNode * node, TAgentLoggedOn * AgentLoggedOn );
	void AgentLoggedOffEvent(CAsnNode * node, TAgentLoggedOff * AgentLoggedOff);
	void AgentNotReadyEvent(CAsnNode * node, TAgentNotReady * AgentNotReady);
	void AgentReadyEvent(CAsnNode * node, TAgentReady * AgentReady);
	void AgentWorkingAfterCallEvent(CAsnNode * node, TAgentWorkingAfterCall * AgentWorkingAfterCall);

	//CallControl
	void ConnectionClearedEvent(CAsnNode * node, TConnectionCleared * ConnectionCleared);
	void EstablishedEvent(CAsnNode * node, TEstablished * Established);
	void DeliveredEvent(CAsnNode * node, TDelivered * Delivered);
	void FailedEvent(CAsnNode * node, TFailed * Failed);
	void HeldEvent(CAsnNode * node, THeld * Held);
	void DivertedEvent(CAsnNode * node, TDiverted * Diverted);
	void ServiceInitiatedEvent(CAsnNode * node, TServiceInitiated * ServiceInitiated);
	void OriginatedEvent(CAsnNode * node, TOriginated * Originated);
	void QueuedEvent(CAsnNode * node, TQueued * Queued);
	void RetrievedEvent(CAsnNode * node, TRetrieved * Retrieved);
	void NetworkReachedEvent(CAsnNode * node, TNetworkReached * NetworkReached);
	void ConferencedEvent(CAsnNode * node, TConferenced * Conferenced);
	void TransferredEvent(CAsnNode * node, TTransferred * Transferred);

	//PhysicalDevice
	void ButtonPressEvent(CAsnNode * node, TButtonPress * ButtonPress);
	/*---------EVENTS-----------*/

	/*---------LOG FUNCTIONS-----------*/
	void logevento(string evento);
	void loghex(unsigned char * data, int size, string description);
	void logerro(string erro);
	void insLog(string file, string text);
	string logObject(TDeviceID DeviceID, int nivel);
	string logObject(TConnectionID ConnectionID, int nivel);
	string logObject(TSubjectDeviceID SubjectDeviceID, int nivel);
	string logObject(TCallingDeviceID CallingDeviceID, int nivel);
	string logObject(TCalledDeviceID CalledDeviceID, int nivel);
	string logObject(TAssociatedCallingDeviceID AssociatedCallingDeviceID, int nivel);
	string logObject(TAssociatedCalledDeviceID AssociatedCalledDeviceID, int nivel);
	string logObject(TNetworkCallingDeviceID NetworkCallingDeviceID, int nivel);
	string logObject(TNetworkCalledDeviceID NetworkCalledDeviceID, int nivel);
	string logObject(TConnectionListItem ConnectionListItem, int nivel);
	string logObject(TConnectionList ConnectionList, int nivel);
	string logObject(TLocalConnectionState LocalConnectionState, int nivel);
	string logObject(TEventCause EventCause, int nivel);
	string logObject(TAgentState AgentState, int nivel);
	string logObject(TConferenced, int nivel);
	string logObject(TConnectionCleared, int nivel);
	string logObject(TDelivered, int nivel);
	string logObject(TDiverted, int nivel);
	string logObject(TEstablished, int nivel);
	string logObject(TFailed, int nivel);
	string logObject(THeld, int nivel);
	string logObject(TNetworkReached, int nivel);
	string logObject(TOriginated, int nivel);
	string logObject(TQueued, int nivel);
	string logObject(TRetrieved, int nivel);
	string logObject(TServiceInitiated, int nivel);
	string logObject(TTransferred, int nivel);

	string logObject(TAgentLoggedOn AgentLoggedOn, int nivel);
	string logObject(TAgentWorkingAfterCall AgentWorkingAfterCall, int nivel);
	string logObject(TAgentLoggedOff AgentLoggedOff, int nivel);
	string logObject(TAgentReady AgentReady, int nivel);
	string logObject(TAgentNotReady AgentNotReady, int nivel);
	string logObject(TAgentBusy AgentBusy, int nivel);
	/*---------LOG FUNCTIONS-----------*/

	/*---------SEND-----------*/
	int SocketSend(unsigned char * data, int size,string Who);
	void CstaSend(unsigned char * data,string Who);
	void CstaInvoke(unsigned char * data, void (*callback)(int result), int h = 0,string Who="");
	/*---------SEND-----------*/

	/*---------RECEIVE-----------*/
	void SocketRead(int handle);
	void ReceiveBuffer();
	void ProcessInvoke(CAsnNode * invoke);

	static void * checkStatus(void * arg);
	static void * CstaMainLoop(void * arg);
	static void * CstaReceiveLoop(void * arg);
	void ProcessResult(CAsnNode * invoke);
	void ProcessError(CAsnNode * invoke);
	/*---------RECEIVE-----------*/

	/*---------MAKE MESSAGE-----------*/
	int MakeInvokeId();
	CAsnNode * CreateConnectionId(int callId, char * deviceId);
	CAsnNode * CreateOperation(int operationValue, int invokeId, CAsnNode * sequence);
	unsigned char * SetDisplayMessage(char * device, char * text, int invokeId);
	unsigned char * GenerateDigitsMessage(int callId, char * device, char * digits, int invokeId);
	unsigned char * DialDigitsMessage(int callId, char * device, char * digits, int invokeId);
	unsigned char * ActHeartBeatMessage(int invokeId, short timeout);
	unsigned char * GetSystemStatusMessage(int invokeId);
	unsigned char * RequestSystemStatusMessage(int invokeId);
	unsigned char * SetForwardingMessage(char * device, char * destination, int forwardType, int Activate, int invokeId);
	unsigned char * ConferenceCallMessage(int heldCallId, char * heldDeviceId, int activeCallId, char * activeDeviceId);
	unsigned char * ParkCallMessage(int callId, int invokeId, char * parking_device, char * parkTo);
	unsigned char * GroupPickupMessage(int callId, int invokeId, char * requesting_device, char * topickup_device);
	unsigned char * SnapshotCallMessage(int callId, int invokeId, char * device);
	unsigned char * MonitorStartMessage(char * device, int invokeId);
	unsigned char * TrunkStartMessage(int device, int invokeId);
	unsigned char * MonitorStopMessage(int crossRefId, int invokeId);
	unsigned char * MakeAutentication(string VersionInfo="");
	unsigned char * MakeStatusResponse();
	unsigned char * MakeCallMessage(char * from, char * to, int invokeId);
	unsigned char * DeflectCallMessage(int callDiverted, char * deviceDiverted, char * newDestination, int invokeId);
	unsigned char * AlternateCallMessage(int callId, char * deviceId, int heldCallId, char * heldDeviceId, int invokeId);
	unsigned char * ReconnectCallMessage(int callId, char * deviceId, int heldCallId, char * heldDeviceId, int invokeId);
	unsigned char * TransferCallMessage(int callId, char * deviceId, int heldCallId, char * heldDeviceId, int invokeId);
	unsigned char * ClearConnectionMessage(int callId, char * deviceId, int invokeId);
	unsigned char * AnswerCallMessage(int callId, char * deviceId, int invokeId);
	unsigned char * HoldCallMessage(int callId, char * deviceId, int invokeId);
	unsigned char * RetrieveCallMessage(int callId, char * deviceId, int invokeId);
	unsigned char * SingleStepTransferMessage(int callToTransfer, char * deviceTransferring, char * newDestination, int invokeId);
	unsigned char * ConsultationCallMessage(int callId, char * deviceId, char * consultedDevice, int invokeId);
	unsigned char * SnapshotDeviceMessage(int invokeId, char * device);
	unsigned char * SetAgentStateMessage(int invokeId, char * device, TReqAgentState agentState, char * agentId = NULL);
	unsigned char * GetAgentStateMessage(int invokeId, char * device);
	/*---------MAKE MESSAGE-----------*/

	HANDLE * CreateResponseEvent(int invokeId,string Creater = "");
	//Sync CreateResponseEvent(int invokeId,string Creater = "");
	void * GetResponseData(int invokeId,char * src);
	void DeleteResponseEvent(int invokeId);
	string CrossRefToDevice(int crossRefIdentifier);
	/*---------CALLBACK---------*/
	void SendCallControlEvent(TCallControlEventType EventType, TCallControlEvent * Event);
	void SendLogicalDeviceEvent(TLogicalDeviceEventType EventType, TLogicalDeviceEvent * Event);
	void SendPhysicalDeviceEvent(TPhysicalDeviceEventType EventType, TPhysicalDeviceEvent * Event);
	void SendSnapshotDeviceDataEvent(TSnapshotDeviceResult * snapshotData);
	void SendDisconnectEvent();
	void SendBeatEvent();
	/*---------CALLBACK---------*/
	//void CstaReceive(unsigned char *data, unsigned int dataSize);
	/**/
	string decodeDelivered(vector <unsigned char> vPayload,unsigned char filtro);
	TCSTABuffer cstaBuffer;
	time_t lastBeat;
	bool beatActed;

	string logFolder;
public:
	string sClassName;
	string sEndCstaHost;
	int iPortaCstaHost;
	void InitThread();
	void logCsta(string evento);
	int GetMonitorIdByDevice(string device);
	void SetLogFolder(char * folder);
	string IntToStr(int Value);
	void RegisterCallControlHandler(void (*)(TCallControlEventType, TCallControlEvent *, void *), void *);
	void RegisterLogicalDeviceHandler(void (*)(TLogicalDeviceEventType, TLogicalDeviceEvent *, void *), void *);
	void RegisterPhysicalDeviceHandler(void (* callback)(TPhysicalDeviceEventType, TPhysicalDeviceEvent*, void *), void * arg);
	void RegisterSnapshotDeviceDataHandler(void (* callback)(TSnapshotDeviceResult *, void *), void * arg);
	void RegisterDisconnectHandler(void (*)(void *), void*);
	void RegisterBeatHandler(void (*)(void *), void *);

	void (* OnConnect)(int result);
	bool GetAgentState(char * device, string * agentId, TAgentState * agentState);
	bool SetAgentState(char * device, TReqAgentState agentState, char * agentId = NULL);
	bool SnapshotDevice(char * device, TSnapshotDeviceResult * resultData = NULL);
	bool DeflectCall(int callId, char * deviceId, char * newDestination);
	bool SetDisplay(char * deviceId, char * text);
	bool GenerateDigits(int callId, char * deviceId, char * digits);
	bool DialDigits(int callId, char * deviceId, char * digits);
	bool ConsultationCall(int callId, char * deviceId, char * newDestination);
	bool SingleStepTransfer(int callId, char * deviceId, char * newDestination);
	bool SnapshotCall(char * device, int callId, TSnapshotCallResult * resultData = NULL);
	bool SetForwarding(char * device, char * destination, int forwardType, bool Activate);
	bool MonitorStart(char * device, TMonitorType monitorType, int * crossRedId);
	bool MonitorStop(char * device);
	bool ClearConnection(int callId, char * device);
	bool AnswerCall(int callId, char * device);
	bool HoldCall(int callId, char * device);
	bool RetrieveCall(int callId, char * device);
	bool ParkCall(char * parking_device, char * parkTo, int callId);
	bool AlternateCall(int callId, char * device, int holdCallId, char * holdDevice);
	bool ReconnectCall(int callId, char * device, int holdCallId, char * holdDevice);
	bool TransferCall(int callId, char * device, int holdCallId, char * holdDevice);
	bool GroupPickup(char * requesting_device, char * topickup_device, int callId);
	bool ConferenceCall(int heldCallId, char * heldDeviceId, int activeCallId, char * activeDeviceId);
	bool MakeCall(char * from, char * to, int * callId = NULL);
	bool CstaConnect(string cstaHost, int cstaPort, bool requireAutentication,string VersionInfo);
	bool InitializeSocket(string host, int portno);
	void SetHeaderSize(int TotalHeaderSize);
	bool GetSystemStatus();
	int GetHeaderSize();
	bool RequestSystemStatus(int * status,string Who);
	bool ActHeartBeat(int timeout);
	void CstaDisconnect();
	void CstaReceive(unsigned char *data, unsigned int dataSize);
	bool MonitorExists(string device);
	void SetLogs(bool V,bool G,string sPathexe)
	{
		bVLogs = V;
		bGlogs = G;
		sPathlogs = sPathexe;
		cstaBuffer.bGlogs = G;
		cstaBuffer.sPathlogs = sPathexe;
	}
	CCSTA();
	~CCSTA();
	bool cstaConnected;
};


