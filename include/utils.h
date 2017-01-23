#include <iostream>
#include <stdio.h>

using namespace std;

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



