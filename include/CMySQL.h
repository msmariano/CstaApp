//============================================================================
// Name        : CMySQL.cpp
// Author      : Marcelo dos Santos Mariano (MSM).
// Version     : 1.0.0.0 - Linux
// Copyright   : Marcelo Mariano
// Description : Classe para acesso ao Banco de Dados MySQL
//               11-12-2012 - Cria��o
//               08-02-2013 - Inserido na funcao dbConnect opcao para reconexao automatica
//============================================================================

#include "../../mysql_c/include/mysql.h"
#include <iostream>
#include <stdio.h>
//Instalacao : sudo apt-get install libmysqlclient-dev
class CMySQL
{
public:
	CMySQL()
	{
		//CLEANUP DAS VARIAVEIS
		myData          = NULL;
		bDbconectado    = false;
		bReconectar             = false;
		myFields        = NULL;
		myRes           = NULL;
		myRow           = NULL;
		iTotFields      = 0;
		iLinhasAfetadas = 0;
		iRet            = 0;
		lNresultados    = 0;
	}
	~CMySQL()
	{

	}
	void SetAcess(string user,string pass,string host = "127.0.0.1")
	{
		sUser = user;
		sPass = pass;
		sHost = host;
	}
	bool dbConnect()
	{
		try
		{
			bDbconectado=false;
			//INICIA MYDATA
			myData = mysql_init(NULL);
			mysql_options(myData,MYSQL_OPT_RECONNECT,"true");
			//SE SUCESSO CONECTA
			if(myData)
			{
				//FUNCAO DE CONEXAO
				if( mysql_real_connect(myData,sHost.c_str(), sUser.c_str(),sPass.c_str(), sDbase.c_str(),
						MYSQL_PORT, NULL, 0))
				{
					//SE SUCESSO
					bDbconectado = true;
					if (bReconectar)
						myData->reconnect = true;
					return true;
				}
			}

			// SE ERRO PEGO O ERRO DO MYSQL
			sMensError = mysql_error(myData);
		}//SE ERRO GERAL PEGO O ERRO DO SISTEMA
		catch (...)
		{
		}
		bDbconectado = false;
		return false;

	}
	void SetHost(string Arg)
	{
		sHost=Arg;
	}
	void SetDb(string Arg)
	{
		sDbase = Arg;
	}
	string GetDb()
	{
		return sDbase;
	}
	string GetError()
	{
		return sMensError;
	}
	bool ExecSql(string sQuery)
	{
		try
		{
			///////////////////////////////////////////////////////////
			//EXECUTO A QUERY NO MYSQL
			if(!mysql_query(myData,sQuery.c_str()))
			{
				//tento armazenar o resultado de um possivel select
				if((myRes = mysql_store_result(myData))!= NULL)
				{
					//TOTAL DE CAMPOS ENCONTRADOS
					iTotFields =mysql_num_fields(myRes);
					//resultados foram encontrados
					if(myRes->row_count )
					{
						//TOTAL DE RESULTADOS
						lNresultados = (long)myRes->row_count;
					}
					//QUERY TEVE SUCESSO MAIS NAO HAVIAM RESULTADOS
					else
					{
						lNresultados = 0;
					}
					return true;
				}
				else//SE CAIR AQUI POSSIVELMENTE ERA ALGO DIFERENTE DE UM SELECT
				{
					//SE NAO OCORREU UM ERRO
					if((iRet = mysql_errno(myData))== 0)
					{
						//PEGO O NUMERO DE LINHAS AFETADOS POR EXEMPLO DE UM UPDATE
						iLinhasAfetadas = (int)mysql_affected_rows(myData);
						return true;
					}
					else //SE OCORREU UM ERRO PEGO O ERRO E RETORNO
					{
						sMensError = mysql_error(myData);
						return false;
					}
				}
			}
			else//SE OCORREU UM ERRO NA QUERY VOU PEGAR O ERRO E RETORNAR FALSE
			{

				sMensError = mysql_error(myData);
				//NUMERO DO ERRO
				iRet = mysql_errno(myData);

			}
		}//UMA EXCESSAO OCORREU NO SISTEMA
		catch (...)
		{
			iRet = -1;
		}
		return false;
	}
	bool Next()
	{
		if((myRow = mysql_fetch_row(myRes))!=NULL)
			return true;
		else
			return false;
	}
	string operator [](int pos)
	{
		//POS NAO PODE SER MAIOR QUE O NUMERO DE CAMPOS
		//SE NAO RETORNA NULL
		if(pos > iTotFields)
			return "";
		//VOU TENTAR PEGAR O VALOR DO CAMPO NA LINHA
		try
		{
			//A LINHA NAO FOI INICIALIZADA
			if(!myRow)
			{
				//PEGO A LINHA
				if((myRow = mysql_fetch_row(myRes))!= NULL)
					//E RETORNO O VALOR DO CAMPO
					return myRow[pos];
				else//SE FALHOU
					//RETORNO NULL
					return "";
			}//A LINHA JA TINHA SIDO INICIALIZADA
			else //RETORNO O VALOR DO CAMPO
				return myRow[pos];
		}//OCORREU UM ERRO NO SISTEMA
		catch (...)
		{
		}
		return "";
	}
	int GetRowsNumber()
	{
		return lNresultados;
	}
	bool case_insensitive_compare( string a, string b) {
		char char1, char2, blank = ' ' ;
		int len1 = a.length() ;
		int len2 = b.length() ;

		for (int i = 0 ; i < min(len1, len2) ; ++i ) {
			char1 = *(a.substr(i,1).data()) ;   // get a single character
			char2 = *(b.substr(i,1).data()) ;
			char1 |= blank ;   // make lowercase for compare
			char2 |= blank ;
			if ( char1 == char2 )
				continue ;
			return false ;
		}
		if (len1 != len2)
			return false ;
		return true ;
	}

private:
	MYSQL_FIELD  * myFields;
	MYSQL_RES    * myRes;
	MYSQL_ROW      myRow;
	MYSQL * myData;
	string sDbase;
	string sQueryInfo;
	bool bDbconectado;
	bool bReconectar;
	string sMensError;
	string sHost;
	int iTotFields;
	int iLinhasAfetadas;
	int iRet;
	long lNresultados;
	string sUser;
	string sPass;

};
