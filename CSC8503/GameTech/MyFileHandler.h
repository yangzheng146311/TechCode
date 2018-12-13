#pragma once
#include<iostream>

#include<vector>
#include<fstream>
#include<string>

using namespace std;
class MyFileHandler {
private:
	 ofstream myfile_o;//use to output data in a txt file
	 ifstream myfile_f;//use to read data from a txt file
public:
	 void ReadFile(string filename,int& playerID,int &playerScore);//Read the key file

	 //void WriteFile(string filename);//Output keyfile
	 vector<string> String_Split(const string& s, const char& c)

	{

		string buff = "";

		vector<string> v;

		for (auto t : s)

		{

			if (t != c)

			{

				buff += t;

			}



			else if (buff != "")

			{

				v.push_back(buff);

				buff = "";

			}

		}



		if (buff != "")

			v.push_back(buff);



		return v;

	}
};