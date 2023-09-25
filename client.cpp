/*
	Original author of the starter code
    Tanzir Ahmed
    Department of Computer Science & Engineering
    Texas A&M University
    Date: 2/8/20
	
	Please include your Name, UIN, and the date below
	Name: Kevin Shi
	UIN: 831008207
	Date: 09/12/2023
*/
#include "common.h"
#include "FIFORequestChannel.h"

using namespace std;

int main (int argc, char *argv[]) {
	int opt;
	int p = -1; // setting the values to -1 for p, t, and e says that "there is no value".
	double t = -1;
	int e = -1;
	int buff_size = MAX_MESSAGE;
	bool newChanReq = false;
	vector<FIFORequestChannel*> channels;
	
	string filename = "";
	while ((opt = getopt(argc, argv, "p:t:e:f:m:c")) != -1) {
		switch (opt) {
			case 'p':
				p = atoi (optarg); //atoi converts ASCII into int. stores value of the argument of the user to p.
				break;
			case 't':
				t = atof (optarg); //optarg gets the argv?
				break;
			case 'e':
				e = atoi (optarg);
				break;
			case 'f':
				filename = optarg;
				break;
			case 'm':
				buff_size = atoi(optarg);
				break;
			case 'c':
				newChanReq = true;
				break;

		}
	}


	// Part 1
	pid_t server_request = fork();

	if(server_request == 0)
	{
		char* argu [] = {(char*)("./server"), (char*)("-m"), (char*)std::to_string(buff_size).c_str(), nullptr};
		execvp(argu[0], argu);
	}

	FIFORequestChannel cont_chan("control", FIFORequestChannel::CLIENT_SIDE);
	channels.push_back(&cont_chan);

	if (newChanReq) 
	{
		// send new channel request to the server
		MESSAGE_TYPE nc = NEWCHANNEL_MSG;
		cont_chan.cwrite(&nc, sizeof(MESSAGE_TYPE));
		char buf0;
		string chanName;
		cont_chan.cread(&buf0, sizeof(char));
		while (buf0 != '\0')
		{
			chanName.push_back(buf0);
			cont_chan.cread(&buf0, sizeof(char));
		}
		cout << chanName;
		FIFORequestChannel* new_chan = new FIFORequestChannel(chanName, FIFORequestChannel::CLIENT_SIDE);
		cout << chanName;
		channels.push_back(new_chan);
	}
	FIFORequestChannel chan = *(channels.back()); 

	if (filename == "" && t == -1 && e == -1) // Part 2
	{ 
		ofstream file;
		filename = "received/x1.csv";
		file.open(filename);
		double t = 0;
		int count = 0;

		while (count < 1000) {
			file << t;
			for (int i = 1; i <= 2; i++) { // x : 2 times : 1, 2
				char buf[MAX_MESSAGE]; //256
				datamsg x(p, t, i);
				
				memcpy(buf, &x, sizeof(datamsg));
				chan.cwrite(buf, sizeof(datamsg)); 
				double reply;
				chan.cread(&reply, sizeof(double)); 
				file << "," << reply;
				// append "reply, " to x1.csv -> e = 1
				// append "reply/n" to x1.csv -> e = 2
			}
			file << "\n";
			t += 0.004;
			count++;
		}
		// close x1.csv
		file.close();
	}
	else if (filename != "")
	{
		// Receive the file length into filesize in bytes?
		filemsg fm(0, 0); // parameters = file offset, length
		int len = sizeof(filemsg) + (filename.size() + 1);
		char* buf2 = new char[len]; // REQUEST BUFFER
		char* buf3 = new char[buff_size]; // RESPONSE BUFFER: create buffer of size buff capacity(m)
		memcpy(buf2, &fm, sizeof(filemsg));
		strcpy(buf2 + sizeof(filemsg), filename.c_str());
		chan.cwrite(buf2, len);  // I want the file length;
		int64_t filesize = 0;
		chan.cread(&filesize, sizeof(int64_t));
	
		// Create file in /received/filename
		ofstream ofile;
		ofile.open("received/" + filename);
		if (!ofile.is_open())
		{
			cout << "Could not open file" << endl;
			return -1;
		}
		// cout << "buff size is: " << buff_size << endl;
		// cout << "file length is: " << filesize << endl;
		// loop over the segments in the file: filesize / buff capacity
		int loop_count = floor((double)filesize / (double)buff_size);
		//cout << "loop count = " << loop_count << endl;
		for (int i = 0; i < loop_count; i++)
		{
			// create filemsg instance
			filemsg* file_req = (filemsg*)buf2;
			// set offset in the file
			file_req->offset = buff_size * i;

			// set the length. be careful of the last segment.
			file_req->length = buff_size;
			// send the request (buf2)
			chan.cwrite(buf2, len);

			// receive the response
			// cread into buf3 length file_req->len ???
			chan.cread(buf3, file_req->length);

			// write buf3 into file: received/filename.
			ofile.write(buf3, buff_size);

			// update remaining bytes and loop again
		}
		// create filemsg instance
		filemsg* file_req = (filemsg*)buf2;
		// set offset in the file
		file_req->offset = buff_size * loop_count;

		// set the length. be careful of the last segment.
		file_req->length = filesize % buff_size;
		// send the request (buf2)
		chan.cwrite(buf2, len);

		// receive the response
		// cread into buf3 length file_req->len ???
		chan.cread(buf3, file_req->length);

		// write buf3 into file: received/filename.
		ofile.write(buf3, filesize % buff_size);

		// update remaining bytes and loop again

		delete[] buf2;
		delete[] buf3;
	}
	else 
	{
		char buf[MAX_MESSAGE]; // 256
    	//datamsg x(1, 0.0, 1); // change from hardcoding to user's values
		datamsg x(p, t, e);
		memcpy(buf, &x, sizeof(datamsg));
		chan.cwrite(buf, sizeof(datamsg)); // question // writing reply
		double reply;
		chan.cread(&reply, sizeof(double)); //answer // the server reads this reply. prints double reply.
		cout << "For person " << p << ", at time " << t << ", the value of ecg " << e << " is " << reply << endl;
	}

	
	if (newChanReq) 
	{
		MESSAGE_TYPE m = QUIT_MSG;
    	chan.cwrite(&m, sizeof(MESSAGE_TYPE));
		delete channels.back();
		chan = *(channels.front());
	}

	// closing the channel    
    MESSAGE_TYPE m = QUIT_MSG;
    chan.cwrite(&m, sizeof(MESSAGE_TYPE));

}
