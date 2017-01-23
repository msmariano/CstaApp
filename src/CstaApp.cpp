//===========================================================================================
// Name        : CstaApp.cpp
// Author      : Marcelo dos Santos Mariano (MSM).
// Version     : 1.0.0.0 - Linux
// Copyright   : Marcelo Mariano 
// Description : Middle para Integração Csta
//               11-12-2012 - Criação
//			         16-01-2013 - InsertDb para receber conexao;
//               22-01-2013 - Definido csta->SetHeaderSize(2) para H4000..
//               22-01-2013 - corrigido for comando snapshotdevice
//		           29-01-2013 - Inserido função lowercase para tratamento mensagens
//               29-01-2013 - Comando monitorstart insere ramal na base
//               29-01-2013 - Comando monitorstop remove ramal da base
//               29-01-2013 - Mensagem "Comando não definido" mostra o comando recebido
//               05-02-2013 - Filtro para Eventos 
//               08-02-2013 - Argumento agenteid para comando SetAgenteState
//               08-02-2013 - Verifica se Csta está conectado antes de enviar comando
//							 08-02-2013 - Mudado Database de CSTA para asterisk			
//               08-02-2013 - Cria automaticamente tabelas na database asterisk
//               15-02-2013 - Envio de comandos entre tags
//							 18-02-2013	- Implantado envio de alarme através de email
//===========================================================================================

#include <iostream>
#include <stdio.h>
#include "../include/AsnNode.h"
#include "../include/CstaTypes.h"
#include "../include/CallList.h"
#include "../include/CCSTA.h"
#include <fstream>
#include "../include/CMySQL.h"
#include "../include/utils.h"
#include <sys/stat.h>
#include "../include/ServerMessages.h"
#include <arpa/inet.h>
#include "../include/CCB64.h"

#include "/usr/include/openssl/crypto.h"
#include "/usr/include/openssl/x509.h"
#include "/usr/include/openssl/pem.h"
#include "/usr/include/openssl/ssl.h"
#include "/usr/include/openssl/err.h"

#define CHK_NULL(x) if ((x)==NULL) exit (1)
#define CHK_ERR(err,s) if ((err)==-1) { perror(s); exit(1); }
#define CHK_SSL(err) if ((err)==-1) { ERR_print_errors_fp(stderr); exit(2); }

using namespace std;

//Variaveis Globais
bool vlogs;
bool glogs;
bool bVlG;

TCallList * CallList;
string sPathexe;
char gEvMask[14];



string gServerMail;
int gPortMail;
string gUserMail;
string gPasswMail;
int gTypeMail;
string gRemetenteMail;
string gDestinatarioMail;
string gAssuntoMail;
string gTextMail;
bool bActMail;
int gFrAlarme;
int GravarEv;

typedef struct
{
	string sEnd;
	int iPorta;
	CMySQL * tDb;
	CCSTA * csta;
	string sVersion;
}stDiscInfo;

stDiscInfo DiscInfo;

//////////////////////////////////////////////////////////////////////////////////////////////////////
//Cliente de Email
class CMail
{
public:
	CMail()
	{
		fdSockets = new int;
		fd = -1;
		err = 0;
		ctx = 0;
		server_cert = 0;
		meth = 0;
		str = 0;
		ssl = 0;
		iType = 0;
		iPortSmtpServer = 25;
		bzero(&addr_serv, sizeof(addr_serv));
	}
	~CMail()
	{

	}
	int HostnameToIp(char * hostname , char* ip)
	{
		struct hostent *he;
		struct in_addr **addr_list;
		int i;
		if ( (he = gethostbyname( hostname ) ) == NULL)
		{
			// get the host info
			herror("gethostbyname");
			return 1;
		}
		addr_list = (struct in_addr **) he->h_addr_list;
		for(i = 0; addr_list[i] != NULL; i++)
		{
			//Return the first one;
			strcpy(ip , inet_ntoa(*addr_list[i]) );
			return 0;
		}

		return 1;
	}
	string SendAut()
	{
		string sMens;
		sMens = "STARTTLS\r\n";
		return sMens;
	}

	string SendEHLO()
	{
		string sMens;
		sMens = "EHLO CstaMonitor\r\n";
		return sMens;
	}
	void Send(string sMens)
	{
		if(iType == 0)
		{
			err = SSL_write (ssl, sMens.c_str(),sMens.size());
			CHK_SSL(err);
		}
		else
		{
				send(fd,sMens.c_str(),sMens.size(),0);
		}
	}
	string Read()
	{
		if(iType == 0)
		{
			err = SSL_read (ssl, buf, sizeof(buf) - 1);
			CHK_SSL(err);
			buf[err] = '\0';
			//printf ("Got %d chars:'%s'\n", err, buf);
			//cout << buf << endl;
		}
		else
		{
			recv(fd,buf, sizeof(buf),0);
		}
		return string(buf);
	}
	bool Connect()
	{
		char ip[100];
		bzero(&addr_serv, sizeof(addr_serv));
		addr_serv.sin_family = AF_INET;
		addr_serv.sin_port = htons(iPortSmtpServer);
		HostnameToIp((char*)sSmtpServer.c_str() , ip);
		if(inet_pton(AF_INET, ip, &addr_serv.sin_addr)<=0)
		{
			sError =  "Não consegui resolver o endereço "+sSmtpServer + ":"+IntToStr(iPortSmtpServer);
			return false;
		}
		else
		{
			fd = socket(AF_INET, SOCK_STREAM, 0);
			if(connect(fd,(struct sockaddr *)&addr_serv, sizeof(struct sockaddr))!=-1)
			{
				return true;
			}
			else
			{
				sError = "Erro ao conectar "+sSmtpServer + ":"+IntToStr(iPortSmtpServer);
			}
		}
		return false;
	}
	void InitializeType()
	{
		if(iType == 0)
		{
			SSLeay_add_ssl_algorithms();
			meth = (SSL_METHOD *)SSLv23_client_method();
			SSL_load_error_strings();
			ctx = SSL_CTX_new (meth);
			CHK_NULL(ctx);
			CHK_SSL(err);
			ssl = SSL_new (ctx);
			CHK_NULL(ssl);
			SSL_set_fd (ssl, fd);
			err = SSL_connect (ssl);
			CHK_SSL(err);
			//printf ("SSL connection using %s\n", SSL_get_cipher (ssl));
			server_cert = SSL_get_peer_certificate (ssl);       
			CHK_NULL(server_cert);
			//printf ("Server certificate:\n");
			str = X509_NAME_oneline (X509_get_subject_name (server_cert),0,0);
			CHK_NULL(str);
			//printf ("\t subject: %s\n", str);
			OPENSSL_free (str);
			str = X509_NAME_oneline (X509_get_issuer_name  (server_cert),0,0);
			CHK_NULL(str);
			//printf ("\t issuer: %s\n", str);
			OPENSSL_free (str);
			X509_free (server_cert);
		}	
	}
	bool OpenComm()
	{
		try
		{
			if(Connect())
			{
				char sRecv[1024] = "";
				if(recv(fd,sRecv,sizeof(sRecv),0)!=-1)
				{
					//cout << sRecv << endl;
					if(send(fd,SendEHLO().c_str(),SendEHLO().size(),0)!=-1)
					{
						char sRecv[1024] = "";
						if(recv(fd,sRecv,sizeof(sRecv),0)!=-1)
						{
							//cout << sRecv << endl;
							if(send(fd,SendAut().c_str(),SendAut().size(),0)!=-1)
							{
								char sRecv[1024] = "";
								if(recv(fd,sRecv,sizeof(sRecv),0)!=-1)
								{
									string sAnswer;
									//cout << sRecv << endl;
									InitializeType();
									Send("AUTH LOGIN\r\n");
									sAnswer = Read();
									//Parse da resposta
									//SE RESPOSTA 535 ERRO NAO EXISTE A OPCAO
									int pos;
									if((pos=sAnswer.find("535",0))>-1)
									{
										//opcao de login inexistente
										return false;
									}

									//ENQUANTO HOUVER PEDIDO DE LOGIN
									while((pos=sAnswer.find("334 ",0))>-1)
									{
										string crypto,retstr;
										//OBJETO PARA CODIFICACAO EM B64
										CCB64 code;
										//PEGO A VARIAVEL DE LOGIN
										sAnswer = sAnswer.substr(pos+4,sAnswer.size());
										//VAMOS DECODIFICAR
										crypto = sAnswer ;
										//FIM DA LINHA PARA PEGAR APENAS A PARTE CODIFICADA
										if((pos = sAnswer.find("\r\n"))>0)
										{
											//cout << "sAnswer:" << sAnswer << endl;
											//DECODIFICO E SALVO A RESPOSTA
											code.SetInfileD(crypto.substr(0,pos));
											code.decode();

											retstr = code.GetRet();
											//SE LOGIN PEDE O USERNAME
											//VOU ENVIAR O USERNAME
											//cout << retstr << endl;
											if( retstr == "Username:")
											{
												//cout << "Solicitando User!!!" << endl;
												//ENVIA O USUARIO CODIFICANDO EM B64
												code.Reset();
												code.SetInfile(sUser);
												code.SetCount(sUser.size());
												code.encode();
												retstr = code.GetOutfile();
												//cout << "Code Resp:" << retstr << endl;
												retstr+="\r\n";
												Send(retstr);
												sAnswer = Read();
											}
											//SE SENHA VOU ENVIA A SENHA CODIFICADA EM B64
											else if ( retstr == "Password:")
											{
												//cout << "Solicitando Pasw!!!" << endl;
												//Envia password codificado em B64
												code.Reset();
												code.SetInfile(sPassw);
												code.SetCount(sPassw.size());
												code.encode();
												retstr = code.GetOutfile();
												//cout << "Code Resp:" << retstr << endl;
												retstr+="\r\n";
												Send(retstr);
												//err = SSL_write (ssl, retstr.c_str(),retstr.size());
												//CHK_SSL(err);
												sAnswer = Read();
												
												//err = SSL_read (ssl, buf, sizeof(buf) - 1);
												//CHK_SSL(err);
												//buf[err] = '\0';
												//printf ("Got %d chars:'%s'\n", err, buf);
												//sAnswer = buf;
											}
										}
									}
									//login aceito
									if((pos = sAnswer.find("235"))>-1)
									{
										//cout << "Login Aceito!!!" << endl;
										//cout << "Enviando email!!!" << endl;

										Send("MAIL FROM: <"+sFrom+">\r\n");
										sAnswer = Read();
										Send("RCPT TO: <"+sTo+">\r\n");
										sAnswer = Read();
										Send("DATA\r\n");
										sAnswer = Read();
										time_t  t;
										struct tm *gmt;
										t = time(0);
										gmt = gmtime(&t);
										string h = asctime(gmt);
										Send("From: "+sFrom+">\r\n"
										"To: <"+sTo+">\r\n"
										"Subject: "+sSubject+"\r\n"
										+GetGMT()+"\r\n"+
										"\r\n"
										""+sText+"\r\n"
										"\r\n"
										".\r\n"
										"QUIT\r\n");
										sAnswer = Read();

										return true;
									}
									else
									{
										sError = "Login negado!!!";
										return false;
									}

									if(iType == 0)
									{
										SSL_shutdown (ssl); 
										close (fd);
										SSL_free (ssl);
										SSL_CTX_free (ctx);
									}
								}
							}
						}
					}
				}

			}
			else
			{
				return false;
			}
		}
		catch(exception &e)
		{
			sError = e.what();
			return false;
		}
		return true;
	}
	string MakeHeader()
	{
		string sHeader;
		sHeader += "MAIL FROM: <"+sRemetente+">\r\n";
		sHeader += "RCPT TO: <"+sDestinatario+">\r\n";
		sHeader += "DATA\r\n";
		return sHeader;
	}
	string GetGMT()
	{

		try
		{
			char hora_gmt[1024]="";
			string S;
			tm * resultado;
			//pega a data do micro
			time_t now = time(NULL);
			//formata para gmt
			resultado = gmtime(&now);
			//se deu certo
			if(resultado)
			{
				//crio o texto
				strftime(hora_gmt,1000,"Date: %a, %d %b %Y %H:%M:%S",resultado);
				S = hora_gmt;
				return S;
			}
			else
			{
				return "";
			}
		}
		catch(exception &e)
		{
			return e.what();
		}
		return "";
	}
	void SetRemetente(string sArg)
	{
		sRemetente = sArg;
	}
	void SetDestinatio(string sArg)
	{
		sDestinatario = sArg;
	}
	void SetUser(string sArg)
	{
		sUser = sArg;
	}
	void SetPassw(string sArg)
	{
		sPassw = sArg;
	}
	void SetSubject(string sArg)
	{
			sSubject = sArg;
	}
	void SetText(string sArg)
	{
		sText = sArg;
	}
	void SetSmtpServer(string sArg)
	{
		sSmtpServer = sArg;		
	}
	void SetPortSmtpServer(int iArg)
	{
		iPortSmtpServer = iArg;
	}
	string GetError()
	{
		return sError;
	}
	void SetType(int iArg)
	{
			iType = iArg;			
	}
	void SetFrom(string sArg)
	{
		sFrom = sArg;
	}
	void SetTo(string sArg)
	{
		CSplit sp;
		sp.Exec(sArg,";");
		for(int i=0;i < sp.GetSize();i++)
		{
			if(i == 0)
				sTo+=sp[i];
			else
				sTo+=sp[i]+";";
		}
		//cout << sTo << endl;
		//sTo = sArg;
	}
private:
	string sRemetente;
	string sDestinatario;
	string sPassw;
	string sUser;
	string sSubject;
	string sFrom;
	string sTo;
	string sText;
	int iType;
	string sError;
	string sSmtpServer;
	int iPortSmtpServer;
	int * fdSockets;
	int fd;
	int err;
	SSL_CTX* ctx;
	SSL*     ssl;
	SSL_METHOD *meth;
	X509*    server_cert;
	char*    str;
	char     buf [4096];
	struct sockaddr_in addr_serv;
	vector <string> dst;
};
/////////////////////////////////////////////////////////////////////////////////////////////////////

/*
 *
 *
 *
 *
 * */

/*
 * ln ./cstaserve /etc/init.d/
 * sudo /etc/init.d/cstaserve start
 * sudo /etc/init.d/cstaserve stop
 * sudo update-rc.d cstaserve  defaults
 * sudo update-rc.d cstaserve start 90 2 3 4 5 . stop 90 0 1 6 .
   sudo update-rc.d -f cstaserve remove
 * */


/*
 * Data Definition
 *      Id: numero sequencial utilizado como chave primaria da tabela;
        Data: Data do evento;
        Hora: Hora do evento;
        Dur: Duracao do evento;
        CallID: identificacao da chamada fornecida pelo PABX;
        Evento: Nome do evento;
        Ramal_ori: Ramal de origem da ligacao;
        Ramal_des: Ramal de destino da ligacao;
        Num_ori: Numero de origem da ligacao;
        Num_des: Numero de destino da ligacao;
        Device: Numero do dispositivo onde evento ocorreu
 *
 * */

void ExpurgarTabelaCsta(CMySQL tDb,CCSTA csta)
{
	if(!tDb.ExecSql("INSERT INTO CSTA_FULL SELECT * FROM CSTA"))
	{
		csta.logCsta(string("Erro expurgo passo 1")+ tDb.GetError());
	}
	if(!tDb.ExecSql("DELETE FROM CSTA"))
	{
		csta.logCsta(string("Erro expurgo passo 2")+ tDb.GetError());
	}
	if(!tDb.ExecSql("OPTIMIZE TABLE `asterisk`.`CSTA`"))
	{
		csta.logCsta(string("Erro expurgo passo 3")+ tDb.GetError());
	}
	if(!tDb.ExecSql("CHECK TABLE `asterisk`.`CSTA` QUICK;"))
	{
		csta.logCsta(string("Erro expurgo passo 4")+ tDb.GetError());
	}
	if(!tDb.ExecSql("REPAIR TABLE `asterisk`.`CSTA` QUICK;"))
	{
		csta.logCsta(string("Erro expurgo passo 5")+ tDb.GetError());
	}
}

string lowercase(string in)
{
	string ret;
	char char1,blank = ' ' ;

	try
	{
		for (unsigned i = 0 ; i < in.size() ; ++i )
		{
			char1 = *(in.substr(i,1).data()) ;   // get a single character
			char1 |= blank ;   // make lowercase for compare
			ret+= char1;
		}
	}
	catch(...)
	{

	}
	return ret ;
}

bool InsertDb(int iCallId,string sEvento,string sRmlOr,string sRmlDes,string sNumOr,string sNumDes,string sDevice,CMySQL * db)
{


	if (GravarEv == 0)
		return true;

	//cout << "Gravando Evento" << endl;

	bool ret = false;

	char chCallId[100]="";
	string sQuery;

	sprintf(chCallId,"%d%d",DateToInt(time(NULL)),iCallId);

	//sprintf(chCallId,"%d",iCallId);


	if(sRmlOr.size() > sDevice.size())
	{
		sRmlOr = "";
	}
	if(sRmlDes.size() > sDevice.size())
	{
		sRmlDes = "";
	}

	if(sNumOr.size() <= sDevice.size())
	{
		sNumOr = "";
	}
	if(sNumDes.size() <= sDevice.size())
	{
		sNumDes = "";
	}

	//CMySQL db;
	//db.SetAcess("root","mypass","127.0.0.1");
	//db.SetDb("CSTA");
	//db.dbConnect();
	//Atualiza duracao ultimo evento do callid
	sQuery = "UPDATE CSTA SET DUR = SUBTIME( CURRENT_TIME( ), HORA) "
			"WHERE ID  = "
			"( "
			"	SELECT ID FROM "
			"	( "
			" 		SELECT MAX(ID) ID FROM CSTA WHERE CALLID = "+string(chCallId)+" AND CONVERT(DATA,DATE) = CONVERT(NOW(),DATE)"
			"	)RET "
			")";


	if(!db->ExecSql(sQuery))
	{
		//csta.logCsta(string("Error MySQL: ") + db.GetError() + string("Query: ") + sQuery);
	}

	if(sEvento == "evTransferred")
	{
		sQuery = "SELECT MIN(ID) ID FROM CSTA WHERE CALLID = "+string(chCallId)+" AND evento  = 'evServiceInitiated' ";

		if(db->ExecSql(sQuery))
		{
			if(db->operator [](0) != "")
			{
				sNumDes = sNumOr;
				sNumOr = "";
				ret = true;
			}
		}

	}

	//insere novo evento
	sQuery = "INSERT INTO CSTA( "
			"Data,"
			"Hora,"
			"dur,"
			"callid,"
			"evento,"
			"ramal_ori,"
			"ramal_des,"
			"num_ori,"
			"num_des,"
			"device"
			")"
			"VALUES("
			"Now(),"
			"Now(),"
			"'',"
			""+string(chCallId)+","
			"'"+sEvento+"',"
			"'"+sRmlOr+"',"
			"'"+sRmlDes+"',"
			"'"+sNumOr+"',"
			"'"+sNumDes+"',"
			"'"+sDevice+"'"
			")";

	//cout << sQuery << endl;

	CMySQL db1;
	db1.SetAcess("root","mar0403","127.0.0.1");
    db1.SetDb("asterisk");
    db1.dbConnect();

	if(!db1.ExecSql(sQuery))
	{
		//cout << db->GetDb() << endl;
		FILE * arq;
		string mens = string("Error MySQL: ") + db1.GetError() +string(" Query: ")+ sQuery+"\r\n";
		arq = fopen(string(sPathexe+"error_my.txt").c_str(),"a");
		if ( arq != NULL)
		{
			fputs(mens.c_str(),arq);
			fclose(arq);
		}

	}
	return ret;

}

void InsertEvent(int iCallId,string sEvento,string sRmlOr,string sRmlDes,string sNumOr,string sNumDes,string sDevice,CServerMensParent * MensServer)
{
	//10/12/12;09:28:43;1234;Delivered;null;3000;041336086;3000
	if(sRmlOr.size() > sDevice.size())
	{
		sRmlOr = "null";
	}
	if(sRmlDes.size() > sDevice.size())
	{
		sRmlDes = "null";
	}

	if(sNumOr.size() <= sDevice.size())
	{
		sNumOr = "null";
	}
	if(sNumDes.size() <= sDevice.size())
	{
		sNumDes = "null";
	}
	if(sRmlOr == "")
		sRmlOr = "null";
	if(sRmlDes == "")
		sRmlDes = "null";
	if(sNumOr == "")
		sNumOr  = "null";
	if(sNumDes == "")
		sNumDes = "null";


	string sMens;
	sMens = DateToStr(time(NULL))+";"
			+TimeToStr(time(NULL))+";"
			+IntToStr(DateToInt(time(NULL)))+IntToStr(iCallId)+";"
			+sEvento+";"
			+sRmlOr+";"
			+sRmlDes+";"
			+sNumOr+";"
			+sNumDes+";"
			+sDevice+";\r\n";
	MensServer->SendAll(sMens);
}



//Funcao de callback de retorno do CSTA
void CallBackCallControl(TCallControlEventType tccet, TCallControlEvent * tcce, void * arg)
{
	string sMens;
	string org,dst,orgtr,dsttr,devtr;
	CServerMensParent * MensServer = (CServerMensParent*)arg;
	CMySQL * dbIns = (CMySQL *)MensServer->Parameter1;
	CallList->UpdateCalls(tccet,tcce,tcce->crossRefId);
	switch (tccet)
	{
	case evServiceInitiated:
		if(gEvMask[0] == '1')
		{	
			InsertDb(tcce->ServiceInitiated.InitiatedConnection.CallID,
					"evServiceInitiated",tcce->Device,"","","",tcce->Device,dbIns);
			InsertEvent(tcce->ServiceInitiated.InitiatedConnection.CallID,
					"evServiceInitiated",tcce->Device,"","","",tcce->Device,MensServer);
		}
		break;
	case evFailed:
		if(gEvMask[1] == '1')
		{
			InsertDb(tcce->Failed.FailedConnection.CallID,
					"evFailed",tcce->Device,tcce->Failed.CalledDevice.DeviceID.DialingNumber,
					"",tcce->Failed.CalledDevice.DeviceID.DialingNumber,tcce->Device,dbIns);
			InsertEvent(tcce->Failed.FailedConnection.CallID,
					"evFailed",tcce->Failed.CallingDevice.DeviceID.DialingNumber,tcce->Failed.CalledDevice.DeviceID.DialingNumber,
					tcce->Failed.CallingDevice.DeviceID.DialingNumber,tcce->Failed.CalledDevice.DeviceID.DialingNumber,tcce->Device,MensServer);
		}
		break;
	case evConnectionCleared:
		if(gEvMask[2] == '1')
		{
			InsertDb(tcce->ConnectionCleared.DroppedConnection.CallID,
					"evConnectionCleared",tcce->ConnectionCleared.ReleasingDevice.DeviceID.DialingNumber,"","","",tcce->Device,dbIns);
			InsertEvent(tcce->ConnectionCleared.DroppedConnection.CallID,
					"evConnectionCleared",tcce->ConnectionCleared.ReleasingDevice.DeviceID.DialingNumber,"","","",tcce->Device,MensServer);
		}
		break;
	case evDelivered:

		//cout << tcce->IvrID << endl;

		if(gEvMask[3] == '1')
		{	
				if(tcce->Device == tcce->Delivered.CallingDevice.DeviceID.DialingNumber)
				{
					dst = tcce->Delivered.AlertingDevice.DeviceID.DialingNumber;
					org = tcce->Device;
				}
				else
				{
					dst = tcce->Device;
					org = tcce->Delivered.CallingDevice.DeviceID.DialingNumber;
				}

				if (org == "")
				{
					org = tcce->IvrID;

				}

				InsertDb(tcce->Delivered.Connection.CallID,"evDelivered",org,dst,org,dst,tcce->Device,dbIns);
				InsertEvent(tcce->Delivered.Connection.CallID,"evDelivered",org,dst,org,dst,tcce->Device,MensServer);
		}
		break;
	case evEstablished:
		if(gEvMask[4] == '1')
		{
				if(tcce->Device == tcce->Established.CallingDevice.DeviceID.DialingNumber)
				{
					InsertDb(tcce->Established.EstablishedConnection.CallID,
							"evEstablished",tcce->Device,tcce->Established.AnsweringDevice.DeviceID.DialingNumber,
							tcce->Device,tcce->Established.AnsweringDevice.DeviceID.DialingNumber,tcce->Device,dbIns);
		
		
					InsertEvent(tcce->Established.EstablishedConnection.CallID,
							"evEstablished",tcce->Device,tcce->Established.AnsweringDevice.DeviceID.DialingNumber,
							tcce->Device,tcce->Established.AnsweringDevice.DeviceID.DialingNumber,tcce->Device,MensServer);
				}
				else
				{
					InsertDb(tcce->Established.EstablishedConnection.CallID,
							"evEstablished",tcce->Established.CallingDevice.DeviceID.DialingNumber,tcce->Device,
							tcce->Established.CallingDevice.DeviceID.DialingNumber,tcce->Device,tcce->Device,dbIns);
		
					InsertEvent(tcce->Established.EstablishedConnection.CallID,
							"evEstablished",tcce->Established.CallingDevice.DeviceID.DialingNumber,tcce->Device,
							tcce->Established.CallingDevice.DeviceID.DialingNumber,tcce->Device,tcce->Device,MensServer);
				}
		}
		break;
	case evQueued:
		if(gEvMask[5] == '1')
		{
				InsertDb(tcce->Queued.QueuedConnection.CallID,
						"evQueued",tcce->Queued.CallingDevice.DeviceID.DialingNumber,tcce->Queued.CalledDevice.DeviceID.DialingNumber,
						tcce->Queued.CallingDevice.DeviceID.DialingNumber,tcce->Queued.CalledDevice.DeviceID.DialingNumber,tcce->Device,dbIns);
				InsertEvent(tcce->Queued.QueuedConnection.CallID,
						"evQueued",tcce->Queued.CallingDevice.DeviceID.DialingNumber,tcce->Queued.CalledDevice.DeviceID.DialingNumber,
						tcce->Queued.CallingDevice.DeviceID.DialingNumber,tcce->Queued.CalledDevice.DeviceID.DialingNumber,tcce->Device,MensServer);
		}
		break;
	case evHeld:
		if(gEvMask[6] == '1')
		{
				InsertDb(tcce->Held.HeldConnection.CallID,
						"evHeld",tcce->Held.HoldingDevice.DeviceID.DialingNumber,"","","",tcce->Device,dbIns);
				InsertEvent(tcce->Held.HeldConnection.CallID,
						"evHeld",tcce->Held.HoldingDevice.DeviceID.DialingNumber,"","","",tcce->Device,MensServer);
		}
		break;
	case evRetrieved:
		if(gEvMask[7] == '1')
		{
				InsertDb(tcce->Retrieved.RetrievedConnection.CallID,
						"evRetrieved",tcce->Retrieved.RetrievingDevice.DeviceID.DialingNumber,"","","",tcce->Device,dbIns);
				InsertEvent(tcce->Retrieved.RetrievedConnection.CallID,
						"evRetrieved",tcce->Retrieved.RetrievingDevice.DeviceID.DialingNumber,"","","",tcce->Device,MensServer);
		}
		break;
	case evTransferred:
		if(gEvMask[8] == '1')
		{
				orgtr = tcce->Transferred.TransferredConnections.Connections[1].endPoint.DialingNumber;
				devtr = tcce->Device;
				dsttr = tcce->Transferred.TransferredConnections.Connections[0].endPoint.DialingNumber;

				cout << "Org:" << orgtr << endl;
				cout << "Devcl:" << devtr << endl;
				cout << "Dst:" << dsttr << endl;
		
				if(InsertDb(tcce->Transferred.TransferredConnections.Connections[0].newConnection.CallID,
						"evTransferred",devtr,orgtr,dsttr,"",tcce->Device,dbIns))
				{
					InsertEvent(tcce->Transferred.TransferredConnections.Connections[0].newConnection.CallID,
									"evTransferred",devtr,orgtr,"",dsttr,tcce->Device,MensServer);
		
				}
				else
				{
					InsertEvent(tcce->Transferred.TransferredConnections.Connections[0].newConnection.CallID,
									"evTransferred",devtr,orgtr,dsttr,"",tcce->Device,MensServer);
				}
		}
		break;
	case evOriginated:
		if(gEvMask[9] == '1')
		{
				InsertDb(tcce->Originated.OriginatedConnection.CallID,
						"evOriginated",
						tcce->Device,tcce->Originated.CalledDevice.DeviceID.DialingNumber,
						tcce->Device,tcce->Originated.CalledDevice.DeviceID.DialingNumber,
						tcce->Device,dbIns);
				InsertEvent(tcce->Originated.OriginatedConnection.CallID,
						"evOriginated",
						tcce->Device,tcce->Originated.CalledDevice.DeviceID.DialingNumber,
						tcce->Device,tcce->Originated.CalledDevice.DeviceID.DialingNumber,
						tcce->Device,MensServer);
		}
		break;
	case evNetworkReached:
		if(gEvMask[10] == '1')
		{
				InsertDb(tcce->NetworkReached.OriginatingNIDConnection.CallID,
						"evNetworkReached",
						tcce->Device,
						tcce->NetworkReached.CalledDevice.DeviceID.DialingNumber,
						tcce->Device,
						tcce->NetworkReached.CalledDevice.DeviceID.DialingNumber,
						tcce->Device,dbIns);
				InsertEvent(tcce->NetworkReached.OriginatingNIDConnection.CallID,
						"evNetworkReached",
						tcce->Device,
						tcce->NetworkReached.CalledDevice.DeviceID.DialingNumber,
						tcce->Device,
						tcce->NetworkReached.CalledDevice.DeviceID.DialingNumber,
						tcce->Device,MensServer);
		}
		break;
	case evDiverted:
		if(gEvMask[11] == '1')
		{
				InsertDb(tcce->Diverted.Connection.CallID,
						"evDiverted",
						tcce->Diverted.CallingDevice.DeviceID.DialingNumber,
						tcce->Diverted.CalledDevice.DeviceID.DialingNumber,
						tcce->Diverted.CallingDevice.DeviceID.DialingNumber,
						tcce->Diverted.CalledDevice.DeviceID.DialingNumber,
						tcce->Device,dbIns);
				InsertEvent(tcce->Diverted.Connection.CallID,
						"evDiverted",
						tcce->Diverted.CallingDevice.DeviceID.DialingNumber,
						tcce->Diverted.CalledDevice.DeviceID.DialingNumber,
						tcce->Diverted.CallingDevice.DeviceID.DialingNumber,
						tcce->Diverted.CalledDevice.DeviceID.DialingNumber,
						tcce->Device,MensServer);
		}
		break;
	case evConferenced:
		if(gEvMask[12] == '1')
		{
				for(int i = 0; i < tcce->Conferenced.ConferenceConnections.Count; i++)
				{
					InsertDb(tcce->Conferenced.ConferenceConnections.Connections[i].newConnection.CallID,
							"evConferenced",
							tcce->Device,
							tcce->Conferenced.ConferenceConnections.Connections[i].endPoint.DialingNumber,
							tcce->Device,
							tcce->Conferenced.ConferenceConnections.Connections[i].endPoint.DialingNumber,
							tcce->Device,dbIns);
					InsertEvent(tcce->Conferenced.ConferenceConnections.Connections[i].newConnection.CallID,
							"evConferenced",
							tcce->Device,
							tcce->Conferenced.ConferenceConnections.Connections[i].endPoint.DialingNumber,
							tcce->Device,
							tcce->Conferenced.ConferenceConnections.Connections[i].endPoint.DialingNumber,
							tcce->Device,MensServer);
				}
		}
		break;
	case evOffered:
		//nao aplicavel
		break;
	case evNetworkCapabilitiesChange:
		//nao aplicavel
		break;
	case evDigitsDialed:
		//nao aplicavel
		break;
	case evCallCleared:
		//nao aplicavel
		break;
	case evBridged:
		//nao aplicavel
		break;
	}
}

//Teste de CallList nao aplicavel
void  BkEnd(TCallItem * Call, void * Arg)
{
	//csta.logCsta(string("Finalizada chamada para ") + Call->Device);
}

//Tratamento do recebimento de comandos CSTA via socket
void ReadMens(char * cMens,void*arg,void*object)
{
	//Debug
	//printf("Teste ReadMens:%s\r\n",cMens);

	string Mens = cMens;
	CCSTA * csta = (CCSTA*)object;
	CServerMensChildren * Ch = (CServerMensChildren*)arg;
	CSplit sp;
	string MensCpy = Mens;
	Mens = lowercase(Mens);
	//Verificar se csta está conectado
	if(!csta->cstaConnected)
	{

		Mens = Mens.substr(0,Mens.size()-2);
		sp.Parser(Mens,";","cstatest");
		//sp.Exec(Mens,";");
		int total = sp[0].size();
		Ch->SendMens("Csta Desconectado["+IntToStr(total)+"]["+sp[0]+"]!!!\r\n");
		if(sp[0] == "setagentstate")
		{
			Ch->SendMens("Comando "+sp[0]+" definido\r\n");
		}
		else if (sp[0] == "simulacao")
		{
			for(int i=0;i < sp.GetSize();i++)
			{
				Ch->SendMens("Parametro["+IntToStr(i)+"]"+sp[i]+"\r\n");
			}
		}
		else
		{
			Ch->SendMens("Comando "+sp[0]+" nao definido\r\n");	
		}					
	}
	else
	{	
			try
			{
				//Retirando \r\n do final
				Mens = Mens.substr(0,Mens.size()-2);

				//if(bVlG)
				{
					cout << Mens << endl;
				}
				sp.Parser(Mens,";","cstacommand");
				//sp.Exec(Mens,";");
				if(sp[0] == "makecall")
				{
					csta->logCsta(string("MakeCall ")+sp[1]+string("->")+sp[2]);
					int callid;
					bool res = csta->MakeCall((char*)sp[1].c_str(),(char*)sp[2].c_str(),&callid);
					if(res)
					{
						Ch->SendMens("MakeCallResult;Ok;"+Ch->IntToStr(callid)+"\r\n");
					}
					else
					{
						Ch->SendMens("MakeCallResult;Error\r\n");
					}
				}
				else if(sp[0] == "monitorstart")
				{
					CMySQL * dbIns = (CMySQL *)Ch->Parameter1;
					TMonitorType monitorType;
					int crossRefId;
					if(sp[2] == "0")
						monitorType = mtDevice;
					else if(sp[2] == "1")
						monitorType = mtTrunk;
		
					if(csta->MonitorStart((char*)sp[1].c_str(),monitorType,&crossRefId))
					{
						Ch->SendMens("MonitorStart;Ok;"+Ch->IntToStr(crossRefId)+"\r\n");
						dbIns->ExecSql("INSERT INTO DEVICE_CSTA(device)VALUES("+sp[1]+")");
					}
					else
						Ch->SendMens("MonitorStart;Error\r\n");
				}
				else if(sp[0] == "clearconnection")
				{
					if(csta->ClearConnection(atoi(sp[2].c_str()), (char*)sp[1].c_str()))
					{
						Ch->SendMens("ClearConnection;Ok\r\n");
					}
					else
					{
						Ch->SendMens("ClearConnection;Error\r\n");
					}
				}
				else if(sp[0] == "answercall")
				{
					if(csta->AnswerCall(atoi(sp[2].c_str()), (char*)sp[1].c_str()))
					{
						Ch->SendMens("AnswerCall;Ok\r\n");
					}
					else
					{
						Ch->SendMens("AnswerCall;Error\r\n");
					}
				}
				else if(sp[0] == "holdcall")
				{
					if(csta->HoldCall(atoi(sp[2].c_str()), (char*)sp[1].c_str()))
					{
						Ch->SendMens("HoldCall;Ok\r\n");
					}
					else
					{
						Ch->SendMens("HoldCall;Error\r\n");
					}
				}
				else if(sp[0] == "retrievecall")
				{
					if(csta->RetrieveCall(atoi(sp[2].c_str()), (char*)sp[1].c_str()))
					{
						Ch->SendMens("RetrieveCall;Ok\r\n");
					}
					else
					{
						Ch->SendMens("RetrieveCall;Error\r\n");
					}
				}
				else if(sp[0] == "parkcall")
				{
					if(csta->ParkCall( (char*)sp[1].c_str(),(char*)sp[2].c_str(),atoi(sp[3].c_str())))
					{
						Ch->SendMens("ParkCall;Ok\r\n");
					}
					else
					{
						Ch->SendMens("ParkCall;Error\r\n");
					}
				}
				else if(sp[0] == "alternatecall")
				{
					if(csta->AlternateCall(atoi(sp[3].c_str()),(char*)sp[1].c_str(),atoi(sp[4].c_str()),(char*)sp[2].c_str()))
					{
						Ch->SendMens("AlternateCall;Ok\r\n");
					}
					else
					{
						Ch->SendMens("AlternateCall;Error\r\n");
					}
				}
				else if(sp[0] == "reconnectcall")
				{
					if(csta->ReconnectCall(atoi(sp[3].c_str()),(char*)sp[1].c_str(),atoi(sp[4].c_str()),(char*)sp[2].c_str()))
					{
						Ch->SendMens("ReconnectCall;Ok\r\n");
					}
					else
					{
						Ch->SendMens("ReconnectCall;Error\r\n");
					}
				}
				else if(sp[0] == "tranfercall")
				{
					if(csta->TransferCall(atoi(sp[3].c_str()),(char*)sp[1].c_str(),atoi(sp[4].c_str()),(char*)sp[2].c_str()))
					{
						Ch->SendMens("TransferCall;Ok\r\n");
					}
					else
					{
						Ch->SendMens("TransferCall;Error\r\n");
					}
				}
				else if(sp[0] == "grouppickup")
				{
					if(csta->GroupPickup((char*)sp[1].c_str(),(char*)sp[2].c_str(),atoi(sp[3].c_str())))
					{
						Ch->SendMens("GroupPickup;Ok\r\n");
					}
					else
					{
						Ch->SendMens("GroupPickup;Error\r\n");
					}
				}
				else if(sp[0] == "conferencecall")
				{
					if(csta->ConferenceCall(atoi(sp[2].c_str()), (char*)sp[1].c_str(),atoi(sp[4].c_str()),(char*)sp[3].c_str()))
					{
						Ch->SendMens("ConferenceCall;Ok\r\n");
					}
					else
					{
						Ch->SendMens("ConferenceCall;Error\r\n");
					}
				}
				else if(sp[0] == "getagentstate")
				{
					string agentId;
					TAgentState agentState;
		
					if(csta->GetAgentState((char*)sp[1].c_str(),&agentId,&agentState))
					{
						string sMens = "GetAgentState;";
						switch(agentState)
						{
						case asAgentNotReady:
							sMens += "asAgentNotReady;"+agentId;
							break;
						case asAgentNull:
							sMens += "asAgentNull;"+agentId;
							break;
						case asAgentReady:
							sMens += "asAgentReady;"+agentId;
							break;
						case asAgentBusy:
							sMens += "asAgentBusy;"+agentId;
							break;
						case asAgentWorkingAfterCall:
							sMens += "asAgentWorkingAfterCall;"+agentId;
							break;
						}
						Ch->SendMens(sMens);
		
					}
					else
					{
						Ch->SendMens("GetAgentState;Error\r\n");
					}
				}
				else if(sp[0] == "setagentstate")
				{
					string sAcao;
					string sAgId = sp[3];
					string sDevice = sp[1];
					TReqAgentState agentState;
					switch(atoi(sp[2].c_str()))
					{
					case 0:
						sAcao = "reqAgentLoggedOn";
						agentState = reqAgentLoggedOn;
						break;
					case 1:
						sAcao = "reqAgentLoggedOff";
						agentState = reqAgentLoggedOff;
						break;
					case 2:
						sAcao = "reqAgentNotReady";
						agentState = reqAgentNotReady;
						break;
					case 3:
						sAcao = "reqAgentReady";
						agentState = reqAgentReady;
						break;
					case 4:
						sAcao = "reqAgentWorkingAfterCall";
						agentState = reqAgentWorkingAfterCall;
						break;
					}
		
					if(
							csta->SetAgentState(
									(char*)sDevice.c_str(),
									agentState,
									(char*)sAgId.c_str()
									)

					)
					{
						Ch->SendMens("SetAgentState;Ok\r\n");
					}
					else
					{
						Ch->SendMens("SetAgentState["+sAcao+"]["+sDevice+"]["+sAgId+"];Error\r\n");
					}
				}
				else if(sp[0] == "snapshotdevice")
				{
					TSnapshotDeviceResult resultData;
					resultData.Count = 0;
					string sMens = "SnapshotDevice;";
					if(csta->SnapshotDevice((char*)sp[1].c_str(),&resultData ))
					{
						csta->logCsta("SnapshotDevice RETORNANDO!!!");
						if(resultData.Count > 0)
						{
							csta->logCsta("TOTAL LIG:"+IntToStr(resultData.Count));
							for(int i = 0 ; i < resultData.Count;i++)
							{
								switch(resultData.snapshotDeviceItem[i].localCallState)
								{
								case csNull:
									sMens += "csNull;";
									break;
								case csInitiated:
									sMens += "csInitiated;";
									break;
								case csAlerting:
									sMens += "csAlerting;";
									break;
								case csConnected:
									sMens += "csConnected;";
									break;
								case csHold:
									sMens += "csHold;";
									break;
								case csQueued:
									sMens += "csQueued;";
									break;
								case csFail:
									sMens += "csFail;";
									break;
								case csConferenced:
									sMens += "csConferenced;";
									break;
								}
								sMens+=IntToStr(resultData.snapshotDeviceItem[i].connectionIdentifier.CallID);
								sMens+=";";
							}
							sMens+="\r\n";
							Ch->SendMens(sMens);
						}
						else
						{
							Ch->SendMens("SnapshotDevice;ListaVazia\r\n");
						}
					}
					else
					{
						Ch->SendMens("SnapshotDevice;Error\r\n");
					}
				}
				else if(sp[0] == "deflectcall")
				{
					if(csta->DeflectCall(atoi(sp[3].c_str()),(char*)sp[1].c_str(),(char*)sp[2].c_str()))
					{
						Ch->SendMens("DeflectCall;Ok\r\n");
					}
					else
					{
						Ch->SendMens("DeflectCall;Error\r\n");
					}
		
				}
				else if(sp[0] == "setdisplay")
				{
					if(csta->SetDisplay((char*)sp[1].c_str(),(char*)sp[2].c_str()))
					{
						Ch->SendMens("SetDisplay;Ok\r\n");
					}
					else
					{
						Ch->SendMens("SetDisplay;Error\r\n");
					}
				}
				else if(sp[0] == "generatedigits")
				{
					if(csta->GenerateDigits(atoi(sp[3].c_str()), (char*)sp[1].c_str(),(char *)sp[2].c_str()))
					{
						Ch->SendMens("GenerateDigits;Ok\r\n");
					}
					else
					{
						Ch->SendMens("GenerateDigits;Error\r\n");
					}
				}
				else if(sp[0] == "dialdigits")
				{
					if(csta->DialDigits(atoi(sp[3].c_str()), (char *)sp[1].c_str(), (char*)sp[2].c_str()))
					{
						Ch->SendMens("DialDigits;Ok\r\n");
					}
					else
					{
						Ch->SendMens("DialDigits;Error\r\n");
					}
				}
				else if(sp[0] == "consultationcall")
				{
					if(csta->ConsultationCall(atoi(sp[3].c_str()),(char *)sp[1].c_str(), (char *)sp[2].c_str()))
					{
						Ch->SendMens("ConsultationCall;Ok\r\n");
					}
					else
					{
						Ch->SendMens("ConsultationCall;Error\r\n");
					}
				}
				else if(sp[0] == "singlesteptransfer")
				{
					if(csta->SingleStepTransfer(atoi(sp[3].c_str()),(char *)sp[1].c_str(), (char *)sp[2].c_str()))
					{
						Ch->SendMens("SingleStepTransfer;Ok\r\n");
					}
					else
					{
						Ch->SendMens("SingleStepTransfer;Error\r\n");
					}
				}
				else if(sp[0] == "snapshotcall")
				{
					TSnapshotCallResult  resultData;
					resultData.Count = 0;
					if(csta->SnapshotCall((char *)sp[1].c_str(),atoi(sp[2].c_str()),&resultData))
					{
						csta->logCsta("SnapShotCall OK !!!!");
						string sMens = "SnapShotCall;";
						if(resultData.Count >0)
						{
							for(int i = 0; i < resultData.Count;i++)
							{
								sMens += resultData.snapshotCallItem[i].connectionIdentifier.DeviceID.DialingNumber +";";
								switch(resultData.snapshotCallItem[i].localCallState)
								{
								case csNull:
									sMens += "csNull;";
									break;
								case csInitiated:
									sMens += "csInitiated;";
									break;
								case csAlerting:
									sMens += "csAlerting;";
									break;
								case csConnected:
									sMens += "csConnected;";
									break;
								case csHold:
									sMens += "csHold;";
									break;
								case csQueued:
									sMens += "csQueued;";
									break;
								case csFail:
									sMens += "csFail;";
									break;
								case csConferenced:
									sMens += "csConferenced;";
									break;
								}
								sMens += IntToStr(resultData.snapshotCallItem[i].connectionIdentifier.CallID)+";";
							}
						}
						else
						{
							Ch->SendMens("SnapShotCall Lista Vazia\r\n");
						}
					}
					else
					{
						Ch->SendMens("SnapshotCall;Error\r\n");
					}
				}
				else if(sp[0] == "setforwarding")
				{
		
					if(csta->SetForwarding((char *)sp[1].c_str(),(char *)sp[2].c_str(),	atoi(sp[3].c_str()), atoi(sp[4].c_str())))
					{
						Ch->SendMens("SetForwarding;Ok\r\n");
					}
					else
					{
						Ch->SendMens("SetForwarding;Error\r\n");
					}
				}
				else if(sp[0] == "monitorstop")
				{
					if(csta->MonitorStop((char *)sp[1].c_str()))
					{
						Ch->SendMens("MonitorStop;Ok\r\n");
						CMySQL * dbIns = (CMySQL *)Ch->Parameter1;
						dbIns->ExecSql("DELETE FROM DEVICE_CSTA WHERE device = "+sp[1]+")");
		
					}
					else
					{
						Ch->SendMens("MonitorStop;Error\r\n");
		
					}
				}
				else
				{
					Ch->SendMens("Comando não definido["+sp[0]+"]:"+MensCpy+"\r\n");
				}
			}
			catch(...)
			{
		
			}
	}
}

void InicializarCsta(CCSTA * csta,CMySQL * tDb,string ip,int porta,string sVersion)
{
	int iVezes = gFrAlarme;
	int iTentativas = 0;
	CMail Mail;
	Mail.SetSmtpServer(gServerMail);
	Mail.SetPortSmtpServer(gPortMail);
	Mail.SetUser(gUserMail);
	Mail.SetPassw(gPasswMail);
	Mail.SetType(gTypeMail);
	Mail.SetFrom(gRemetenteMail);
	Mail.SetTo(gDestinatarioMail);
	

	
	tDb->SetDb("asterisk");
	//tDb->SetDb("CSTA");
	tDb->dbConnect();
	bool bAut = true;
	string sV = "";
	if(sVersion == "H4000V6")
	{
		bAut = false;
		sV = "v6.0";
		csta->SetHeaderSize(2);
	}
	else if(sVersion == "Panasonic")
	{
		bAut = false;
		sV = "Panasonic";
		csta->SetHeaderSize(2);
	}
	else if(sVersion == "H4000")
	{
		bAut = false;
		sV = "";
		csta->SetHeaderSize(2);
	}
	else
	{
		csta->SetHeaderSize(3);
	}

	if(bActMail)
	{
		csta->logCsta("Enviando alarme para "+gDestinatarioMail+"!!!");
		csta->logCsta("Frequencia:"+IntToStr(iVezes));
		Mail.SetSubject("Monitor CstaApp");
		Mail.SetText("Inicialização Conexao CSTA "+DateToStr(time(NULL)) +" "+ TimeToStr(time(NULL)) );
		if(!Mail.OpenComm())
		{
			csta->logCsta("Erro ao enviar alarme!!!");
			csta->logCsta(Mail.GetError());	
		}
	}

	while(!csta->CstaConnect(ip,porta,bAut,sV))
	{
		if( iVezes == -1)
		{
			if(bActMail)
			{
				csta->logCsta("Enviando alarme para "+gDestinatarioMail+"!!!");
				Mail.SetSubject("Monitor CstaApp");
				Mail.SetText("Tentativa # "+IntToStr(iTentativas++)+" de conexao CSTA "+DateToStr(time(NULL)) +" "+ TimeToStr(time(NULL)) );
				if(!Mail.OpenComm())
				{
					csta->logCsta("Erro ao enviar alarme!!!");
					csta->logCsta(Mail.GetError());	
				}
			}			
		}
		else if ( iVezes > 0)
		{
				iVezes--;
				if(bActMail)
				{
					csta->logCsta("Enviando alarme para "+gDestinatarioMail+"!!!");
					Mail.SetSubject("Monitor CstaApp");
					Mail.SetText("Tentativa # "+IntToStr(iTentativas++)+" de conexao CSTA "+DateToStr(time(NULL)) +" "+ TimeToStr(time(NULL)) );
					if(!Mail.OpenComm())
					{
						csta->logCsta("Erro ao enviar alarme!!!");
						csta->logCsta(Mail.GetError());	
					}
				}				
		}
		sleep(10);
	}
	
	if(bAut)
		csta->ActHeartBeat(30);
	TMonitorType mt = mtDevice;
	int id;

	if(tDb->ExecSql("SELECT DEVICE FROM DEVICE_CSTA"))
	{
		do
		{
			csta->logCsta(string("Ativando monitoracao device ")+ tDb->operator [](0).c_str());
			csta->MonitorStart((char*)tDb->operator [](0).c_str(),mt,&id);
			CallList->AddMonitorItem(tDb->operator [](0).c_str(),id,mtDevice);
		}
		while(tDb->Next());
	}
	if(bActMail)
	{
		csta->logCsta("Enviando alarme para "+gDestinatarioMail+"!!!");
		Mail.SetSubject("Monitor CstaApp");
		Mail.SetText("Csta conectado com sucesso ao Pabx "+DateToStr(time(NULL)) +" "+ TimeToStr(time(NULL)) );
		if(!Mail.OpenComm())
		{
			csta->logCsta("Erro ao enviar alarme!!!");
			csta->logCsta(Mail.GetError());	
		}
	}
}
void  DiscCsta(void * arg)
{
	stDiscInfo * DiscInfo = (stDiscInfo *)arg;
	DiscInfo->csta->logCsta("CstaApp perdeu conexao pabx.Reiniciando!!!");
	InicializarCsta(DiscInfo->csta,DiscInfo->tDb,DiscInfo->sEnd,DiscInfo->iPorta,DiscInfo->sVersion);

}

int main(int argc,char*argv[])
{
	GravarEv = 1;
	bActMail = false;
	///////////////////////////////////////////////////////////////////////
	/* The service create
	 * Reference
	 * http://www.netzmafia.de/skripten/unix/linux-daemon-howto.html
	 * Steps to service create
	 *
	 * -Fork off the parent process
	 * -Change file mode mask (umask)
	 * -Open any logs for writing
	 * -Create a unique Session ID (SID)
	 * -Change the current working directory to a safe place
	 * -Close standard file descriptors
	 * -Enter actual daemon code
	 *
	 *
	 * */

	//testes de função lowercase
	if(argc > 2 && string(argv[1]) == "-lowercase")
	{
		cout << lowercase(argv[2]) << endl;
		return 0;
	}
	//teste de TimeMaiorADiffB
	else if(argc > 2 && string(argv[1]) == "-timediff") 
	{
		cout << "Iniciando teste timediff" << endl;
		time_t tBegin = time(NULL);
		int iCont = 0;
		char sInt[10];
		while(1)
		{
				if(TimeMaiorADiffB(atoi(argv[2]),time(NULL),tBegin))
				{
					sprintf(sInt,"%d",iCont);
					cout << "Maior" << sInt << endl;
				}
				else
				{
					sprintf(sInt,"%d",iCont);
					cout << "Menor" << sInt << endl;
				}
				sleep(5);
				iCont+=5;
		}
	}
	//teste de email
	else if(argc == 11 && string(argv[1]) == "-testmail")
	{
		cout << "Testando Email!!!" << endl;
		CMail Mail;
		while(1)
		{
			//./CstaApp -testmail pop.gmail.com 25 marcelodossantosmariano@gmail.com senha 0 marcelodossantosmariano@gmail.com marcelodossantosmariano@gmail.com "Teste de email" "testando email" 
			Mail.SetSmtpServer(argv[2]);
			Mail.SetPortSmtpServer(atoi(argv[3]));
			Mail.SetUser(argv[4]);
			Mail.SetPassw(argv[5]);
			Mail.SetType(atoi(argv[6]));
			Mail.SetFrom(argv[7]);
			Mail.SetTo(argv[8]);
			Mail.SetSubject(argv[9]);
			Mail.SetText(argv[10]);
			
			if(!Mail.OpenComm())
			{
				cout << Mail.GetError() << endl;
			}
			else
			{
				cout << "Email enviado!!!" << endl;		
			}		
			sleep(10);
		}
		return 0;
	}
	//install monitoração
	else if(argc > 1 && string(argv[1]) == "-installmon")
	{
		FILE * arq;
		//Pega caminho da app
		string sPathexe = argv[0];
		CSplit sp;
		int total = sp.Exec(sPathexe,"/");
		sPathexe = "";
		for(int i = 0; i < total-1;i++)
		{
			sPathexe += sp[i]+"/";
		}
		string arqmon = sPathexe+"moncsta";
		
		string script =
				"#!/bin/sh \n"
				"# /etc/init.d/cstaserve \n"
				"# Author: Marcelo Mariano \n"
				"# \n"
				"start() { \n"
				"nohup "+arqmon+" > foo.out 2> foo.err < /dev/null &  \n"
				"} \n"
				"stop() { \n"
				" kill -9 `pgrep moncsta` 2>/dev/null \n"
				"} \n"
				"case $1 in \n"
				" start) \n"
				"   start \n"
				" ;; \n"
				" stop)\n"
				"   stop \n"
				" ;; \n"
				" restart) \n"
				"   stop \n"
				"   sleep 1 \n"
				"   start \n"
				" ;; \n"
				" *) \n"
				" echo \"Usage: $0 start|stop|restart\" \n"
				" exit 1 \n"
				" ;; \n"
				" esac \n";
		arq = fopen(string(sPathexe+"cstaservemon").c_str(),"w");
		if ( arq != NULL)
		{
			fputs(script.c_str(),arq);
			fclose(arq);
		}
		return 0;

	}	
	//teste de acesso MySQL
	else if(argc == 2 && string(argv[1]) == "-testmysql")
	{			
	}
	//Instalacao e configuração de servico
	else if(argc > 1 && string(argv[1]) == "-install")
	{
		char ip[50]="";
		char porta[10]="";
		char portaap[10]="";
		char vl[2]="";
		char logs[2]="";
		char servico[2] = "";
		char user[100]="";
		char pass[100]="";

		string config;
		FILE * arq;
		string script =
				"#!/bin/sh \n"
				"# /etc/init.d/cstaserve \n"
				"# Author: Marcelo Mariano \n"
				"# \n"
				"start() { \n"
				""+string(argv[0])+" -d \n"
				"} \n"
				"stop() { \n"
				" kill -9 `pgrep CstaApp` 2>/dev/null \n"
				"} \n"
				"case $1 in \n"
				" start) \n"
				"   start \n"
				" ;; \n"
				" stop)\n"
				"   stop \n"
				" ;; \n"
				" restart) \n"
				"   stop \n"
				"   sleep 1 \n"
				"   start \n"
				" ;; \n"
				" *) \n"
				" echo \"Usage: $0 start|stop|restart\" \n"
				" exit 1 \n"
				" ;; \n"
				" esac \n";

		//Pega caminho da app
		sPathexe = argv[0];
		CSplit sp;
		int total = sp.Exec(sPathexe,"/");
		sPathexe = "";
		for(int i = 0; i < total-1;i++)
		{
			sPathexe += sp[i]+"/";
		}


		cout << "Criando arquivo de configuração:" << endl;
		cout << "Digite o endereco ip de acesso ao pabx:" << endl;
		cin.getline(ip,50);
		config+="IP="+string(ip)+"\n";
		cout << "Digite a porta de acesso ao pabx:" << endl;
		cin.getline(porta,10);
		config+="PORTA="+string(porta)+"\n";
		cout << "Digite a porta de acesso a aplicacao:" << endl;
		cin.getline(portaap,10);
		config+="PORTAAPP="+string(portaap)+"\n";
		cout << "Deseja gerar logs?[Y|N]:" << endl;
		cin.getline(logs,2);

		cout << "Usuario do MySQL:" << endl;
		cin.getline(user,100);
		config+="MYUSER="+string(user)+"\n";

		cout << "Senha do MySQL:" << endl;
		cin.getline(pass,100);
		config+="MYPASS="+string(pass)+"\n";


		config+="LOGS="+string(logs)+"\n";
		cout << "Deseja visualizar eventos na tela logs?[Y|N] digite N se rodara em servico:" << endl;
		cin.getline(vl,2);
		config+="VL="+string(vl)+"\n";
		cout << "Criar servico para esta aplicacao[Y|N]" << endl;
		cin.getline(servico,2);

		arq = fopen(string(sPathexe+"config.txt").c_str(),"w");
		if ( arq != NULL)
		{
			fputs(config.c_str(),arq);
			fclose(arq);
		}



		if(servico[0] == 'Y')
		{
			cout << "Criando script de controle start|stop" << endl;
			arq = fopen(string(sPathexe+"cstaserve").c_str(),"w");
			if ( arq != NULL)
			{
				fputs(script.c_str(),arq);
				fclose(arq);
			}

			execl("/bin/update-rc.d","-f","cstaserve","remove",(char*)0);
			if(!fork())
			{
				close(0); close(1); close(2);
				execl("/bin/rm","-f","/etc/init.d/cstaserve",(char*)0);
			}
			if(!fork())
			{
				close(0); close(1); close(2);
				execl("/bin/ln","-T",string(sPathexe+"cstaserve").c_str(),"-s",string("/etc/init.d/cstaserve").c_str(),(char*)0);
			}
			if(!fork())
			{
				close(0); close(1); close(2);
				execl("/bin/update-rc.d","cstaserve","defaults",(char*)0);
			}
		}
		return 0;
	}
	else if(argc > 1 && string(argv[1]) == "-d")
	{
		/* Our process ID and Session ID */
		pid_t pid, sid;

		/* Fork off the parent process */
		pid = fork();
		if (pid < 0) {
			exit(EXIT_FAILURE);
		}
		/* If we got a good PID, then
		           we can exit the parent process. */
		if (pid > 0) {
			exit(EXIT_SUCCESS);
		}

		/* Change the file mode mask */
		umask(0);

		/* Open any logs here */

		/* Create a new SID for the child process */
		sid = setsid();
		if (sid < 0) {
			/* Log the failure */
			exit(EXIT_FAILURE);
		}
		/* Change the current working directory */
		if ((chdir("/")) < 0) {
			/* Log the failure */
			exit(EXIT_FAILURE);
		}

		/* Close out the standard file descriptors */
		close(STDIN_FILENO);
		close(STDOUT_FILENO);
		close(STDERR_FILENO);

	}
	else if(argc > 1 && string(argv[1]) == "-a")
	{
			cout << "Iniciando CstaApp como Aplicação!!!" << endl;
	}
	else
	{
		cout << "CstaApp - Middleware" << endl;
		cout << "By Marcelo dos Santos Mariano" << endl;
		cout << "Parametros invalidos"<< endl;
		cout << "Candidatos sao:" << endl;
		cout << "-d Inicia aplicacao em background" << endl;
		cout << "-a inicia aplicacao normalmente" << endl;
		cout << "-install configuracao da aplicacao" << endl;
		return 0;
	}
	///////////////////////////////////////////////////////////////////////

	CCSTA csta;
	CServerMensParent MensServer;
	CMySQL tDb;
	CMySQL CMySQL_Mens;
	string ip,sUser,sPass,sVersion;
	int porta;
	int portaapp=34000;
	bool bCrDb = false;
	vlogs = glogs = false;
	for(int i = 0 ; i < 14 ; i++)
		gEvMask[i] = '1';



	//Pega caminho da app
	sPathexe = argv[0];
	CSplit sp;
	int total = sp.Exec(sPathexe,"/");
	sPathexe = "";
	for(int i = 0; i < total-1;i++)
	{
		sPathexe += sp[i]+"/";
	}
	if(argc == 2 && string(argv[1]) == "-vl")
	{
		bVlG = true;
		cout << "App path:" << sPathexe << endl;
	}

	/*
	 * Vefirica se deve carregar configuracoes do arquivo
	 * */
	if(argc == 2 || argc == 1)
	{
		const int MAX=80;
		char buff[MAX];
		ifstream fin(string(sPathexe +"config.txt").c_str());

		if(!fin)
		{
			//se visualizar logs de inicializacao
			if(argc == 2 && string(argv[1]) == "-vl")
			{
				bVlG = true;
				cout << "!!!Configuracao nao encontrada!!!" << endl;
			}
			return 0;
		}

		/*
		 * busca configuracoes linha por linha separado por =
		 * */
		while(fin)
		{
			fin.getline(buff, MAX);
			//se visualizar logs

			CSplit sp;
			sp.Exec(buff,"=");

			if(sp[0] == "IP") //endereço socket csta
			{
				ip=sp[1];
			}
			//CONFIGURAÇÃO ALARMES POR EMAIL
			else if(sp[0] == "EMAILACT")
			{
				if(sp[1] == "1")
					bActMail = true;
			}
			else if(sp[0] == "SERVERMAIL")
			{
				gServerMail = sp[1];				
			}
			else if(sp[0] == "PORTMAIL")
			{
				gPortMail = atoi(sp[1].c_str());
			}
			else if(sp[0] == "USERMAIL")
			{
				gUserMail =  sp[1];				
			}
			else if(sp[0] == "PASSMAIL")
			{
				gPasswMail = sp[1];		
			}
			else if(sp[0] == "TYPEMAIL")
			{
				gTypeMail = atoi(sp[1].c_str());	
			}
			else if(sp[0] == "REMETENTEMAIL")
			{
				gRemetenteMail = sp[1];
			}
			else if(sp[0] == "DESTINATARIOMAIL")
			{
				gDestinatarioMail = sp[1];
			}
			else if(sp[0] == "FREQALARME" )
			{
				gFrAlarme = atoi(sp[1].c_str());
			}
			////////////////////////////
			else if(sp[0] == "MYUSER")
			{
				sUser = sp[1];
			}
			else if(sp[0] == "MYPASS")
			{
				sPass = sp[1];
			}
			else if(sp[0] == "VERSION")
			{
				sVersion = sp[1];
			}
			else if(sp[0] == "PORTAAPP")
			{
				portaapp = atoi(sp[1].c_str());
			}
			else if(sp[0] == "EVMASK")
			{
				//evServiceInitiated[0|0|1];evFailed[1|0|1];evConnectionCleared[2|0|1];evDelivered[3|0|1];evEstablished[4|0|1];
				//evQueued[5|0|1];evHeld[6|0|1];evRetrieved[7|0|1];evTransferred[8|0|1];evOriginated[9|0|1];evNetworkReached[10|0|1];
				//evDiverted[11|0|1];evConferenced[12|0|1];
				CSplit spPar;
			  int total = spPar.Exec(sp[1],";");
			  //gEvMask
			  for(int i = 0 ; i < total && i < 14; i++)
			  	gEvMask[i] = spPar[i].c_str()[0];				
			  //teste de confg.
			  //cout << "Mascara de Eventos:" << gEvMask << endl;
			}
			else if(sp[0] == "PORTA")//porta csta
			{
				porta = atoi(sp[1].c_str());
			}
			else if(sp[0] == "GRAVAREV")//porta csta
			{
				GravarEv = atoi(sp[1].c_str());
			}
			else if(sp[0]=="VL")//visualizar log
			{
				if(sp[1] == "Y")
				{
					vlogs = true;
					//se visualizar logs de inicializacao
					if(argc == 2 && string(argv[1]) == "-vl")
					{
						bVlG = true;
						cout << "Visualizacao de logs ativada" << endl;
					}
				}
			}
			else if(sp[0] == "LOGS")//gravar logs
			{
				if(sp[1] == "Y")
				{
					glogs = true;
					//se visualizar logs de inicializacao
					if(argc == 2 && string(argv[1]) == "-vl")
					{
						bVlG = true;
						cout << "Gravacao de logs ativada" << endl;
					}

				}
			}
			//se visualizar logs de inicializacao
			if(argc == 2 && string(argv[1]) == "-vl")
			{
				bVlG = true;
				cout << buff << endl;
			}
		}
	}
	/*
	 * se configuracao por parametros*/
	else if(argc >= 3)
	{
		ip 	  = argv[1];
		porta = atoi(argv[2]);
	}

	//Configura logs
	csta.SetLogs(vlogs,glogs,sPathexe);




	csta.logCsta("==================================================================");
	csta.logCsta("                           CSTA APP(1.0.0.4)                               ");
	csta.logCsta("==================================================================");

	csta.logCsta(sUser);
	csta.logCsta(sPass);

	tDb.SetAcess(sUser,sPass,"127.0.0.1");
	while(!tDb.dbConnect())
	{
		csta.logCsta(string("Aguardando conectar ao Mysql...")+tDb.GetError());
		sleep(10);
	}
	//else

	MensServer.object = &csta;
	MensServer.cbRead = ReadMens;
	if(!MensServer.Open(portaapp))
	{
		csta.logCsta("Erro ao inciar servidor de mensagens!");
		exit(1);
	}
	else
		csta.logCsta("Servidor de mensagens iniciado com sucesso!");


	csta.logCsta("!!!Connect Mysql Sucess!!!");
	if(!tDb.ExecSql("SHOW DATABASES"))
	{
		csta.logCsta(string("Erro ao listar databases - ")+tDb.GetError());
		return -1;
	}
	else
	{
		csta.logCsta("Encontrado(s) " + csta.IntToStr(tDb.GetRowsNumber())+string(" databases"));
		do
		{
			if(tDb.case_insensitive_compare(tDb[0],string("asterisk")))
				bCrDb = true;
			csta.logCsta(string("       -")+tDb[0]);
		}
		while(tDb.Next());
	}
	/*
	 * Criando base de dados e tabela
	 * Caso nao existam
	 * */
	if(!bCrDb&&!tDb.ExecSql("CREATE DATABASE asterisk"))
	{
		csta.logCsta(string("Erro criar databases - ")+tDb.GetError());
	}
	/*
	 * Verifica tabela(s)
	 *
	 * */
	bCrDb = false;
	if(!tDb.ExecSql("SHOW TABLES FROM asterisk"))
	{
		do
		{
			if(tDb.case_insensitive_compare(tDb[0],string("csta")))
				bCrDb = true;
			csta.logCsta(string("       -")+ tDb[0]);
		}
		while(tDb.Next());

	}
	/*
	 * Table nao existe...criando
	 * */
	if(!bCrDb)
	{
		tDb.SetDb("asterisk");
		//tDb.SetDb("CSTA");
		tDb.dbConnect();
		if(!tDb.ExecSql("CREATE TABLE IF NOT EXISTS `CSTA` ( "
				"`id` int(11) NOT NULL AUTO_INCREMENT,"
				"`Data` timestamp NULL DEFAULT NULL,"
				"`Hora` time DEFAULT NULL,"
				"`dur` time DEFAULT NULL ,"
				"`callid` int(11) NOT NULL,"
				"`evento` varchar(30) NOT NULL,"
				"`ramal_ori` varchar(10) NOT NULL,"
				"`ramal_des` varchar(10) NOT NULL,"
				"`num_ori` varchar(30) NOT NULL,"
				"`num_des` varchar(30) NOT NULL,"
				"`device` varchar(30) NOT NULL,"
				"PRIMARY KEY (`id`),"
				"UNIQUE KEY `id` (`id`) "
				") ENGINE=InnoDB DEFAULT CHARSET=latin1;"))
		{
			csta.logCsta(string("Erro criar tabela csta - ")+tDb.GetError());
		}

	}
	bCrDb = false;
	if(!tDb.ExecSql("SHOW TABLES FROM asterisk"))
	{
		do
		{
			if(tDb.case_insensitive_compare(tDb[0],string("DEVICE_CSTA")))
				bCrDb = true;
			csta.logCsta(string( "       -")+tDb[0]);
		}
		while(tDb.Next());

	}
	if(!bCrDb)
	{
		tDb.SetDb("asterisk");
		//tDb.SetDb("CSTA");
		tDb.dbConnect();
		if(!tDb.ExecSql("CREATE TABLE IF NOT EXISTS DEVICE_CSTA ( "
				"ID int(11) NOT NULL AUTO_INCREMENT,"
				"device varchar(30) NOT NULL, "
				"PRIMARY KEY (ID) "
				") ENGINE=InnoDB DEFAULT CHARSET=latin1 AUTO_INCREMENT=1 "))
		{
			csta.logCsta(string("Erro criar tabela device - ")+ tDb.GetError());
		}
	}



	if(!tDb.ExecSql("CREATE TABLE IF NOT EXISTS CSTA_FULL LIKE CSTA"))
	{
		csta.logCsta(string("Erro criar tabela csta_full - ")+ tDb.GetError());
	}

	CallList = new TCallList;
	CallList->RegisterEndCallHandler(BkEnd,NULL);
	csta.InitThread();
	//Registra Acesso MySQL para registros
	CMySQL_Mens.SetAcess("root","mypass","127.0.0.1");
	CMySQL_Mens.SetDb("asterisk");
	//CMySQL_Mens.SetDb("CSTA");
	CMySQL_Mens.dbConnect();
	MensServer.Parameter1 = &CMySQL_Mens;
	csta.RegisterCallControlHandler(CallBackCallControl,&MensServer);
	DiscInfo.iPorta = porta;
	DiscInfo.sEnd = ip;
	DiscInfo.tDb = &tDb;
	DiscInfo.csta = &csta;
	DiscInfo.sVersion = sVersion;
	csta.RegisterDisconnectHandler(DiscCsta,&DiscInfo);
	InicializarCsta(&csta,&tDb,ip,porta,sVersion);
    string sDataAtual = DateToStr(time(NULL));
    //expurgo inicial;
    ExpurgarTabelaCsta(tDb,csta);
	while(1)
	{
      if (sDataAtual != DateToStr(time(NULL)))
      {
          ExpurgarTabelaCsta(tDb,csta);
    	  sDataAtual = DateToStr(time(NULL));
      }
	  sleep(30);
	}

	csta.logCsta("==================================================================");
	csta.logCsta("                               FIM                                ");
	csta.logCsta("==================================================================");

	return 1;
}
