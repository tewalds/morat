
#pragma once


#include <cstdio>
#include <cstdlib>
#include <functional>
#include <string>
#include <vector>

#include "string.h"

using namespace std;
using namespace placeholders; //for bind

struct GTPResponse {
	bool success;
	string id;
	string response;

	GTPResponse() { }

	GTPResponse(bool s, string r = ""){
		success = s;
		response = r;
		rtrim(response);
	}

	GTPResponse(string r){
		GTPResponse(true, r);
	}

	string to_s(){
		return (success ? '=' : '?') + id + ' ' + response + "\n\n";
	}
};

typedef function<GTPResponse(vecstr)> gtp_callback_fn;

struct GTPCallback {
	string name;
	string desc;
	gtp_callback_fn func;

	GTPCallback() { }
	GTPCallback(string n, string d, gtp_callback_fn fn) : name(n), desc(d), func(fn) { }
};

class GTPBase {
	FILE * in, * out;
	vector<GTPCallback> callbacks;
	unsigned int longest_cmd;
	bool running;

public:

	GTPBase(FILE * i = stdin, FILE * o = stdout){
		in = i;
		out = o;
		longest_cmd = 0;
		running = false;

		newcallback("list_commands",    bind(&GTPBase::gtp_list_commands,    this, _1, false), "List the commands");
		newcallback("help",             bind(&GTPBase::gtp_list_commands,    this, _1, true),  "List the commands, with descriptions");
		newcallback("quit",             bind(&GTPBase::gtp_quit,             this, _1), "Quit the program");
		newcallback("exit",             bind(&GTPBase::gtp_quit,             this, _1), "Alias for quit");
		newcallback("protocol_version", bind(&GTPBase::gtp_protocol_version, this, _1), "Show the gtp protocol version");
	}

	void setinfile(FILE * i){
		in = i;
	}

	void setoutfile(FILE * o){
		out = o;
	}

	void newcallback(const string name, const gtp_callback_fn & fn, const string desc = ""){
		newcallback(GTPCallback(name, desc, fn));
		if(longest_cmd < name.length())
			longest_cmd = name.length();
	}

	void newcallback(const GTPCallback & a){
		callbacks.push_back(a);
	}

	int find_callback(const string & name){
		for(unsigned int i = 0; i < callbacks.size(); i++)
			if(callbacks[i].name == name)
				return i;
		return -1;
	}

	GTPResponse cmd(string line){
		vecstr parts = explode(line, " ");
		string id;

		if(parts.size() > 1 && atoi(parts[0].c_str())){
			id = parts[0];
			parts.erase(parts.begin());
		}

		string name = parts[0];
		parts.erase(parts.begin());

		int cb = find_callback(name);
		GTPResponse response;

		if(cb < 0){
			response = GTPResponse(false, "Unknown command");
		}else{
			response = callbacks[cb].func(parts);
		}

		response.id = id;

		return response;
	}

	bool run(){
		running = true;
		char buf[1001];

		while(running && fgets(buf, 1000, in)){
			string line(buf);

			trim(line);

			if(line.length() == 0 || line[0] == '#')
				continue;

			GTPResponse response = cmd(line);

			if(out){
				string output = response.to_s();

				fwrite(output.c_str(), 1, output.length(), out);
				fflush(out);
			}
		}
		return running;
	}

	GTPResponse gtp_protocol_version(vecstr args){
		return GTPResponse(true, "2");
	}

	GTPResponse gtp_quit(vecstr args){
		running = false;
		return true;
	}

	GTPResponse gtp_list_commands(vecstr args, bool showdesc){
		string ret = "\n";
		for(unsigned int i = 0; i < callbacks.size(); i++){
			ret += callbacks[i].name;
			if(showdesc && callbacks[i].desc.length() > 0){
				ret += string(longest_cmd + 2 - callbacks[i].name.length(), ' ');
				ret += callbacks[i].desc;
			}
			ret += "\n";
		}
		return GTPResponse(true, ret);
	}
};
