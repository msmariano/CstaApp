/*
 * main.cpp
 *
 *  Created on: 26 de jan de 2017
 *      Author: msmariano
 */
#include <iostream>
#include <sstream>
#include <string.h>
#include <stdio.h>
#include <vector>
#include <fstream>

using namespace std;

int StrHexToInt(char * byte);

class CSplit
{
public:
	CSplit()
	{
		size = 0;
	}
	~CSplit()
	{

	}
	string operator[](int position)
	{
		if(position<size)
		{
			return string(fields[position].c_str());
		}
		else
		{
			return "";
		}
	}
	int Parser(string line,string separator,string block)
	{
		//<cstatest>simulacao;1;2;3;4;5</cstatest>
		string sLeft  = "<"+block+">";
		string sRight = "</"+block+">";
		string sParameter;
		int posRight = 0;
		int posLeft  = 0;
		//cout << line << endl;
		//cout << sLeft << endl;
		//cout << sRight << endl;
		if((posLeft=line.find(sLeft))>-1)
		{
			if((posRight=line.find(sRight,posLeft))>-1)
			{
				posLeft = posLeft+sLeft.size();
				//printf("posLeft:%d\r\n",posLeft);
				//printf("posRight:%d\r\n",posRight);
				sParameter = line.substr(posLeft,posRight-posLeft);
				//cout << sParameter << endl;
				return Exec(sParameter,separator);
			}
		}
		return 0;
	}
	int Exec(string line,string separator)
	{
		int pos  =  0;

		while((pos=line.find(separator,0))>-1)
		{
			string sTemp;
			sTemp = line.substr(0,pos);
			line  = line.substr(pos+1,line.size());
			fields.push_back(sTemp);
		}
		fields.push_back(line);
		size = fields.size();
		return size;
	}
	int GetSize()
	{
		return size;
	}
private:
	int size;
	vector<string> fields;
};

class CAsn
{

public:
	CAsn()
	{
		next = NULL;


	}
	~CAsn()
	{
		if(next != NULL)
		{
			delete next;
		}

	}


private:
		CAsn * next;



};

int main (int argc , char * argv[])
{
	const int MAX=1024;
	string buff;
	CSplit sp;
	/*
	A1 78
		02 01 02
		02 01 15
		30 70
		55 04 01 11 00 23
		A0 68
		   A4 66
		   6B 12
		   	  30 10
		   	  80 04 30 30 46 34
		   	  A1 08
		   	  	  30 06
		   	  	  	  80 04 37 30 34 39
		   	  	  	  63 08
		   	  	  	  	  30 06
		   	  	  	  	  	  80 04 37 30	34 39
		   	  	  	  61 08
		   	  	  	  	  30 06
		   	  	  	  	  	  81 04 01 55 00 00
		   	  	  	 62 08 30 06
		   	  	  	 	 	  80 04 37 30 34 39
		   	  	  	 64 02 88 00

		   	  	  	 4E 01 02
		   	  	  	 0A 01 16
					7E 28
					A1 26
					A6 11
					A6 0F
					30 0D
						86 0B 4E 34 31 33 36 36 31 37 30 32 34
					A6 11
					B1 0F 30 0D
					    86 0B 4E 3C 37 30 34 39 3E 37 30 34 39 	*/
	//if(argc == 2 && string(argv[1]) == "-hex")
	{
		ifstream fin(string("/home/msmariano/CstaApp/TesteASN/Debug/hex.txt").c_str());

		 char c;
		  while (fin.get(c))          // loop getting single characters
		    buff = buff + c;

	}

	//CAsn * teste = new CAsn();


	vector <unsigned char> vPayload;
	sp.Exec(buff," ");
	for (int i = 0 ; i < sp.GetSize();i++)
	{
		vPayload.push_back(StrHexToInt((char*)sp[i].c_str()));
		//teste->push(teste->StrHexToInt((char*)sp[i].c_str()));
	}

	for (int i = 0 ; i < vPayload.size();)
	{
		unsigned char valor = vPayload[i];
		unsigned char size  = vPayload[i+1];
		if( (vPayload[i] <= 137 && vPayload[i] >= 128) || vPayload[i] == 2 ||  vPayload[i] == 85)
		{

			/*string s;
			for(int j=0;j< vPayload[i+1];j++)
			{
				s+= vPayload[j+i+2];
				valor = vPayload[j+i+2];
				if ((vPayload[i] <= 137 && vPayload[i] >= 128) )
				{
					s = "";
					s+=(char)vPayload[j+i+2];
					//cout << s ;
				}
				//else
					//cout << valor;

			}
			//cout << endl;*/
			//printf("Dado: %x %x\n", valor,size);

			if((valor >= 0x80 && valor <= 0x89))
			{
				for (int k =0; k < size;k++)
				{
					cout <<  (char)vPayload[i+2+k];
				}
					cout << endl;
			}

			i = i + 2 + vPayload[i+1];
		}
		else
		{
			//printf("Estrutura: %x %x\n", valor,size);
			//cout << "type: " << valor << endl;
			//cout << "size: " << size << endl;

			i+=2;
		}

	}


    //teste->parser();

	//delete teste;

	cout << "Teste ASN" << endl;
	return 0;
}


int StrHexToInt(char * byte)
{
	unsigned int x;
	stringstream ss;
	ss << std::hex << byte;
	ss >> x;
    return (int)x;
}




