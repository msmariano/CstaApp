#include <iostream>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <semaphore.h>
#include <vector>
//#include "../include/utils.h"
#define MAX_TEXT 1024

using namespace std;


class CServerMensChildren
{
public:
        void (* cbRead)(char *,void*,void*);
        void * object;
        string sClassName;
        CServerMensChildren()
        {
                fdCli = 0;
                hCliThread = 0;
                hReadThread = 0;
                sizeatual = 0;
                sem_init(&semaBuffer, 0, 1);
                bContinue = true;
                cbRead = NULL;
                object = NULL;
                sClassName = "CServerMensChildren";
                Parameter1 = NULL;
                sRecv="";
        }
        ~CServerMensChildren()
        {
        	//cout << "Cliente desconectado!!!" << endl;
        }
        string IntToStr(int iArg)
        {
                char c[128]="";
                sprintf(c,"%d",iArg);
                return string(c);
        }
        void SendMens(string sMens)
        {
                send(fdCli,sMens.c_str(),sMens.size(),0);
        }
        static void * thRead(void*arg)
        {
                CServerMensChildren * ptrChildren = (CServerMensChildren*)arg;
                while(ptrChildren->bContinue)
                {
                        sem_wait(&ptrChildren->semaBuffer);
                        if(ptrChildren->sizeatual != (int)ptrChildren->sRecv.size())
                        {
                                if(ptrChildren->sRecv.size()>MAX_TEXT)
                                {
                                        ptrChildren->sRecv = "";
                                }
                                else //Verifica fim de linha;
                                {
                                        try
                                        {
                                                if(ptrChildren->sRecv.size()>=2)
                                                {
                                                        if(ptrChildren->sRecv.substr(ptrChildren->sRecv.size()-2,2) == "\r\n")
                                                        {
                                                                //cout << ptrChildren->sRecv;
                                                                if(ptrChildren->cbRead != NULL)
                                                                {
                                                                		CSplit sp;
                                                                		string  sSockOp = ptrChildren->sRecv;
                                                                		sp.Parser(sSockOp,";","sockop");
                                                                		if(sp[0] == "close")
                                                                		{
                                                                			string sMens = "Close Socket";
                                                                			send(ptrChildren->fdCli,sMens.c_str(),sMens.size(),0);
                                                                			usleep(100);
                                                                			shutdown(ptrChildren->fdCli,SHUT_RDWR);
                                                                			//close(ptrChildren->fdCli);
                                                                			//send(ptrChildren->fdCli,"\r\n",2,0);
                                                                		}
                                                                		else
                                                                		{
                                                                			ptrChildren->sClassName = "CServerMensChildren";
                                                                			ptrChildren->cbRead((char*)ptrChildren->sRecv.c_str(),ptrChildren,ptrChildren->object);
                                                                		}
                                                                }
                                                                ptrChildren->sRecv = "";

                                                        }
                                                }
                                        }
                                        catch(exception &e)
                                        {
                                                e.what();
                                                ptrChildren->bContinue = false;
                                        }
                                }

                                ptrChildren->sizeatual = ptrChildren->sRecv.size();
                        }
                        sem_post(&ptrChildren->semaBuffer);
                        usleep(100000);
                }
                //cout << "Cliente desconectou!" << endl;
                pthread_exit(0);
        }
        static void * thRun(void*arg)
        {
                CServerMensChildren * ptrChildren = (CServerMensChildren*)arg;
                string sMens = "Connected ["+ptrChildren->IntToStr(ptrChildren->fdCli)+"]\r\n";
                send(ptrChildren->fdCli,sMens.c_str(),sMens.size(),0);
                //char chBuff[1024]="";
                //memset(chBuff,0,1024);
                //recv(ptrChildren->fdCli,chBuff,1024,0);
                while(ptrChildren->bContinue)
                {
                        char buff[1024]="";
                        memset(buff,0,1024);

                        if(recv(ptrChildren->fdCli,buff,1024,0) > 0)
                        {
                                sem_wait(&ptrChildren->semaBuffer);
                                ptrChildren->sRecv += string(buff);
                                sem_post(&ptrChildren->semaBuffer);
                                continue;
                        }
                        else
                        {
                        		ptrChildren->log("Closet conection["+ptrChildren->IntToStr(ptrChildren->fdCli)+"]\r\n");
                                ptrChildren->bContinue = false;
                        }
                        usleep(1000);
                }

                ptrChildren->fdCli = -1;
                //cout << "Cliente desconectando. Deletando Filho!!!" << endl;
                //delete ptrChildren;
                pthread_exit(0);
        }
        void log(string logData)
        {
        	FILE * arq = fopen(string(sPathlogs+"logServidor"+DateToStr(time(NULL))+".txt").c_str(),"a");
			if ( arq != NULL)
			{
				string log = DateToStr(time(NULL)) +" "+ TimeToStr(time(NULL)) +" "+logData+"\r\n";
				fputs(log.c_str(),arq);
				fclose(arq);
			}
        }
        void Go()
        {
        	int rc = 0;
        	rc = pthread_create(&hCliThread,0,thRun,this);
        	if (rc) this->log("Erro thread Run");
        	else pthread_detach(hCliThread);

            rc = pthread_create(&hReadThread,0,thRead,this);
            if (rc) this->log("Erro thread Read");
            else pthread_detach(hReadThread);
        }
        int fdCli;
        void * Parameter1;
        string sPathlogs;
private:
        pthread_t hCliThread;
        pthread_t hReadThread;
        string sRecv;
        int sizeatual;
        sem_t semaBuffer;
        bool bContinue;
};






class CServerMensParent
{
public:
        void (* cbRead)(char*,void*,void*);
        void * object;
        vector<CServerMensChildren*> Connections;
        CServerMensParent()
        {
                datach= (unsigned char*)new char[1024];
                memset(datach,0,1024);
                fdSockets = new int;
                fdServer = -1;
                hServerThread = 0;
                cbRead = NULL;
                object = NULL;
                Parameter1 = NULL;
        }
        ~CServerMensParent()
        {

        }

        void log(string logData)
		{
			FILE * arq = fopen(string(sPathlogs+"logServidor"+DateToStr(time(NULL))+".txt").c_str(),"a");
			if ( arq != NULL)
			{
				string log = DateToStr(time(NULL)) +" "+ TimeToStr(time(NULL)) +" "+logData+"\r\n";
				fputs(log.c_str(),arq);
				fclose(arq);
			}
		}

        bool AddData(unsigned char * Arg,int size);

        bool Open(int portno)
        {
            int rb = 0;
        	struct sockaddr_in addr_serv;
			bzero(&addr_serv, sizeof(addr_serv));
			addr_serv.sin_family = AF_INET;
			addr_serv.sin_port = htons(portno);
			addr_serv.sin_addr.s_addr = INADDR_ANY;
			fdServer = socket(AF_INET, SOCK_STREAM, 0);

			// funcao para reusar porta enquanto bloqueada pelo sistema operacional
			int optval = 1;
			setsockopt(fdServer, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

			// tratamento para caso nao consiga reservar a porta, nao continua
			rb = bind(fdServer, (struct sockaddr *)&addr_serv, sizeof(struct sockaddr));
			if (rb < 0) return false;

			this->log("Listening...");
			if(listen(fdServer,50)!=-1)
			{
				//cout << "Criando Processo Pai ServerMessages" << endl;
				this->log("Aceito");
				if(pthread_create(&hServerThread,0,thListen,this) == -1)
				{
					return false;
				}
				return true;
			}
			else
			{
				this->log("Listen retornou erro!!!");
				return false;
			}
        }
        void SendAll(string sMens)
        {
                for(int i = 0;i < (int)Connections.size();i++)
                {
                        Connections[i]->SendMens(sMens);
                }
        }
        static void * thListen(void*arg)
        {
                struct sockaddr_in addr_cli;
                CServerMensParent * ptrServer = (CServerMensParent*)arg;
                int fdClient=0;
                socklen_t tamanho = sizeof (struct sockaddr_in);
                while((fdClient =accept(ptrServer->fdServer,(struct sockaddr *)&addr_cli,&tamanho))!=1)
                {
                		//int optval = 1;
                		//setsockopt(fdClient, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
                        CServerMensChildren * newChildren = new CServerMensChildren;
                        ptrServer->Connections.push_back(newChildren);
                        newChildren->fdCli = fdClient;
                        newChildren->cbRead = ptrServer->cbRead;
                        newChildren->object = ptrServer->object;
                        newChildren->sPathlogs = ptrServer->sPathlogs;
                        newChildren->Go();
                        //cout << "Conexao aceita.Filho criado!!!" << endl;                        
                }
                ptrServer->log("Saiu do laço accept e fecha thread");
                pthread_exit(0);
        }
        void * thRead(void*arg);
        void * OnRecvTxt(string Data);
        void * Parameter1;
        string sPathlogs;
private:
        unsigned char * datach;
        int * fdSockets;
        int fdServer;
        pthread_t hServerThread;
};
