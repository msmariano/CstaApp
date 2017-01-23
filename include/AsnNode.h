#ifndef AsnNodeH
#define AsnNodeH
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <math.h>
#include <vector>
#include <time.h>
#include <inttypes.h>
typedef enum {ntPrimitive, ntConstructed} TNodeType;
typedef enum {ncUniversal, ncApplication, ncContext_Specific, ncPrivate} TNodeClass;
typedef unsigned char BYTE;

using namespace std;



class CstaAppVersion
{
public:
	CstaAppVersion()
	{

	}
	~CstaAppVersion()
	{

	}
	string GetVerion()
	{
		return "1.0";
	}
};

class CAsnNode{
private:
	TNodeType NodeType;
	TNodeClass NodeClass;
	unsigned int TagNumber;

	vector<CAsnNode*> SubNodes;

	unsigned CurrentIndex;

public:
	CAsnNode(TNodeClass nClass, TNodeType nType, int tNumber)
	{
		NodeClass = nClass;
		NodeType = nType;
		TagNumber = tNumber;
		dataSize = 0;
		CurrentIndex = 0;
		PrimitiveData = NULL;
	}

	~CAsnNode()
	{
		CAsnNode * tempNode;
		for(unsigned i = 0;i < SubNodes.size();i++)
		{
			tempNode = (CAsnNode * )SubNodes[i];
			delete tempNode;
		}


		if(NodeType == ntPrimitive && (dataSize > 0))
			delete PrimitiveData;

	}
	void logAsn(string logText)
	{
		char * tmpLog;

		tmpLog = new char[logText.size()+50];
		try
		{
			FILE * arq;
			if((arq=fopen(".logAsnNode.txt","r"))!=NULL)
			{
				fclose(arq);

				time_t curtime;
				struct tm *loctime;
				arq = fopen(".logAsnNode.txt","a");

				curtime = time (NULL);
				loctime = localtime (&curtime);

				char date[1024]="";

				strftime(date,sizeof(date),"%Y%m%d %H%M%S ",loctime);

				strcpy(tmpLog, date);
				strcat(tmpLog, logText.c_str());
				if ( arq != NULL)
				{

					fputs(tmpLog,arq);
					fputs("\n", arq);
					fclose(arq);
				}
			}


		}
		catch(...)
		{
		}
		delete tmpLog;
	}
	int GetNumberOfBytes(int data)
	{
		int nBytes;

		nBytes = 0;
		while(data >= 1)
		{
			nBytes += 1;
			data /= 2;
		}
		if(nBytes % 8 == 0)
			nBytes /= 8;
		else
			nBytes = nBytes/8 + 1;

		if(nBytes == 0)
			nBytes = 1;

		return nBytes;
	}
	void SetNodeType(unsigned char nType)
	{
		NodeType = (TNodeType)((nType&0x20)>>5);
	}
	void SetNodeClass(unsigned char nClass)
	{
		NodeClass = (TNodeClass)((nClass&0xC0)>>6);
	}
	void SetTagNumber(unsigned char nTagNumber)
	{
		TagNumber = (nTagNumber&0x1F);
	}
	TNodeType GetNodeType()
	{
		return NodeType;
	}
	TNodeClass GetNodeClass()
	{
		return NodeClass;
	}
	unsigned int GetTagNumber()
	{
		return TagNumber;
	}

	int GetHeaderSize()
	{
		int nodeSize;
		int result;

		nodeSize = GetSequenceSize();
		if(nodeSize <= 127)
		{
			result = 2;
		}else
		{
			result = 2+GetNumberOfBytes(nodeSize);
		}
		return result;
	}
	int GetNodeSize()
	{
		return GetHeaderSize()+GetSequenceSize();
	}
	int GetSequenceSize()
	{
		int result;

		result = 0;
		if(NodeType == ntPrimitive)
		{
			result = dataSize;
		}else
		{
			for(unsigned i =0;i < SubNodes.size();i++)
			{
				CAsnNode * ptrNode;
				ptrNode = SubNodes[i];
				result += ptrNode->GetNodeSize();
			}
		}

		return result;
	}
	unsigned int ParseSequenceSize(unsigned char * sequence)
	{
		int sequenceSize;
		int lengthOctet;
		sequenceSize = 0;
		lengthOctet = sequence[1];
		if(lengthOctet < 128)
		{
			sequenceSize = lengthOctet;
		}else
		{
			lengthOctet = lengthOctet & 0x7F;
			for(int i = 2;i < (lengthOctet+2);i++)
			{
				sequenceSize = (sequenceSize<<8)+sequence[i];
			}
		}

		return sequenceSize;
	}
	CAsnNode * GetNextSequence()
	{
		try{
			CAsnNode * tempNode;
			if(CurrentIndex < SubNodes.size())
			{
				tempNode = SubNodes[CurrentIndex];
				CurrentIndex++;
				return tempNode;
			}

		}catch(...)
		{
			return NULL;
		}
		return NULL;
	}

	int GetIntegerData(string Src = "")
	{


		int result;
		result = 0;
		if(NodeType == ntPrimitive)
		{
			//cout << Src << " Tam: "<< IntToStr(dataSize) << endl;

			for ( int i = 0 ; i < dataSize && dataSize < 5; i++)
			{
				result = (result << 8)|PrimitiveData[i];
			}
		}
		return result;
	}

	string GetStringData()
	{
		string result;
		if(NodeType == ntPrimitive)
		{
			for(int i = 0;i < dataSize;i++)
			{
				result += (char)PrimitiveData[i];
			}
		}
		return result;
	}
	unsigned char * c_str(void)
	{
		unsigned char * tempBuffer;
		tempBuffer = new unsigned char[GetNodeSize()];
		tempBuffer[0] = 0;
		switch(NodeClass)
		{
		case ncUniversal:
			tempBuffer[0] += 0x00;
			break;
		case ncApplication:
			tempBuffer[0] += 0x40;
			break;
		case ncContext_Specific:
			tempBuffer[0] += 0x80;
			break;
		case ncPrivate:
			tempBuffer[0] += 0xC0;
			break;
		}

		int tempIndex = 2;
		if(TagNumber <= 30)
		{
			tempBuffer[0] += TagNumber;
		}

		if(GetSequenceSize() <= 127)
		{
			tempBuffer[1] = GetSequenceSize();
		}else
		{
			int nBytes = GetNumberOfBytes(GetSequenceSize());
			int tempSize = GetSequenceSize();

			tempBuffer[1] = 0x80+nBytes;

			for(int i = 0;i < nBytes;i++)
			{
				tempBuffer[1+nBytes-i] = tempSize & 0xFF;
				tempSize = tempSize >> 8;
				tempIndex++;
			}
		}

		if(NodeType == ntPrimitive)
		{
			memcpy(&tempBuffer[tempIndex], PrimitiveData, dataSize);
		}
		else{
			tempBuffer[0] += 0x20;


			for(unsigned iList = 0; iList < SubNodes.size();iList++)
			{
				CAsnNode * tempNode;
				tempNode = SubNodes[iList];

				unsigned char * tempMessage = tempNode->c_str();
				memcpy(&tempBuffer[tempIndex], tempMessage, tempNode->GetNodeSize());
				tempIndex += tempNode->GetNodeSize();

				delete tempMessage;
			}
		}

		return tempBuffer;
	}

	CAsnNode * AddInv( uint64_t pInt64Data)
	{
		uint64_t tempData;
		uint64_t byteMask;
		int nBytes;

		tempData = pInt64Data;
		nBytes = 0;
		while(tempData >= 1)
		{
			nBytes += 1;
			tempData /= 2;
		}
		if(nBytes % 8 == 0)
			nBytes /= 8;
		else
			nBytes = nBytes/8 + 1;

		PrimitiveData = new unsigned char[nBytes];
		dataSize = nBytes;

		for(int i = 0;i < nBytes;i++)
		{
			byteMask = 0xFF * pow(2, (8*i));
			PrimitiveData[i] = (pInt64Data & byteMask) >> (i * 8);
		}
		return this;
	}
	unsigned char * GetSequence(unsigned char * MArg, int dataSize = 10)
	{
		try
		{
			int LengthOctet;
			int NumLengthOctets;

			unsigned char * M = MArg;

			if(dataSize >= 2)
			{
				LengthOctet = M[1];
				if ( LengthOctet == 0)
				{
					return &M[0];

				}
				if ( LengthOctet <= 127)
				{
					if(dataSize >= 3)
					{
						return &M[2];
					}
				}
				else if ( LengthOctet == 128)
				{
					if(dataSize >= 3)
					{
						return &M[2];
					}
				}
				else
				{
					NumLengthOctets = LengthOctet-128;
					if(dataSize > (NumLengthOctets+2))
					{
						return &M[NumLengthOctets+2];
					}
				}
			}
		}
		catch(...)
		{
		}
		return MArg;

	}
	string ClassText(TNodeClass nClass)
	{
		switch(nClass)
		{
		case ncUniversal:
			return "Universal";
		case ncApplication:
			return "Application";
		case ncContext_Specific:
			return "Context_Specific";
		case ncPrivate:
			return "Private";
		}
		return "???";
	}

	CAsnNode * GetSpecificSequence(TNodeClass nClass, unsigned int tNumber)
	{
		try{
			CAsnNode * tempNode;
			for(unsigned i = CurrentIndex;i < SubNodes.size();i++)
			{
				tempNode = (CAsnNode *)SubNodes[i];
				if(tempNode->NodeClass == nClass)
					if(tempNode->TagNumber == tNumber)
					{
						CurrentIndex = i+1;
						return tempNode;
					}
			}

		}catch(...)
		{
			return NULL;
		}
		return NULL;
	}








	string RetornoHex(unsigned char * Mensagem,int t)
	{
		try
		{
			string S,S1;

			S1+"'";
			for (int k =0; k < t ;k++)
			{
				char text[1024] = "";
				if ( k > 0)
					sprintf(text," %.2X",Mensagem[k]);
				else
					sprintf(text,"%.2X",Mensagem[k]);
				S = string(text)+" ";
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

	CAsnNode * Add(uint64_t pInt64Data)
	{
		uint64_t tempData;
		uint64_t byteMask;
		int nBytes;

		tempData = pInt64Data;
		nBytes = 0;
		while(tempData >= 1)
		{
			nBytes += 1;
			tempData /= 2;
		}
		if(nBytes % 8 == 0)
			nBytes /= 8;
		else
			nBytes = nBytes/8 + 1;

		if(nBytes == 0)
			nBytes = 1;

		PrimitiveData = (unsigned char*)new char[nBytes];
		dataSize = nBytes;


		for(int i = 0;i < nBytes;i++)
		{
			byteMask = 0xFF * pow(2, (8*i));
			PrimitiveData[nBytes-i-1] = (pInt64Data & byteMask) >> (i * 8);
		}
		return this;
	}

	CAsnNode * Add(uint64_t pInt64Data, int nBytes)
	{
		uint64_t byteMask;


		PrimitiveData = (unsigned char*)new char[nBytes];
		dataSize = nBytes;

		for(int i = 0;i < nBytes;i++)
		{
			byteMask = 0xFF * pow(2, (8*i));
			PrimitiveData[nBytes-i-1] = (pInt64Data & byteMask) >> (i * 8);
		}
		return this;
	}

	CAsnNode * Add(unsigned char * data, int nBytes)
	{
		PrimitiveData = (unsigned char*)new char[nBytes];
		dataSize = nBytes;


		for(int i = 0;i < nBytes;i++)
		{
			PrimitiveData[i] = data[i];
		}
		return this;
	}
	string IntToStr(int Value)
	{
		char cVal[1024]="";
		sprintf(cVal,"%d",Value);
		return string(cVal);
	}

	CAsnNode * Add(char * pStringData)
	{
		int strSize;

		strSize = strlen(pStringData);
		dataSize = strSize;


		PrimitiveData = (unsigned char*)new char[strSize];

		for(int iStr = 0, iData = 0; iStr < strSize; iStr++, iData++)
		{
			PrimitiveData[iData] = (unsigned char)pStringData[iStr];
		}
		return this;
	}

	CAsnNode * Add(CAsnNode * cAsnNode)
	{
		SubNodes.push_back(cAsnNode);
		return this;
	}

	string LogText(int logNivel)
	{
		string returnText;
		string tempTab;
		int SequenceSize;

		for(int tNivel = 0;tNivel<logNivel;tNivel++)
		{
			tempTab +="\t";
		}
		if(NodeType==ntConstructed)
		{
			returnText += tempTab+ClassText(NodeClass)+" "+IntToStr(TagNumber)+"\n";
			returnText += tempTab+"{\n";

			for(unsigned i = 0;i < SubNodes.size();i++)
			{
				CAsnNode * tempSubNode;
				tempSubNode = (CAsnNode *)SubNodes[i];
				returnText += tempSubNode->LogText(logNivel+1);
			}

			returnText += tempTab+"}\n";
		}else if(GetNodeType()==ntPrimitive)
		{
			string s;

			if(NodeClass == ncUniversal && TagNumber == 4 && NodeType == ntPrimitive)
			{
				SequenceSize = GetSequenceSize();
				for(int a = 0;a < SequenceSize;a++)
					s += (char)PrimitiveData[a];
			}
			returnText += tempTab + ClassText(NodeClass)+" "+IntToStr(TagNumber)+" = ";
			if(GetSequenceSize()>0)
			{
				returnText += RetornoHex(PrimitiveData, GetSequenceSize());
			}else
			{
				returnText += "(NULL)";
			}
			if(s.size()>0)
			{
				returnText += "("+s+")";
			}
			returnText += "\n";
		}
		return returnText;
	}

	void Rewind()
	{
		CurrentIndex = 0;
	}

	unsigned int ParseHeaderSize(unsigned char * sequence)
	{
		int headerSize;
		int lengthOctet;
		headerSize = 0;

		lengthOctet = sequence[1];

		if(lengthOctet <= 127)
		{
			headerSize = 2;
		}else
		{
			lengthOctet = lengthOctet & 0x7F;
			headerSize = 2+lengthOctet;
		}

		return headerSize;
	}

	int GetNodeData(unsigned char * buffer, int bufSize)
	{
		int result;

		result = 0;
		if(dataSize < bufSize)
		{
			memcpy(buffer, PrimitiveData, dataSize);
			result = dataSize;
		}

		return result;
	}

	CAsnNode(unsigned char *Sequence)
	{
		SetNodeType(Sequence[0]);
		SetNodeClass(Sequence[0]);
		SetTagNumber(Sequence[0]);

		CurrentIndex = 0;

		if(NodeType==ntConstructed)
		{
			int TempDataSize;
			TempDataSize = ParseSequenceSize(Sequence);
			Sequence = GetSequence(Sequence);
			while(TempDataSize>0)
			{
				CAsnNode *newNode = new CAsnNode(Sequence);
				SubNodes.push_back(newNode);
				TempDataSize -= newNode->GetNodeSize();
				Sequence = &Sequence[newNode->GetNodeSize()];
			}
		}else if(GetNodeType()==ntPrimitive)
		{
			dataSize = ParseSequenceSize(Sequence);
			try{
				if(dataSize > 0)
				{
					PrimitiveData = (unsigned char*)new char[dataSize];
					Sequence = GetSequence(Sequence);
					memcpy(PrimitiveData, Sequence, dataSize);
				}
			}catch(...)
			{


			}

		}
	}

	CAsnNode(unsigned char * sequence, unsigned int messageSize) :
		NodeType(ntPrimitive),
		NodeClass(ncUniversal),
		TagNumber(0),
		dataSize(0),
		CurrentIndex(0)
	{

		unsigned int offset;
		unsigned int nodeType;
		unsigned int nodeClass;
		unsigned int tagNumber;

		unsigned int sequenceSize;
		unsigned int headerSize;

		CAsnNode * ptrNode;

		offset = 0;

		try
		{
			if(messageSize >= 2)
			{

				nodeType = (*sequence&0x20)>>5;
				nodeClass = (*sequence&0xC0)>>6;
				tagNumber = *sequence&0x1F;

				NodeType = (TNodeType)nodeType;
				NodeClass = (TNodeClass)nodeClass;
				TagNumber = tagNumber;
				headerSize = ParseHeaderSize(sequence);

				if(messageSize >= headerSize)
				{
					sequenceSize = ParseSequenceSize(sequence);

					if(messageSize >= (headerSize+sequenceSize))
					{
						if(NodeType == ntPrimitive)
						{

							dataSize = sequenceSize;
							if(dataSize > 0)
							{
								PrimitiveData = new unsigned char[sequenceSize];
								memcpy(PrimitiveData, &sequence[headerSize], sequenceSize);
							}

						}else
						{
							while(offset < sequenceSize)
							{
								ptrNode = new CAsnNode(&sequence[headerSize+offset], sequenceSize - offset);
								offset += ptrNode->GetNodeSize();
								SubNodes.push_back(ptrNode);
							}
						}
					}
				}
			}
		}
		catch(...)
		{

		}
	}
    unsigned char dump[4092];
    unsigned char * PrimitiveData;
    int tamDump;
    int dataSize;



};

#endif
