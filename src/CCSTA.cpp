//============================================================================
// Name        : CCSTA.cpp
// Author      : Marcelo dos Santos Mariano (MSM).
// Version     : 1.0.0.0 - Linux
// Copyright   : Marcelo Mariano
// Description : Codificação e Decodificação Mensagens CSTA
//               11-12-2013 - Criação
//               18-02-2013 - Colocado shutdown para fechamento socket
//============================================================================


#include "../include/CCSTA.h"
#include <sstream>



#define DEFAULT_TIMEOUT 10
#define MAX_PACKET_SIZE 32768

using namespace std;
string gVersionInfo;


HANDLE * CreateEvent(string str)
{

	HANDLE * ret = new HANDLE;
	sem_t sema;
	sem_init(&sema, 0, 1);
	ret->sema = sema;
	ret->val =1;
	ret->str = str;
	return ret;
}
int WaitForSingleObject(HANDLE * id,int time)
{

	int ret = -1;
	while(true)
	{
		sem_wait(&id->sema);
		if(id->val == 0 || time == 0)
		{
			if(time == 0)
			{
				ret = 0;
			}
			else
				ret = 1;
			sem_post(&id->sema);
			break;
		}
		sem_post(&id->sema);
		if(time > -1)
		{
			time--;
			sleep(1);
		}
		usleep(1000);
	}

	return ret;
}

void SetEvent(HANDLE *  id)
{
	sem_wait(&id->sema);

	id->val = 0;

	sem_post(&id->sema);
}



bool TimeMaiorADiffB(int diff,time_t A,time_t B)
{
	if (difftime(A,B)> diff)
		return true;
	return false;
}

string TimeToStr(time_t t)
{
	struct tm *loctime;
	loctime = localtime (&t);
    char cTime[1024]="";
    strftime(cTime,sizeof(cTime),"%H%M%S",loctime);
    return string(cTime);
}
string DateToStr(time_t t)
{
	struct tm *loctime;
	loctime = localtime (&t);
    char cDate[1024]="";
    strftime(cDate,sizeof(cDate),"%Y%m%d",loctime);
    return string(cDate);
}

int DateToInt(time_t t)
{
	struct tm *loctime;
	loctime = localtime (&t);
    return loctime->tm_year  +  loctime->tm_mon + loctime->tm_yday ;
}

string IntToStr(int iArg)
{
	char ch[128]="";
	sprintf(ch,"%d",iArg);
	return string(ch);
}



//=================================================================================================
//                                  class TEventDispatcher inicio
//=================================================================================================
TEventDispatcher::TEventDispatcher()
{
	cbkCallControlEvent = NULL;
	cbkLogicalDeviceEvent = NULL;
	cbkPhysicalDeviceEvent = NULL;
	cbkSnapshotDeviceData = NULL;
	cbkDisconnect = NULL;
	cbkBeat=NULL;
	sem_init(&sema, 0, 1);
	controle = true;
	pthread_create(&hThread,0,thDispatchEvent,this);
}

TEventDispatcher::~TEventDispatcher()
{
	controle=false;
	pthread_cancel(hThread);
	pthread_join(hThread, NULL);
	sem_destroy(&sema);
}

void TEventDispatcher::RegisterCallControlHandler(void (*callback)(TCallControlEventType, TCallControlEvent *, void *), void *arg)
{
	ptrCallControlArg = arg;
	cbkCallControlEvent = callback;
}
void TEventDispatcher::RegisterLogicalDeviceHandler(void (*callback)(TLogicalDeviceEventType, TLogicalDeviceEvent *, void *), void *arg)
{
	ptrLogicalDeviceArg = arg;
	cbkLogicalDeviceEvent = callback;
}
void TEventDispatcher::RegisterPhysicalDeviceHandler(void (* callback)(TPhysicalDeviceEventType, TPhysicalDeviceEvent*, void *), void * arg)
{
	ptrPhysicalDeviceArg = arg;
	cbkPhysicalDeviceEvent = callback;
}
void TEventDispatcher::RegisterSnapshotDeviceDataHandler(void (* callback)(TSnapshotDeviceResult *, void *), void * arg)
{
	ptrSnapshotDeviceDataArg = arg;
	cbkSnapshotDeviceData = callback;
}
void TEventDispatcher::RegisterDisconnectHandler(void (*callback)(void *), void*arg)
{
	ptrDisconnectArg = arg;
	cbkDisconnect = callback;
}

void TEventDispatcher::RegisterBeatHandler(void (*callback)(void *), void*arg)
{
	ptrBeatArg = arg;
	cbkBeat = callback;
}

void TEventDispatcher::AddCallControlEvent(TCallControlEventType EventType, TCallControlEvent * Event)
{
	TEventPointer * eventPointer;
	TCallControlEventData * eventData;
	sem_wait(&sema);
	try
	{
		eventPointer = new TEventPointer;
		eventPointer->eventType = etCallControl;

		eventData = new TCallControlEventData;
		eventData->Event = *Event;
		eventData->EventType = EventType;

		eventPointer->eventData = eventData;

		events.push_back(eventPointer);
	}
	catch(...)
	{
	}
	sem_post(&sema);

}
void TEventDispatcher::AddLogicalDeviceEvent(TLogicalDeviceEventType EventType, TLogicalDeviceEvent * Event)
{
	TEventPointer * eventPointer;
	TLogicalDeviceEventData * eventData;

	sem_wait(&sema);
	try
	{
		eventPointer = new TEventPointer;
		eventPointer->eventType = etLogicalDevice;

		eventData = new TLogicalDeviceEventData;
		eventData->Event = *Event;
		eventData->EventType = EventType;

		eventPointer->eventData = eventData;

		events.push_back(eventPointer);
	}
	catch(...)
	{
	}
	sem_post(&sema);
}
void TEventDispatcher::AddPhysicalDeviceEvent(TPhysicalDeviceEventType EventType, TPhysicalDeviceEvent * Event)
{
	TEventPointer * eventPointer;
	TPhysicalDeviceEventData * eventData;

	sem_wait(&sema);
	try
	{
		eventPointer = new TEventPointer;
		eventPointer->eventType = etPhysicalDevice;

		eventData = new TPhysicalDeviceEventData;
		eventData->Event = *Event;
		eventData->EventType = EventType;

		eventPointer->eventData = eventData;

		events.push_back(eventPointer);

	}
	catch(...)
	{

	}
	sem_post(&sema);
}
void TEventDispatcher::AddSnapshotDeviceDataEvent(TSnapshotDeviceResult * snapshotData)
{
	TEventPointer * eventPointer;
	TSnapshotDeviceResult * eventData;

	sem_wait(&sema);
	try
	{
		eventPointer = new TEventPointer;
		eventPointer->eventType = etSnapshotDeviceData;

		eventData = new TSnapshotDeviceResult;
		*eventData = *snapshotData;

		eventPointer->eventData = eventData;

		events.push_back(eventPointer);

	}
	catch(...)
	{

	}
	sem_post(&sema);
}
void TEventDispatcher::AddDisconnectEvent()
{
	TEventPointer * eventPointer;

	sem_wait(&sema);
	try
	{
		eventPointer = new TEventPointer;
		eventPointer->eventType = etDisconnect;
		eventPointer->eventData = NULL;

		events.push_back(eventPointer);

	}
	catch(...)
	{
	}
	sem_post(&sema);
}

void TEventDispatcher::AddBeatEvent()
{
	TEventPointer * eventPointer;

	sem_wait(&sema);
	try
	{
		eventPointer = new TEventPointer;
		eventPointer->eventType = etBeat;
		eventPointer->eventData = NULL;

		events.push_back(eventPointer);

	}
	catch(...)
	{

	}
	sem_post(&sema);
}
void TEventDispatcher::Clear()
{
	events.erase(events.begin(),events.end());
}
void * TEventDispatcher::thDispatchEvent(void * arg)
{
	TEventDispatcher * ptrDispatcher;
	ptrDispatcher = (TEventDispatcher *) arg;

	TEventPointer * eventPointer;
	while(1)
	{
		try
		{
			eventPointer = NULL;
			sem_wait(&ptrDispatcher->sema);


			try
			{
				if(ptrDispatcher->events.size())
				{

					eventPointer = (TEventPointer *)ptrDispatcher->events[0];
					ptrDispatcher->events.erase(ptrDispatcher->events.begin());
				}
			}catch(...)
			{

			}
			sem_post(&ptrDispatcher->sema);
			if(eventPointer)
			{
				try
				{
					switch(eventPointer->eventType)
					{
					case etCallControl:
					{
						TCallControlEventData * eventData;
						eventData = (TCallControlEventData *)eventPointer->eventData;

						if(ptrDispatcher->cbkCallControlEvent)
						{
							ptrDispatcher->cbkCallControlEvent(eventData->EventType, &eventData->Event, ptrDispatcher->ptrCallControlArg);
						}
						delete eventData;
					}
					break;
					case etLogicalDevice:
					{
						TLogicalDeviceEventData * eventData;
						eventData = (TLogicalDeviceEventData *)eventPointer->eventData;

						if(ptrDispatcher->cbkLogicalDeviceEvent)
						{
							ptrDispatcher->cbkLogicalDeviceEvent(eventData->EventType, &eventData->Event, ptrDispatcher->ptrLogicalDeviceArg);
						}
						delete eventData;
					}
					break;
					case etPhysicalDevice:
					{
						TPhysicalDeviceEventData * eventData;
						eventData = (TPhysicalDeviceEventData *)eventPointer->eventData;

						if(ptrDispatcher->cbkPhysicalDeviceEvent)
						{
							ptrDispatcher->cbkPhysicalDeviceEvent(eventData->EventType, &eventData->Event, ptrDispatcher->ptrPhysicalDeviceArg);
						}
						delete eventData;
					}
					break;
					case etDisconnect:
					{
						if(ptrDispatcher->cbkDisconnect)
						{
							ptrDispatcher->cbkDisconnect(ptrDispatcher->ptrDisconnectArg);
						}
					}
					break;
					case etBeat:
					{
						if(ptrDispatcher->cbkBeat)
						{
							ptrDispatcher->cbkBeat(ptrDispatcher->ptrBeatArg);
						}
					}
					break;
					case etSnapshotDeviceData:
					{
						TSnapshotDeviceResult * eventData;
						eventData = (TSnapshotDeviceResult *)eventPointer->eventData;

						if(ptrDispatcher->cbkSnapshotDeviceData)
						{
							ptrDispatcher->cbkSnapshotDeviceData(eventData, ptrDispatcher->ptrSnapshotDeviceDataArg);
						}
						delete eventData;
					}
					break;
					}
				}catch(...)
				{

				}
				delete eventPointer;
			}
		}catch(...)
		{
		}

		usleep(30000);
	}
	pthread_exit(0);
}
//=================================================================================================
//                                       TEventDispatcher fim
//=================================================================================================


int CCSTA::MakeInvokeId()
{
	int result;

	sem_wait(&sema);

	try
	{

		result = currentInvokeId++;

	}
	catch(...)
	{

	}
	sem_post(&sema);

	return result;
}



string RetornoHex(BYTE * Mensagem,int t)
{
	try
	{
		string S,S1;
		S1+"'";

		for (int k =0; k < t ;k++)
		{
			char cbyte[8]="";
			if ( k > 0)
				sprintf(cbyte," %.2X",Mensagem[k]);
			else
				sprintf(cbyte,"%.2X",Mensagem[k]);
			S = string(cbyte)+" ";
			S1+=S;
		}
		S1+"'H";
		return S1;
	}
	catch(...)
	{
	}
	return "";
}

CCSTA::CCSTA() : cstaBuffer(3)
{
	//logCsta("!!!Instanciando CCSTA!!!");
	hBufSize = 0;
	dBufSize = 0;
	PacketSize = 0;
	TotalHeaderSize = 3;
	handle_socket = 0;
	currentInvokeId = 1;
	beatActed = false;
	socket_active = false;
	HeaderBuffer = new char[3];
	DataBuffer= new char[MAX_PACKET_SIZE];
	logFolder = ".logs/";
	OnConnect = NULL;
	sem_init(&semaBuffer, 0, 1);
	sem_init(&sema, 0, 1);
	cstaConnected = false;
	sClassName = "CCSTA";
	//eventDispatcher = new TEventDispatcher;

}

CCSTA::~CCSTA()
{
	for(unsigned i = 0;i < CstaRequests.size();i++)
	{
		TRequest * ptrRequest;
		ptrRequest = (TRequest *)CstaRequests[i];
		delete ptrRequest;
	}

	for(unsigned i = 0;i < monitorList.size();i++)
	{
		TCSTAMonitorItem * ptrMonitor;
		ptrMonitor = (TCSTAMonitorItem *)monitorList[i];
		delete ptrMonitor;
	}
	try
	{
	 shutdown(handle_socket,SHUT_RDWR);
	}
	catch(...)
	{
	}
	close(handle_socket);

	//pthread_cancel(hMainLoop);
	//pthread_cancel(hCheckStatus);

	delete [] DataBuffer;
	delete HeaderBuffer;
	/*delete eventDispatcher;*/
}

void CCSTA::InitThread()
{
	logCsta("!!!Incializando threads!!!");
	eventDispatcher = new TEventDispatcher;
	pthread_create(&hMainLoop,0,CstaMainLoop,this);
	pthread_create(&hReceiveLoop,0,CstaReceiveLoop,this);
	pthread_create(&hCheckStatus,0,checkStatus,this);
}

void CCSTA::loghex(unsigned char * data, int size, string description)
{
	string log;

	try{
		for(int i = 0;i<size;i++)
		{
			char cbyte[80]="";
			string sConv;
			bool bConcat;
			if(i%16==0&&i!=0)
			{
				int j=i-16;
				sConv="";
				bConcat=true;
				for(;j<i;j++)
				{
					if((data[j]>='A'&&data[j]<='Z')
							|| (data[j]>='a'&&data[j]<='z')
							|| (data[j]>='0'&&data[j]<='9'))
					{
						char c[8]="";
						sprintf(c," %c",data[j]);
						sConv+=c;
					}
					else
						sConv+=".";
				}
				sConv+="\r\n";
				sprintf(cbyte,"%.2X ", data[i]);
			}
			else if(i%8==0&&i!=0)
				sprintf(cbyte,"   %.2X ", data[i]);
			else
				sprintf(cbyte,"%.2X ", data[i]);

			if(bConcat)
			{
				bConcat=false;
				log+=sConv;
			}
			log+=cbyte;
		}
		if((size-((size/16)*16))>0)
		{
			string sConv;
			int j = (size/16)*16;
			sConv="";
			for(;j<size;j++)
			{
				if((data[j]>='A'&&data[j]<='Z')
						|| (data[j]>='a'&&data[j]<='z')
						|| (data[j]>='0'&&data[j]<='9'))
				{
					char c[8]="";
					sprintf(c," %c",data[j]);
					sConv+=c;
				}
				else
					sConv+=".";
			}
			log+=sConv;
		}
	}catch(...)
	{
	}

	try
	{
		logCsta(description + string("\r\n")
				+ log
				+ string("\r\n")
				+ IntToStr(size)
				+ string(" Byte(s)\r\n"));
	}
	catch(...)
	{
	}
}

void CCSTA::logCsta(string logData)
{
	try
	{
		if(bVLogs)
		{
		 cout << logData << endl;
		}
		if(bGlogs)
		{
			FILE * arq = fopen(string(sPathlogs+"logCsta"+DateToStr(time(NULL))+".txt").c_str(),"a");
			if ( arq != NULL)
			{
				string log = DateToStr(time(NULL)) +" "+ TimeToStr(time(NULL)) +" "+logData+"\r\n";
				fputs(log.c_str(),arq);
				fclose(arq);
			}
		}
	}
	catch(...)
	{
	}
}




void CCSTA::logevento(string evento)
{
	try
	{
		//salvar log aqui
	}
	catch(...)
	{
	}
}

void CCSTA::logerro(string erro)
{
	try
	{
		//salvar log aqui
	}
	catch(...)
	{
	}
}

int CCSTA::SocketSend(unsigned char * data, int size,string Who)
{
	try
	{
		loghex(data, size, "SocketSend["+IntToStr(size)+"] por "+Who+": ");
		return send(handle_socket,(char*)data,size,0);
	}catch(...)
	{

	}
	return 0;
}

void CCSTA::CstaSend(unsigned char * data,string Who)
{
	try{
		int dataSize;
		unsigned char * sendbuf;

		CAsnNode * sent = new CAsnNode(data);
		//sent->logAsn(string("\nMESSAGE SENT:\n"+sent->LogText(0)));
		delete sent;
		dataSize = data[1];

		sendbuf = (unsigned char*)new char[dataSize+5];
		if(TotalHeaderSize == 3)
		{
			sendbuf[0] = 0x26;
			sendbuf[1] = 0x80;
			sendbuf[2] = dataSize+2;
		}
		else
		{
			sendbuf[0] = 0;
			sendbuf[1] = dataSize+2;
		}


		memcpy(&sendbuf[TotalHeaderSize], data, dataSize+2);
		SocketSend(sendbuf , dataSize+TotalHeaderSize+2,Who);
		delete sendbuf;
	}catch(...)
	{

	}
}

void CCSTA::CstaInvoke(unsigned char * data, void (*callback)(int result), int h,string Who)
{
	try
	{
		int dataSize;
		int requestType;
		unsigned char * sendbuf;
		TRequest *request;
		CAsnNode * sent = new CAsnNode(data);
		string aaa = "\nMESSAGE SENT:\n"+sent->LogText(0);
		/*sent->logAsn(aaa.c_str());*/
		delete sent;

		requestType = data[0];
		dataSize = data[1];

		request = new TRequest;
		request->h = h;
		request->reqType = requestType;
		request->callback = callback;

		//CstaRequests.push_back(request);

		sendbuf = (unsigned char*)new char[dataSize+5];
		if(TotalHeaderSize == 3)
		{
			sendbuf[0] = 0x26;
			sendbuf[1] = 0x80;
			sendbuf[2] = dataSize+2;
		}
		else
		{
			sendbuf[0] = 0;
			sendbuf[1] = dataSize+2;
		}
		memcpy(&sendbuf[TotalHeaderSize], data, dataSize+2);
		SocketSend(sendbuf , dataSize+5,Who);
		delete sendbuf;
	}catch(...)
	{
	}
}



unsigned char * CCSTA::MakeStatusResponse()
{
	char data[] = {0xA2,0x0B,0x02,0x01,0x01,0x30,
			0x06, 0x02, 0x02, 0x00, 0xD3, 0x05, 0x00};


	unsigned char * output;
	output = (unsigned char*)new char[sizeof(data)];
	memcpy(output, data, sizeof(data));
	try
	{
		/*CAsnNode * main;
                CAsnNode * invoke_id;
                CAsnNode * sequence;
                CAsnNode * operation_value;
                CAsnNode * result;

                main = new CAsnNode(ncContext_Specific, ntConstructed, 2);
                invoke_id = new CAsnNode(ncUniversal, ntPrimitive, 2);
                sequence = new CAsnNode(ncUniversal, ntConstructed, 16);
                operation_value = new CAsnNode(ncUniversal, ntPrimitive, 2);
                result = new CAsnNode(ncUniversal, ntPrimitive, 5);

                invoke_id->Add(1);

                operation_value->AddInv(0xD300);

                sequence->Add(operation_value);
                sequence->Add(result);

                main->Add(invoke_id);
                main->Add(sequence);
                output = main->c_str();
                delete main;  */



	}catch(...)
	{

	}
	return (unsigned char*)output;
}


void CCSTA::GetDeviceID(CAsnNode * node, TDeviceID * DeviceID)
{
	CAsnNode *tempNode1 = NULL,
			*tempNode2 = NULL;

	string DialingNumber;
	int DeviceNumber;
	DeviceNumber = 0;
	if(node!=NULL)
	{
		if((tempNode1 = node->GetSpecificSequence(ncUniversal, 16))!=NULL)
		{
			if((tempNode2 = tempNode1->GetSpecificSequence(ncContext_Specific,0))!=NULL)
			{
				DialingNumber = tempNode2->GetStringData();
			}
			if((tempNode2 = tempNode1->GetSpecificSequence(ncContext_Specific,2))!=NULL)
			{
				DialingNumber = tempNode2->GetStringData();
			}
			if((tempNode2 = tempNode1->GetSpecificSequence(ncContext_Specific,4))!=NULL)
			{
				DialingNumber = tempNode2->GetStringData();
			}

			if((tempNode2 = tempNode1->GetSpecificSequence(ncContext_Specific,1))!=NULL)
			{
				DeviceNumber = tempNode2->GetIntegerData();
			}
		}
	}
	DeviceID->DialingNumber = DialingNumber;
	DeviceID->DeviceNumber = DeviceNumber;
}

void CCSTA::GetSubjectDeviceID(CAsnNode * node, TSubjectDeviceID * SubjectDeviceID)
{
	CAsnNode *tempNode1 = NULL,
			*tempNode2 = NULL;

	string DialingNumber;
	int DeviceNumber;
	DeviceNumber = 0;
	if(node!=NULL)
	{
		if((tempNode1 = node->GetSpecificSequence(ncUniversal, 16))!=NULL)
		{
			if((tempNode2 = tempNode1->GetSpecificSequence(ncContext_Specific,0))!=NULL)
			{
				DialingNumber = tempNode2->GetStringData();
			}
			if((tempNode2 = tempNode1->GetSpecificSequence(ncContext_Specific,2))!=NULL)
			{
				DialingNumber = tempNode2->GetStringData();
			}
			if((tempNode2 = tempNode1->GetSpecificSequence(ncContext_Specific,4))!=NULL)
			{
				DialingNumber = tempNode2->GetStringData();
			}

			if((tempNode2 = tempNode1->GetSpecificSequence(ncContext_Specific,1))!=NULL)
			{
				DeviceNumber = tempNode2->GetIntegerData();
			}
		}
	}
	SubjectDeviceID->DeviceID.DialingNumber = DialingNumber;
	SubjectDeviceID->DeviceID.DeviceNumber = DeviceNumber;
}

void CCSTA::GetCallingDeviceID(CAsnNode * node, TCallingDeviceID * CallingDeviceID)
{
	CAsnNode *tempNode1 = NULL,
			*tempNode2 = NULL;

	string DialingNumber;
	int DeviceNumber;
	DeviceNumber = 0;
	if(node!=NULL)
	{
		if((tempNode1 = node->GetSpecificSequence(ncUniversal, 16))!=NULL)
		{
			if((tempNode2 = tempNode1->GetSpecificSequence(ncContext_Specific,0))!=NULL)
			{
				DialingNumber = tempNode2->GetStringData();
			}
			if((tempNode2 = tempNode1->GetSpecificSequence(ncContext_Specific,2))!=NULL)
			{
				DialingNumber = tempNode2->GetStringData();
			}
			if((tempNode2 = tempNode1->GetSpecificSequence(ncContext_Specific,4))!=NULL)
			{
				DialingNumber = tempNode2->GetStringData();
			}

			if((tempNode2 = tempNode1->GetSpecificSequence(ncContext_Specific,1))!=NULL)
			{
				DeviceNumber = tempNode2->GetIntegerData();
			}
		}
	}
	CallingDeviceID->DeviceID.DialingNumber = DialingNumber;
	CallingDeviceID->DeviceID.DeviceNumber = DeviceNumber;
}

void CCSTA::GetCalledDeviceID(CAsnNode * node, TCalledDeviceID * CalledDeviceID)
{
	CAsnNode *tempNode1 = NULL,
			*tempNode2 = NULL;

	string DialingNumber;
	int DeviceNumber;
	DeviceNumber = 0;
	if(node!=NULL)
	{
		if((tempNode1 = node->GetSpecificSequence(ncUniversal, 16))!=NULL)
		{
			if((tempNode2 = tempNode1->GetSpecificSequence(ncContext_Specific,0))!=NULL)
			{
				DialingNumber = tempNode2->GetStringData();
			}
			if((tempNode2 = tempNode1->GetSpecificSequence(ncContext_Specific,2))!=NULL)
			{
				DialingNumber = tempNode2->GetStringData();
			}
			if((tempNode2 = tempNode1->GetSpecificSequence(ncContext_Specific,4))!=NULL)
			{
				DialingNumber = tempNode2->GetStringData();
			}

			if((tempNode2 = tempNode1->GetSpecificSequence(ncContext_Specific,1))!=NULL)
			{
				DeviceNumber = tempNode2->GetIntegerData();
			}
		}
	}
	CalledDeviceID->DeviceID.DialingNumber = DialingNumber;
	CalledDeviceID->DeviceID.DeviceNumber = DeviceNumber;
}

void CCSTA::GetNetworkCallingDeviceID(CAsnNode * node, TNetworkCallingDeviceID * NetworkCallingDeviceID)
{
	CAsnNode *tempNode1 = NULL,
			*tempNode2 = NULL;

	string DialingNumber;
	int DeviceNumber;
	DeviceNumber = 0;
	if(node!=NULL)
	{
		if((tempNode1 = node->GetSpecificSequence(ncUniversal, 16))!=NULL)
		{
			if((tempNode2 = tempNode1->GetSpecificSequence(ncContext_Specific,0))!=NULL)
			{
				DialingNumber = tempNode2->GetStringData();
			}
			if((tempNode2 = tempNode1->GetSpecificSequence(ncContext_Specific,2))!=NULL)
			{
				DialingNumber = tempNode2->GetStringData();
			}
			if((tempNode2 = tempNode1->GetSpecificSequence(ncContext_Specific,4))!=NULL)
			{
				DialingNumber = tempNode2->GetStringData();
			}

			if((tempNode2 = tempNode1->GetSpecificSequence(ncContext_Specific,1))!=NULL)
			{
				DeviceNumber = tempNode2->GetIntegerData();
			}
		}
	}
	NetworkCallingDeviceID->DeviceID.DialingNumber = DialingNumber;
	NetworkCallingDeviceID->DeviceID.DeviceNumber = DeviceNumber;
}

void CCSTA::GetNetworkCalledDeviceID(CAsnNode * node, TNetworkCalledDeviceID * NetworkCalledDeviceID)
{
	CAsnNode *tempNode1 = NULL,
			*tempNode2 = NULL;

	string DialingNumber;
	int DeviceNumber;
	DeviceNumber = 0;
	if(node!=NULL)
	{
		if((tempNode1 = node->GetSpecificSequence(ncUniversal, 16))!=NULL)
		{
			if((tempNode2 = tempNode1->GetSpecificSequence(ncContext_Specific,0))!=NULL)
			{
				DialingNumber = tempNode2->GetStringData();
			}
			if((tempNode2 = tempNode1->GetSpecificSequence(ncContext_Specific,2))!=NULL)
			{
				DialingNumber = tempNode2->GetStringData();
			}
			if((tempNode2 = tempNode1->GetSpecificSequence(ncContext_Specific,4))!=NULL)
			{
				DialingNumber = tempNode2->GetStringData();
			}

			if((tempNode2 = tempNode1->GetSpecificSequence(ncContext_Specific,1))!=NULL)
			{
				DeviceNumber = tempNode2->GetIntegerData();
			}
		}
	}
	NetworkCalledDeviceID->DeviceID.DialingNumber = DialingNumber;
	NetworkCalledDeviceID->DeviceID.DeviceNumber = DeviceNumber;
}

void CCSTA::GetAssociatedCallingDeviceID(CAsnNode * node, TAssociatedCallingDeviceID * AssociatedCallingDeviceID)
{
	CAsnNode *tempNode1 = NULL,
			*tempNode2 = NULL;

	string DialingNumber;
	int DeviceNumber;
	DeviceNumber = 0;
	if(node!=NULL)
	{
		if((tempNode1 = node->GetSpecificSequence(ncUniversal, 16))!=NULL)
		{
			if((tempNode2 = tempNode1->GetSpecificSequence(ncContext_Specific,0))!=NULL)
			{
				DialingNumber = tempNode2->GetStringData();
			}
			if((tempNode2 = tempNode1->GetSpecificSequence(ncContext_Specific,2))!=NULL)
			{
				DialingNumber = tempNode2->GetStringData();
			}
			if((tempNode2 = tempNode1->GetSpecificSequence(ncContext_Specific,4))!=NULL)
			{
				DialingNumber = tempNode2->GetStringData();
			}

			if((tempNode2 = tempNode1->GetSpecificSequence(ncContext_Specific,1))!=NULL)
			{
				DeviceNumber = tempNode2->GetIntegerData();
			}
		}
	}
	AssociatedCallingDeviceID->DeviceID.DialingNumber = DialingNumber;
	AssociatedCallingDeviceID->DeviceID.DeviceNumber = DeviceNumber;
}

void CCSTA::GetAssociatedCalledDeviceID(CAsnNode * node, TAssociatedCalledDeviceID * AssociatedCalledDeviceID)
{
	CAsnNode *tempNode1 = NULL,
			*tempNode2 = NULL;

	string DialingNumber;
	int DeviceNumber;
	DeviceNumber = 0;
	if(node!=NULL)
	{
		if((tempNode1 = node->GetSpecificSequence(ncUniversal, 16))!=NULL)
		{
			if((tempNode2 = tempNode1->GetSpecificSequence(ncContext_Specific,0))!=NULL)
			{
				DialingNumber = tempNode2->GetStringData();
			}
			if((tempNode2 = tempNode1->GetSpecificSequence(ncContext_Specific,2))!=NULL)
			{
				DialingNumber = tempNode2->GetStringData();
			}
			if((tempNode2 = tempNode1->GetSpecificSequence(ncContext_Specific,4))!=NULL)
			{
				DialingNumber = tempNode2->GetStringData();
			}

			if((tempNode2 = tempNode1->GetSpecificSequence(ncContext_Specific,1))!=NULL)
			{
				DeviceNumber = tempNode2->GetIntegerData();
			}
		}
	}
	AssociatedCalledDeviceID->DeviceID.DialingNumber = DialingNumber;
	AssociatedCalledDeviceID->DeviceID.DeviceNumber = DeviceNumber;
}

void CCSTA::GetConnectionID(CAsnNode * node, TConnectionID * ConnectionID)
{
	int CallID;
	CAsnNode
	*tempNode1 = NULL,
	*tempNode2 = NULL,
	*pDevice = NULL;
	CallID = 0;
	string sCallID;

	if(node!=NULL)
	{
		if((tempNode1 = node->GetSpecificSequence(ncUniversal, 16))!=NULL)
		{
			if((tempNode2 = tempNode1->GetSpecificSequence(ncContext_Specific,0))!=NULL)
			{

				if (gVersionInfo == "Panasonic")
				{
					sCallID = tempNode2->GetStringData();

					unsigned int x;
					stringstream ss;
					ss << std::hex << sCallID;
					ss >> x;

					CallID = (int)x;
				}
				else
					CallID = tempNode2->GetIntegerData();


			}

			pDevice = tempNode1->GetSpecificSequence(ncContext_Specific,1);
		}else if((tempNode1 = node->GetSpecificSequence(ncContext_Specific, 0))!=NULL)
		{
			CallID = tempNode1->GetIntegerData();
		}else if((tempNode1 = node->GetSpecificSequence(ncContext_Specific, 1))!=NULL)
		{
			pDevice = tempNode1;
		}
	}

	GetDeviceID(pDevice, &ConnectionID->DeviceID);
	ConnectionID->CallID = CallID;
}

void CCSTA::GetConnectionListItem(CAsnNode * node, TConnectionListItem * ConnectionListItem)
{
	CAsnNode
	*pNewConnection = NULL,
	*pOldConnection = NULL,
	*pEndPoint = NULL,
	*pAssociatedNID = NULL,
	*tempNode1 = NULL,
	*tempNode2 = NULL;
	if(node!=NULL)
	{
		if((tempNode1 = node)!=NULL)
		{
			if((tempNode2 = tempNode1->GetSpecificSequence(ncContext_Specific, 0))!=NULL)
			{
				pNewConnection = tempNode2->GetSpecificSequence(ncApplication, 11);
			}
			if((tempNode2 = tempNode1->GetSpecificSequence(ncContext_Specific, 1))!=NULL)
			{
				pOldConnection = tempNode2->GetSpecificSequence(ncApplication, 11);
			}
			pEndPoint = tempNode1->GetSpecificSequence(ncContext_Specific, 2);
			pAssociatedNID = tempNode1->GetSpecificSequence(ncContext_Specific, 3);
		}

	}

	GetConnectionID(pNewConnection, &ConnectionListItem->newConnection);
	GetConnectionID(pOldConnection, &ConnectionListItem->oldConnection);
	GetDeviceID(pEndPoint, &ConnectionListItem->endPoint);
	GetDeviceID(pAssociatedNID, &ConnectionListItem->associatedNID);
}

void CCSTA::GetConnectionList(CAsnNode * node, TConnectionList * ConnectionList)
{
	int Count;
	CAsnNode*tempNode = NULL;
	Count = 0;
	if(node != NULL)
	{
		while((tempNode = node->GetSpecificSequence(ncUniversal, 16))!=NULL)
		{
			GetConnectionListItem(tempNode, &ConnectionList->Connections[Count]);
			Count++;
		}
	}
	ConnectionList->Count = Count;
}

void CCSTA::ConnectionClearedEvent(CAsnNode * node, TConnectionCleared * ConnectionCleared)
{
	TLocalConnectionState LocalConnectionInfo;
	TEventCause Cause;
	CAsnNode
	*pDroppedConnection = NULL,
	*pReleasingDevice = NULL,
	*tempNode = NULL;

	Cause = (TEventCause)0;
	LocalConnectionInfo = csNull;
	if(node!=NULL)
	{
		pDroppedConnection = node->GetSpecificSequence(ncApplication, 11);
		pReleasingDevice = node->GetSpecificSequence(ncApplication, 3);
		if((tempNode = node->GetSpecificSequence(ncApplication, 14))!=NULL)
		{
			LocalConnectionInfo = (TLocalConnectionState)tempNode->GetIntegerData();
		}
		if((tempNode = node->GetSpecificSequence(ncUniversal, 10))!=NULL)
		{
			Cause = (TEventCause)tempNode->GetIntegerData();
		}
	}

	GetConnectionID(pDroppedConnection, &ConnectionCleared->DroppedConnection);
	GetSubjectDeviceID(pReleasingDevice, &ConnectionCleared->ReleasingDevice);
	ConnectionCleared->Cause = Cause;
	ConnectionCleared->LocalConnectionInfo = LocalConnectionInfo;

}

void CCSTA::EstablishedEvent(CAsnNode * node, TEstablished * Established)
{

	TLocalConnectionState LocalConnectionInfo;
	TEventCause Cause;
	CAsnNode
	*pEstablishedConnection = NULL,
	*pAnsweringDevice = NULL,
	*pCallingDevice = NULL,
	*pCalledDevice = NULL,
	*pOriginatingNIDConnection = NULL,
	*pNetworkCallingDeviceID = NULL,
	*pNetworkCalledDeviceID = NULL,
	*pAssociatedCallingDeviceID = NULL,
	*pAssociatedCalledDeviceID = NULL,
	*tempNode = NULL;

	Cause = (TEventCause)0;
	LocalConnectionInfo = csNull;

	if(node!=NULL)
	{
		pEstablishedConnection = node->GetSpecificSequence(ncApplication, 11);
		pAnsweringDevice = node->GetSpecificSequence(ncApplication, 3);
		pCallingDevice = node->GetSpecificSequence(ncApplication, 1);
		pCalledDevice = node->GetSpecificSequence(ncApplication, 2);
		pOriginatingNIDConnection = node->GetSpecificSequence(ncApplication, 11);
		if((tempNode = node->GetSpecificSequence(ncApplication, 14))!=NULL)
		{
			LocalConnectionInfo = (TLocalConnectionState)tempNode->GetIntegerData();
		}
		if((tempNode = node->GetSpecificSequence(ncUniversal, 10))!=NULL)
		{
			Cause = (TEventCause)tempNode->GetIntegerData();
		}
		pNetworkCallingDeviceID = node->GetSpecificSequence(ncApplication,7);
		pNetworkCalledDeviceID = node->GetSpecificSequence(ncApplication,8);
		pAssociatedCallingDeviceID = node->GetSpecificSequence(ncApplication, 5);
		pAssociatedCalledDeviceID = node->GetSpecificSequence(ncApplication, 6);

	}

	GetConnectionID( pEstablishedConnection, &Established->EstablishedConnection);
	GetSubjectDeviceID(pAnsweringDevice, &Established->AnsweringDevice);
	GetCallingDeviceID(pCallingDevice, &Established->CallingDevice);
	GetCalledDeviceID(pCalledDevice, &Established->CalledDevice);
	GetConnectionID(pOriginatingNIDConnection, &Established->OriginatingNIDConnection);
	GetNetworkCallingDeviceID(pNetworkCallingDeviceID, &Established->NetworkCallingDeviceID);
	GetNetworkCalledDeviceID(pNetworkCalledDeviceID, &Established->NetworkCalledDeviceID);
	GetAssociatedCallingDeviceID(pAssociatedCallingDeviceID, &Established->AssociatedCallingDeviceID);
	GetAssociatedCalledDeviceID(pAssociatedCalledDeviceID, &Established->AssociatedCalledDeviceID);
	Established->Cause = Cause;
	Established->LocalConnectionInfo = LocalConnectionInfo;
}

void CCSTA::DeliveredEvent(CAsnNode * node, TDelivered * Delivered)
{
	TLocalConnectionState LocalConnectionInfo;
	TEventCause Cause;
	CAsnNode
	*pConnection = NULL,
	*pAlertingDevice = NULL,
	*pCallingDevice = NULL,
	*pCalledDevice = NULL,
	*pOriginatingNIDConnection = NULL,
	*pNetworkCallingDeviceID = NULL,
	*pNetworkCalledDeviceID = NULL,
	*pAssociatedCallingDeviceID = NULL,
	*pAssociatedCalledDeviceID = NULL,
	*tempNode = NULL;
	Cause = (TEventCause)0;
	LocalConnectionInfo = csNull;

	if(node!=NULL)
	{
		pConnection = node->GetSpecificSequence(ncApplication, 11);
		pAlertingDevice = node->GetSpecificSequence(ncApplication, 3);
		pCallingDevice = node->GetSpecificSequence(ncApplication, 1);
		pCalledDevice = node->GetSpecificSequence(ncApplication, 2);
		pOriginatingNIDConnection = node->GetSpecificSequence(ncApplication, 11);
		if((tempNode = node->GetSpecificSequence(ncApplication, 14))!=NULL)
		{
			LocalConnectionInfo = (TLocalConnectionState)tempNode->GetIntegerData();
		}
		if((tempNode = node->GetSpecificSequence(ncUniversal, 10))!=NULL)
		{
			Cause = (TEventCause)tempNode->GetIntegerData();
		}
		pNetworkCallingDeviceID = node->GetSpecificSequence(ncApplication,7);
		pNetworkCalledDeviceID = node->GetSpecificSequence(ncApplication,8);
		pAssociatedCallingDeviceID = node->GetSpecificSequence(ncApplication, 5);
		pAssociatedCalledDeviceID = node->GetSpecificSequence(ncApplication, 6);
	}

	Delivered->Cause = Cause;
	Delivered->LocalConnectionInfo = LocalConnectionInfo;
	GetConnectionID(pConnection, &Delivered->Connection);
	GetSubjectDeviceID(pAlertingDevice, &Delivered->AlertingDevice);
	GetCallingDeviceID(pCallingDevice, &Delivered->CallingDevice);
	GetCalledDeviceID(pCalledDevice, &Delivered->CalledDevice);
	GetConnectionID(pOriginatingNIDConnection, &Delivered->OriginatingNIDConnection);
	GetNetworkCallingDeviceID(pNetworkCallingDeviceID, &Delivered->NetworkCallingDeviceID);
	GetNetworkCalledDeviceID(pNetworkCalledDeviceID, &Delivered->NetworkCalledDeviceID);
	GetAssociatedCallingDeviceID(pAssociatedCallingDeviceID, &Delivered->AssociatedCallingDeviceID);
	GetAssociatedCalledDeviceID(pAssociatedCalledDeviceID, &Delivered->AssociatedCalledDeviceID);


}

void CCSTA::FailedEvent(CAsnNode * node, TFailed * Failed)
{
	TLocalConnectionState LocalConnectionInfo;
	TEventCause Cause;
	CAsnNode
	*pFailedConnection = NULL,
	*pFailingDevice = NULL,
	*pCallingDevice = NULL,
	*pCalledDevice = NULL,
	*pOriginatingNIDConnection = NULL,
	*pNetworkCallingDeviceID = NULL,
	*pNetworkCalledDeviceID = NULL,
	*pAssociatedCallingDeviceID = NULL,
	*pAssociatedCalledDeviceID = NULL,
	*tempNode = NULL;
	Cause = (TEventCause)0;
	LocalConnectionInfo = csNull;
	if(node!=NULL)
	{
		pFailedConnection = node->GetSpecificSequence(ncApplication, 11);
		pFailingDevice = node->GetSpecificSequence(ncApplication, 3);
		pCallingDevice = node->GetSpecificSequence(ncApplication, 1);
		pCalledDevice = node->GetSpecificSequence(ncApplication, 2);
		pOriginatingNIDConnection = node->GetSpecificSequence(ncApplication, 11);
		if((tempNode = node->GetSpecificSequence(ncApplication, 14))!=NULL)
		{
			LocalConnectionInfo = (TLocalConnectionState)tempNode->GetIntegerData();
		}
		if((tempNode = node->GetSpecificSequence(ncUniversal, 10))!=NULL)
		{
			Cause = (TEventCause)tempNode->GetIntegerData();
		}
		pNetworkCallingDeviceID = node->GetSpecificSequence(ncApplication,7);
		pNetworkCalledDeviceID = node->GetSpecificSequence(ncApplication,8);
		pAssociatedCallingDeviceID = node->GetSpecificSequence(ncApplication, 5);
		pAssociatedCalledDeviceID = node->GetSpecificSequence(ncApplication, 6);
	}
	Failed->Cause = Cause;
	Failed->LocalConnectionInfo = LocalConnectionInfo;
	GetConnectionID(pFailedConnection, &Failed->FailedConnection);
	GetSubjectDeviceID(pFailingDevice, &Failed->FailingDevice);
	GetCallingDeviceID(pCallingDevice, &Failed->CallingDevice);
	GetCalledDeviceID(pCalledDevice, &Failed->CalledDevice);
	GetConnectionID(pOriginatingNIDConnection, &Failed->OriginatingNIDConnection);
	GetNetworkCallingDeviceID(pNetworkCallingDeviceID, &Failed->NetworkCallingDeviceID);
	GetNetworkCalledDeviceID(pNetworkCalledDeviceID, &Failed->NetworkCalledDeviceID);
	GetAssociatedCallingDeviceID(pAssociatedCallingDeviceID, &Failed->AssociatedCallingDeviceID);
	GetAssociatedCalledDeviceID(pAssociatedCalledDeviceID, &Failed->AssociatedCalledDeviceID);
}

void CCSTA::HeldEvent(CAsnNode * node, THeld * Held)
{
	TLocalConnectionState LocalConnectionInfo;
	TEventCause Cause;
	CAsnNode
	*pHeldConnection = NULL,
	*pHoldingDevice = NULL,
	*tempNode = NULL;

	Cause = (TEventCause)0;
	LocalConnectionInfo = csNull;

	if(node!=NULL)
	{
		pHeldConnection = node->GetSpecificSequence(ncApplication, 11);
		pHoldingDevice = node->GetSpecificSequence(ncApplication, 3);
		if((tempNode = node->GetSpecificSequence(ncApplication, 14))!=NULL)
		{
			LocalConnectionInfo = (TLocalConnectionState)tempNode->GetIntegerData();
		}
		if((tempNode = node->GetSpecificSequence(ncUniversal, 10))!=NULL)
		{
			Cause = (TEventCause)tempNode->GetIntegerData();
		}
	}
	Held->Cause = Cause;
	Held->LocalConnectionInfo = LocalConnectionInfo;
	GetConnectionID(pHeldConnection, &Held->HeldConnection);
	GetSubjectDeviceID(pHoldingDevice, &Held->HoldingDevice);
}

void CCSTA::DivertedEvent(CAsnNode * node, TDiverted * Diverted)
{
	TLocalConnectionState LocalConnectionInfo;
	TEventCause Cause;
	CAsnNode
	*pConnection = NULL,
	*pDivertingDevice = NULL,
	*pNewDestination = NULL,
	*pCallingDevice = NULL,
	*pCalledDevice = NULL,
	*pNetworkCallingDeviceID = NULL,
	*pNetworkCalledDeviceID = NULL,
	*pAssociatedCallingDeviceID = NULL,
	*pAssociatedCalledDeviceID = NULL,
	*tempNode = NULL;

	Cause = (TEventCause)0;
	LocalConnectionInfo = csNull;

	if(node!=NULL)
	{
		pConnection = node->GetSpecificSequence(ncApplication, 11);
		pDivertingDevice = node->GetSpecificSequence(ncApplication, 3);
		pNewDestination = node->GetSpecificSequence(ncApplication, 3);
		pCallingDevice = node->GetSpecificSequence(ncApplication, 1);
		pCalledDevice = node->GetSpecificSequence(ncApplication, 2);
		if((tempNode = node->GetSpecificSequence(ncApplication, 14))!=NULL)
		{
			LocalConnectionInfo = (TLocalConnectionState)tempNode->GetIntegerData();
		}
		if((tempNode = node->GetSpecificSequence(ncUniversal, 10))!=NULL)
		{
			Cause = (TEventCause)tempNode->GetIntegerData();
		}
		pNetworkCallingDeviceID = node->GetSpecificSequence(ncApplication,7);
		pNetworkCalledDeviceID = node->GetSpecificSequence(ncApplication,8);
		pAssociatedCallingDeviceID = node->GetSpecificSequence(ncApplication, 5);
		pAssociatedCalledDeviceID = node->GetSpecificSequence(ncApplication, 6);
	}

	Diverted->Cause = Cause;
	Diverted->LocalConnectionInfo = LocalConnectionInfo;
	GetConnectionID(pConnection, &Diverted->Connection);
	GetSubjectDeviceID(pDivertingDevice, &Diverted->DivertingDevice);
	GetSubjectDeviceID(pNewDestination, &Diverted->NewDestination);
	GetCallingDeviceID(pCallingDevice, &Diverted->CallingDevice);
	GetCalledDeviceID(pCalledDevice, &Diverted->CalledDevice);
	GetNetworkCallingDeviceID(pNetworkCallingDeviceID, &Diverted->NetworkCallingDeviceID);
	GetNetworkCalledDeviceID(pNetworkCalledDeviceID, &Diverted->NetworkCalledDeviceID);
	GetAssociatedCallingDeviceID(pAssociatedCallingDeviceID, &Diverted->AssociatedCallingDeviceID);
	GetAssociatedCalledDeviceID(pAssociatedCalledDeviceID, &Diverted->AssociatedCalledDeviceID);
}

void CCSTA::ServiceInitiatedEvent(CAsnNode * node, TServiceInitiated * ServiceInitiated)
{
	TLocalConnectionState LocalConnectionInfo;
	TEventCause Cause;
	CAsnNode
	*pInitiatedConnection = NULL,
	*pInitiatingDevice = NULL,
	*pNetworkCallingDeviceID = NULL,
	*pNetworkCalledDeviceID = NULL,
	*pAssociatedCallingDeviceID = NULL,
	*pAssociatedCalledDeviceID = NULL,
	*tempNode = NULL;

	Cause = (TEventCause)0;
	LocalConnectionInfo = csNull;

	if(node != NULL)
	{
		pInitiatedConnection = node->GetSpecificSequence(ncApplication, 11);
		pInitiatingDevice = node->GetSpecificSequence(ncApplication, 3);
		if((tempNode = node->GetSpecificSequence(ncApplication, 14))!=NULL)
		{
			LocalConnectionInfo = (TLocalConnectionState)tempNode->GetIntegerData();
		}
		if((tempNode = node->GetSpecificSequence(ncUniversal, 10))!=NULL)
		{
			Cause = (TEventCause)tempNode->GetIntegerData();
		}
		pNetworkCallingDeviceID = node->GetSpecificSequence(ncApplication,7);
		pNetworkCalledDeviceID = node->GetSpecificSequence(ncApplication,8);
		pAssociatedCallingDeviceID = node->GetSpecificSequence(ncApplication, 5);
		pAssociatedCalledDeviceID = node->GetSpecificSequence(ncApplication, 6);
	}


	ServiceInitiated->Cause = Cause;
	ServiceInitiated->LocalConnectionInfo = LocalConnectionInfo;
	GetConnectionID(pInitiatedConnection, &ServiceInitiated->InitiatedConnection);
	GetSubjectDeviceID(pInitiatingDevice, &ServiceInitiated->InitiatingDevice);
	GetNetworkCallingDeviceID(pNetworkCallingDeviceID, &ServiceInitiated->NetworkCallingDeviceID);
	GetNetworkCalledDeviceID(pNetworkCalledDeviceID, &ServiceInitiated->NetworkCalledDeviceID);
	GetAssociatedCallingDeviceID(pAssociatedCallingDeviceID, &ServiceInitiated->AssociatedCallingDeviceID);
	GetAssociatedCalledDeviceID(pAssociatedCalledDeviceID, &ServiceInitiated->AssociatedCalledDeviceID);

}

void CCSTA::OriginatedEvent(CAsnNode * node, TOriginated * Originated)
{
	TLocalConnectionState LocalConnectionInfo;
	TEventCause Cause;
	CAsnNode
	*pOriginatedConnection = NULL,
	*pCallingDevice = NULL,
	*pCalledDevice = NULL,
	*pNetworkCallingDeviceID = NULL,
	*pNetworkCalledDeviceID = NULL,
	*pAssociatedCallingDeviceID = NULL,
	*pAssociatedCalledDeviceID = NULL,
	*tempNode = NULL;

	Cause = (TEventCause)0;
	LocalConnectionInfo = csNull;

	if(node!=NULL)
	{
		pOriginatedConnection = node->GetSpecificSequence(ncApplication, 11);
		pCallingDevice = node->GetSpecificSequence(ncApplication, 3);
		pCalledDevice = node->GetSpecificSequence(ncApplication, 2);
		if((tempNode = node->GetSpecificSequence(ncApplication, 14))!=NULL)
		{
			LocalConnectionInfo = (TLocalConnectionState)tempNode->GetIntegerData();
		}
		if((tempNode = node->GetSpecificSequence(ncUniversal, 10))!=NULL)
		{
			Cause = (TEventCause)tempNode->GetIntegerData();
		}
		pNetworkCallingDeviceID = node->GetSpecificSequence(ncApplication,7);
		pNetworkCalledDeviceID = node->GetSpecificSequence(ncApplication,8);
		pAssociatedCallingDeviceID = node->GetSpecificSequence(ncApplication, 5);
		pAssociatedCalledDeviceID = node->GetSpecificSequence(ncApplication, 6);
	}

	Originated->Cause = Cause;
	Originated->LocalConnectionInfo = LocalConnectionInfo;
	GetConnectionID(pOriginatedConnection, &Originated->OriginatedConnection);
	GetSubjectDeviceID(pCallingDevice, &Originated->CallingDevice);
	GetCalledDeviceID(pCalledDevice, &Originated->CalledDevice);
	GetNetworkCallingDeviceID(pNetworkCallingDeviceID, &Originated->NetworkCallingDeviceID);
	GetNetworkCalledDeviceID(pNetworkCalledDeviceID, &Originated->NetworkCalledDeviceID);
	GetAssociatedCallingDeviceID(pAssociatedCallingDeviceID, &Originated->AssociatedCallingDeviceID);
	GetAssociatedCalledDeviceID(pAssociatedCalledDeviceID, &Originated->AssociatedCalledDeviceID);
}

void CCSTA::QueuedEvent(CAsnNode * node, TQueued * Queued)
{
	int NumberQueued;
	int CallsInFront;
	TLocalConnectionState LocalConnectionInfo;
	TEventCause Cause;

	CAsnNode
	*pQueuedConnection = NULL,
	*pQueue = NULL,
	*pCallingDevice = NULL,
	*pCalledDevice = NULL,
	*pNetworkCallingDeviceID = NULL,
	*pNetworkCalledDeviceID = NULL,
	*pAssociatedCallingDeviceID = NULL,
	*pAssociatedCalledDeviceID = NULL,
	*tempNode = NULL;

	NumberQueued = 0;
	CallsInFront = 0;

	Cause = (TEventCause)0;
	LocalConnectionInfo = csNull;

	if(node!=NULL)
	{
		pQueuedConnection = node->GetSpecificSequence(ncApplication, 11);
		pQueue = node->GetSpecificSequence(ncApplication, 3);
		pCallingDevice = node->GetSpecificSequence(ncApplication, 1);
		pCalledDevice = node->GetSpecificSequence(ncApplication, 2);
		if((tempNode = node->GetSpecificSequence(ncContext_Specific, 0))!=NULL)
		{
			NumberQueued = tempNode->GetIntegerData();
		}
		if((tempNode = node->GetSpecificSequence(ncContext_Specific, 1))!=NULL)
		{
			CallsInFront = tempNode->GetIntegerData();
		}
		if((tempNode = node->GetSpecificSequence(ncApplication, 14))!=NULL)
		{
			LocalConnectionInfo = (TLocalConnectionState)tempNode->GetIntegerData();
		}
		if((tempNode = node->GetSpecificSequence(ncUniversal, 10))!=NULL)
		{
			Cause = (TEventCause)tempNode->GetIntegerData();
		}
		pNetworkCallingDeviceID = node->GetSpecificSequence(ncApplication,7);
		pNetworkCalledDeviceID = node->GetSpecificSequence(ncApplication,8);
		pAssociatedCallingDeviceID = node->GetSpecificSequence(ncApplication, 5);
		pAssociatedCalledDeviceID = node->GetSpecificSequence(ncApplication, 6);
	}

	Queued->Cause = Cause;
	Queued->LocalConnectionInfo = LocalConnectionInfo;
	Queued->NumberQueued = NumberQueued;
	Queued->CallsInFront = CallsInFront;
	GetConnectionID(pQueuedConnection, &Queued->QueuedConnection);
	GetSubjectDeviceID(pQueue, &Queued->Queue);
	GetCallingDeviceID(pCallingDevice, &Queued->CallingDevice);
	GetCalledDeviceID(pCalledDevice, &Queued->CalledDevice);
	GetNetworkCallingDeviceID(pNetworkCallingDeviceID, &Queued->NetworkCallingDeviceID);
	GetNetworkCalledDeviceID(pNetworkCalledDeviceID, &Queued->NetworkCalledDeviceID);
	GetAssociatedCallingDeviceID(pAssociatedCallingDeviceID, &Queued->AssociatedCallingDeviceID);
	GetAssociatedCalledDeviceID(pAssociatedCalledDeviceID, &Queued->AssociatedCalledDeviceID);
}

void CCSTA::RetrievedEvent(CAsnNode * node, TRetrieved * Retrieved)
{
	TLocalConnectionState LocalConnectionInfo;
	TEventCause Cause;
	CAsnNode
	*pRetrievedConnection = NULL,
	*pRetrievingDevice = NULL,
	*tempNode = NULL;

	Cause = (TEventCause)0;
	LocalConnectionInfo = csNull;

	if(node!=NULL)
	{
		pRetrievedConnection = node->GetSpecificSequence(ncApplication, 11);
		pRetrievingDevice = node->GetSpecificSequence(ncApplication, 3);
		if((tempNode = node->GetSpecificSequence(ncApplication, 14))!=NULL)
		{
			LocalConnectionInfo = (TLocalConnectionState)tempNode->GetIntegerData();
		}
		if((tempNode = node->GetSpecificSequence(ncUniversal, 10))!=NULL)
		{
			Cause = (TEventCause)tempNode->GetIntegerData();
		}
	}
	Retrieved->Cause = Cause;
	Retrieved->LocalConnectionInfo = LocalConnectionInfo;
	GetConnectionID(pRetrievedConnection, &Retrieved->RetrievedConnection);
	GetSubjectDeviceID(pRetrievingDevice, &Retrieved->RetrievingDevice);
}

void CCSTA::NetworkReachedEvent(CAsnNode * node, TNetworkReached * NetworkReached)
{
	TLocalConnectionState LocalConnectionInfo;
	TEventCause Cause;
	CAsnNode
	*pOutboundConnection = NULL,
	*pNetworkInterfaceUsed = NULL,
	*pCallingDevice = NULL,
	*pCalledDevice = NULL,
	*pOriginatingNIDConnection = NULL,
	*pNetworkCallingDeviceID = NULL,
	*pNetworkCalledDeviceID = NULL,
	*pAssociatedCallingDeviceID = NULL,
	*tempNode = NULL;

	Cause = (TEventCause)0;
	LocalConnectionInfo = csNull;

	if(node!=NULL)
	{
		pOutboundConnection = node->GetSpecificSequence(ncApplication, 11);
		pNetworkInterfaceUsed = node->GetSpecificSequence(ncApplication, 3);
		pCallingDevice = node->GetSpecificSequence(ncApplication, 1);
		pCalledDevice = node->GetSpecificSequence(ncApplication, 2);
		pOriginatingNIDConnection = node->GetSpecificSequence(ncApplication, 11);
		if((tempNode = node->GetSpecificSequence(ncApplication, 14))!=NULL)
		{
			LocalConnectionInfo = (TLocalConnectionState)tempNode->GetIntegerData();
		}
		if((tempNode = node->GetSpecificSequence(ncUniversal, 10))!=NULL)
		{
			Cause = (TEventCause)tempNode->GetIntegerData();
		}
		pNetworkCallingDeviceID = node->GetSpecificSequence(ncApplication,7);
		pNetworkCalledDeviceID = node->GetSpecificSequence(ncApplication,8);
		pAssociatedCallingDeviceID = node->GetSpecificSequence(ncApplication, 5);
	}

	NetworkReached->Cause = Cause;
	NetworkReached->LocalConnectionInfo = LocalConnectionInfo;
	GetConnectionID(pOutboundConnection, &NetworkReached->OutboundConnection);
	GetSubjectDeviceID(pNetworkInterfaceUsed, &NetworkReached->NetworkInterfaceUsed);
	GetCallingDeviceID(pCallingDevice, &NetworkReached->CallingDevice);
	GetCalledDeviceID(pCalledDevice, &NetworkReached->CalledDevice);
	GetConnectionID(pOriginatingNIDConnection, &NetworkReached->OriginatingNIDConnection);
	GetNetworkCallingDeviceID(pNetworkCallingDeviceID, &NetworkReached->NetworkCallingDeviceID);
	GetNetworkCalledDeviceID(pNetworkCalledDeviceID, &NetworkReached->NetworkCalledDeviceID);
	GetAssociatedCallingDeviceID(pAssociatedCallingDeviceID, &NetworkReached->AssociatedCallingDeviceID);
}

void CCSTA::ConferencedEvent(CAsnNode * node, TConferenced * Conferenced)
{
	TLocalConnectionState LocalConnectionInfo;
	TEventCause Cause;
	CAsnNode
	*pPrimaryOldCall = NULL,
	*pSecondaryOldCall = NULL,
	*pConferencingDevice = NULL,
	*pAddedParty = NULL,
	*pConferenceConnections = NULL,
	*tempNode1 = NULL;


	LocalConnectionInfo = csNull;
	Cause = (TEventCause)0;

	if(node!=NULL)
	{
		pPrimaryOldCall = node->GetSpecificSequence(ncApplication, 11);
		pSecondaryOldCall = node->GetSpecificSequence(ncApplication, 11);
		pConferencingDevice = node->GetSpecificSequence(ncApplication, 3);
		pAddedParty = node->GetSpecificSequence(ncApplication, 3);
		pConferenceConnections = node->GetSpecificSequence(ncUniversal, 16);

		if((tempNode1 = node->GetSpecificSequence(ncApplication, 14))!=NULL)
		{
			LocalConnectionInfo = (TLocalConnectionState)tempNode1->GetIntegerData();
		}
		if((tempNode1 = node->GetSpecificSequence(ncUniversal, 10))!=NULL)
		{
			Cause = (TEventCause)tempNode1->GetIntegerData();
		}
	}

	Conferenced->Cause = Cause;
	Conferenced->LocalConnectionInfo = LocalConnectionInfo;
	GetConnectionID(pPrimaryOldCall, &Conferenced->PrimaryOldCall);
	GetConnectionID(pSecondaryOldCall, &Conferenced->SecondaryOldCall);
	GetSubjectDeviceID(pConferencingDevice, &Conferenced->ConferencingDevice);
	GetSubjectDeviceID(pAddedParty, &Conferenced->AddedParty);
	GetConnectionList(pConferenceConnections, &Conferenced->ConferenceConnections);
}

void CCSTA::TransferredEvent(CAsnNode * node, TTransferred * Transferred)
{

	TLocalConnectionState LocalConnectionInfo;
	TEventCause Cause;
	CAsnNode
	*pPrimaryOldCall = NULL,
	*pSecondaryOldCall = NULL,
	*pTransferringDevice = NULL,
	*pTransferredToDevice = NULL,
	*pTransferredConnections = NULL,
	*tempNode1 = NULL;


	LocalConnectionInfo = csNull;
	Cause = (TEventCause)0;

	if(node!=NULL)
	{
		pPrimaryOldCall = node->GetSpecificSequence(ncApplication, 11);
		pSecondaryOldCall = node->GetSpecificSequence(ncContext_Specific, 0);
		pTransferringDevice = node->GetSpecificSequence(ncApplication, 3);
		pTransferredToDevice = node->GetSpecificSequence(ncApplication, 3);
		if((tempNode1 = node->GetSpecificSequence(ncContext_Specific, 1))!=NULL)
		{
			pTransferredConnections = tempNode1;
		}
		if((tempNode1 = node->GetSpecificSequence(ncApplication, 14))!=NULL)
		{
			LocalConnectionInfo = (TLocalConnectionState)tempNode1->GetIntegerData();
		}
		if((tempNode1 = node->GetSpecificSequence(ncUniversal, 10))!=NULL)
		{
			Cause = (TEventCause)tempNode1->GetIntegerData();
		}
	}

	Transferred->Cause = Cause;
	Transferred->LocalConnectionInfo = LocalConnectionInfo;

	GetConnectionID(pPrimaryOldCall, &Transferred->PrimaryOldCall);
	GetConnectionID(pSecondaryOldCall, &Transferred->SecondaryOldCall);
	GetSubjectDeviceID(pTransferringDevice, &Transferred->TransferringDevice);
	GetSubjectDeviceID(pTransferredToDevice, &Transferred->TransferredToDevice);
	GetConnectionList(pTransferredConnections, &Transferred->TransferredConnections);
}



void CCSTA::CallControlEvent(CAsnNode * node, int crossRefIdentifier)
{
	bool eventRecognized;
	CAsnNode * specificEvent;
	TCallControlEventType callControlEventType;
	TCallControlEvent * Event;


	eventRecognized = false;
	Event = new TCallControlEvent;
	specificEvent = node->GetNextSequence();
	callControlEventType = (TCallControlEventType)specificEvent->GetTagNumber();

	logevento("Call Control - Device "+CrossRefToDevice(crossRefIdentifier));

	switch(callControlEventType)
	{
	case evBridged:  //bridged
	Event->CallControlEvText="evBridged";
	break;
	case evCallCleared: //callCleared
	Event->CallControlEvText="evCallClearead";
	break;

	case evConferenced: //conferenced
	Event->CallControlEvText="evConferenced";
	ConferencedEvent(specificEvent, &Event->Conferenced);
	logevento(logObject(Event->Conferenced, 0)+"\n\n");
	eventRecognized = true;
	break;
	case evConnectionCleared: //ConnectionCleared
	Event->CallControlEvText="evConnectionClearead";
	ConnectionClearedEvent(specificEvent, &Event->ConnectionCleared);
	logevento(logObject(Event->ConnectionCleared, 0)+"\n\n");
	eventRecognized = true;
	break;
	case evDelivered: //Delivered

	memcpy(specificEvent->dump,node->dump,node->tamDump);
	specificEvent->tamDump = node->tamDump;



/*
A1 78 02 01 02 02 01 15    30 70 55 04 01 11 00 23 . x...... 0 p U.....
A0 68 A4 66 6B 12 30 10    80 04 30 30 46 34 A1 08 . h. f k. 0... 0 0 F 4..
30 06 80 04 37 30 34 39    63 08 30 06 80 04 37 30  0... 7 0 4 9 c. 0... 7 0
34 39 61 08 30 06 81 04    01 55 00 00 62 08 30 06  4 9 a. 0.... U.. b. 0.
80 04 37 30 34 39 64 02    88 00 4E 01 02 0A 01 16 .. 7 0 4 9 d... N.....
7E 28 A1 26 A6 11 A6 0F    30 0D 86 0B 4E 34 31 33 ........ 0... N 4 1 3
36 36 31 37 30 32 34 A6    11 B1 0F 30 0D 86 0B 4E  6 6 1 7 0 2 4.... 0... N
3C 37 30 34 39 3E 37 30    34 39 00 00 . 7 0 4 9. 7 0 4 9..
124 Byte(s)*/


	if (node->tamDump > 121 && gVersionInfo == "Panasonic")
	{
		char ivrId[100] = "";
		char ivrId1[100] = "";
        memcpy(ivrId,  &specificEvent->dump[90], specificEvent->dump[89]  );


        for(int i =0,j=0;i<specificEvent->dump[89];i++)
        {
        	if (ivrId[i] >= '0' &&  ivrId[i] <= '9' )
        	{
        		ivrId1[j++] = ivrId[i];
        	}

        }

        Event->IvrID = ivrId1;

	}



	Event->CallControlEvText="evDelivered";
	DeliveredEvent(specificEvent, &Event->Delivered);




	logevento(logObject(Event->Delivered, 0)+"\n\n");
	eventRecognized = true;
	break;
	case evDigitsDialed: //DigitsDialed
	Event->CallControlEvText="evDigitsDialed";
	break;
	case evDiverted: //Diverted
	Event->CallControlEvText="evDiverted";
	DivertedEvent(specificEvent, &Event->Diverted);
	logevento(logObject(Event->Diverted, 0)+"\n\n");
	eventRecognized = true;
	break;
	case evEstablished: //Established
	Event->CallControlEvText="evEstablished";
	EstablishedEvent(specificEvent, &Event->Established);
	logevento(logObject(Event->Established, 0)+"\n\n");
	eventRecognized = true;
	break;
	case evFailed: //Failed
	Event->CallControlEvText="evFailed";
	FailedEvent(specificEvent, &Event->Failed);
	logevento(logObject(Event->Failed, 0)+"\n\n");
	eventRecognized = true;
	break;
	case evHeld: //Held
	Event->CallControlEvText="evHeld";
	HeldEvent(specificEvent, &Event->Held);
	logevento(logObject(Event->Held, 0)+"\n\n");
	eventRecognized = true;
	break;
	case evNetworkCapabilitiesChange://NetworkCapsChanged
	Event->CallControlEvText="evNetworkCapabilitiesChange";
	break;
	case evNetworkReached://NetworkReached
	Event->CallControlEvText="evNetworkReachead";
	NetworkReachedEvent(specificEvent, &Event->NetworkReached);
	logevento(logObject(Event->NetworkReached, 0)+"\n\n");
	eventRecognized = true;
	break;
	case evOffered://Offered
	Event->CallControlEvText="evOffered";
	break;
	case evOriginated://Originated
	Event->CallControlEvText="evOriginated";
	OriginatedEvent(specificEvent, &Event->Originated);
	logevento(logObject(Event->Originated, 0)+"\n\n");
	eventRecognized = true;
	break;
	case evQueued://Queued

	Event->CallControlEvText="evQueued";
	QueuedEvent(specificEvent, &Event->Queued);
	logevento(logObject(Event->Queued, 0)+"\n\n");
	eventRecognized = true;
	break;
	case evRetrieved://Retrieved
	Event->CallControlEvText="evRetrieved";
	RetrievedEvent(specificEvent, &Event->Retrieved);
	logevento(logObject(Event->Retrieved, 0)+"\n\n");
	eventRecognized = true;
	break;
	case evServiceInitiated://ServiceInitiated
	Event->CallControlEvText="evServiceInitiated";
	ServiceInitiatedEvent(specificEvent, &Event->ServiceInitiated);
	logevento(logObject(Event->ServiceInitiated, 0)+"\n\n");
	eventRecognized = true;
	break;
	case evTransferred://Transferred
	Event->CallControlEvText="evTransferred";
	TransferredEvent(specificEvent, &Event->Transferred);
	logevento(logObject(Event->Transferred, 0)+"\n\n");
	eventRecognized = true;
	break;
	}
	Event->Device = CrossRefToDevice(crossRefIdentifier);
	Event->crossRefId = crossRefIdentifier;
	Event->monitorType = GetMonitorType(crossRefIdentifier);
	if(eventRecognized)
	{
		SendCallControlEvent(callControlEventType, Event);
	}

	delete Event;
}

void CCSTA::ButtonPressEvent(CAsnNode * node, TButtonPress * ButtonPress)
{
	int Button;
	CAsnNode
	*pDevice = NULL,
	*pButton = NULL;

	Button = 0;
	if(node!=NULL)
	{
		pDevice = node->GetSpecificSequence(ncApplication, 3);
		pButton = node->GetSpecificSequence(ncUniversal, 4);
		if(pButton != NULL)
		{
			Button = pButton->GetIntegerData();
		}
	}

	GetSubjectDeviceID(pDevice, &ButtonPress->Device);
	ButtonPress->Button = Button;
}

void CCSTA::PhysicalDeviceEvent(CAsnNode * node, int crossRefIdentifier)
{
	CAsnNode * specificEvent;
	TPhysicalDeviceEventType physicalDeviceEventType;
	TPhysicalDeviceEvent * Event;
	bool eventRecognized;

	eventRecognized = false;
	Event = new TPhysicalDeviceEvent;

	specificEvent = node->GetNextSequence();
	physicalDeviceEventType = (TPhysicalDeviceEventType)specificEvent->GetTagNumber();


	switch(physicalDeviceEventType)
	{
	case evButtonInformation:
		break;
	case evButtonPress:
		ButtonPressEvent(specificEvent, &Event->ButtonPress);
		eventRecognized = true;
		break;
	case evDisplayUpdated:
		break;
	case evHookswitch:
		eventRecognized = true;
		{
			CAsnNode*pHookOnOff = NULL,*pDevice=NULL;
			if((pDevice=specificEvent->GetNextSequence())!=NULL)
			{
				Event->Device=pDevice->GetStringData();
			}
			if(specificEvent->GetNextSequence())
			{
				pHookOnOff=specificEvent->GetNextSequence();
				if(pHookOnOff)
				{
					Event->hookType=(THookEventType)pHookOnOff->GetIntegerData();
				}
			}
		}
		break;
	case evLampMode:
		break;
	case evMessageWaiting:
		break;
	case evMicrophoneGain:
		break;
	case evMicrophoneMute:
		break;
	case evRingerStatus:
		eventRecognized = true;
		break;
	case evSpeakerMute:
		break;
	case evSpeakerVolume:
		break;
	}
	Event->Device = CrossRefToDevice(crossRefIdentifier);
	Event->crossRefId = crossRefIdentifier;

	if(eventRecognized)
	{
		SendPhysicalDeviceEvent(physicalDeviceEventType, Event);
	}
	delete Event;
}

void CCSTA::AgentLoggedOnEvent(CAsnNode * node, TAgentLoggedOn * AgentLoggedOn )
{
	CAsnNode * tempNode = NULL;
	CAsnNode * tempNode2 = NULL;

	if(node!=NULL)
	{
		if((tempNode = node->GetSpecificSequence(ncApplication, 3))!=NULL)
		{
			if((tempNode = tempNode->GetSpecificSequence(ncUniversal, 16))!=NULL)
			{
				if((tempNode2 = tempNode->GetSpecificSequence(ncContext_Specific, 0))!=NULL)
				{
					AgentLoggedOn->AgentDevice.DeviceID.DialingNumber = tempNode2->GetStringData();
					AgentLoggedOn->AgentDevice.DeviceID.DeviceNumber = 0;
				}else if((tempNode2 = tempNode->GetSpecificSequence(ncContext_Specific, 4))!=NULL)
				{
					AgentLoggedOn->AgentDevice.DeviceID.DialingNumber = tempNode2->GetStringData();
					AgentLoggedOn->AgentDevice.DeviceID.DeviceNumber = 0;
				}
			}
		}

		if((tempNode = node->GetSpecificSequence(ncContext_Specific, 2))!=NULL)
		{
			AgentLoggedOn->AgentID = tempNode->GetStringData();
		}

		if((tempNode = node->GetSpecificSequence(ncUniversal, 16))!=NULL)
		{
			if((tempNode2 = tempNode->GetSpecificSequence(ncContext_Specific, 0))!=NULL)
			{
				AgentLoggedOn->ACDGroup.DialingNumber = tempNode2->GetStringData();
				AgentLoggedOn->ACDGroup.DeviceNumber = 0;
			}else if((tempNode2 = tempNode->GetSpecificSequence(ncContext_Specific, 1))!=NULL)
			{
				AgentLoggedOn->ACDGroup.DeviceNumber = tempNode2->GetIntegerData();
			}
			else if((tempNode2 = tempNode->GetSpecificSequence(ncContext_Specific, 4))!=NULL)
			{
				AgentLoggedOn->ACDGroup.DialingNumber = tempNode2->GetStringData();
				AgentLoggedOn->ACDGroup.DeviceNumber = 0;
			}
		}
	}

}

void CCSTA::AgentLoggedOffEvent(CAsnNode * node, TAgentLoggedOff * AgentLoggedOff)
{
	CAsnNode * tempNode = NULL;
	CAsnNode * tempNode2 = NULL;

	if(node!=NULL)
	{
		if((tempNode = node->GetSpecificSequence(ncApplication, 3))!=NULL)
		{
			if((tempNode = tempNode->GetSpecificSequence(ncUniversal, 16))!=NULL)
			{
				if((tempNode2 = tempNode->GetSpecificSequence(ncContext_Specific, 0))!=NULL)
				{
					AgentLoggedOff->AgentDevice.DeviceID.DialingNumber = tempNode2->GetStringData();
					AgentLoggedOff->AgentDevice.DeviceID.DeviceNumber = 0;
				}else if((tempNode2 = tempNode->GetSpecificSequence(ncContext_Specific, 4))!=NULL)
				{
					AgentLoggedOff->AgentDevice.DeviceID.DialingNumber = tempNode2->GetStringData();
					AgentLoggedOff->AgentDevice.DeviceID.DeviceNumber = 0;
				}
			}
		}

		if((tempNode = node->GetSpecificSequence(ncContext_Specific, 2))!=NULL)
		{
			AgentLoggedOff->AgentID = tempNode->GetStringData();
		}

		if((tempNode = node->GetSpecificSequence(ncUniversal, 16))!=NULL)
		{
			if((tempNode2 = tempNode->GetSpecificSequence(ncContext_Specific, 0))!=NULL)
			{
				AgentLoggedOff->ACDGroup.DialingNumber = tempNode2->GetStringData();
				AgentLoggedOff->ACDGroup.DeviceNumber = 0;
			}else if((tempNode2 = tempNode->GetSpecificSequence(ncContext_Specific, 1))!=NULL)
			{
				AgentLoggedOff->ACDGroup.DeviceNumber = tempNode2->GetIntegerData();
			}
			else if((tempNode2 = tempNode->GetSpecificSequence(ncContext_Specific, 4))!=NULL)
			{
				AgentLoggedOff->ACDGroup.DialingNumber = tempNode2->GetStringData();
				AgentLoggedOff->ACDGroup.DeviceNumber = 0;
			}
		}
	}
}

void CCSTA::AgentBusyEvent(CAsnNode * node, TAgentBusy * AgentBusy)
{
	CAsnNode * tempNode = NULL;
	CAsnNode * tempNode2 = NULL;

	if(node!=NULL)
	{
		if((tempNode = node->GetSpecificSequence(ncApplication, 3))!=NULL)
		{
			if((tempNode = tempNode->GetSpecificSequence(ncUniversal, 16))!=NULL)
			{
				if((tempNode2 = tempNode->GetSpecificSequence(ncContext_Specific, 0))!=NULL)
				{
					AgentBusy->AgentDevice.DeviceID.DialingNumber = tempNode2->GetStringData();
					AgentBusy->AgentDevice.DeviceID.DeviceNumber = 0;
				}else if((tempNode2 = tempNode->GetSpecificSequence(ncContext_Specific, 4))!=NULL)
				{
					AgentBusy->AgentDevice.DeviceID.DialingNumber = tempNode2->GetStringData();
					AgentBusy->AgentDevice.DeviceID.DeviceNumber = 0;
				}
			}
		}

		if((tempNode = node->GetSpecificSequence(ncUniversal, 4))!=NULL)
		{
			AgentBusy->AgentID = tempNode->GetStringData();
		}

		if((tempNode = node->GetSpecificSequence(ncUniversal, 16))!=NULL)
		{
			if((tempNode2 = tempNode->GetSpecificSequence(ncContext_Specific, 0))!=NULL)
			{
				AgentBusy->ACDGroup.DialingNumber = tempNode2->GetStringData();
				AgentBusy->ACDGroup.DeviceNumber = 0;
			}else if((tempNode2 = tempNode->GetSpecificSequence(ncContext_Specific, 1))!=NULL)
			{
				AgentBusy->ACDGroup.DeviceNumber = tempNode2->GetIntegerData();
			}
			else if((tempNode2 = tempNode->GetSpecificSequence(ncContext_Specific, 4))!=NULL)
			{
				AgentBusy->ACDGroup.DialingNumber = tempNode2->GetStringData();
				AgentBusy->ACDGroup.DeviceNumber = 0;
			}
		}
	}
}

void CCSTA::AgentNotReadyEvent(CAsnNode * node, TAgentNotReady * AgentNotReady)
{
	CAsnNode * tempNode = NULL;
	CAsnNode * tempNode2 = NULL;

	if(node!=NULL)
	{
		if((tempNode = node->GetSpecificSequence(ncApplication, 3))!=NULL)
		{
			if((tempNode = tempNode->GetSpecificSequence(ncUniversal, 16))!=NULL)
			{
				if((tempNode2 = tempNode->GetSpecificSequence(ncContext_Specific, 0))!=NULL)
				{
					AgentNotReady->AgentDevice.DeviceID.DialingNumber = tempNode2->GetStringData();
					AgentNotReady->AgentDevice.DeviceID.DeviceNumber = 0;
				}else if((tempNode2 = tempNode->GetSpecificSequence(ncContext_Specific, 4))!=NULL)
				{
					AgentNotReady->AgentDevice.DeviceID.DialingNumber = tempNode2->GetStringData();
					AgentNotReady->AgentDevice.DeviceID.DeviceNumber = 0;
				}
			}
		}

		if((tempNode = node->GetSpecificSequence(ncUniversal, 4))!=NULL)
		{
			AgentNotReady->AgentID = tempNode->GetStringData();
		}

		if((tempNode = node->GetSpecificSequence(ncUniversal, 16))!=NULL)
		{
			if((tempNode2 = tempNode->GetSpecificSequence(ncContext_Specific, 0))!=NULL)
			{
				AgentNotReady->ACDGroup.DialingNumber = tempNode2->GetStringData();
				AgentNotReady->ACDGroup.DeviceNumber = 0;
			}else if((tempNode2 = tempNode->GetSpecificSequence(ncContext_Specific, 1))!=NULL)
			{
				AgentNotReady->ACDGroup.DeviceNumber = tempNode2->GetIntegerData();
			}
			else if((tempNode2 = tempNode->GetSpecificSequence(ncContext_Specific, 4))!=NULL)
			{
				AgentNotReady->ACDGroup.DialingNumber = tempNode2->GetStringData();
				AgentNotReady->ACDGroup.DeviceNumber = 0;
			}
		}
	}
}

void CCSTA::AgentReadyEvent(CAsnNode * node, TAgentReady * AgentReady)
{
	CAsnNode * tempNode = NULL;
	CAsnNode * tempNode2 = NULL;

	if(node!=NULL)
	{
		if((tempNode = node->GetSpecificSequence(ncApplication, 3))!=NULL)
		{
			if((tempNode = tempNode->GetSpecificSequence(ncUniversal, 16))!=NULL)
			{
				if((tempNode2 = tempNode->GetSpecificSequence(ncContext_Specific, 0))!=NULL)
				{
					AgentReady->AgentDevice.DeviceID.DialingNumber = tempNode2->GetStringData();
					AgentReady->AgentDevice.DeviceID.DeviceNumber = 0;
				}else if((tempNode2 = tempNode->GetSpecificSequence(ncContext_Specific, 4))!=NULL)
				{
					AgentReady->AgentDevice.DeviceID.DialingNumber = tempNode2->GetStringData();
					AgentReady->AgentDevice.DeviceID.DeviceNumber = 0;
				}
			}
		}

		if((tempNode = node->GetSpecificSequence(ncUniversal, 4))!=NULL)
		{
			AgentReady->AgentID = tempNode->GetStringData();
		}

		if((tempNode = node->GetSpecificSequence(ncUniversal, 16))!=NULL)
		{
			if((tempNode2 = tempNode->GetSpecificSequence(ncContext_Specific, 4))!=NULL)
			{
				AgentReady->ACDGroup.DialingNumber = tempNode2->GetStringData();
				AgentReady->ACDGroup.DeviceNumber = 0;
			}else if((tempNode2 = tempNode->GetSpecificSequence(ncContext_Specific, 1))!=NULL)
			{
				AgentReady->ACDGroup.DeviceNumber = tempNode2->GetIntegerData();
			}
			else if((tempNode2 = tempNode->GetSpecificSequence(ncContext_Specific, 4))!=NULL)
			{
				AgentReady->ACDGroup.DialingNumber = tempNode2->GetStringData();
				AgentReady->ACDGroup.DeviceNumber = 0;
			}
		}
	}
}

void CCSTA::AgentWorkingAfterCallEvent(CAsnNode * node, TAgentWorkingAfterCall * AgentWorkingAfterCall)
{
	CAsnNode * tempNode = NULL;
	CAsnNode * tempNode2 = NULL;

	if(node!=NULL)
	{
		if((tempNode = node->GetSpecificSequence(ncApplication, 3))!=NULL)
		{
			if((tempNode = tempNode->GetSpecificSequence(ncUniversal, 16))!=NULL)
			{
				if((tempNode2 = tempNode->GetSpecificSequence(ncContext_Specific, 0))!=NULL)
				{
					AgentWorkingAfterCall->AgentDevice.DeviceID.DialingNumber = tempNode2->GetStringData();
					AgentWorkingAfterCall->AgentDevice.DeviceID.DeviceNumber = 0;
				}else if((tempNode2 = tempNode->GetSpecificSequence(ncContext_Specific, 4))!=NULL)
				{
					AgentWorkingAfterCall->AgentDevice.DeviceID.DialingNumber = tempNode2->GetStringData();
					AgentWorkingAfterCall->AgentDevice.DeviceID.DeviceNumber = 0;
				}
			}
		}

		if((tempNode = node->GetSpecificSequence(ncUniversal, 4))!=NULL)
		{
			AgentWorkingAfterCall->AgentID = tempNode->GetStringData();
		}

		if((tempNode = node->GetSpecificSequence(ncUniversal, 16))!=NULL)
		{
			if((tempNode2 = tempNode->GetSpecificSequence(ncContext_Specific, 0))!=NULL)
			{
				AgentWorkingAfterCall->ACDGroup.DialingNumber = tempNode2->GetStringData();
				AgentWorkingAfterCall->ACDGroup.DeviceNumber = 0;
			}else if((tempNode2 = tempNode->GetSpecificSequence(ncContext_Specific, 1))!=NULL)
			{
				AgentWorkingAfterCall->ACDGroup.DeviceNumber = tempNode2->GetIntegerData();
			}
			else if((tempNode2 = tempNode->GetSpecificSequence(ncContext_Specific, 4))!=NULL)
			{
				AgentWorkingAfterCall->ACDGroup.DialingNumber = tempNode2->GetStringData();
				AgentWorkingAfterCall->ACDGroup.DeviceNumber = 0;
			}
		}
	}
}


void CCSTA::LogicalDeviceEvent(CAsnNode * node, int crossRefIdentifier)
{
	bool eventRecognized;
	CAsnNode * specificEvent;
	TLogicalDeviceEventType logicalDeviceEventType;
	TLogicalDeviceEvent * Event;

	Event = new TLogicalDeviceEvent;
	eventRecognized = false;
	specificEvent = node->GetNextSequence();
	logicalDeviceEventType = (TLogicalDeviceEventType)specificEvent->GetTagNumber();
	logevento("Logical Device - Device "+CrossRefToDevice(crossRefIdentifier));
	switch(logicalDeviceEventType)
	{
	case evAgentBusy:
		AgentBusyEvent(specificEvent, &Event->AgentBusy);
		logevento(logObject(Event->AgentBusy, 0)+"\n\n");
		eventRecognized = true;
		break;
	case evAgentLoggedOn:
		AgentLoggedOnEvent(specificEvent, &Event->AgentLoggedOn);
		logevento(logObject(Event->AgentLoggedOn, 0)+"\n\n");
		eventRecognized = true;
		break;
	case evAgentLoggedOff:
		AgentLoggedOffEvent(specificEvent, &Event->AgentLoggedOff);
		logevento(logObject(Event->AgentLoggedOff, 0)+"\n\n");
		eventRecognized = true;
		break;
	case evAgentNotReady:
		AgentNotReadyEvent(specificEvent, &Event->AgentNotReady);
		logevento(logObject(Event->AgentNotReady, 0)+"\n\n");
		eventRecognized = true;
		break;
	case evAgentReady:
		AgentReadyEvent(specificEvent, &Event->AgentReady);
		logevento(logObject(Event->AgentReady, 0)+"\n\n");
		eventRecognized = true;
		break;
	case evAgentWorkingAfterCall:
		AgentWorkingAfterCallEvent(specificEvent, &Event->AgentWorkingAfterCall);
		logevento(logObject(Event->AgentWorkingAfterCall, 0)+"\n\n");
		eventRecognized = true;
		break;
	case evAutoAnswer:
		break;
	case evAutoWorkMode:
		break;
	case evCallback:
		break;
	case evCallbackMessage:
		break;
	case evCallerIDStatus:
		break;
	case evDoNotDisturb:
		break;
	case evForwarding:
		break;
	case evRouteingMode:
		break;
	}

	Event->Device = CrossRefToDevice(crossRefIdentifier);
	Event->crossRefId = crossRefIdentifier;
	Event->monitorType = GetMonitorType(crossRefIdentifier);

	if(eventRecognized)
	{
		SendLogicalDeviceEvent(logicalDeviceEventType, Event);
	}

	delete Event;
}

TMonitorType CCSTA::GetMonitorType(int crossRefId)
{
	TCSTAMonitorItem * ptrItem;
	TMonitorType result;

	result = mtDevice;

	for(unsigned i = 0;i < monitorList.size();i++)
	{
		ptrItem = (TCSTAMonitorItem *)monitorList[i];
		if(ptrItem->crossRefId == crossRefId)
		{
			result = ptrItem->monitorType;
			break;
		}
	}

	return result;
}

string CCSTA::GetDeviceByMonitorId(int crossRefId)
{
	TCSTAMonitorItem * ptrItem;
	string result;

	for(unsigned i = 0;i < monitorList.size();i++)
	{
		ptrItem = (TCSTAMonitorItem *)monitorList[i];
		if(ptrItem->crossRefId == crossRefId)
		{
			result = ptrItem->device;
			break;
		}
	}

	return result;
}

string CCSTA::CrossRefToDevice(int crossRefIdentifier)
{
	return GetDeviceByMonitorId(crossRefIdentifier);
}

void CCSTA::CstaSnapshotDeviceData(CAsnNode * event)
{
	TSnapshotDeviceResult snapshotData;

	GetSnapshotDeviceResult(event, &snapshotData);

	SendSnapshotDeviceDataEvent(&snapshotData);
}

void CCSTA::CstaEvent(CAsnNode * event)
{
	int crossRefIdentifier;
	int eventType;
	CAsnNode * specificEvent;
	CAsnNode * tempNode;





	try
	{
		if(event != NULL)
		{
			if((tempNode = event->GetSpecificSequence(ncApplication, 21)) != NULL)
			{
				crossRefIdentifier = tempNode->GetIntegerData();
				if((specificEvent = event->GetNextSequence())!=NULL)
				{
					eventType = specificEvent->GetTagNumber();
					switch(eventType)
					{
					case 0:
						memcpy(specificEvent->dump,event->dump,event->tamDump);
						specificEvent->tamDump = event->tamDump;
						CallControlEvent(specificEvent, crossRefIdentifier);
						break;
					case 3:
						PhysicalDeviceEvent(specificEvent, crossRefIdentifier);
						break;
					case 4:
						LogicalDeviceEvent(specificEvent, crossRefIdentifier);
						break;
					}
				}
			}
		}
	}
	catch(...)
	{

	}
}

void CCSTA::ProcessError(CAsnNode * invoke)
{
	int invoke_id;
	bool bFind = false;
	CAsnNode * tempInvoke;
	unsigned char * tempMessage;
	tempMessage = invoke->c_str();
	tempInvoke = new CAsnNode(tempMessage);
	delete tempMessage;
	logCsta("!!!Process Error!!!");
	CAsnNode * node_invoke_id;
	if((node_invoke_id = invoke->GetSpecificSequence(ncUniversal, 2))!=NULL)
	{
		invoke_id = node_invoke_id->GetIntegerData();

		for(unsigned i = 0;i < CstaRequests.size();i++)
		{
			TResponseData * tmpResponseData;
			tmpResponseData = (TResponseData *)CstaRequests[i];
			if(tmpResponseData->InvokeId == invoke_id)
			{
				bFind = true;
				tmpResponseData->ptrData = tempInvoke;
				tmpResponseData->h.unlock();
			}
		}
	}

	if(!bFind)
	{
		delete tempInvoke;
	}
}

void CCSTA::ProcessResult(CAsnNode * invoke)
{
	int invoke_id;
	bool bFind = false;
	CAsnNode * tempInvoke;
	unsigned char * tempMessage;
	tempMessage = invoke->c_str();
	tempInvoke = new CAsnNode(tempMessage);
	delete tempMessage;
	CAsnNode * node_invoke_id;
	if((node_invoke_id = invoke->GetSpecificSequence(ncUniversal, 2))!=NULL)
	{
		invoke_id = node_invoke_id->GetIntegerData();
		for(unsigned i = 0;i < CstaRequests.size();i++)
		{
			TResponseData * tmpResponseData;
			tmpResponseData = (TResponseData *)CstaRequests[i];
			if(tmpResponseData->InvokeId == invoke_id)
			{
				bFind = true;
				tmpResponseData->ptrData = tempInvoke;
				SetEvent(tmpResponseData->handle);
				logCsta("!!!Process Result["+tmpResponseData->handle->str+"]!!!");
				i= CstaRequests.size();
			}
		}
	}
	if(!bFind)
	{
		delete tempInvoke;
	}
}

void CCSTA::ProcessInvoke(CAsnNode * invoke)
{
	try
	{
		int operationValue;
		CAsnNode * tempnode;

		tempnode = invoke->GetSpecificSequence(ncUniversal, 2);


		if(tempnode == NULL)
		{

			return;
		}
		tempnode = invoke->GetSpecificSequence(ncUniversal, 2);
		if(tempnode == NULL)
		{

			return;
		}
		operationValue = tempnode->GetIntegerData();

		switch(operationValue)
		{
		case 211: //systemStatus
			char * response;
			response = (char*)MakeStatusResponse();
			CstaSend((unsigned char*)response,"SystemStatus");
			initStatus.unlock();
			delete response;
			break;
		case 21: //CSTAEventReport
			if(( tempnode = invoke->GetSpecificSequence(ncUniversal, 16))!=NULL)
			{
				memcpy(tempnode->dump,invoke->dump,invoke->tamDump);
			    tempnode->tamDump = invoke->tamDump;
				CstaEvent(tempnode);
			}else
			{

			}
			break;
		case 77:
			CstaSnapshotDeviceData(invoke);
			break;
		case 51:
			lastBeat = time(NULL);
			SendBeatEvent();
			break;

		}
	}catch(...)
	{

	}
}

void CCSTA::CstaReceive(unsigned char *data, unsigned int dataSize)
{
	try{
		CAsnNode * result = new CAsnNode(data, dataSize);
		memcpy(result->dump,data,dataSize);
		result->tamDump = dataSize;

		result->logAsn(string("\nMESSAGE RECEIVED:\n"+result->LogText(0)).c_str());
		switch(result->GetNodeClass())
		{
		case ncApplication:
			break;
		case ncContext_Specific:
			if(result->GetTagNumber() == 1)
				ProcessInvoke(result);
			else if(result->GetTagNumber() == 2)
				ProcessResult(result);
			else if(result->GetTagNumber() == 3)
				ProcessError(result);
			break;
		case ncUniversal:
		case ncPrivate:
			break;
		}
		delete result;
	}catch(...)
	{

	}
}

void CCSTA::SocketRead(int handle)
{
	unsigned char dados[MAX_PACKET_SIZE];
	int totalReceived = 0;
	try
	{
		totalReceived = recv(handle,dados,MAX_PACKET_SIZE,0);
		loghex(dados, totalReceived, "Socket Recv:");
	}
	catch(...)
	{

	}
	sem_wait(&semaBuffer);
	try
	{
		if(totalReceived > 0)
		{
			try
			{
				cstaBuffer.Add(dados, totalReceived);
			}
			catch(...)
			{

			}
		}
		else
		{
			try
			{
				logCsta("Csta Desconectado");
				monitorList.erase(monitorList.begin(),monitorList.end());
				CstaRequests.erase(CstaRequests.begin(),CstaRequests.end());
				cstaBuffer.Clear();
				initStatus.Restart();
				eventDispatcher->Clear();
				SendDisconnectEvent();
			}
			catch(...)
			{

			}
			socket_active = false;
			beatActed = false;
		}
	}
	catch(...)
	{

	}
	sem_post(&semaBuffer);
}

void CCSTA::ReceiveBuffer()
{

	TCSTAMessage message;
	unsigned int messageSize;
	sem_wait(&semaBuffer);
	try
	{
		if((messageSize = cstaBuffer.GetMessage(message.msgData, CSTA_MAX_SIZE)) > 0)
		{
			CstaReceive(&message.msgData[TotalHeaderSize], messageSize);
		}

	}catch(...)
	{

	}
	sem_post(&semaBuffer);

}

void * CCSTA::checkStatus(void * arg)
{
	CCSTA * ptrCsta;
	ptrCsta = (CCSTA *)arg;
	while(1)
	{
		sleep(5);
		try
		{
			if(ptrCsta->beatActed)
			{
				if(TimeMaiorADiffB(30,time(NULL),ptrCsta->lastBeat))
				{
					ptrCsta->logCsta("HeartBeatTimeout");
					ptrCsta->beatActed = false;
					if(ptrCsta->socket_active)
					{
						ptrCsta->logCsta("HeartBeatTimeout reiniciando...");
						ptrCsta->cstaConnected = false;
						ptrCsta->socket_active = false;
						try
						{
						 shutdown(ptrCsta->handle_socket,SHUT_RDWR);
						}
						catch(...)
						{
						}
						close(ptrCsta->handle_socket);
					}
				}
				else
				{
					ptrCsta->logCsta("!!!Heart Beat oK diff["+ptrCsta->IntToStr(difftime(time(NULL),ptrCsta->lastBeat))+"]!!!");
				}
			}
		}catch(...)
		{
		}
	}
	pthread_exit(0);
}

void * CCSTA::CstaMainLoop(void * arg)
{
	try
	{
		CCSTA * pCsta;

		pCsta = (CCSTA *)arg;
		pCsta->logCsta("!!!Rodando CstaMainLoop!!!");
		while(1)
		{
			if(pCsta->socket_active)
			{
				pCsta->SocketRead(pCsta->handle_socket);
			}
			usleep(30000);
		}
		//pCsta->logCsta("!!!Parando CstaMainLoop!!!");
	}catch(...)
	{
	}
	return 0;
}
void * CCSTA::CstaReceiveLoop(void * arg)
{
	try
	{
		CCSTA * pCsta;

		pCsta = (CCSTA *)arg;
		while(1)
		{
			pCsta->ReceiveBuffer();
			usleep(30000);
		}
	}catch(...)
	{
	}
	return 0;
}
bool CCSTA::InitializeSocket(string host, int portno)
{
	struct hostent * server;
	struct sockaddr_in serv_addr;
	server = gethostbyname(host.c_str());
    logCsta("InitializeSocket "+host+":"+IntToStr(portno));
	if(socket_active)
		return true;

	try
	{
		if (server != NULL)
		{
			handle_socket = socket(AF_INET, SOCK_STREAM, 0);
			if(handle_socket != -1)
			{
				char cAdd[64];
				sprintf(cAdd,"%d.%d.%d.%d",(unsigned char)server->h_addr[0],
						(unsigned char)server->h_addr[1],
						(unsigned char)server->h_addr[2],
						(unsigned char)server->h_addr[3]);
				logCsta(string("IP Resolvido: ")+cAdd);

				bzero((char *) &serv_addr, sizeof(serv_addr));
				serv_addr.sin_family = AF_INET;
				bcopy((char *)server->h_addr,
				      (char *)&serv_addr.sin_addr.s_addr,
				      server->h_length);
				serv_addr.sin_port = htons(portno);

				if (connect(handle_socket,(struct sockaddr*)&serv_addr,sizeof(serv_addr)) == -1)
				{
					logCsta("!!!connect error!!!");
					socket_active = false;
					try
					{
					 shutdown(handle_socket,SHUT_RDWR);
					}
					catch(...)
					{
					}
					close(handle_socket);

				}
				else
				{
					socket_active = true;
					logCsta("!!!Conectado!!!");
				}
		   }
		}
		else
		{

		}
	}
	catch (...)
	{

	}
	return socket_active;
}

unsigned char * CCSTA::GetSystemStatusMessage(int invokeId)
{
	unsigned char * output;
	CAsnNode * main;
	CAsnNode * invoke_id;
	CAsnNode * sequence;
	CAsnNode * operation_value;
	CAsnNode * status;

	main = new CAsnNode(ncContext_Specific, ntConstructed, 1);
	invoke_id = new CAsnNode(ncUniversal, ntPrimitive, 2);
	operation_value = new CAsnNode(ncUniversal, ntPrimitive, 2);
	sequence = new CAsnNode(ncUniversal, ntConstructed, 16);
	status = new CAsnNode(ncUniversal, ntPrimitive, 10);



	status->Add(2);
	invoke_id->Add(invokeId);
	operation_value->AddInv(0xD300);


	sequence->Add(status);

	main->Add(invoke_id);
	main->Add(operation_value);
	main->Add(sequence);

	output = main->c_str();
	delete main;

	return output;
}

unsigned char * CCSTA::RequestSystemStatusMessage(int invokeId)
{
	unsigned char * output;
	CAsnNode * main;
	CAsnNode * invoke_id;
	CAsnNode * sequence;
	CAsnNode * operation_value;
	main = new CAsnNode(ncContext_Specific, ntConstructed, 1);
	invoke_id = new CAsnNode(ncUniversal, ntPrimitive, 2);
	operation_value = new CAsnNode(ncUniversal, ntPrimitive, 2);
	sequence = new CAsnNode(ncUniversal, ntConstructed, 16);
	//status = new CAsnNode(ncUniversal, ntPrimitive, 10);
	//status->Add(2);
	invoke_id->Add(invokeId);
	operation_value->AddInv(0xD200);
	//sequence->Add(status);
	main->Add(invoke_id);
	main->Add(operation_value);
	main->Add(sequence);
	output = main->c_str();
	delete main;
	return output;

}

unsigned char * CCSTA::SetForwardingMessage(char * device, char * destination, int forwardType, int Activate, int invokeId)
{

	CAsnNode * main;
	CAsnNode * invoke_id;
	CAsnNode * sequence;
	CAsnNode * operation_value;
	CAsnNode * forward_device;
	CAsnNode * forward_device_value;
	CAsnNode * forwarding_type;
	CAsnNode * activate_forward;
	CAsnNode * forward_destination;
	CAsnNode * forward_destination_value;

	unsigned char * output;

	main = new CAsnNode(ncContext_Specific, ntConstructed, 1);
	invoke_id = new CAsnNode(ncUniversal, ntPrimitive, 2);
	sequence = new CAsnNode(ncUniversal, ntConstructed, 16);
	operation_value = new CAsnNode(ncUniversal, ntPrimitive, 2);
	forward_device = new CAsnNode(ncUniversal, ntConstructed, 16);
	forward_device_value = new CAsnNode(ncContext_Specific, ntPrimitive, 0);
	forward_destination = new CAsnNode(ncUniversal, ntConstructed, 16);
	forward_destination_value = new CAsnNode(ncContext_Specific, ntPrimitive, 0);
	forwarding_type = new CAsnNode(ncUniversal, ntPrimitive, 10);
	activate_forward = new CAsnNode(ncUniversal, ntPrimitive, 1);

	forward_device_value->Add(device);
	forward_device->Add(forward_device_value);

	forward_destination_value->Add(destination);
	forward_destination->Add(forward_destination_value);

	forwarding_type->Add(forwardType);

	activate_forward->Add(Activate);

	sequence->Add(forward_device);
	sequence->Add(forwarding_type);
	sequence->Add(activate_forward);
	sequence->Add(forward_destination);

	invoke_id->Add(invokeId);
	operation_value->Add(317);

	main->Add(invoke_id);
	main->Add(operation_value);
	main->Add(sequence);
	output = main->c_str();

	delete main;
	return output;
}


unsigned char * CCSTA::ActHeartBeatMessage(int invokeId, short timeout)
{
	CAsnNode * privateSequence = new CAsnNode(ncPrivate, ntConstructed, 1);
	CAsnNode * privateData = new CAsnNode(ncUniversal, ntPrimitive, 6);
	CAsnNode * params = new CAsnNode(ncUniversal, ntPrimitive, 4);
	CAsnNode * sequence = new CAsnNode(ncUniversal, ntConstructed, 16);
	CAsnNode * main;
	unsigned char sequence1[] = { 0x2B, 0x0C, 0x02, 0x88, 0x53, 0x0F };
	unsigned char sequence2[] = { 0x1A, 0x04, 0x06, 0xC4, 0x22, 0x04, 0x03, 0x00, 0x23, 0x04, 0x14, 0x00 };

	memcpy(&sequence2[sizeof(sequence2)-2], &timeout, sizeof(short));
	privateData->Add(sequence1, sizeof(sequence1));
	params->Add(sequence2, sizeof(sequence2));

	privateSequence->Add(privateData);
	privateSequence->Add(params);

	sequence->Add(privateSequence);

	unsigned char * output;
	main = CreateOperation(51, invokeId, sequence);
	output = main->c_str();

	delete main;
	return output;
}

unsigned char * CCSTA::SnapshotDeviceMessage(int invokeId, char * device)
{

	CAsnNode * main;
	CAsnNode * sequence;
	CAsnNode * device_sequence;
	CAsnNode * device_identifier;

	sequence = new CAsnNode(ncUniversal, ntConstructed, 16);
	device_sequence = new CAsnNode(ncUniversal, ntConstructed, 16);
	device_identifier = new CAsnNode(ncContext_Specific, ntPrimitive, 0);

	device_identifier->Add(device);
	device_sequence->Add(device_identifier);

	sequence->Add(device_sequence);

	unsigned char * output;
	main = CreateOperation(74, invokeId, sequence);
	output = main->c_str();


	delete main;
	return output;
}

unsigned char * CCSTA::SetAgentStateMessage(int invokeId, char * device, TReqAgentState agentState, char * agentId)
{

	CAsnNode * main;
	CAsnNode * sequence;
	CAsnNode * device_sequence;
	CAsnNode * device_identifier;
	CAsnNode * req_agent_state;
	CAsnNode * agent_id;

	sequence = new CAsnNode(ncUniversal, ntConstructed, 16);
	device_sequence = new CAsnNode(ncUniversal, ntConstructed, 16);
	device_identifier = new CAsnNode(ncContext_Specific, ntPrimitive, 0);
	req_agent_state = new CAsnNode(ncUniversal, ntPrimitive, 10);

	device_identifier->Add(device);
	device_sequence->Add(device_identifier);

	req_agent_state->Add((int)agentState, 1);

	sequence->Add(device_sequence);
	sequence->Add(req_agent_state);

	if(agentId != NULL && strlen(agentId) > 0)
	{
		agent_id = new CAsnNode(ncContext_Specific, ntPrimitive, 2);
		agent_id->Add(agentId);
		sequence->Add(agent_id);
	}

	unsigned char * output;
	main = CreateOperation(312, invokeId, sequence);
	output = main->c_str();

	delete main;
	return output;
}

unsigned char * CCSTA::GetAgentStateMessage(int invokeId, char * device)
{

	CAsnNode * main;
	CAsnNode * sequence;
	CAsnNode * device_sequence;
	CAsnNode * device_identifier;

	sequence = new CAsnNode(ncUniversal, ntConstructed, 16);
	device_sequence = new CAsnNode(ncUniversal, ntConstructed, 16);
	device_identifier = new CAsnNode(ncContext_Specific, ntPrimitive, 0);

	device_identifier->Add(device);
	device_sequence->Add(device_identifier);

	sequence->Add(device_sequence);

	unsigned char * output;
	main = CreateOperation(304, invokeId, sequence);
	output = main->c_str();


	delete main;
	return output;
}

unsigned char * CCSTA::SnapshotCallMessage(int callId, int invokeId, char * device)
{

	CAsnNode * main;
	CAsnNode * invoke_id;
	CAsnNode * sequence;
	CAsnNode * operation_value;
	CAsnNode * connection;
	CAsnNode * connection_sequence;
	CAsnNode * connection_callId;
	CAsnNode * connection_deviceId;
	CAsnNode * connection_deviceId_sequence;
	CAsnNode * connection_deviceId_data;


	unsigned char * output;

	main = new CAsnNode(ncContext_Specific, ntConstructed, 1);
	invoke_id = new CAsnNode(ncUniversal, ntPrimitive, 2);
	sequence = new CAsnNode(ncUniversal, ntConstructed, 16);
	operation_value = new CAsnNode(ncUniversal, ntPrimitive, 2);
	connection = new CAsnNode(ncApplication, ntConstructed, 11);
	connection_sequence = new CAsnNode(ncUniversal, ntConstructed, 16);
	connection_callId = new CAsnNode(ncContext_Specific, ntPrimitive, 0);
	connection_deviceId = new CAsnNode(ncContext_Specific, ntConstructed, 1);
	connection_deviceId_sequence = new CAsnNode(ncUniversal, ntConstructed, 16);
	connection_deviceId_data = new CAsnNode(ncContext_Specific, ntPrimitive, 0);

	connection_deviceId_data->Add(device);
	connection_deviceId_sequence->Add(connection_deviceId_data);
	connection_deviceId->Add(connection_deviceId_sequence);

	connection_callId->Add(callId);

	connection_sequence->Add(connection_callId);
	connection_sequence->Add(connection_deviceId);
	connection->Add(connection_sequence);
	sequence->Add(connection);

	invoke_id->Add(invokeId);
	operation_value->Add(75);

	main->Add(invoke_id);
	main->Add(operation_value);
	main->Add(sequence);
	output = main->c_str();
	delete main;
	return output;
}

unsigned char * CCSTA::GroupPickupMessage(int callId, int invokeId, char * requesting_device, char * topickup_device)
{

	CAsnNode * main;
	CAsnNode * invoke_id;
	CAsnNode * sequence;
	CAsnNode * operation_value;
	CAsnNode * connection;
	CAsnNode * connection_sequence;
	CAsnNode * connection_callId;
	CAsnNode * connection_deviceId;
	CAsnNode * connection_deviceId_sequence;
	CAsnNode * connection_deviceId_data;
	CAsnNode * req_deviceId_sequence;
	CAsnNode * req_deviceId_data;


	unsigned char * output;

	main = new CAsnNode(ncContext_Specific, ntConstructed, 1);
	invoke_id = new CAsnNode(ncUniversal, ntPrimitive, 2);
	sequence = new CAsnNode(ncUniversal, ntConstructed, 16);
	operation_value = new CAsnNode(ncUniversal, ntPrimitive, 2);
	connection = new CAsnNode(ncApplication, ntConstructed, 11);
	connection_sequence = new CAsnNode(ncUniversal, ntConstructed, 16);
	connection_callId = new CAsnNode(ncContext_Specific, ntPrimitive, 0);
	connection_deviceId = new CAsnNode(ncContext_Specific, ntConstructed, 1);
	connection_deviceId_sequence = new CAsnNode(ncUniversal, ntConstructed, 16);
	connection_deviceId_data = new CAsnNode(ncContext_Specific, ntPrimitive, 0);



	connection_deviceId_data->Add(topickup_device);
	connection_deviceId_sequence->Add(connection_deviceId_data);
	connection_deviceId->Add(connection_deviceId_sequence);

	connection_callId->Add(callId);

	connection_sequence->Add(connection_callId);
	connection_sequence->Add(connection_deviceId);
	connection->Add(connection_sequence);
	sequence->Add(connection);

	req_deviceId_sequence = new CAsnNode(ncUniversal, ntConstructed, 16);
	req_deviceId_data = new CAsnNode(ncContext_Specific, ntPrimitive, 0);

	req_deviceId_data->Add(requesting_device);
	req_deviceId_sequence->Add(req_deviceId_data);

	sequence->Add(req_deviceId_sequence);

	invoke_id->Add(invokeId);
	operation_value->AddInv(0xDC00);

	main->Add(invoke_id);
	main->Add(operation_value);
	main->Add(sequence);
	output = main->c_str();

	delete main;
	return output;
}

unsigned char * CCSTA::ParkCallMessage(int callId, int invokeId, char * parking_device, char * parkTo)
{

	CAsnNode * main;
	CAsnNode * invoke_id;
	CAsnNode * sequence;
	CAsnNode * operation_value;
	CAsnNode * connection;
	CAsnNode * connection_sequence;
	CAsnNode * connection_callId;
	CAsnNode * connection_deviceId;
	CAsnNode * connection_deviceId_sequence;
	CAsnNode * connection_deviceId_data;
	CAsnNode * req_deviceId_sequence;
	CAsnNode * req_deviceId_data;


	unsigned char * output;

	main = new CAsnNode(ncContext_Specific, ntConstructed, 1);
	invoke_id = new CAsnNode(ncUniversal, ntPrimitive, 2);
	sequence = new CAsnNode(ncUniversal, ntConstructed, 16);
	operation_value = new CAsnNode(ncUniversal, ntPrimitive, 2);
	connection = new CAsnNode(ncApplication, ntConstructed, 11);
	connection_sequence = new CAsnNode(ncUniversal, ntConstructed, 16);
	connection_callId = new CAsnNode(ncContext_Specific, ntPrimitive, 0);
	connection_deviceId = new CAsnNode(ncContext_Specific, ntConstructed, 1);
	connection_deviceId_sequence = new CAsnNode(ncUniversal, ntConstructed, 16);
	connection_deviceId_data = new CAsnNode(ncContext_Specific, ntPrimitive, 0);



	connection_deviceId_data->Add(parking_device);
	connection_deviceId_sequence->Add(connection_deviceId_data);
	connection_deviceId->Add(connection_deviceId_sequence);

	connection_callId->Add(callId);

	connection_sequence->Add(connection_callId);
	connection_sequence->Add(connection_deviceId);
	connection->Add(connection_sequence);
	sequence->Add(connection);

	req_deviceId_sequence = new CAsnNode(ncUniversal, ntConstructed, 16);
	req_deviceId_data = new CAsnNode(ncContext_Specific, ntPrimitive, 0);

	req_deviceId_data->Add(parkTo);
	req_deviceId_sequence->Add(req_deviceId_data);

	sequence->Add(req_deviceId_sequence);

	invoke_id->Add(invokeId);
	operation_value->Add(18);

	main->Add(invoke_id);
	main->Add(operation_value);
	main->Add(sequence);
	output = main->c_str();

	delete main;
	return output;
}

CAsnNode * CCSTA::CreateConnectionId(int callId, char * deviceId)
{

	CAsnNode * connection;
	CAsnNode * connection_sequence;
	CAsnNode * connection_callId;
	CAsnNode * connection_deviceId;
	CAsnNode * connection_deviceId_sequence;
	CAsnNode * connection_deviceId_data;

	connection = new CAsnNode(ncApplication, ntConstructed, 11);

	if(callId != 0 && strlen(deviceId) > 0)
	{
		connection_sequence = new CAsnNode(ncUniversal, ntConstructed, 16);
		connection_callId = new CAsnNode(ncContext_Specific, ntPrimitive, 0);
		connection_deviceId = new CAsnNode(ncContext_Specific, ntConstructed, 1);
		connection_deviceId_sequence = new CAsnNode(ncUniversal, ntConstructed, 16);
		connection_deviceId_data = new CAsnNode(ncContext_Specific, ntPrimitive, 0);

		connection_deviceId_data->Add(deviceId);
		connection_deviceId_sequence->Add(connection_deviceId_data);
		connection_deviceId->Add(connection_deviceId_sequence);
		if(callId <= 65535)
		{
			connection_callId->Add(callId, 2);
		}else
		{
			connection_callId->Add(callId);
		}

		connection_sequence->Add(connection_callId);
		connection_sequence->Add(connection_deviceId);
		connection->Add(connection_sequence);
	}else if(callId != 0)
	{
		connection_sequence = new CAsnNode(ncContext_Specific, ntPrimitive, 0);
		connection_sequence->Add(deviceId);
		connection->Add(connection_sequence);
	}else
	{
		connection_sequence = new CAsnNode(ncContext_Specific, ntPrimitive, 1);
		connection_sequence->Add(callId);
		connection->Add(connection_sequence);
	}

	return connection;
}

unsigned char * CCSTA::SetDisplayMessage(char * device, char * text, int invokeId)
{
	CAsnNode * main;
	CAsnNode * sequence;
	CAsnNode * nodeDevice;
	CAsnNode * nodeDialingNumber;
	CAsnNode * nodeDisplayContents;

	unsigned char * output;

	sequence            = new CAsnNode(ncUniversal, ntConstructed, 16);
	nodeDevice          = new CAsnNode(ncUniversal, ntConstructed, 16);
	nodeDialingNumber   = new CAsnNode(ncContext_Specific, ntPrimitive, 0);
	nodeDisplayContents = new CAsnNode(ncUniversal, ntPrimitive, 22);

	nodeDialingNumber->Add(device);
	nodeDisplayContents->Add(text);

	nodeDevice->Add(nodeDialingNumber);

	sequence->Add(nodeDevice);
	sequence->Add(nodeDisplayContents);
	main = CreateOperation(274, invokeId, sequence);

	output = main->c_str();
	delete main;

	return output;
}

unsigned char * CCSTA::GenerateDigitsMessage(int callId, char * device, char * digits, int invokeId)
{
	CAsnNode * main;
	CAsnNode * sequence;
	CAsnNode * nodeDigits;
	unsigned char * output;

	sequence = new CAsnNode(ncUniversal, ntConstructed, 16);
	nodeDigits = new CAsnNode(ncUniversal, ntPrimitive, 22);

	nodeDigits->Add(digits);

	sequence->Add(CreateConnectionId(callId, device));
	sequence->Add(nodeDigits);
	main = CreateOperation(232, invokeId, sequence);

	output = main->c_str();
	delete main;

	return output;
}

unsigned char * CCSTA::DialDigitsMessage(int callId, char * device, char * digits, int invokeId)
{
	CAsnNode * main;
	CAsnNode * sequence;
	CAsnNode * nodeDigits;
	CAsnNode * sequenceDigits;
	unsigned char * output;

	sequence = new CAsnNode(ncUniversal, ntConstructed, 16);
	nodeDigits = new CAsnNode(ncContext_Specific, ntPrimitive, 0);
	sequenceDigits = new CAsnNode(ncUniversal, ntConstructed, 16);

	nodeDigits->Add(digits);
	sequenceDigits->Add(nodeDigits);

	sequence->Add(CreateConnectionId(callId, device));
	sequence->Add(sequenceDigits);
	main = CreateOperation(219, invokeId, sequence);

	output = main->c_str();
	delete main;

	return output;
}




unsigned char * CCSTA::SingleStepTransferMessage(int callToTransfer, char * deviceTransferring, char * newDestination, int invokeId)
{
	CAsnNode * main;
	CAsnNode * sequence;
	CAsnNode * new_destination;
	CAsnNode * new_destination_device_id;
	CAsnNode * invoke_id;
	CAsnNode * operation_value;
	unsigned char * output;

	main = new CAsnNode(ncContext_Specific, ntConstructed, 1);
	invoke_id = new CAsnNode(ncUniversal, ntPrimitive, 2);
	operation_value = new CAsnNode(ncUniversal, ntPrimitive, 2);

	sequence = new CAsnNode(ncUniversal, ntConstructed, 16);
	new_destination = new CAsnNode(ncUniversal, ntConstructed, 16);
	new_destination_device_id = new CAsnNode(ncContext_Specific, ntPrimitive, 0);

	sequence->Add(CreateConnectionId(callToTransfer, deviceTransferring));

	new_destination_device_id->Add(newDestination);
	new_destination->Add(new_destination_device_id);
	sequence->Add(new_destination);

	invoke_id->Add(invokeId);
	operation_value->Add(50);

	main->Add(invoke_id);
	main->Add(operation_value);
	main->Add(sequence);

	output = main->c_str();

	delete main;
	return output;
}

unsigned char * CCSTA::ConsultationCallMessage(int callId, char * deviceId, char * consultedDevice, int invokeId)
{
	CAsnNode * main;
	CAsnNode * sequence;
	CAsnNode * nodeConsultedDevice;
	CAsnNode * new_destination;
	CAsnNode * new_destination_device_id;
	CAsnNode * invoke_id;
	CAsnNode * operation_value;
	CAsnNode * tmpConnectionId;
	unsigned char * output;

	main = new CAsnNode(ncContext_Specific, ntConstructed, 1);
	invoke_id = new CAsnNode(ncUniversal, ntPrimitive, 2);
	operation_value = new CAsnNode(ncUniversal, ntPrimitive, 2);

	sequence = new CAsnNode(ncUniversal, ntConstructed, 16);
	nodeConsultedDevice = new CAsnNode(ncContext_Specific, ntConstructed, 1);
	new_destination = new CAsnNode(ncUniversal, ntConstructed, 16);
	new_destination_device_id = new CAsnNode(ncContext_Specific, ntPrimitive, 0);

	tmpConnectionId = CreateConnectionId(callId, deviceId);
	tmpConnectionId->SetNodeClass(0xA0);
	tmpConnectionId->SetTagNumber(0);

	sequence->Add(tmpConnectionId);
	new_destination_device_id->Add(consultedDevice);
	new_destination->Add(new_destination_device_id);
	nodeConsultedDevice->Add(new_destination);
	sequence->Add(nodeConsultedDevice);

	invoke_id->Add(invokeId);
	operation_value->Add(7);

	main->Add(invoke_id);
	main->Add(operation_value);
	main->Add(sequence);

	output = main->c_str();

	delete main;
	return output;
}

unsigned char * CCSTA::AnswerCallMessage(int callId, char * deviceId, int invokeId)
{
	CAsnNode * main;
	CAsnNode * sequence;


	sequence = new CAsnNode(ncUniversal, ntConstructed, 16);


	sequence->Add(CreateConnectionId(callId, deviceId));

	unsigned char * output;
	main = CreateOperation(2, invokeId, sequence);
	output = main->c_str();


	delete main;
	return output;
}

unsigned char * CCSTA::AlternateCallMessage(int callId, char * deviceId, int heldCallId, char * heldDeviceId, int invokeId)
{
	CAsnNode * main;
	CAsnNode * sequence;


	sequence = new CAsnNode(ncUniversal, ntConstructed, 16);

	sequence->Add(CreateConnectionId(heldCallId, heldDeviceId));
	sequence->Add(CreateConnectionId(callId, deviceId));

	unsigned char * output;
	main = CreateOperation(1, invokeId, sequence);
	output = main->c_str();


	delete main;
	return output;
}

unsigned char * CCSTA::ReconnectCallMessage(int callId, char * deviceId, int heldCallId, char * heldDeviceId, int invokeId)
{
	CAsnNode * main;
	CAsnNode * sequence;


	sequence = new CAsnNode(ncUniversal, ntConstructed, 16);

	sequence->Add(CreateConnectionId(heldCallId, heldDeviceId));
	sequence->Add(CreateConnectionId(callId, deviceId));

	unsigned char * output;
	main = CreateOperation(13, invokeId, sequence);
	output = main->c_str();


	delete main;
	return output;
}

unsigned char * CCSTA::TransferCallMessage(int callId, char * deviceId, int heldCallId, char * heldDeviceId, int invokeId)
{
	CAsnNode * main;
	CAsnNode * sequence;


	sequence = new CAsnNode(ncUniversal, ntConstructed, 16);

	sequence->Add(CreateConnectionId(heldCallId, heldDeviceId));
	sequence->Add(CreateConnectionId(callId, deviceId));

	unsigned char * output;
	main = CreateOperation(16, invokeId, sequence);
	output = main->c_str();


	delete main;
	return output;
}

unsigned char * CCSTA::RetrieveCallMessage(int callId, char * deviceId, int invokeId)
{
	CAsnNode * main;
	CAsnNode * sequence;


	sequence = new CAsnNode(ncUniversal, ntConstructed, 16);


	sequence->Add(CreateConnectionId(callId, deviceId));

	unsigned char * output;
	main = CreateOperation(14, invokeId, sequence);
	output = main->c_str();


	delete main;
	return output;
}

unsigned char * CCSTA::HoldCallMessage(int callId, char * deviceId, int invokeId)
{
	CAsnNode * main;
	CAsnNode * sequence;


	sequence = new CAsnNode(ncUniversal, ntConstructed, 16);


	sequence->Add(CreateConnectionId(callId, deviceId));

	unsigned char * output;
	main = CreateOperation(9, invokeId, sequence);
	output = main->c_str();


	delete main;
	return output;
}

unsigned char * CCSTA::ClearConnectionMessage(int callId, char * deviceId, int invokeId)
{
	CAsnNode * main;
	CAsnNode * sequence;


	sequence = new CAsnNode(ncUniversal, ntConstructed, 16);


	sequence->Add(CreateConnectionId(callId, deviceId));

	unsigned char * output;
	main = CreateOperation(5, invokeId, sequence);
	output = main->c_str();


	delete main;
	return output;
}

unsigned char * CCSTA::DeflectCallMessage(int callDiverted, char * deviceDiverted, char * newDestination, int invokeId)
{
	CAsnNode * main;
	CAsnNode * sequence;
	CAsnNode * new_destination;
	CAsnNode * new_destination_device_id;
	CAsnNode * invoke_id;
	CAsnNode * operation_value;
	unsigned char * output;

	main = new CAsnNode(ncContext_Specific, ntConstructed, 1);
	invoke_id = new CAsnNode(ncUniversal, ntPrimitive, 2);
	operation_value = new CAsnNode(ncUniversal, ntPrimitive, 2);

	sequence = new CAsnNode(ncUniversal, ntConstructed, 16);
	new_destination = new CAsnNode(ncUniversal, ntConstructed, 16);
	new_destination_device_id = new CAsnNode(ncContext_Specific, ntPrimitive, 0);

	sequence->Add(CreateConnectionId(callDiverted, deviceDiverted));

	new_destination_device_id->Add(newDestination);
	new_destination->Add(new_destination_device_id);
	sequence->Add(new_destination);

	invoke_id->Add(invokeId);
	operation_value->AddInv(0xDA00);

	main->Add(invoke_id);
	main->Add(operation_value);
	main->Add(sequence);

	output = main->c_str();

	delete main;
	return output;
}

unsigned char * CCSTA::ConferenceCallMessage(int heldCallId, char * heldDeviceId, int activeCallId, char * activeDeviceId)
{

	CAsnNode * main;
	CAsnNode * invoke_id;
	CAsnNode * sequence;
	CAsnNode * operation_value;

	unsigned char * output;

	main = new CAsnNode(ncContext_Specific, ntConstructed, 1);
	invoke_id = new CAsnNode(ncUniversal, ntPrimitive, 2);
	sequence = new CAsnNode(ncUniversal, ntConstructed, 16);
	operation_value = new CAsnNode(ncUniversal, ntPrimitive, 2);

	sequence->Add(CreateConnectionId(heldCallId, heldDeviceId));
	sequence->Add(CreateConnectionId(activeCallId, activeDeviceId));

	operation_value->Add(6);

	invoke_id->Add(atoi(string(activeDeviceId).c_str()));

	main->Add(invoke_id);
	main->Add(operation_value);
	main->Add(sequence);
	output = main->c_str();

	delete main;
	return output;

}

unsigned char * CCSTA::MakeCallMessage(char * from, char * to, int invokeId)
{

	CAsnNode * main;
	CAsnNode * invoke_id;
	CAsnNode * sequence;
	CAsnNode * operation_value;
	CAsnNode * calling_sequence;
	CAsnNode * calling_number;
	CAsnNode * called_sequence;
	CAsnNode * called_number;

	unsigned char * output;

	main = new CAsnNode(ncContext_Specific, ntConstructed, 1);
	invoke_id = new CAsnNode(ncUniversal, ntPrimitive, 2);
	sequence = new CAsnNode(ncUniversal, ntConstructed, 16);
	operation_value = new CAsnNode(ncUniversal, ntPrimitive, 2);
	calling_sequence = new CAsnNode(ncUniversal, ntConstructed, 16);
	calling_number = new CAsnNode(ncContext_Specific, ntPrimitive, 0);
	called_sequence = new CAsnNode(ncUniversal, ntConstructed, 16);
	called_number = new CAsnNode(ncContext_Specific, ntPrimitive, 0);

	CAsnNode * codProje = new CAsnNode(ncContext_Specific, ntPrimitive, 1);


	calling_number->Add(from);
	calling_sequence->Add(calling_number);

	called_number->Add(to);
	called_sequence->Add(called_number);



	sequence->Add(calling_sequence);
	sequence->Add(called_sequence);

	//codProje->Add((char*)"1647");
	//sequence->Add(codProje);

	invoke_id->Add(invokeId);
	operation_value->Add(10);

	main->Add(invoke_id);
	main->Add(operation_value);
	main->Add(sequence);
	output = main->c_str();

	delete main;
	return output;
}

CAsnNode * CCSTA::CreateOperation(int operationValue, int invokeId, CAsnNode * sequence)
{
	CAsnNode * operation;
	CAsnNode * invoke_id;
	CAsnNode * operation_value;

	operation = new CAsnNode(ncContext_Specific, ntConstructed, 1);
	invoke_id = new CAsnNode(ncUniversal, ntPrimitive, 2);
	operation_value = new CAsnNode(ncUniversal, ntPrimitive, 2);


	invoke_id->Add(invokeId);
	operation_value->Add(operationValue);

	operation->Add(invoke_id);
	operation->Add(operation_value);
	operation->Add(sequence);

	return operation;
}

unsigned char * CCSTA::MonitorStartMessage(char * device, int invokeId)
{
	unsigned char * output;
	try
	{
		CAsnNode * main;
		CAsnNode * argument;
		CAsnNode * monitor_object;
		CAsnNode * device_object;
		CAsnNode * req_filter;
		CAsnNode * call_control;
		CAsnNode * call_associated;
		CAsnNode * media_attachment;
		CAsnNode * physical_device_feature;
		CAsnNode * logical_device_feature;
		CAsnNode * maintenance;
		CAsnNode * voice_unit;
		CAsnNode * filter_private;
		argument = new CAsnNode(ncUniversal, ntConstructed,16);
		monitor_object = new CAsnNode(ncUniversal, ntConstructed,16);
		device_object = new CAsnNode(ncContext_Specific, ntPrimitive,0);
		req_filter = new CAsnNode(ncContext_Specific, ntConstructed,0);
		call_control  = new CAsnNode(ncContext_Specific, ntPrimitive,0);
		call_associated = new CAsnNode(ncContext_Specific, ntPrimitive,6);
		media_attachment = new CAsnNode(ncContext_Specific, ntPrimitive,7);
		physical_device_feature = new CAsnNode(ncContext_Specific, ntPrimitive,8);
		logical_device_feature = new CAsnNode(ncContext_Specific, ntPrimitive,9);
		maintenance = new CAsnNode(ncContext_Specific, ntPrimitive,3);
		voice_unit = new CAsnNode(ncContext_Specific, ntPrimitive,5);
		filter_private = new CAsnNode(ncContext_Specific, ntPrimitive,4);
		device_object->Add(device);
		//call_control->Add(0x01000000);
		call_control->Add(0x06000000);
		call_associated->Add(0x0100);
		media_attachment->Add(0x0100);
		physical_device_feature->Add(0x010000);
		logical_device_feature->Add(0x020000);
		//logical_device_feature->Add(0x010000);
		maintenance->Add(0x0100);
		voice_unit->Add(0x0100);
		filter_private->Add(0x0100);
		monitor_object->Add(device_object);
		req_filter->Add(call_control);
		req_filter->Add(call_associated);
		req_filter->Add(media_attachment);
		req_filter->Add(physical_device_feature);
		req_filter->Add(logical_device_feature);
		req_filter->Add(maintenance);
		req_filter->Add(voice_unit);
		req_filter->Add(filter_private);
		argument->Add(monitor_object);
		argument->Add(req_filter);
		main = CreateOperation(71, invokeId, argument);
		output = main->c_str();
		delete main;
	}
	catch(...)
	{

	}
	return output;
}

unsigned char * CCSTA::TrunkStartMessage(int device, int invokeId)
{
	unsigned char * output;
	try{
		CAsnNode * main;
		CAsnNode * argument;
		CAsnNode * monitor_object;
		CAsnNode * device_object;
		CAsnNode * req_filter;
		CAsnNode * call_control;
		CAsnNode * call_associated;
		CAsnNode * media_attachment;
		CAsnNode * physical_device_feature;
		CAsnNode * logical_device_feature;
		CAsnNode * maintenance;
		CAsnNode * voice_unit;
		CAsnNode * filter_private;

		argument = new CAsnNode(ncUniversal, ntConstructed,16);
		monitor_object = new CAsnNode(ncUniversal, ntConstructed,16);
		device_object = new CAsnNode(ncContext_Specific, ntPrimitive,1);
		req_filter = new CAsnNode(ncContext_Specific, ntConstructed,0);
		call_control  = new CAsnNode(ncContext_Specific, ntPrimitive,0);
		call_associated = new CAsnNode(ncContext_Specific, ntPrimitive,6);
		media_attachment = new CAsnNode(ncContext_Specific, ntPrimitive,7);
		physical_device_feature = new CAsnNode(ncContext_Specific, ntPrimitive,8);
		logical_device_feature = new CAsnNode(ncContext_Specific, ntPrimitive,9);
		maintenance = new CAsnNode(ncContext_Specific, ntPrimitive,3);
		voice_unit = new CAsnNode(ncContext_Specific, ntPrimitive,5);
		filter_private = new CAsnNode(ncContext_Specific, ntPrimitive,4);

		device_object->Add(device);

		call_control->Add(0x01000000);
		call_associated->Add(0x0100);
		media_attachment->Add(0x0100);
		physical_device_feature->Add(0x010000);
		logical_device_feature->Add(0x010000);
		maintenance->Add(0x0100);
		voice_unit->Add(0x0100);
		filter_private->Add(0x0100);

		monitor_object->Add(device_object);

		req_filter->Add(call_control);
		req_filter->Add(call_associated);
		req_filter->Add(media_attachment);
		req_filter->Add(physical_device_feature);
		req_filter->Add(logical_device_feature);
		req_filter->Add(maintenance);
		req_filter->Add(voice_unit);
		req_filter->Add(filter_private);

		argument->Add(monitor_object);
		argument->Add(req_filter);

		main = CreateOperation(71, invokeId, argument);
		output = main->c_str();

		delete main;

	}catch(...)
	{

	}
	return output;
}

unsigned char * CCSTA::MonitorStopMessage(int crossRefId, int invokeId)
{
	unsigned char * output;
	try{
		CAsnNode * main;
		CAsnNode * invoke;
		CAsnNode * operation;
		CAsnNode * argument;
		CAsnNode * cross_ref_id;

		main = new CAsnNode(ncContext_Specific, ntConstructed,1);
		invoke = new CAsnNode(ncUniversal, ntPrimitive,2);
		operation = new CAsnNode(ncUniversal, ntPrimitive,2);
		argument = new CAsnNode(ncUniversal, ntConstructed,16);
		cross_ref_id = new CAsnNode(ncApplication, ntPrimitive, 21);

		invoke->Add(invokeId);
		operation->Add(73);
		unsigned int tmpId;
		tmpId = crossRefId;
		tmpId = tmpId << 8;
		cross_ref_id->AddInv(tmpId);

		argument->Add(cross_ref_id);

		main->Add(invoke);
		main->Add(operation);
		main->Add(argument);
		output = main->c_str();

		delete main;

	}catch(...)
	{

	}
	return output;
}


unsigned char * CCSTA::MakeAutentication(string VersionInfo)
{
	unsigned char * output;
	if( VersionInfo=="v6.0")
	{
		logCsta("H4000 versão 6.0 MakeAutentication");
		char seq[] = { 0x00, 0x29,0x60, 0x27, 0xa1, 0x07, 0x06, 0x05, 0x2b, 0x0c,
				0x00, 0x81, 0x5a, 0xac, 0x07, 0x80, 0x05, 0x43,
				0x41, 0x50, 0x2d, 0x49, 0xbe, 0x13, 0x28, 0x11,
				0x06, 0x07, 0x2b, 0x0c, 0x00, 0x82, 0x1d, 0x81,
				0x48, 0xa0, 0x06, 0xa0, 0x04, 0x03, 0x02, 0x00,
				0x08 };
		SocketSend((unsigned char*)seq,sizeof(seq),string("MakeAutentication"));
		usleep(100000);
		return 0;
	}
	else if (VersionInfo == "Panasonic")
	{
		gVersionInfo = "Panasonic";
		logCsta("Panasonic MakeAutentication");
		char seq[] = { 0x00, 0x25, 0x60, 0x23, 0x80, 0x02, 0x07, 0x80, 0xa1, 0x07,
					0x06 ,0x05 ,0x2b ,0x0c ,0x00 ,0x81, 0x5a ,0xbe ,
					0x14 ,0x28 ,0x12 ,0x06 ,0x07 ,0x2b, 0x0c ,0x00 ,
					0x82 ,0x1d ,0x81 ,0x48 ,0xa0 ,0x07, 0xa0 ,0x05 ,
					0x03 ,0x03 ,0x00 ,0x08 ,0x00};


		SocketSend((unsigned char*)seq,sizeof(seq),string("MakeAutentication"));
		usleep(100000);
		return 0;
	}

	try
	{
		CAsnNode * main;
		CAsnNode * context_name;
		CAsnNode * context_value;
		CAsnNode * acse_autentication;
		CAsnNode * aut_external;
		CAsnNode * asn_encoding;
		CAsnNode * asn_autentication;
		CAsnNode * asn_aut_data;
		CAsnNode * asn_aut_name;
		CAsnNode * asn_aut_pass;
		CAsnNode * user_info;
		CAsnNode * user_sequence_of;
		CAsnNode * user_sequence;
		CAsnNode * user_encoding;
		CAsnNode * user_csta_version;

		main = new CAsnNode(ncApplication, ntConstructed, 0);
		context_name = new CAsnNode(ncContext_Specific, ntConstructed, 1);
		context_value = new CAsnNode(ncUniversal, ntPrimitive, 6);
		acse_autentication = new CAsnNode(ncContext_Specific, ntPrimitive, 10);
		aut_external = new CAsnNode(ncContext_Specific, ntConstructed, 12);
		asn_encoding = new CAsnNode(ncContext_Specific, ntConstructed, 2);
		asn_autentication = new CAsnNode(ncContext_Specific, ntConstructed, 0);
		asn_aut_data = new CAsnNode(ncContext_Specific, ntConstructed, 0);
		asn_aut_name = new CAsnNode(ncUniversal, ntPrimitive, 4);
		asn_aut_pass = new CAsnNode(ncUniversal, ntPrimitive, 4);
		user_info = new CAsnNode(ncContext_Specific, ntConstructed, 30);
		user_sequence_of = new CAsnNode(ncUniversal, ntConstructed, 8);
		user_sequence = new CAsnNode(ncContext_Specific, ntConstructed, 0);
		user_encoding = new CAsnNode(ncContext_Specific, ntConstructed, 0);
		user_csta_version = new CAsnNode(ncUniversal, ntPrimitive, 3);

		asn_aut_name->Add((char*)"AMHOST");
		asn_aut_pass->Add((char*)"77777");
		asn_aut_data->Add(asn_aut_name);
		asn_aut_data->Add(asn_aut_pass);
		asn_autentication->Add(asn_aut_data);
		asn_encoding->Add(asn_autentication);
		aut_external->Add(asn_encoding);

		acse_autentication->Add(0x0680);

		context_value->Add(0x2B0C00815A);
		context_name->Add(context_value);
		unsigned char csta_version[] = {0x00, 0x16, 0x00};
		user_csta_version->Add(csta_version, sizeof(csta_version));
		//user_csta_version->AddDataString("001600");
		user_encoding->Add(user_csta_version);
		user_sequence->Add(user_encoding);
		user_sequence_of->Add(user_sequence);
		user_info->Add(user_sequence_of);

		main->Add(context_name);
		main->Add(acse_autentication);
		main->Add(aut_external);
		main->Add(user_info);
		output = main->c_str();

		delete main;

	}catch(...)
	{

	}
	return output;
}

bool CCSTA::CstaConnect(string cstaHost, int cstaPort, bool requireAutentication,string VersionInfo)
{
	sEndCstaHost = cstaHost;
	iPortaCstaHost =  cstaPort;
	cstaConnected = false;
	try
	{
		logCsta("Iniciando comunicacao por Socket");
		if(InitializeSocket( cstaHost , cstaPort ))
		{
			logCsta("Conectado no ip "+string(cstaHost)+" porta: "+IntToStr(cstaPort));
			if(requireAutentication)
			{
				logCsta("Aguardando autenticacao...");
				unsigned char * buf;
			    buf = MakeAutentication();
				CstaInvoke(buf, OnConnect, -1,"MakeAutentication");
				logCsta("!!!Aguardando Resposta!!!");
				initStatus.lock(true,DEFAULT_TIMEOUT);
				logCsta("!!!Pegando Timeout!!!");
				if(!initStatus.GetTimeout())
				{
					cstaConnected = true;
					logCsta("!!!csta sucess!!!");
				}
				else
				{
					logCsta("!!!MakeAutentication no answer!!!");
				}
				delete buf;
			}
			else
			{
				if(VersionInfo=="v6.0")
				{
					logCsta("Versao 6.0");
					MakeAutentication(VersionInfo);
				}
				else if(VersionInfo=="Panasonic")
				{
					logCsta("Panasonic");
					MakeAutentication(VersionInfo);
				}
				cstaConnected = true;
			}
		}
	}
	catch(...)
	{
		cstaConnected = false;

	}
	return cstaConnected;
}

bool CCSTA::SetForwarding(char * device, char * destination, int forwardType, bool Activate)
{
	try
	{       unsigned char * message;
	int activation;

	if(Activate)
		activation = 0xFF;
	else
		activation = 0x00;

	message = SetForwardingMessage(device, destination, forwardType, activation, MakeInvokeId());
	CstaSend(message,"SetForwarding");
	delete message;

	}
	catch(...)
	{

	}

	return false;
}

bool CCSTA::MonitorExists(string device)
{
	bool result;
	TCSTAMonitorItem * ptrItem;
	for(unsigned int i = 0;i < monitorList.size();i++)
	{
		ptrItem = (TCSTAMonitorItem *)monitorList[i];
		if(ptrItem->device == device)
		{
			result = true;
			break;
		}
	}
	return result;
}


void CCSTA::DeleteMonitorItem(string device)
{
	TCSTAMonitorItem * ptrItem;

	for(unsigned int i = 0;i < monitorList.size();i++)
	{
		ptrItem = (TCSTAMonitorItem *)monitorList[i];

		if(ptrItem->device == device)
		{
			monitorList.erase(monitorList.begin()+i);
			delete ptrItem;
		}
	}
}

int CCSTA::GetMonitorIdByDevice(string device)
{
	TCSTAMonitorItem * ptrItem;
	int result;

	result = -1;
	for(unsigned int i = 0;i < monitorList.size();i++)
	{
		ptrItem = (TCSTAMonitorItem *)monitorList[i];

		if(ptrItem->device == device)
		{
			result = ptrItem->crossRefId;
			break;
		}
	}
	return result;
}

bool CCSTA::MonitorStop(char * device)
{
	int invokeId;
	HANDLE * h;
	bool result;

	result = false;
	try
	{
		invokeId = MakeInvokeId();

		if(MonitorExists(device))
		{
			unsigned char * message;
			message = MonitorStopMessage(GetMonitorIdByDevice(device), invokeId);
			h = CreateResponseEvent(invokeId);
			CstaSend(message,"MonitorStop");


			if(WaitForSingleObject(h,DEFAULT_TIMEOUT)!=-1)
			{
				CAsnNode * response;
				response = (CAsnNode *)GetResponseData(invokeId,(char*)"ActHeartBeat");

				if(response!= NULL && response->GetTagNumber() == 2)
				{
					result = true;
					DeleteMonitorItem(device);
					logCsta("Monitoração Desativada. Ramal: "+string(device)+" CrossRefId: "+IntToStr(GetMonitorIdByDevice(device)));
				}else{
					logCsta("Falha ao Desativar monitoração. Ramal: "+string(device)+" CrossRefId: "+IntToStr(GetMonitorIdByDevice(device)));
				}
			}
			delete message;
		}else{
			logCsta("Monitor n�o existe. Ramal: "+string(device));
		}
	}catch(...)
	{
	}
	return result;

}
HANDLE * CCSTA::CreateResponseEvent(int invokeId,string Creater)
{
	TResponseData * Response=NULL;
		sem_wait(&sema);
		{
			try
			{

				Response = new TResponseData;
				Response->SetHandle(Creater);
				Response->InvokeId = invokeId;
				Response->ptrData = NULL;
				logCsta("Inserindo Result invokeId:"+IntToStr(invokeId));
				CstaRequests.push_back(Response);

			}catch(...)
			{
			}
		}
		sem_post(&sema);
		return Response->handle;

}

/*Sync CCSTA::CreateResponseEvent(int invokeId,string Creater)
{
	TResponseData * Response=NULL;
	sem_wait(&sema);
	{
		try
		{
 			
			Response = new TResponseData;
			Response->h.SetCreater(Creater);
			Response->InvokeId = invokeId;
			Response->ptrData = NULL;
			logCsta("Inserindo Result invokeId:"+IntToStr(invokeId));
			CstaRequests.push_back(Response);

		}catch(...)
		{
		}
	}
	sem_post(&sema);
	return Response->h;

}*/

void * CCSTA::GetResponseData(int invokeId,char * src)
{
	TResponseData * tmpResponse;
	void * result = NULL;

	sem_wait(&sema);
	{
		try
		{
			for(unsigned i = 0;i < CstaRequests.size();i++)
			{

				tmpResponse = (TResponseData *)CstaRequests[i];
				if(tmpResponse != NULL && tmpResponse->InvokeId == invokeId)
				{
					logCsta("Retornando Result invokeId:"+IntToStr(invokeId));
					if (tmpResponse->ptrData != NULL)
					  result = tmpResponse->ptrData;
					else
						logCsta("ptrData Result NULL: "+string(src));

					break;
				}
			}
		}catch(...)
		{
		}

	}
	sem_post(&sema);

	return result;
}

void CCSTA::DeleteResponseEvent(int invokeId)
{
	TResponseData * tmpResponse;
	CAsnNode * ptrNode;

	sem_wait(&sema);
	{
		try
		{
			for(unsigned i = 0;i < CstaRequests.size();i++)
			{

				tmpResponse = (TResponseData *)CstaRequests[i];
				if(tmpResponse != NULL && tmpResponse->InvokeId == invokeId)
				{
					if(tmpResponse->ptrData != NULL)
					{
						ptrNode = (CAsnNode *)tmpResponse->ptrData;
						delete ptrNode;
					}
					CstaRequests.erase(CstaRequests.begin()+i);
					delete tmpResponse->handle;
					delete tmpResponse;
					i--;
				}
			}
		}catch(...)
		{
		}

	}
	sem_post(&sema);
}

void CCSTA::AddMonitorItem(string device, int crossRefId, TMonitorType monitorType)
{
	TCSTAMonitorItem * ptrItem;
	bool exists;

	exists = false;

	for(unsigned int i = 0;i < monitorList.size();i++)
	{
		ptrItem = (TCSTAMonitorItem *)monitorList[i];

		if(ptrItem != NULL && ptrItem->device == device && ptrItem->crossRefId == crossRefId)
		{
			exists = true;
		}
	}

	if(!exists)
	{
		ptrItem = new TCSTAMonitorItem;
		ptrItem->device = device;
		ptrItem->crossRefId = crossRefId;
		ptrItem->monitorType = monitorType;
		monitorList.push_back(ptrItem);
	}
}

bool CCSTA::ActHeartBeat(int timeout)
{
	bool bResult;

	bResult = false;

	try
	{
		logCsta("!!!Ativando Heart Beat!!!");

		HANDLE * h;
		int InvokeId;
		unsigned char * message;
		InvokeId = MakeInvokeId();
		message = ActHeartBeatMessage(InvokeId, timeout);


		h = CreateResponseEvent(InvokeId,"ActHeartBeat");

		CstaSend(message,"ActHeartBeat");

		logCsta(string("Sincronizacao:")+string(h->str));

		if(WaitForSingleObject(h,DEFAULT_TIMEOUT)!=-1)
		{

			CAsnNode * result;

			result = (CAsnNode *)GetResponseData(InvokeId,(char*)"ActHeartBeat");

			if (result != NULL and result->GetTagNumber() == 2)
			{

				beatActed = true;
				lastBeat = time(NULL);
				logCsta("!!!HeartBeat Acted Success!!!:"+IntToStr(timeout) );
				bResult = true;
			}
			else
			{
				logCsta("HeartBeat Act Fail:"+IntToStr(timeout) );
			}
		}
		else
		{
			logCsta("HeartBeat Act Fail[No Answer]:"+IntToStr(timeout) );
		}

		DeleteResponseEvent(InvokeId);
		delete message;
	}
	catch(...)
	{

	}

	return bResult;
}

bool CCSTA::GetSystemStatus()
{
	bool bResult;

	bResult = false;

	try
	{       HANDLE * h;
	int InvokeId;
	unsigned char * message;
	InvokeId = MakeInvokeId();
	message = GetSystemStatusMessage(InvokeId);
	h = CreateResponseEvent(InvokeId,"GetSystemStatus");

	CstaSend(message,"GetSystemStatus");


	if(WaitForSingleObject(h,DEFAULT_TIMEOUT)!=-1)
	{

		bResult = true;

	}else{
	}

	DeleteResponseEvent(InvokeId);
	delete message;
	}
	catch(...)
	{

	}

	return bResult;
}

bool CCSTA::RequestSystemStatus(int * status,string Who)
{
	bool bResult;
	int rStatus;
	bResult = false;
	try
	{
		logCsta("requerendo RequestSystemStatus por "+Who);
		HANDLE * h;
		int InvokeId;
		unsigned char * message;
		InvokeId = MakeInvokeId();
		message = RequestSystemStatusMessage(InvokeId);
		h = CreateResponseEvent(InvokeId);
		CstaSend(message,"SetForwarding");

		if(WaitForSingleObject(h,DEFAULT_TIMEOUT)!=-1)
		{
			CAsnNode * result;
			result = (CAsnNode *)GetResponseData(InvokeId,(char*)"RequestSystemStatus");
			if (result != NULL && result->GetTagNumber() == 2)
			{
				if((result = result->GetSpecificSequence(ncUniversal, 16))!= NULL)
				{
					if((result = result->GetSpecificSequence(ncUniversal, 16))!= NULL)
					{
						if((result = result->GetSpecificSequence(ncUniversal, 10))!= NULL)
						{
							bResult = true;
							rStatus = result->GetIntegerData();
							logCsta("RequestSystemStatus Status: "+IntToStr(rStatus) );
							if(status)
							{
								*status = rStatus;
							}
						}
					}
				}
			}
			else
			{
				logCsta("Falha RequestSystemStatus");
			}
		}
		else
		{
			logCsta("Timeout RequestSystemStatus");
		}
		DeleteResponseEvent(InvokeId);
		delete message;
	}
	catch(...)
	{

	}
	return bResult;
}

bool CCSTA::MonitorStart(char * device, TMonitorType monitorType, int * crossRefId)
{
	bool bResult;

	bResult = false;

	try
	{       HANDLE * h;
	int InvokeId;
	unsigned char * message;

	logCsta("MonitorStart: "+string((char*)device));
	//if(!MonitorExists(device))
	{

		InvokeId = MakeInvokeId();

		if(monitorType == mtDevice)
			message = MonitorStartMessage(device, InvokeId);

		else if(monitorType == mtTrunk)
			message = TrunkStartMessage(atoi(device), InvokeId);

		h = CreateResponseEvent(InvokeId);
		h->str = "MonitorStart";

		CstaSend(message,"MonitorStart");


		if(WaitForSingleObject(h,DEFAULT_TIMEOUT)!=-1)
		{
			CAsnNode * result;
			result = (CAsnNode *)GetResponseData(InvokeId,(char*)"MonitorStart");



			if (result != NULL && result->GetTagNumber() == 2)
			{
				bResult = true;
				if((result = result->GetSpecificSequence(ncUniversal, 16))!=NULL)
				{
					if((result = result->GetSpecificSequence(ncUniversal, 16))!=NULL)
					{
						if((result = result->GetSpecificSequence(ncApplication, 21))!=NULL)
						{
							int tempCrossRefId;
							if(crossRefId != NULL)
							{
								*crossRefId = result->GetIntegerData();
							}
							tempCrossRefId = result->GetIntegerData();
							logCsta("Monitoração Ativada Ramal: "+string((char*)device)+" CrossRefId: "+IntToStr(tempCrossRefId) );
							AddMonitorItem(device, tempCrossRefId, monitorType);
						}
					}
				}
			}
			else if(result == NULL)
			{
				logCsta("Result NULL na funcao MonitorStart");
				if(monitorType == mtDevice)
					logCsta("Falha ao ativar Monitoração Ramal: "+string((char*)device));
				else
					logCsta("Falha ao ativar Monitoração Tronco: "+IntToStr(atoi(device)));
			}
			else{
				if(monitorType == mtDevice)
					logCsta("Falha ao ativar Monitoração Ramal: "+string((char*)device));
				else
					logCsta("Falha ao ativar Monitoração Tronco: "+IntToStr(atoi(device)));
			}
		}

		DeleteResponseEvent(InvokeId);
		delete message;

	}/*else{
                        logCsta("Monitor j� Iniciado Ramal: "+string((char*)device));
                }*/
	}
	catch(...)
	{

	}

	return bResult;
}

bool CCSTA::SnapshotCall(char * device, int callId, TSnapshotCallResult * resultData)
{
	bool bResult;
	bResult = false;
	try
	{       unsigned char * message;
	int invokeId;
	HANDLE * h;

	invokeId = MakeInvokeId();

	h = CreateResponseEvent(invokeId);

	message = SnapshotCallMessage(callId, invokeId, device);
	CstaSend(message,"SnapShotCall");


	if(WaitForSingleObject(h,DEFAULT_TIMEOUT)!=-1)
	{
		CAsnNode * result;
		result = (CAsnNode *)GetResponseData(invokeId,(char*)"SnapshotCall");

		if(result != NULL && result->GetTagNumber() == 2)
		{
			bResult = true;
			if(resultData != NULL)
			{
				GetSnapshotCallResult(result, resultData);
			}
		}
	}
	DeleteResponseEvent(invokeId);
	delete message;

	}
	catch(...)
	{

	}

	return bResult;
}

bool CCSTA::SetAgentState(char * device, TReqAgentState agentState, char * agentId)
{
	bool bResult;

	bResult = false;
	try
	{       unsigned char * message;
	int invokeId;
	HANDLE * h;

	invokeId = MakeInvokeId();

	h = CreateResponseEvent(invokeId);
	message = SetAgentStateMessage(invokeId, device, agentState, agentId);
	CstaSend(message,"SetAgentState");


	if(WaitForSingleObject(h,DEFAULT_TIMEOUT)!=-1)
	{
		CAsnNode * result;
		result = (CAsnNode *)GetResponseData(invokeId,(char*)"SetAgentState");

		if(result != NULL && result->GetTagNumber() == 2)
		{
			bResult = true;
			logCsta("SetAgentState Executado Ramal: "+string(device));
		}
	}
	DeleteResponseEvent(invokeId);
	delete message;

	}
	catch(...)
	{

	}

	return bResult;
}

bool CCSTA::GetAgentState(char * device, string * agentId, TAgentState * agentState)
{
	bool bResult;
	try
	{       unsigned char * message;
	int invokeId;
	HANDLE * h;
	invokeId = MakeInvokeId();

	h = CreateResponseEvent(invokeId);
	message = GetAgentStateMessage(invokeId, device);
	CstaSend(message,"GetAgentState");
	bResult = false;

	if(WaitForSingleObject(h,DEFAULT_TIMEOUT)!=-1)
	{
		CAsnNode * result;
		CAsnNode * tempNode;
		CAsnNode * tempNode2;
		result = (CAsnNode *)GetResponseData(invokeId,(char*)"GetAgentState");

		if(result != NULL && result->GetTagNumber() == 2)
		{
			logCsta("GetAgentState Executado Ramal: "+string(device));
			if((tempNode = result->GetSpecificSequence(ncUniversal, 16))!=NULL)
			{
				if((tempNode = tempNode->GetSpecificSequence(ncUniversal, 16))!=NULL)
				{
					if((tempNode = tempNode->GetSpecificSequence(ncUniversal, 16))!=NULL)
					{
						if((tempNode = tempNode->GetSpecificSequence(ncUniversal, 16))!=NULL)
						{
							if((tempNode2 = tempNode->GetSpecificSequence(ncUniversal, 4))!=NULL)
							{
								bResult = true;
								if(agentId)
									*agentId = tempNode2->GetStringData();
							}
							if((tempNode2 = tempNode->GetSpecificSequence(ncUniversal, 16))!=NULL)
							{
								if((tempNode2 = tempNode2->GetSpecificSequence(ncUniversal, 16))!=NULL)
								{
									if((tempNode2 = tempNode2->GetSpecificSequence(ncUniversal, 10))!=NULL)
									{
										bResult = true;
										if(agentState)
											*agentState = (TAgentState)tempNode2->GetIntegerData();
									}
								}
							}
						}
					}
				}
			}
		}
	}
	DeleteResponseEvent(invokeId);
	delete message;

	}
	catch(...)
	{

	}
	return bResult;
}

void CCSTA::GetSnapshotDeviceResult(CAsnNode * result, TSnapshotDeviceResult * data)
{
	CAsnNode * tmpNode;
	CAsnNode * tmpNodeConnection;
	CAsnNode * tmpNodeConnectionID;
	CAsnNode * tmpNodeConnectionState;
	bool bCall;

	data->Count = 0;

	if((tmpNode = result->GetSpecificSequence(ncUniversal, 16))!=NULL)
		if((tmpNode = tmpNode->GetSpecificSequence(ncApplication, 22))!=NULL)
		{
			while((tmpNodeConnection = tmpNode->GetSpecificSequence(ncUniversal, 16))!=NULL)
			{
				bCall = false;
				data->snapshotDeviceItem[data->Count].connectionIdentifier.CallID = 0;
				data->snapshotDeviceItem[data->Count].connectionIdentifier.DeviceID.DeviceNumber = 0;
				data->snapshotDeviceItem[data->Count].localCallState = csNull;
				if((tmpNodeConnectionID = tmpNodeConnection->GetSpecificSequence(ncApplication, 11))!=NULL)
				{
					GetConnectionID(tmpNodeConnectionID, &data->snapshotDeviceItem[data->Count].connectionIdentifier);
					if(data->snapshotDeviceItem[data->Count].connectionIdentifier.CallID != 0)
					{
						bCall = true;
					}
				}
				if((tmpNodeConnectionState = tmpNodeConnection->GetSpecificSequence(ncContext_Specific, 0))!=NULL)
				{
					if((tmpNodeConnectionState = tmpNodeConnectionState->GetSpecificSequence(ncApplication, 14))!=NULL)
					{
						data->snapshotDeviceItem[data->Count].localCallState = (TLocalConnectionState)tmpNodeConnectionState->GetIntegerData();
					}else
					{
						data->snapshotDeviceItem[data->Count].localCallState = csNull;
					}
				}
				if(bCall)
					data->Count++;
			}
		}
}

void CCSTA::GetSnapshotCallResult(CAsnNode * result, TSnapshotCallResult * data)
{
	CAsnNode * tmpNode;
	CAsnNode * tmpNodeConnection;
	CAsnNode * tmpNodeData;

	data->Count = 0;

	if((tmpNode = result->GetSpecificSequence(ncUniversal, 16))!=NULL)
		if((tmpNode = tmpNode->GetSpecificSequence(ncUniversal, 16))!=NULL)
			if((tmpNode = tmpNode->GetSpecificSequence(ncApplication, 23))!=NULL)
			{
				while((tmpNodeConnection = tmpNode->GetSpecificSequence(ncUniversal, 16))!=NULL)
				{
					data->snapshotCallItem[data->Count].device.DeviceNumber = 0;
					data->snapshotCallItem[data->Count].connectionIdentifier.CallID = 0;
					data->snapshotCallItem[data->Count].connectionIdentifier.DeviceID.DeviceNumber = 0;
					data->snapshotCallItem[data->Count].localCallState = csNull;

					if((tmpNodeData = tmpNodeConnection->GetSpecificSequence(ncApplication, 3))!=NULL)
					{
						GetDeviceID(tmpNodeData, &data->snapshotCallItem[data->Count].device);
					}

					if((tmpNodeData = tmpNodeConnection->GetSpecificSequence(ncApplication, 11))!=NULL)
					{
						GetConnectionID(tmpNodeData, &data->snapshotCallItem[data->Count].connectionIdentifier);
					}
					if((tmpNodeData = tmpNodeConnection->GetSpecificSequence(ncApplication, 14))!=NULL)
					{
						data->snapshotCallItem[data->Count].localCallState = (TLocalConnectionState)tmpNodeData->GetIntegerData();
					}
					data->Count++;
				}
			}
}

bool CCSTA::SnapshotDevice(char * device, TSnapshotDeviceResult * resultData)
{
	bool bResult;
	bResult = false;
	try
	{       unsigned char * message;
	int invokeId;

	HANDLE * h;

	invokeId = MakeInvokeId();
	h = CreateResponseEvent(invokeId);
	message = SnapshotDeviceMessage(invokeId, device);
	CstaSend(message,"SnapShotDevice");


	if(WaitForSingleObject(h,DEFAULT_TIMEOUT)!=-1)
	{
		CAsnNode * result;
		result = (CAsnNode *)GetResponseData(invokeId,(char*)"SnapshotDevice");

		if(result != NULL && result->GetTagNumber() == 2)
		{
			bResult = true;
			if(resultData != NULL)
			{
				if((result = result->GetSpecificSequence(ncUniversal, 16)) != NULL)
				{
					GetSnapshotDeviceResult(result, resultData);
				}
			}
		}else{
			logCsta("Falha SnapshotDevice Ramal: "+string(device));
		}
	}
	DeleteResponseEvent(invokeId);
	delete message;

	}
	catch(...)
	{

	}
	return bResult;
}

bool CCSTA::ClearConnection(int callId, char * device)
{
	bool bResult;
	bResult = false;
	try
	{       unsigned char * message;
	int invokeId;

	HANDLE * h;

	invokeId = MakeInvokeId();
	h = CreateResponseEvent(invokeId);
	message = ClearConnectionMessage(callId, device, invokeId);
	CstaSend(message,"ClearConnection");


	if(WaitForSingleObject(h,DEFAULT_TIMEOUT)!=-1)
	{
		CAsnNode * result;
		result = (CAsnNode *)GetResponseData(invokeId,(char*)"ClearConnection");

		if(result != NULL && result->GetTagNumber() == 2)
		{
			bResult = true;

		}else{
			logCsta("Falha ClearConnection Ramal: "+string(device)+" CallId: "+IntToStr(callId));
		}
	}
	DeleteResponseEvent(invokeId);
	delete message;

	}
	catch(...)
	{

	}
	return bResult;
}

bool CCSTA::AnswerCall(int callId, char * device)
{
	bool bResult;
	bResult = false;
	try
	{       unsigned char * message;
	int invokeId;

	HANDLE * h;

	invokeId = MakeInvokeId();
	h = CreateResponseEvent(invokeId);
	message = AnswerCallMessage(callId, device, invokeId);
	CstaSend(message,"AnswerCall");


	if(WaitForSingleObject(h,DEFAULT_TIMEOUT)!=-1)
	{
		CAsnNode * result;
		result = (CAsnNode *)GetResponseData(invokeId,(char*)"AnswerCall");

		if(result != NULL && result->GetTagNumber() == 2)
		{
			bResult = true;
		}else{
			logCsta("Falha AnswerCall Ramal: "+string(device)+" CallId: "+IntToStr(callId));
		}
	}
	DeleteResponseEvent(invokeId);
	delete message;

	}
	catch(...)
	{

	}
	return bResult;
}

bool CCSTA::AlternateCall(int callId, char * device, int holdCallId, char * holdDevice)
{
	bool bResult;
	bResult = false;
	try
	{       unsigned char * message;
	int invokeId;

	HANDLE * h;

	invokeId = MakeInvokeId();
	h = CreateResponseEvent(invokeId);
	message = AlternateCallMessage(callId, device, holdCallId, holdDevice, invokeId);
	CstaSend(message,"AlternateCall");


	if(WaitForSingleObject(h,DEFAULT_TIMEOUT)!=-1)
	{
		CAsnNode * result;
		result = (CAsnNode *)GetResponseData(invokeId,(char*)"AlternateCall");

		if(result != NULL && result->GetTagNumber() == 2)
		{
			bResult = true;

		}else{
			logCsta("Falha AlternateCall Ramal: "+string(device)+" CallId: "+IntToStr(callId));
		}
	}
	DeleteResponseEvent(invokeId);
	delete message;

	}
	catch(...)
	{

	}
	return bResult;
}

bool CCSTA::TransferCall(int callId, char * device, int holdCallId, char * holdDevice)
{
	bool bResult;
	bResult = false;

	try
	{       unsigned char * message;
	int invokeId;

	HANDLE * h;

	invokeId = MakeInvokeId();
	h = CreateResponseEvent(invokeId);
	message = TransferCallMessage(callId, device, holdCallId, holdDevice, invokeId);
	CstaSend(message,"TransferCall");


	if(WaitForSingleObject(h,DEFAULT_TIMEOUT)!=-1)
	{
		CAsnNode * result;
		result = (CAsnNode *)GetResponseData(invokeId,(char*)"TransferCall");

		if(result != NULL && result->GetTagNumber() == 2)
		{
			bResult = true;

		}else{
			logCsta("Falha TransferCall Ramal: "+string(device)+" CallId: "+IntToStr(callId));
		}
	}
	DeleteResponseEvent(invokeId);
	delete message;

	}
	catch(...)
	{

	}
	return bResult;
}

bool CCSTA::ReconnectCall(int callId, char * device, int holdCallId, char * holdDevice)
{
	bool bResult;
	bResult = false;
	try
	{       unsigned char * message;
	int invokeId;

	HANDLE * h;

	invokeId = MakeInvokeId();
	h = CreateResponseEvent(invokeId);
	message = ReconnectCallMessage(callId, device, holdCallId, holdDevice, invokeId);
	CstaSend(message,"ReconnectCall");


	if(WaitForSingleObject(h,DEFAULT_TIMEOUT)!=-1)
	{
		CAsnNode * result;
		result = (CAsnNode *)GetResponseData(invokeId,(char*)"ReconnectCall");

		if(result != NULL && result->GetTagNumber() == 2)
		{
			bResult = true;

		}else{
			logCsta("Falha ReconnectCall Ramal: "+string(device)+" CallId: "+IntToStr(callId));
		}
	}
	DeleteResponseEvent(invokeId);
	delete message;

	}
	catch(...)
	{

	}
	return bResult;
}





bool CCSTA::RetrieveCall(int callId, char * device)
{
	bool bResult;
	bResult = false;
	try
	{       unsigned char * message;
	int invokeId;

	HANDLE * h;

	invokeId = MakeInvokeId();
	h = CreateResponseEvent(invokeId);
	message = RetrieveCallMessage(callId, device, invokeId);
	CstaSend(message,"RetrieveCall");


	if(WaitForSingleObject(h,DEFAULT_TIMEOUT)!=-1)
	{
		CAsnNode * result;
		result = (CAsnNode *)GetResponseData(invokeId,(char*)"RetrieveCall");

		if(result != NULL && result->GetTagNumber() == 2)
		{
			bResult = true;

		}else{
			logCsta("Falha RetrieveCall Ramal: "+string(device)+" CallId: "+IntToStr(callId));
		}
	}
	DeleteResponseEvent(invokeId);
	delete message;

	}
	catch(...)
	{

	}
	return bResult;
}

bool CCSTA::HoldCall(int callId, char * device)
{
	bool bResult;
	bResult = false;
	try
	{       unsigned char * message;
	int invokeId;

	HANDLE * h;

	invokeId = MakeInvokeId();
	h = CreateResponseEvent(invokeId);
	message = HoldCallMessage(callId, device, invokeId);
	CstaSend(message,"HoldCall");


	if(WaitForSingleObject(h,DEFAULT_TIMEOUT)!=-1)
	{
		CAsnNode * result;
		result = (CAsnNode *)GetResponseData(invokeId,(char*)"HoldCall");

		if(result != NULL && result->GetTagNumber() == 2)
		{
			bResult = true;

		}else{
			logCsta("Falha HoldCall Ramal: "+string(device)+" CallId: "+IntToStr(callId));
		}
	}
	DeleteResponseEvent(invokeId);
	delete message;

	}
	catch(...)
	{

	}
	return bResult;
}

bool CCSTA::ParkCall(char * parking_device, char * parkTo, int callId)
{
	try
	{       unsigned char * message;


	message = ParkCallMessage(callId, MakeInvokeId(), parking_device, parkTo );
	CstaSend(message,"ParkCall");
	delete message;

	}
	catch(...)
	{

	}


	return false;
}

bool CCSTA::GroupPickup(char * requesting_device, char * topickup_device, int callId)
{
	try
	{       unsigned char * message;

	message = GroupPickupMessage(callId, MakeInvokeId(), requesting_device, topickup_device );
	CstaSend(message,"GroupPickup");
	delete message;

	}
	catch(...)
	{

	}

	return false;
}

bool CCSTA::SetDisplay(char * deviceId, char * text)
{
	bool bResult;

	bResult = false;

	try
	{
		unsigned char * message;
		int invokeId;
		HANDLE * h;



		invokeId = MakeInvokeId();
		h = CreateResponseEvent(invokeId);


		message = SetDisplayMessage(deviceId, text, invokeId);
		CstaSend(message,"SetDisplay");


		if(WaitForSingleObject(h,DEFAULT_TIMEOUT)!=-1)
		{
			CAsnNode * result;

			result = (CAsnNode *)GetResponseData(invokeId,(char*)"SetDisplay");

			if(result != NULL && result->GetTagNumber() == 2)
			{
				bResult = true;
				logCsta("SetDisplay executado device:"+string(deviceId)+" text:"+string(text));
			}else
			{
				logCsta("Falha SetDisplay device:"+string(deviceId)+" text:"+string(text));
			}
		}else
		{
			logCsta("Timeout SetDisplay device:"+string(deviceId)+" text:"+string(text));
		}

		DeleteResponseEvent(invokeId);

		delete message;
	}
	catch(...)
	{

	}

	return bResult;
}
bool CCSTA::GenerateDigits(int callId, char * deviceId, char * digits)
{
	bool bResult;

	bResult = false;

	try
	{
		unsigned char * message;
		int invokeId;
		HANDLE * h;



		invokeId = MakeInvokeId();
		h = CreateResponseEvent(invokeId);


		message = GenerateDigitsMessage(callId, deviceId, digits, invokeId);
		CstaSend(message,"GenerateDigits");


		if(WaitForSingleObject(h,DEFAULT_TIMEOUT)!=-1)
		{
			CAsnNode * result;

			result = (CAsnNode *)GetResponseData(invokeId,(char*)"GenerateDigits");

			if(result != NULL && result->GetTagNumber() == 2)
			{
				bResult = true;
			}
		}

		DeleteResponseEvent(invokeId);

		delete message;
	}
	catch(...)
	{

	}

	return bResult;
}

bool CCSTA::DialDigits(int callId, char * deviceId, char * digits)
{
	bool bResult;

	bResult = false;

	try
	{
		unsigned char * message;
		int invokeId;
		HANDLE * h;



		invokeId = MakeInvokeId();
		h = CreateResponseEvent(invokeId);


		message = DialDigitsMessage(callId, deviceId, digits, invokeId);
		CstaSend(message,"DialDigits");


		if(WaitForSingleObject(h,DEFAULT_TIMEOUT)!=-1)
		{
			CAsnNode * result;

			result = (CAsnNode *)GetResponseData(invokeId,(char*)"DialDigits");

			if(result != NULL && result->GetTagNumber() == 2)
			{
				bResult = true;
			}
		}

		DeleteResponseEvent(invokeId);

		delete message;
	}
	catch(...)
	{

	}

	return bResult;
}

bool CCSTA::ConsultationCall(int callId, char * deviceId, char * newDestination)
{
	bool bResult;

	bResult = false;

	try
	{
		unsigned char * message;
		int invokeId;
		HANDLE * h;



		invokeId = MakeInvokeId();
		h = CreateResponseEvent(invokeId);


		message = ConsultationCallMessage(callId, deviceId, newDestination, invokeId);
		CstaSend(message,"ConsultationCall");


		if(WaitForSingleObject(h,DEFAULT_TIMEOUT)!=-1)
		{
			CAsnNode * result;

			result = (CAsnNode *)GetResponseData(invokeId,(char*)"ConsultationCall");

			if(result != NULL && result->GetTagNumber() == 2)
			{
				bResult = true;

			}
		}

		DeleteResponseEvent(invokeId);

		delete message;
	}
	catch(...)
	{

	}

	return bResult;
}

bool CCSTA::SingleStepTransfer(int callId, char * deviceId, char * newDestination)
{
	bool bResult;

	bResult = false;

	try
	{
		unsigned char * message;
		int invokeId;
		HANDLE * h;



		invokeId = MakeInvokeId();
		h = CreateResponseEvent(invokeId);


		message = SingleStepTransferMessage(callId, deviceId, newDestination, invokeId);
		CstaSend(message,"SingleStepTransfer");


		if(WaitForSingleObject(h,DEFAULT_TIMEOUT)!=-1)
		{
			CAsnNode * result;

			result = (CAsnNode *)GetResponseData(invokeId,(char*)"SingleStepTransfer");

			if(result != NULL && result->GetTagNumber() == 2)
			{
				bResult = true;
				logCsta("Transfer�ncia feita. Origem: "+string(deviceId)+" Destino: "+string(newDestination));
			}else
			{
				logCsta("Falha ao Fazer Transfer�ncia. Origem: "+string(deviceId)+" Destino: "+string(newDestination));
			}
		}

		DeleteResponseEvent(invokeId);

		delete message;


	}
	catch(...)
	{


	}

	return bResult;
}

bool CCSTA::DeflectCall(int callId, char * deviceId, char * newDestination)
{
	bool bResult;

	bResult = false;
	try
	{
		unsigned char * message;
		int invokeId;
		HANDLE * h;



		invokeId = MakeInvokeId();
		h = CreateResponseEvent(invokeId);


		message = DeflectCallMessage(callId, deviceId, newDestination, invokeId);
		CstaSend(message,"DeflectCall");

		if(WaitForSingleObject(h,DEFAULT_TIMEOUT)!=-1)
		{
			CAsnNode * result;

			result = (CAsnNode *)GetResponseData(invokeId,(char*)"DeflectCall");

			if(result != NULL && result->GetTagNumber() == 2)
			{
				bResult = true;
				logCsta("Desvio feito. Origem: "+string(deviceId)+" Destino: "+string(newDestination));
			}else
			{
				logCsta("Falha ao Fazer Desvio. Origem: "+string(deviceId)+" Destino: "+string(newDestination));
			}
		}

		DeleteResponseEvent(invokeId);

		delete message;


	}
	catch(...)
	{

	}

	return bResult;
}

bool CCSTA::MakeCall(char * from, char * to, int * callId)
{
	bool bResult;

	bResult = false;
	try
	{       unsigned char * message;
	int invokeId;
	HANDLE * h;

	invokeId = MakeInvokeId();


	h = CreateResponseEvent(invokeId);

	message = MakeCallMessage(from, to, invokeId);
	CstaSend(message,"MakeCall");


	if(WaitForSingleObject(h,DEFAULT_TIMEOUT)!=-1)
	{
		CAsnNode * result;
		CAsnNode * tempNode;


		result = (CAsnNode *)GetResponseData(invokeId,(char*)"MakeCall");

		if(result != NULL && result->GetTagNumber() == 2)
		{
			bResult = true;
			logCsta("Ligacao feita. Origem: "+string(from)+" Destino: "+string(to));
			if((tempNode = result->GetSpecificSequence(ncUniversal, 16))!=NULL)
			{
				if((tempNode = tempNode->GetSpecificSequence(ncUniversal, 16))!=NULL)
				{
					if((tempNode = tempNode->GetSpecificSequence(ncApplication, 11))!=NULL)
					{
						if((tempNode = tempNode->GetSpecificSequence(ncUniversal, 16))!=NULL)
						{
							if((tempNode = tempNode->GetSpecificSequence(ncContext_Specific, 0))!=NULL)
							{
								int tmpCallId = tempNode->GetIntegerData();
								if(callId != NULL)
								{
									*callId =tmpCallId;
								}
							}
						}
					}
				}
			}
		}
		else
		{
			logCsta("Falha ao Fazer ligacao. Origem: "+string(from)+" Destino: "+string(to));
		}
	}

	DeleteResponseEvent(invokeId);

	delete message;


	}
	catch(...)
	{

	}


	return bResult;
}

bool CCSTA::ConferenceCall(int heldCallId, char * heldDeviceId, int activeCallId, char * activeDeviceId)
{
	try
	{
		unsigned char * message;


		message = ConferenceCallMessage(heldCallId, heldDeviceId, activeCallId, activeDeviceId);
		CstaSend(message,"ConferenceCall");
		delete message;


	}
	catch(...)
	{

	}

	return false;
}

void CCSTA::SetHeaderSize(int newHeaderSize)
{
	TotalHeaderSize = newHeaderSize;
	cstaBuffer.SetHeaderSize(newHeaderSize);
}

string CCSTA::logObject(TDeviceID DeviceID, int nivel)
{
	string tempTab;
	string logText;
	for(int t = 0;t<nivel;t++)
	{
		tempTab +="\t";
	}

	logText += tempTab + "DeviceID.DialingNumber = "+DeviceID.DialingNumber+"\n";
	logText += tempTab + "DeviceID.DeviceNumber = "+IntToStr(DeviceID.DeviceNumber)+"\n";
	return logText;
}
string CCSTA::logObject(TConnectionID ConnectionID, int nivel)
{
	string tempTab;
	string logText;
	for(int t = 0;t<nivel;t++)
	{
		tempTab +="\t";
	}
	logText += tempTab + "ConnectionID.CallID = "+IntToStr(ConnectionID.CallID)+"\n";
	logText += tempTab + "ConnectionID.DeviceID\n";
	logText += tempTab + "{\n";
	logText += logObject(ConnectionID.DeviceID, nivel+1);
	logText += tempTab + "}\n";
	return logText;

}
string CCSTA::logObject(TSubjectDeviceID SubjectDeviceID, int nivel)
{
	string tempTab;
	string logText;
	for(int t = 0;t<nivel;t++)
	{
		tempTab +="\t";
	}

	logText += tempTab + "SubjectDeviceID.DialingNumber = "+SubjectDeviceID.DeviceID.DialingNumber+"\n";
	logText += tempTab + "SubjectDeviceID.DeviceNumber = "+IntToStr(SubjectDeviceID.DeviceID.DeviceNumber)+"\n";
	return logText;
}
string CCSTA::logObject(TCallingDeviceID CallingDeviceID, int nivel)
{
	string tempTab;
	string logText;
	for(int t = 0;t<nivel;t++)
	{
		tempTab +="\t";
	}

	logText += tempTab + "CallingDeviceID.DialingNumber = "+CallingDeviceID.DeviceID.DialingNumber+"\n";
	logText += tempTab + "CallingDeviceID.DeviceNumber = "+IntToStr(CallingDeviceID.DeviceID.DeviceNumber)+"\n";
	return logText;
}
string CCSTA::logObject(TCalledDeviceID CalledDeviceID, int nivel)
{
	string tempTab;
	string logText;
	for(int t = 0;t<nivel;t++)
	{
		tempTab +="\t";
	}

	logText += tempTab + "CalledDeviceID.DialingNumber = "+CalledDeviceID.DeviceID.DialingNumber+"\n";
	logText += tempTab + "CalledDeviceID.DeviceNumber = "+IntToStr(CalledDeviceID.DeviceID.DeviceNumber)+"\n";
	return logText;
}
string CCSTA::logObject(TAssociatedCallingDeviceID AssociatedCallingDeviceID, int nivel)
{
	string tempTab;
	string logText;
	for(int t = 0;t<nivel;t++)
	{
		tempTab +="\t";
	}

	logText += tempTab + "AssociatedCallingDeviceID.DialingNumber = "+AssociatedCallingDeviceID.DeviceID.DialingNumber+"\n";
	logText += tempTab + "AssociatedCallingDeviceID.DeviceNumber = "+IntToStr(AssociatedCallingDeviceID.DeviceID.DeviceNumber)+"\n";
	return logText;
}
string CCSTA::logObject(TAssociatedCalledDeviceID AssociatedCalledDeviceID, int nivel)
{
	string tempTab;
	string logText;
	for(int t = 0;t<nivel;t++)
	{
		tempTab +="\t";
	}

	logText += tempTab + "AssociatedCalledDeviceID.DialingNumber = "+AssociatedCalledDeviceID.DeviceID.DialingNumber+"\n";
	logText += tempTab + "AssociatedCalledDeviceID.DeviceNumber = "+IntToStr(AssociatedCalledDeviceID.DeviceID.DeviceNumber)+"\n";
	return logText;
}
string CCSTA::logObject(TNetworkCallingDeviceID NetworkCallingDeviceID, int nivel)
{
	string tempTab;
	string logText;
	for(int t = 0;t<nivel;t++)
	{
		tempTab +="\t";
	}

	logText += tempTab + "NetworkCallingDeviceID.DialingNumber = "+NetworkCallingDeviceID.DeviceID.DialingNumber+"\n";
	logText += tempTab + "NetworkCallingDeviceID.DeviceNumber = "+IntToStr(NetworkCallingDeviceID.DeviceID.DeviceNumber)+"\n";
	return logText;
}
string CCSTA::logObject(TNetworkCalledDeviceID NetworkCalledDeviceID, int nivel)
{
	string tempTab;
	string logText;
	for(int t = 0;t<nivel;t++)
	{
		tempTab +="\t";
	}

	logText += tempTab + "NetworkCalledDeviceID.DialingNumber = "+NetworkCalledDeviceID.DeviceID.DialingNumber+"\n";
	logText += tempTab + "NetworkCalledDeviceID.DeviceNumber = "+IntToStr(NetworkCalledDeviceID.DeviceID.DeviceNumber)+"\n";
	return logText;
}
string CCSTA::logObject(TConnectionListItem ConnectionListItem, int nivel)
{
	string tempTab;
	string logText;
	for(int t = 0;t<nivel;t++)
	{
		tempTab +="\t";
	}
	logText += tempTab + "ConnectionListItem\n";
	logText += tempTab + "{\n";
	logText += tempTab + "\tnewConnection\n";
	logText += tempTab + "\t{\n";
	logText += logObject(ConnectionListItem.newConnection, nivel+2);
	logText += tempTab + "\t}\n";
	logText += tempTab + "\toldConnection\n";
	logText += tempTab + "\t{\n";
	logText += logObject(ConnectionListItem.oldConnection, nivel+2);
	logText += tempTab + "\t}\n";
	logText += tempTab + "\tassociatedNID\n";
	logText += tempTab + "\t{\n";
	logText += logObject(ConnectionListItem.associatedNID, nivel+2);
	logText += tempTab + "\t}\n";
	logText += tempTab + "\tendPoint\n";
	logText += tempTab + "\t{\n";
	logText += logObject(ConnectionListItem.endPoint, nivel+2);
	logText += tempTab + "\t}\n";
	logText += tempTab + "}\n";
	return logText;

}
string CCSTA::logObject(TConnectionList ConnectionList, int nivel)
{
	string tempTab;
	string logText;
	for(int t = 0;t<nivel;t++)
	{
		tempTab +="\t";
	}


	logText += tempTab + "ConnectionList\n";
	logText += tempTab + "{\n";
	for(int listVal = 0;listVal < ConnectionList.Count;listVal++)
	{
		logText += logObject(ConnectionList.Connections[listVal], nivel+1);
	}
	logText += tempTab + "}\n";
	return logText;
}
string CCSTA::logObject(TLocalConnectionState LocalConnectionState, int nivel)
{
	string tempTab;
	string logText;
	for(int t = 0;t<nivel;t++)
	{
		tempTab +="\t";
	}


	logText += tempTab + "(LocalConnectionState = "+IntToStr((int)LocalConnectionState)+")\n";
	return logText;
}
string CCSTA::logObject(TEventCause EventCause, int nivel)
{
	string tempTab;
	string logText;
	for(int t = 0;t<nivel;t++)
	{
		tempTab +="\t";
	}


	logText += tempTab + "(EventCause = "+IntToStr((int)EventCause)+")\n";
	return logText;
}
string CCSTA::IntToStr(int Value)
{
	char cVal[1024]="";
	sprintf(cVal,"%d",Value);
	return string(cVal);
}

string CCSTA::logObject(TAgentState AgentState, int nivel)
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

string CCSTA::logObject(TAgentBusy AgentBusy, int nivel)
{
	string tempTab;
	string logText;


	for(int t = 0;t<nivel;t++)
	{
		tempTab +="\t";
	}

	logText += tempTab + "AgentBusy\n";
	logText += tempTab + "{\n";

	logText += tempTab + "\tAgentDevice\n";
	logText += tempTab + "\t{\n";
	logText += logObject(AgentBusy.AgentDevice, nivel+2);
	logText += tempTab + "\t}\n";
	logText += tempTab + "\tAgentID = "+AgentBusy.AgentID+"\n";
	logText += tempTab + "\tACDGroup\n";
	logText += tempTab + "\t{\n";
	logText += logObject(AgentBusy.ACDGroup, nivel+2);
	logText += tempTab + "\t}\n";

	logText += tempTab + "}\n";

	return logText;
}

string CCSTA::logObject(TAgentNotReady AgentNotReady, int nivel)
{
	string tempTab;
	string logText;


	for(int t = 0;t<nivel;t++)
	{
		tempTab +="\t";
	}

	logText += tempTab + "AgentNotReady\n";
	logText += tempTab + "{\n";

	logText += tempTab + "\tAgentDevice\n";
	logText += tempTab + "\t{\n";
	logText += logObject(AgentNotReady.AgentDevice, nivel+2);
	logText += tempTab + "\t}\n";
	logText += tempTab + "\tAgentID = "+AgentNotReady.AgentID+"\n";
	logText += tempTab + "\tACDGroup\n";
	logText += tempTab + "\t{\n";
	logText += logObject(AgentNotReady.ACDGroup, nivel+2);
	logText += tempTab + "\t}\n";

	logText += tempTab + "}\n";

	return logText;
}

string CCSTA::logObject(TAgentReady AgentReady, int nivel)
{
	string tempTab;
	string logText;


	for(int t = 0;t<nivel;t++)
	{
		tempTab +="\t";
	}

	logText += tempTab + "AgentReady\n";
	logText += tempTab + "{\n";

	logText += tempTab + "\tAgentDevice\n";
	logText += tempTab + "\t{\n";
	logText += logObject(AgentReady.AgentDevice, nivel+2);
	logText += tempTab + "\t}\n";
	logText += tempTab + "\tAgentID = "+AgentReady.AgentID+"\n";
	logText += tempTab + "\tACDGroup\n";
	logText += tempTab + "\t{\n";
	logText += logObject(AgentReady.ACDGroup, nivel+2);
	logText += tempTab + "\t}\n";

	logText += tempTab + "}\n";

	return logText;
}

string CCSTA::logObject(TAgentLoggedOn AgentLoggedOn, int nivel)
{
	string tempTab;
	string logText;


	for(int t = 0;t<nivel;t++)
	{
		tempTab +="\t";
	}

	logText += tempTab + "AgentLoggedOn\n";
	logText += tempTab + "{\n";

	logText += tempTab + "\tAgentDevice\n";
	logText += tempTab + "\t{\n";
	logText += logObject(AgentLoggedOn.AgentDevice, nivel+2);
	logText += tempTab + "\t}\n";
	logText += tempTab + "\tAgentID = "+AgentLoggedOn.AgentID+"\n";
	logText += tempTab + "\tACDGroup\n";
	logText += tempTab + "\t{\n";
	logText += logObject(AgentLoggedOn.ACDGroup, nivel+2);
	logText += tempTab + "\t}\n";
	logText += tempTab + "\tAgentPassword = "+AgentLoggedOn.AgentPassword+"\n";

	logText += tempTab + "}\n";

	return logText;
}

string CCSTA::logObject(TAgentLoggedOff AgentLoggedOff, int nivel)
{
	string tempTab;
	string logText;


	for(int t = 0;t<nivel;t++)
	{
		tempTab +="\t";
	}

	logText += tempTab + "AgentLoggedOff\n";
	logText += tempTab + "{\n";

	logText += tempTab + "\tAgentDevice\n";
	logText += tempTab + "\t{\n";
	logText += logObject(AgentLoggedOff.AgentDevice, nivel+2);
	logText += tempTab + "\t}\n";
	logText += tempTab + "\tAgentID = "+AgentLoggedOff.AgentID+"\n";
	logText += tempTab + "\tACDGroup\n";
	logText += tempTab + "\t{\n";
	logText += logObject(AgentLoggedOff.ACDGroup, nivel+2);
	logText += tempTab + "\t}\n";
	logText += tempTab + "\tAgentPassword = "+AgentLoggedOff.AgentPassword+"\n";

	logText += tempTab + "}\n";

	return logText;
}

string CCSTA::logObject(TAgentWorkingAfterCall AgentWorkingAfterCall, int nivel)
{
	string tempTab;
	string logText;


	for(int t = 0;t<nivel;t++)
	{
		tempTab +="\t";
	}

	logText += tempTab + "AgentWorkingAfterCall\n";
	logText += tempTab + "{\n";

	logText += tempTab + "\tAgentDevice\n";
	logText += tempTab + "\t{\n";
	logText += logObject(AgentWorkingAfterCall.AgentDevice, nivel+2);
	logText += tempTab + "\t}\n";
	logText += tempTab + "\tAgentID = "+AgentWorkingAfterCall.AgentID+"\n";
	logText += tempTab + "\tACDGroup\n";
	logText += tempTab + "\t{\n";
	logText += logObject(AgentWorkingAfterCall.ACDGroup, nivel+2);
	logText += tempTab + "\t}\n";
	logText += tempTab + "\tPendingAgentState = "+IntToStr(AgentWorkingAfterCall.PendingAgentState)+"\n";

	logText += tempTab + "}\n";

	return logText;
}




string CCSTA::logObject(TConferenced Conferenced, int nivel)
{
	string tempTab;
	string logText;
	for(int t = 0;t<nivel;t++)
	{
		tempTab +="\t";
	}

	logText += tempTab + "\nConferenced\n";
	logText += tempTab + "{\n";

	logText += tempTab + "\tPrimaryOldCall\n";
	logText += tempTab + "\t{\n";
	logText += logObject(Conferenced.PrimaryOldCall, nivel+2);
	logText += tempTab + "\t}\n";

	logText += tempTab + "\tSecondaryOldCall\n";
	logText += tempTab + "\t{\n";
	logText += logObject(Conferenced.SecondaryOldCall, nivel+2);
	logText += tempTab + "\t}\n";

	logText += tempTab + "\tConferencingDevice\n";
	logText += tempTab + "\t{\n";
	logText += logObject(Conferenced.ConferencingDevice, nivel+2);
	logText += tempTab + "\t}\n";

	logText += tempTab + "\tAddedParty\n";
	logText += tempTab + "\t{\n";
	logText += logObject(Conferenced.AddedParty, nivel+2);
	logText += tempTab + "\t}\n";

	logText += tempTab + "\tConferenceConnections\n";
	logText += tempTab + "\t{\n";
	logText += logObject(Conferenced.ConferenceConnections, nivel+2);
	logText += tempTab + "\t}\n";

	logText += tempTab + "\tLocalConnectionInfo = " + logObject(Conferenced.LocalConnectionInfo, 0)+"\n";

	logText += tempTab + "\tCause = " + logObject(Conferenced.Cause, 0)+"\n";

	logText += tempTab + "}\n";

	return logText;
}
string CCSTA::logObject(TConnectionCleared ConnectionCleared, int nivel)
{
	string tempTab;
	string logText;
	for(int t = 0;t<nivel;t++)
	{
		tempTab +="\t";
	}

	logText += tempTab + "\nConnectionCleared\n";
	logText += tempTab + "{\n";
	logText += tempTab + "\tDroppedConnection\n";
	logText += tempTab + "\t{\n";
	logText += logObject(ConnectionCleared.DroppedConnection, nivel+2);
	logText += tempTab + "\t}\n";
	logText += tempTab + "\tReleasingDevice\n";
	logText += tempTab + "\t{\n";
	logText += logObject(ConnectionCleared.ReleasingDevice, nivel+2);
	logText += tempTab + "\t}\n";
	logText += tempTab + "\tLocalConnectionInfo = " + logObject(ConnectionCleared.LocalConnectionInfo, 0)+"\n";
	logText += tempTab + "\tCause = " + logObject(ConnectionCleared.Cause, 0)+"\n";
	logText += tempTab + "}\n";

	return logText;
}
string CCSTA::logObject(TDelivered Delivered, int nivel)
{
	string tempTab;
	string logText;
	for(int t = 0;t<nivel;t++)
	{
		tempTab +="\t";
	}


	logText += tempTab + "\nDelivered\n";
	logText += tempTab + "{\n";

	logText += tempTab + "\tConnection\n";
	logText += tempTab + "\t{\n";
	logText += logObject(Delivered.Connection, nivel+2);
	logText += tempTab + "\t}\n";

	logText += tempTab + "\tAlertingDevice\n";
	logText += tempTab + "\t{\n";
	logText += logObject(Delivered.AlertingDevice, nivel+2);
	logText += tempTab + "\t}\n";

	logText += tempTab + "\tCallingDevice\n";
	logText += tempTab + "\t{\n";
	logText += logObject(Delivered.CallingDevice, nivel+2);
	logText += tempTab + "\t}\n";

	logText += tempTab + "\tCalledDevice\n";
	logText += tempTab + "\t{\n";
	logText += logObject(Delivered.CalledDevice, nivel+2);
	logText += tempTab + "\t}\n";

	logText += tempTab + "\tOriginatingNIDConnection\n";
	logText += tempTab + "\t{\n";
	logText += logObject(Delivered.OriginatingNIDConnection, nivel+2);
	logText += tempTab + "\t}\n";

	logText += tempTab + "\tLocalConnectionInfo = " + logObject(Delivered.LocalConnectionInfo, 0)+"\n";

	logText += tempTab + "\tCause = " + logObject(Delivered.Cause, 0)+"\n";

	logText += tempTab + "\tNetworkCallingDeviceID\n";
	logText += tempTab + "\t{\n";
	logText += logObject(Delivered.NetworkCallingDeviceID, nivel+2);
	logText += tempTab + "\t}\n";

	logText += tempTab + "\tNetworkCalledDeviceID\n";
	logText += tempTab + "\t{\n";
	logText += logObject(Delivered.NetworkCalledDeviceID, nivel+2);
	logText += tempTab + "\t}\n";

	logText += tempTab + "\tAssociatedCallingDeviceID\n";
	logText += tempTab + "\t{\n";
	logText += logObject(Delivered.AssociatedCallingDeviceID, nivel+2);
	logText += tempTab + "\t}\n";

	logText += tempTab + "\tAssociatedCalledDeviceID\n";
	logText += tempTab + "\t{\n";
	logText += logObject(Delivered.AssociatedCalledDeviceID, nivel+2);
	logText += tempTab + "\t}\n";

	logText += tempTab + "}\n";

	return logText;
}
string CCSTA::logObject(TDiverted Diverted, int nivel)
{
	string tempTab;
	string logText;
	for(int t = 0;t<nivel;t++)
	{
		tempTab +="\t";
	}

	logText += tempTab + "\nDiverted\n";
	logText += tempTab + "{\n";

	logText += tempTab + "\tConnection\n";
	logText += tempTab + "\t{\n";
	logText += logObject(Diverted.Connection, nivel+2);
	logText += tempTab + "\t}\n";

	logText += tempTab + "\tDivertingDevice\n";
	logText += tempTab + "\t{\n";
	logText += logObject(Diverted.DivertingDevice, nivel+2);
	logText += tempTab + "\t}\n";

	logText += tempTab + "\tNewDestination\n";
	logText += tempTab + "\t{\n";
	logText += logObject(Diverted.NewDestination, nivel+2);
	logText += tempTab + "\t}\n";

	logText += tempTab + "\tCallingDevice\n";
	logText += tempTab + "\t{\n";
	logText += logObject(Diverted.CallingDevice, nivel+2);
	logText += tempTab + "\t}\n";

	logText += tempTab + "\tCalledDevice\n";
	logText += tempTab + "\t{\n";
	logText += logObject(Diverted.CalledDevice, nivel+2);
	logText += tempTab + "\t}\n";

	logText += tempTab + "\tLocalConnectionInfo = " + logObject(Diverted.LocalConnectionInfo, 0)+"\n";

	logText += tempTab + "\tCause = " + logObject(Diverted.Cause, 0)+"\n";

	logText += tempTab + "\tNetworkCallingDeviceID\n";
	logText += tempTab + "\t{\n";
	logText += logObject(Diverted.NetworkCallingDeviceID, nivel+2);
	logText += tempTab + "\t}\n";

	logText += tempTab + "\tNetworkCalledDeviceID\n";
	logText += tempTab + "\t{\n";
	logText += logObject(Diverted.NetworkCalledDeviceID, nivel+2);
	logText += tempTab + "\t}\n";

	logText += tempTab + "\tAssociatedCallingDeviceID\n";
	logText += tempTab + "\t{\n";
	logText += logObject(Diverted.AssociatedCallingDeviceID, nivel+2);
	logText += tempTab + "\t}\n";

	logText += tempTab + "\tAssociatedCalledDeviceID\n";
	logText += tempTab + "\t{\n";
	logText += logObject(Diverted.AssociatedCalledDeviceID, nivel+2);
	logText += tempTab + "\t}\n";

	logText += tempTab + "}\n";

	return logText;
}
string CCSTA::logObject(TEstablished Established, int nivel)
{
	string tempTab;
	string logText;
	for(int t = 0;t<nivel;t++)
	{
		tempTab +="\t";
	}

	logText += tempTab + "\nEstablished\n";
	logText += tempTab + "{\n";

	logText += tempTab + "\tEstablishedConnection\n";
	logText += tempTab + "\t{\n";
	logText += logObject(Established.EstablishedConnection, nivel+2);
	logText += tempTab + "\t}\n";

	logText += tempTab + "\tCallingDevice\n";
	logText += tempTab + "\t{\n";
	logText += logObject(Established.CallingDevice, nivel+2);
	logText += tempTab + "\t}\n";

	logText += tempTab + "\tCalledDevice\n";
	logText += tempTab + "\t{\n";
	logText += logObject(Established.CalledDevice, nivel+2);
	logText += tempTab + "\t}\n";

	logText += tempTab + "\tAnsweringDevice\n";
	logText += tempTab + "\t{\n";
	logText += logObject(Established.AnsweringDevice, nivel+2);
	logText += tempTab + "\t}\n";

	logText += tempTab + "\tOriginatingNIDConnection\n";
	logText += tempTab + "\t{\n";
	logText += logObject(Established.OriginatingNIDConnection, nivel+2);
	logText += tempTab + "\t}\n";

	logText += tempTab + "\tLocalConnectionInfo = " + logObject(Established.LocalConnectionInfo, 0)+"\n";

	logText += tempTab + "\tCause = " + logObject(Established.Cause, 0)+"\n";

	logText += tempTab + "\tNetworkCallingDeviceID\n";
	logText += tempTab + "\t{\n";
	logText += logObject(Established.NetworkCallingDeviceID, nivel+2);
	logText += tempTab + "\t}\n";

	logText += tempTab + "\tNetworkCalledDeviceID\n";
	logText += tempTab + "\t{\n";
	logText += logObject(Established.NetworkCalledDeviceID, nivel+2);
	logText += tempTab + "\t}\n";

	logText += tempTab + "\tAssociatedCallingDeviceID\n";
	logText += tempTab + "\t{\n";
	logText += logObject(Established.AssociatedCallingDeviceID, nivel+2);
	logText += tempTab + "\t}\n";

	logText += tempTab + "\tAssociatedCalledDeviceID\n";
	logText += tempTab + "\t{\n";
	logText += logObject(Established.AssociatedCalledDeviceID, nivel+2);
	logText += tempTab + "\t}\n";

	logText += tempTab + "}\n";

	return logText;
}
string CCSTA::logObject(TFailed Failed, int nivel)
{
	string tempTab;
	string logText;
	for(int t = 0;t<nivel;t++)
	{
		tempTab +="\t";
	}

	logText += tempTab + "\nFailed\n";
	logText += tempTab + "{\n";
	logText += tempTab + "\tFailedConnection\n";
	logText += tempTab + "\t{\n";
	logText += logObject(Failed.FailedConnection, nivel+2);
	logText += tempTab + "\t}\n";
	logText += tempTab + "\tFailingDevice\n";
	logText += tempTab + "\t{\n";
	logText += logObject(Failed.FailingDevice, nivel+2);
	logText += tempTab + "\t}\n";
	logText += tempTab + "\tCallingDevice\n";
	logText += tempTab + "\t{\n";
	logText += logObject(Failed.CallingDevice, nivel+2);
	logText += tempTab + "\t}\n";
	logText += tempTab + "\tCalledDevice\n";
	logText += tempTab + "\t{\n";
	logText += logObject(Failed.CalledDevice, nivel+2);
	logText += tempTab + "\t}\n";
	logText += tempTab + "\tOriginatingNIDConnection\n";
	logText += tempTab + "\t{\n";
	logText += logObject(Failed.OriginatingNIDConnection, nivel+2);
	logText += tempTab + "\t}\n";
	logText += tempTab + "\tLocalConnectionInfo = " + logObject(Failed.LocalConnectionInfo, 0)+"\n";
	logText += tempTab + "\tCause = " + logObject(Failed.Cause, 0)+"\n";
	logText += tempTab + "\tNetworkCallingDeviceID\n";
	logText += tempTab + "\t{\n";
	logText += logObject(Failed.NetworkCallingDeviceID, nivel+2);
	logText += tempTab + "\t}\n";
	logText += tempTab + "\tNetworkCalledDeviceID\n";
	logText += tempTab + "\t{\n";
	logText += logObject(Failed.NetworkCalledDeviceID, nivel+2);
	logText += tempTab + "\t}\n";
	logText += tempTab + "\tAssociatedCallingDeviceID\n";
	logText += tempTab + "\t{\n";
	logText += logObject(Failed.AssociatedCallingDeviceID, nivel+2);
	logText += tempTab + "\t}\n";
	logText += tempTab + "\tAssociatedCalledDeviceID\n";
	logText += tempTab + "\t{\n";
	logText += logObject(Failed.AssociatedCalledDeviceID, nivel+2);
	logText += tempTab + "\t}\n";
	logText += tempTab + "}\n";

	return logText;
}
string CCSTA::logObject(THeld Held, int nivel)
{
	string tempTab;
	string logText;
	for(int t = 0;t<nivel;t++)
	{
		tempTab +="\t";
	}

	logText += tempTab + "\nHeld\n";
	logText += tempTab + "{\n";

	logText += tempTab + "\tHeldConnection\n";
	logText += tempTab + "\t{\n";
	logText += logObject(Held.HeldConnection, nivel+2);
	logText += tempTab + "\t}\n";

	logText += tempTab + "\tHoldingDevice\n";
	logText += tempTab + "\t{\n";
	logText += logObject(Held.HoldingDevice, nivel+2);
	logText += tempTab + "\t}\n";

	logText += tempTab + "\tLocalConnectionInfo = " + logObject(Held.LocalConnectionInfo, 0)+"\n";

	logText += tempTab + "\tCause = " + logObject(Held.Cause, 0)+"\n";

	logText += tempTab + "}\n";

	return logText;
}
string CCSTA::logObject(TNetworkReached NetworkReached, int nivel)
{
	string tempTab;
	string logText;
	for(int t = 0;t<nivel;t++)
	{
		tempTab +="\t";
	}

	logText += tempTab + "\nNetworkReached\n";
	logText += tempTab + "{\n";

	logText += tempTab + "\tOutboundConnection\n";
	logText += tempTab + "\t{\n";
	logText += logObject(NetworkReached.OutboundConnection, nivel+2);
	logText += tempTab + "\t}\n";

	logText += tempTab + "\tNetworkInterfaceUsed\n";
	logText += tempTab + "\t{\n";
	logText += logObject(NetworkReached.NetworkInterfaceUsed, nivel+2);
	logText += tempTab + "\t}\n";

	logText += tempTab + "\tCallingDevice\n";
	logText += tempTab + "\t{\n";
	logText += logObject(NetworkReached.CallingDevice, nivel+2);
	logText += tempTab + "\t}\n";

	logText += tempTab + "\tCalledDevice\n";
	logText += tempTab + "\t{\n";
	logText += logObject(NetworkReached.CalledDevice, nivel+2);
	logText += tempTab + "\t}\n";

	logText += tempTab + "\tOriginatingNIDConnection\n";
	logText += tempTab + "\t{\n";
	logText += logObject(NetworkReached.OriginatingNIDConnection, nivel+2);
	logText += tempTab + "\t}\n";

	logText += tempTab + "\tLocalConnectionInfo = " + logObject(NetworkReached.LocalConnectionInfo, 0)+"\n";

	logText += tempTab + "\tCause = " + logObject(NetworkReached.Cause, 0)+"\n";

	logText += tempTab + "\tNetworkCallingDeviceID\n";
	logText += tempTab + "\t{\n";
	logText += logObject(NetworkReached.NetworkCallingDeviceID, nivel+2);
	logText += tempTab + "\t}\n";

	logText += tempTab + "\tNetworkCalledDeviceID\n";
	logText += tempTab + "\t{\n";
	logText += logObject(NetworkReached.NetworkCalledDeviceID, nivel+2);
	logText += tempTab + "\t}\n";

	logText += tempTab + "\tAssociatedCallingDeviceID\n";
	logText += tempTab + "\t{\n";
	logText += logObject(NetworkReached.AssociatedCallingDeviceID, nivel+2);
	logText += tempTab + "\t}\n";

	logText += tempTab + "}\n";

	return logText;
}
string CCSTA::logObject(TOriginated Originated, int nivel)
{
	string tempTab;
	string logText;
	for(int t = 0;t<nivel;t++)
	{
		tempTab +="\t";
	}

	logText += tempTab + "\nOriginated\n";
	logText += tempTab + "{\n";

	logText += tempTab + "\tOriginatedConnection\n";
	logText += tempTab + "\t{\n";
	logText += logObject(Originated.OriginatedConnection, nivel+2);
	logText += tempTab + "\t}\n";

	logText += tempTab + "\tCallingDevice\n";
	logText += tempTab + "\t{\n";
	logText += logObject(Originated.CallingDevice, nivel+2);
	logText += tempTab + "\t}\n";

	logText += tempTab + "\tCalledDevice\n";
	logText += tempTab + "\t{\n";
	logText += logObject(Originated.CalledDevice, nivel+2);
	logText += tempTab + "\t}\n";

	logText += tempTab + "\tLocalConnectionInfo = " + logObject(Originated.LocalConnectionInfo, 0)+"\n";

	logText += tempTab + "\tCause = " + logObject(Originated.Cause, 0)+"\n";

	logText += tempTab + "\tNetworkCallingDeviceID\n";
	logText += tempTab + "\t{\n";
	logText += logObject(Originated.NetworkCallingDeviceID, nivel+2);
	logText += tempTab + "\t}\n";

	logText += tempTab + "\tNetworkCalledDeviceID\n";
	logText += tempTab + "\t{\n";
	logText += logObject(Originated.NetworkCalledDeviceID, nivel+2);
	logText += tempTab + "\t}\n";

	logText += tempTab + "\tAssociatedCallingDeviceID\n";
	logText += tempTab + "\t{\n";
	logText += logObject(Originated.AssociatedCallingDeviceID, nivel+2);
	logText += tempTab + "\t}\n";

	logText += tempTab + "\tAssociatedCalledDeviceID\n";
	logText += tempTab + "\t{\n";
	logText += logObject(Originated.AssociatedCalledDeviceID, nivel+2);
	logText += tempTab + "\t}\n";

	logText += tempTab + "}\n";

	return logText;
}
string CCSTA::logObject(TQueued Queued, int nivel)
{
	string tempTab;
	string logText;
	for(int t = 0;t<nivel;t++)
	{
		tempTab +="\t";
	}

	logText += tempTab + "\nQueued\n";
	logText += tempTab + "{\n";

	logText += tempTab + "\tQueuedConnection\n";
	logText += tempTab + "\t{\n";
	logText += logObject(Queued.QueuedConnection, nivel+2);
	logText += tempTab + "\t}\n";

	logText += tempTab + "\tQueue\n";
	logText += tempTab + "\t{\n";
	logText += logObject(Queued.Queue, nivel+2);
	logText += tempTab + "\t}\n";

	logText += tempTab + "\tCallingDevice\n";
	logText += tempTab + "\t{\n";
	logText += logObject(Queued.CallingDevice, nivel+2);
	logText += tempTab + "\t}\n";

	logText += tempTab + "\tCalledDevice\n";
	logText += tempTab + "\t{\n";
	logText += logObject(Queued.CalledDevice, nivel+2);
	logText += tempTab + "\t}\n";

	logText += tempTab + "\tNumberQueued = ("+IntToStr(Queued.NumberQueued)+")\n";
	logText += tempTab + "\tCallsInFront = ("+IntToStr(Queued.CallsInFront)+")\n";

	logText += tempTab + "\tLocalConnectionInfo = " + logObject(Queued.LocalConnectionInfo, 0)+"\n";

	logText += tempTab + "\tCause = " + logObject(Queued.Cause, 0)+"\n";

	logText += tempTab + "\tNetworkCallingDeviceID\n";
	logText += tempTab + "\t{\n";
	logText += logObject(Queued.NetworkCallingDeviceID, nivel+2);
	logText += tempTab + "\t}\n";

	logText += tempTab + "\tNetworkCalledDeviceID\n";
	logText += tempTab + "\t{\n";
	logText += logObject(Queued.NetworkCalledDeviceID, nivel+2);
	logText += tempTab + "\t}\n";

	logText += tempTab + "\tAssociatedCallingDeviceID\n";
	logText += tempTab + "\t{\n";
	logText += logObject(Queued.AssociatedCallingDeviceID, nivel+2);
	logText += tempTab + "\t}\n";

	logText += tempTab + "\tAssociatedCalledDeviceID\n";
	logText += tempTab + "\t{\n";
	logText += logObject(Queued.AssociatedCalledDeviceID, nivel+2);
	logText += tempTab + "\t}\n";

	logText += tempTab + "}\n";

	return logText;
}
string CCSTA::logObject(TRetrieved Retrieved, int nivel)
{
	string tempTab;
	string logText;
	for(int t = 0;t<nivel;t++)
	{
		tempTab +="\t";
	}

	logText += tempTab + "\nRetrieved\n";
	logText += tempTab + "{\n";

	logText += tempTab + "\tRetrievedConnection\n";
	logText += tempTab + "\t{\n";
	logText += logObject(Retrieved.RetrievedConnection, nivel+2);
	logText += tempTab + "\t}\n";

	logText += tempTab + "\tRetrievingDevice\n";
	logText += tempTab + "\t{\n";
	logText += logObject(Retrieved.RetrievingDevice, nivel+2);
	logText += tempTab + "\t}\n";

	logText += tempTab + "\tLocalConnectionInfo = " + logObject(Retrieved.LocalConnectionInfo, 0)+"\n";

	logText += tempTab + "\tCause = " + logObject(Retrieved.Cause, 0)+"\n";
	logText += tempTab + "}\n";
	return logText;
}
string CCSTA::logObject(TServiceInitiated ServiceInitiated, int nivel)
{
	string tempTab;
	string logText;
	for(int t = 0;t<nivel;t++)
	{
		tempTab +="\t";
	}

	logText += tempTab + "\nServiceInitiated\n";
	logText += tempTab + "{\n";

	logText += tempTab + "\tInitiatedConnection\n";
	logText += tempTab + "\t{\n";
	logText += logObject(ServiceInitiated.InitiatedConnection, nivel+2);
	logText += tempTab + "\t}\n";

	logText += tempTab + "\tInitiatingDevice\n";
	logText += tempTab + "\t{\n";
	logText += logObject(ServiceInitiated.InitiatingDevice, nivel+2);
	logText += tempTab + "\t}\n";

	logText += tempTab + "\tLocalConnectionInfo = " + logObject(ServiceInitiated.LocalConnectionInfo, 0)+"\n";

	logText += tempTab + "\tCause = " + logObject(ServiceInitiated.Cause, 0)+"\n";

	logText += tempTab + "\tNetworkCallingDeviceID\n";
	logText += tempTab + "\t{\n";
	logText += logObject(ServiceInitiated.NetworkCallingDeviceID, nivel+2);
	logText += tempTab + "\t}\n";

	logText += tempTab + "\tNetworkCalledDeviceID\n";
	logText += tempTab + "\t{\n";
	logText += logObject(ServiceInitiated.NetworkCalledDeviceID, nivel+2);
	logText += tempTab + "\t}\n";

	logText += tempTab + "\tAssociatedCallingDeviceID\n";
	logText += tempTab + "\t{\n";
	logText += logObject(ServiceInitiated.AssociatedCallingDeviceID, nivel+2);
	logText += tempTab + "\t}\n";

	logText += tempTab + "\tAssociatedCalledDeviceID\n";
	logText += tempTab + "\t{\n";
	logText += logObject(ServiceInitiated.AssociatedCalledDeviceID, nivel+2);
	logText += tempTab + "\t}\n";

	logText += tempTab + "}\n";

	return logText;
}
string CCSTA::logObject(TTransferred Transferred, int nivel)
{
	string tempTab;
	string logText;
	for(int t = 0;t<nivel;t++)
	{
		tempTab +="\t";
	}

	logText += tempTab + "\nTransferred\n";
	logText += tempTab + "{\n";

	logText += tempTab + "\tPrimaryOldCall\n";
	logText += tempTab + "\t{\n";
	logText += logObject(Transferred.PrimaryOldCall, nivel+2);
	logText += tempTab + "\t}\n";

	logText += tempTab + "\tSecondaryOldCall\n";
	logText += tempTab + "\t{\n";
	logText += logObject(Transferred.SecondaryOldCall, nivel+2);
	logText += tempTab + "\t}\n";

	logText += tempTab + "\tTransferringDevice\n";
	logText += tempTab + "\t{\n";
	logText += logObject(Transferred.TransferringDevice, nivel+2);
	logText += tempTab + "\t}\n";

	logText += tempTab + "\tTransferredToDevice\n";
	logText += tempTab + "\t{\n";
	logText += logObject(Transferred.TransferredToDevice, nivel+2);
	logText += tempTab + "\t}\n";

	logText += tempTab + "\tTransferredConnections\n";
	logText += tempTab + "\t{\n";
	logText += logObject(Transferred.TransferredConnections, nivel+2);
	logText += tempTab + "\t}\n";

	logText += tempTab + "\tLocalConnectionInfo = " + logObject(Transferred.LocalConnectionInfo, 0)+"\n";

	logText += tempTab + "\tCause = " + logObject(Transferred.Cause, 0)+"\n";

	logText += tempTab + "}\n";

	return logText;

}

void CCSTA::RegisterDisconnectHandler(void (* callback)(void *), void * arg)
{
	eventDispatcher->RegisterDisconnectHandler(callback, arg);
}

void CCSTA::RegisterBeatHandler(void (* callback)(void *), void * arg)
{
	eventDispatcher->RegisterBeatHandler(callback, arg);
}

void CCSTA::RegisterCallControlHandler(void (* callback)(TCallControlEventType, TCallControlEvent*, void *), void * arg)
{
	eventDispatcher->RegisterCallControlHandler(callback, arg);
}

void CCSTA::RegisterLogicalDeviceHandler(void (* callback)(TLogicalDeviceEventType, TLogicalDeviceEvent *, void *), void * arg)
{
	eventDispatcher->RegisterLogicalDeviceHandler(callback, arg);
}

void CCSTA::RegisterPhysicalDeviceHandler(void (* callback)(TPhysicalDeviceEventType, TPhysicalDeviceEvent*, void *), void * arg)
{
	eventDispatcher->RegisterPhysicalDeviceHandler(callback, arg);
}

void CCSTA::RegisterSnapshotDeviceDataHandler(void (* callback)(TSnapshotDeviceResult *, void *), void * arg)
{
	eventDispatcher->RegisterSnapshotDeviceDataHandler(callback, arg);
}


void TResponseData::SetHandle(string Arg)
{
	handle = CreateEvent(Arg);
}

TResponseData::TResponseData()
{
	InvokeId = -1;
	TimeOut = 0;
	ptrData = NULL;
	iHandle = 0;
	CallbackSet = false;
	handle = NULL;
}

TResponseData::~TResponseData()
{
}


string LCIText(TLocalConnectionState lcs)
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



/* ---------- TCSTABuffer ---------- */

TCSTABuffer::TCSTABuffer(unsigned int headerSize) :
    		mBufferSize(0),
    		mHeaderSize(headerSize)
{
	bGlogs = false;
	sPathlogs = "";
}
TCSTABuffer::~TCSTABuffer()
{
	Clear();

}
void TCSTABuffer::loghex(unsigned char * data, int size, string description)
{
	string log;

	try{
		for(int i = 0;i<size;i++)
		{
			char bChar[8]="";
			sprintf(bChar," %.2X", data[i]);
			log+=bChar;
		}

	}catch(...)
	{
	}

	try
	{
		if(bGlogs)
		{
			FILE * arq = fopen(string(sPathlogs+"BufferHex"+DateToStr(time(NULL))+".txt").c_str(),"a");
			if ( arq != NULL)
			{
				log = DateToStr(time(NULL)) +" "+ TimeToStr(time(NULL)) +" "+log+"\r\n";
				fputs(log.c_str(),arq);
				fclose(arq);
			}
		}


	}
	catch(...)
	{
	}
}


void TCSTABuffer::Add(unsigned char * pBuffer, int dSize)
{
	bool bOk = true;
	int msgSize;
	TCSTAMessage * ptrMessage;
	memcpy(&mBuffer[mBufferSize], pBuffer, dSize);
	mBufferSize += dSize;

	loghex(pBuffer, dSize, "AddBuffer: ");
	while(bOk)
	{
		if(mBufferSize >= mHeaderSize)
		{
			msgSize = mBuffer[mHeaderSize-2] & 0x7F;
			msgSize <<= 8;
			msgSize |= mBuffer[mHeaderSize-1];

			if(mBufferSize >= (msgSize+mHeaderSize))
			{
				ptrMessage = new TCSTAMessage;
				ptrMessage->msgSize = msgSize+mHeaderSize;
				memcpy(ptrMessage->msgData, mBuffer, ptrMessage->msgSize);
				messageList.push_back(ptrMessage);
				memcpy(mBuffer, &mBuffer[ptrMessage->msgSize], mBufferSize-ptrMessage->msgSize);
				mBufferSize -= ptrMessage->msgSize;
			}
			else
			{
				bOk = false;
			}
		}
		else
		{
			bOk = false;
		}
	}
}
void TCSTABuffer::SetHeaderSize(unsigned int headerSize)
{
	mHeaderSize = headerSize;
}

unsigned int TCSTABuffer::GetMessage(unsigned char * pBuffer, int dMaxSize)
{
	unsigned int result;
	TCSTAMessage * ptrMessage;
	result = 0;

	if(messageList.size())
	{
		ptrMessage = (TCSTAMessage *)messageList[0];
		messageList.erase(messageList.begin());
		if(ptrMessage->msgSize <= dMaxSize)
		{

			memcpy(pBuffer, ptrMessage->msgData, ptrMessage->msgSize);
			result = ptrMessage->msgSize;
		}
		delete ptrMessage;
	}

	return result;
}

void TCSTABuffer::Clear()
{
	TCSTAMessage * ptrMessage;

	while(messageList.size())
	{
		ptrMessage = (TCSTAMessage *)messageList[0];
		messageList.erase(messageList.begin());
		delete ptrMessage;
	}
	if(mBufferSize)
	{
		delete mBuffer;
	}
}
/* ---------- TCSTABuffer ---------- */

void CCSTA::SetLogFolder(char * folder)
{
	logFolder = string(folder);
}

void CCSTA::SendCallControlEvent(TCallControlEventType EventType, TCallControlEvent * Event)
{
	eventDispatcher->AddCallControlEvent(EventType, Event);
}
void CCSTA::SendLogicalDeviceEvent(TLogicalDeviceEventType EventType, TLogicalDeviceEvent * Event)
{
	eventDispatcher->AddLogicalDeviceEvent(EventType, Event);
}
void CCSTA::SendPhysicalDeviceEvent(TPhysicalDeviceEventType EventType, TPhysicalDeviceEvent * Event)
{
	eventDispatcher->AddPhysicalDeviceEvent(EventType, Event);
}
void CCSTA::SendSnapshotDeviceDataEvent(TSnapshotDeviceResult * snapshotData)
{
	eventDispatcher->AddSnapshotDeviceDataEvent(snapshotData);
}
void CCSTA::SendDisconnectEvent()
{
	eventDispatcher->AddDisconnectEvent();
}

void CCSTA::SendBeatEvent()
{
	eventDispatcher->AddBeatEvent();
}

void CCSTA::CstaDisconnect()
{
	cstaConnected = false;
	logCsta("CstaDisconnect");
	shutdown(handle_socket,SHUT_RDWR);
	close(handle_socket);
}

int CCSTA::GetHeaderSize()
{
	return TotalHeaderSize;
}
