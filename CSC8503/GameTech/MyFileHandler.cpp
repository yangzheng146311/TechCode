#include "MyFileHandler.h"

void MyFileHandler::ReadFile(string filename, int& playerID, int &playerScore)
{
	myfile_f.open(filename);
	string str;
	while (!myfile_f.eof())

	{

		getline(MyFileHandler::myfile_f, str);

		if (str.length() > 0)

		{

			vector<string> v = String_Split(str, ' ');



			if (v[0] != "ID")

			{
				std::stoi(str);
				playerID = std::stoi(v[0]);
				playerScore = std::stoi(v[1]);


			}

		}
	}

	MyFileHandler::myfile_f.close();
}


