//============================================================================
// Name        : CCB64.h
// Author      : Marcelo dos Santos Mariano
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================


#include <iostream>
#include <stdio.h>
#include <string.h>
#include <vector>

//CONSTANTES PARA CODIFICACAO EM B64
static const char cd64[]="|$$$}rstuvwxyz{$$$$$$$>?@ABCDEFGHIJKLMNOPQRSTUVW$$$$$$XYZ[\\]^_`abcdefghijklmnopq";
static const char cb64[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";


using namespace std;


class CCB64
{
public:
	CCB64()
	{
		Count=0;
		infile = 0;
		infileD = "";
		outfile = "";
		outD.clear();
		bInstInf = false;
	}
	~CCB64()
	{
		if(bInstInf)
			delete infile;
	}
	void decodeblock( unsigned char in[4], unsigned char out[3] )
	{
		try
		{
			out[ 0 ] = (unsigned char ) (in[0] << 2 | in[1] >> 4);
			out[ 1 ] = (unsigned char ) (in[1] << 4 | in[2] >> 2);
			out[ 2 ] = (unsigned char ) (((in[2] << 6) & 0xc0) | in[3]);
		}
		catch(...)
		{
		}
	}
	void decode(void )
	{
		try
		{
			outD.clear();
			char in[4], out[4],v;
			int i,len,pos=0,pos1=0;
			infileD += "==";
			//cout << infileD << endl;
			memset(in,0,4);
			memset(out,0,4);
			do
			{
				for( len = 0, i = 0; i < 4 && infileD[pos]; i++)
				{
					v = 0;
					while( pos < (int)infileD.size() && v == 0 )
					{
						v = (unsigned char) infileD[pos++];
						v = (unsigned char) ((v < 43 || v > 122) ? 0 : cd64[ v - 43 ]);
						if( v )
						{
							v = (unsigned char) ((v == '$') ? 0 : v - 61);
						}
					}
					if( pos < (int)infileD.size() )
					{
						len++;
						if( v )
						{
							in[ i ] = (unsigned char) (v - 1);
						}
					}
					else
					{
						in[i] = 0;
					}
				}
				if( len )
				{
					decodeblock((unsigned char *)in,(unsigned char *)out );
					for( i = 0; i < len - 1; i++ ,pos1++)
					{
						char p[2] = "";
						p[0] = out[i];
						//cout << p[0] ;
						outD.push_back(p[0]);
					}
					memset(in,0,3);
					memset(out,0,4);
				}
			}
			while( pos < (int)infileD.size() );
			//cout << endl;
		}
		catch(...)
		{
		}
	}
	string GetRet()
	{
		string ret;
		for(unsigned i=0;i < outD.size();i++)
		{
			ret += outD[i];
		}

		return ret;
	}
	void encodeblock(unsigned char in[4], unsigned char out[4],int len  )
	{
		try
		{
			out[0] = cb64[ in[0] >> 2 ];
			out[1] = cb64[ ((in[0] & 0x03) << 4) | ((in[1] & 0xf0) >> 4) ];
			out[2] = (unsigned char) (len > 1 ? cb64[ ((in[1] & 0x0f) << 2) | ((in[2] & 0xc0) >> 6) ] : '=');
			out[3] = (unsigned char) (len > 2 ? cb64[ in[2] & 0x3f ] : '=');
		}
		catch(...)
		{
		}
	}
	void encode( void )
	{
		try
		{
			char in[3], out[4];
			int i,len,blocksout,j=0,pos=0,linesize=72;
			while( pos < Count )
			{
				len = 0;
				for( i = 0; i < 3 ; i++,pos++ )
				{
					in[i] = (unsigned char) infile[pos];
					if( pos < Count )
					{
						len++;
					}
					else
					{
						in[i] = 0;
					}
				}
				if( len )
				{
					encodeblock((unsigned char *)in,(unsigned char *)out,len );
					for( i =0; i+j < j+4; i++ )
					{
						outfile += out[i];
						if((outfile.size()%77) == 0 )
						outfile += "\r\n";
					}
					j+=i;
					blocksout++;
				}
				if( blocksout >= (linesize/4) || infile[pos] )
				{
					blocksout = 0;
				}
			}
		}
		catch(...)
		{
		}
	}
	void Reset()
	{
		infile = 0;
		infileD = "";
		outfile = "";
		outD.clear();
		if(bInstInf)
			delete infile;
	}
	void SetInfileD(string sArg)
	{
		//cout << "SetInfileD:" << sArg << endl;
		infileD = sArg;
	}
	void SetInfile(string sArg)
	{
		infile = new char[sArg.size()+1];
		memset(infile,0,sArg.size()+1);
		strncpy(infile,sArg.c_str(),sArg.size());
		bInstInf = true;
	}
	string GetOutfile()
	{
		return outfile;
	}
	void SetCount(int iArg)
	{
		Count = iArg;
	}
private:
	char * infile;
	string outfile;
	string infileD;
	vector<char> outD;
	int Count;
	bool bInstInf;
};


