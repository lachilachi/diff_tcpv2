// Copyright 2016-2019 Proyectos y Sistemas de Mantenimiento SL (eProsima).
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * @file HelloWorld_main.cpp
 *
 */

#include "HelloWorldPublisher.h"
#include "HelloWorldSubscriber.h"

#include <fastrtps/Domain.h>
#include <fastrtps/log/Log.h>
#include <fstream>
#include <string>

#include "optionparser.h"
using namespace std;
using namespace eprosima;
using namespace fastrtps;
using namespace rtps;
// for string delimiter
vector<string> split (string s, string delimiter) {
    size_t pos_start = 0, pos_end, delim_len = delimiter.length();
    string token;
    vector<string> res;

    while ((pos_end = s.find (delimiter, pos_start)) != string::npos) {
        token = s.substr (pos_start, pos_end - pos_start);
        pos_start = pos_end + delim_len;
        res.push_back (token);
    }

    res.push_back (s.substr (pos_start));
    return res;
}

std::string exec(const char* cmd) {
    char buffer[128];
    std::string result = "";
    FILE* pipe = popen(cmd, "r");
    if (!pipe) throw std::runtime_error("popen() failed!");
    try {
        while (fgets(buffer, sizeof buffer, pipe) != NULL) {
            result += buffer;
        }
    } catch (...) {
        pclose(pipe);
        throw;
    }
    pclose(pipe);
    return result;
}
struct Arg: public option::Arg
{
    static void print_error(const char* msg1, const option::Option& opt, const char* msg2)
    {
        fprintf(stderr, "%s", msg1);
        fwrite(opt.name, opt.namelen, 1, stderr);
        fprintf(stderr, "%s", msg2);
    }

    static option::ArgStatus Unknown(const option::Option& option, bool msg)
    {
        if (msg) print_error("Unknown option '", option, "'\n");
        return option::ARG_ILLEGAL;
    }

    static option::ArgStatus Required(const option::Option& option, bool msg)
    {
        if (option.arg != 0 && option.arg[0] != 0)
        return option::ARG_OK;

        if (msg) print_error("Option '", option, "' requires an argument\n");
        return option::ARG_ILLEGAL;
    }

    static option::ArgStatus Numeric(const option::Option& option, bool msg)
    {
        char* endptr = 0;
        if (option.arg != 0 && strtol(option.arg, &endptr, 10))
        {
        }
        if (endptr != option.arg && *endptr == 0)
        {
            return option::ARG_OK;
        }

        if (msg)
        {
            print_error("Option '", option, "' requires a numeric argument\n");
        }
        return option::ARG_ILLEGAL;
    }

    static option::ArgStatus String(const option::Option& option, bool msg)
    {
        if (option.arg != 0)
        {
            return option::ARG_OK;
        }
        if (msg)
        {
            print_error("Option '", option, "' requires a numeric argument\n");
        }
        return option::ARG_ILLEGAL;
    }
};



enum  optionIndex {
    UNKNOWN_OPT,
    HELP,
    SAMPLES,
    INTERVAL,
    IP,
    PORT,
    TLS,
    WHITELIST
};

/*

        std::cout << "There was an error with the input arguments." << std::endl << std::endl;
        std::coutFILE "The publisher is going to work as a TCP server and if the test" << std::endl;
        std::cout << "is through a NAT it must have its public IP in the wan_ip argument." << std::endl << std::endl;
        std::cout << "The optional arguments are: publisher [times] [interval] [wan_ip] [port] " << std::endl;
        std::cout << "\t- times: Number of messages to send (default: unlimited = 0). " << std::endl;
        std::cout << "\t\t If times is set greater than 0, no messages will be sent until a subscriber matches. " << std::endl;
        std::cout << "\t- interval: Milliseconds between messages (default: 100). " << std::endl;
        std::cout << "\t- wap_ip: Public IP Address of the publisher. " << std::endl;
        std::cout << "\t- port: Physical Port to listening incoming connections, this port must be allowed in" << std::endl;
        std::cout << "\t\tthe router of the publisher if the test is going to use WAN IP. " << std::endl << std::endl;

        std::cout << "The subscriber is going to work as a TCP client. If the test is through a NAT" << std::endl;
        std::cout << "server_ip must have the WAN IP of the publisher and if the test is on LAN" << std::endl;
        std::cout << "it must have the LAN IP of the publisher" << std::endl << std::endl;
        std::cout << "The optional arguments are: subscriber [server_ip] [port] " << std::endl;
        std::cout << "\t- server_ip: IP Address of the publisher. " << std::endl;
        std::cout << "\t- port: Physical Port where the publisher is listening for connections." << std::endl << std::endl;
*/

const option::Descriptor usage[] = {
    { UNKNOWN_OPT, 0,"", "",                Arg::None,
        "Usage: HelloWorldExampleTCP <publisher|subscriber>  file name testmp3.mp3/mp4\n\nGeneral options:" },
    { HELP,    0,"h", "help",               Arg::None,      "  -h \t--help  \tProduce help message." },
    { TLS, 0, "t", "tls",          Arg::None,      "  -t \t--tls \tUse TLS." },
    { WHITELIST, 0, "w", "whitelist",       Arg::String,    "  -w \t--whitelist \tUse Whitelist." },

    { UNKNOWN_OPT, 0,"", "",                Arg::None,      "\nPublisher options:"},
    { SAMPLES,0,"s","samples",              Arg::Numeric,
        "  -s <num>, \t--samples=<num>  \tNumber of samples (0, default, infinite)." },
    { INTERVAL,0,"i","interval",            Arg::Numeric,
        "  -i <num>, \t--interval=<num>  \tTime between samples in milliseconds (Default: 100)." },
    { IP,0,"a","address",                   Arg::String,
        "  -a <address>, \t--address=<address> \tPublic IP Address of the publisher (Default: None)." },
    { PORT, 0, "p", "port",                 Arg::Numeric,
        "  -p <num>, \t--port=<num>  \tPhysical Port to listening incoming connections (Default: 5100)." },

    { UNKNOWN_OPT, 0,"", "",                Arg::None,      "\nSubscriber options:"},
    { IP,0,"a","address",                   Arg::String,
        "  -a <address>, \t--address=<address> \tIP Address of the publisher (Default: 127.0.0.1)." },
    { PORT, 0, "p", "port",                 Arg::Numeric,
        "  -p <num>, \t--port=<num>  \tPhysical Port where the publisher is listening for connections (Default: 5100)." },
   

    { 0, 0, 0, 0, 0, 0 }
};

using namespace eprosima;
using namespace fastrtps;
using namespace rtps;
int main(int argc, char** argv)
{
    int columns;

#if defined(_WIN32)
    char* buf = nullptr;
    size_t sz = 0;
    if (_dupenv_s(&buf, &sz, "COLUMNS") == 0 && buf != nullptr)
    {
        columns = strtol(buf, nullptr, 10);
        free(buf);
    }
    else
    {
        columns = 80;
    }
#else
    columns = getenv("COLUMNS") ? atoi(getenv("COLUMNS")) : 80;
#endif

    std::cout << "Starting " << std::endl;
    int type = 1;
    int count = 0;
    long sleep = 100;
    std::string wan_ip;
    int port = 5100;
    bool use_tls = false;
    std::vector<std::string> whitelist;
     std::string filename;
    int file_type = 1;
    if (argc > 2)
    {
        if (strcmp(argv[1], "publisher") == 0)
        {
                    type = 1;
                    filename = argv[2];
                    // std::cout<<
                    if(filename.find(".mp3")!=-1)
                    {
                        file_type = 1;
                    }else
                    {
                        file_type = 2;
                    }
        }
        else if (strcmp(argv[1], "subscriber") == 0)
        {
            type = 2;
        }

        argc -= (argc > 0);
        argv += (argc > 0); // skip program name argv[0] if present
        --argc; ++argv; // skip pub/sub argument
        option::Stats stats(usage, argc, argv);
        std::vector<option::Option> options(stats.options_max);
        std::vector<option::Option> buffer(stats.buffer_max);
        option::Parser parse(usage, argc, argv, &options[0], &buffer[0]);

        if (parse.error())
        {
            return 1;
        }

        if (options[HELP])
        {
            option::printUsage(fwrite, stdout, usage, columns);
            return 0;
        }

        for (int i = 0; i < parse.optionsCount(); ++i)
        {
            option::Option& opt = buffer[i];
            switch (opt.index())
            {
                case HELP:
                    // not possible, because handled further above and exits the program
                    break;

                case SAMPLES:
                    count = strtol(opt.arg, nullptr, 10);
                    break;

                case INTERVAL:
                    sleep = strtol(opt.arg, nullptr, 10);
                    break;

                case IP:
                {
                    if (opt.arg != nullptr)
                    {
                        wan_ip = std::string(opt.arg);
                    }
                    else
                    {
                        option::printUsage(fwrite, stdout, usage, columns);
                        return 0;
                    }
                    break;
                }

                case PORT:
                    port = strtol(opt.arg, nullptr, 10);
                    break;

                case TLS:
                    use_tls = true;
                    break;

                case WHITELIST:
                    whitelist.emplace_back(opt.arg);
                    break;

                case UNKNOWN_OPT:
                    option::printUsage(fwrite, stdout, usage, columns);
                    return 0;
                    break;
            }
        }
    }
    else
    {
        option::printUsage(fwrite, stdout, usage, columns);
        return 0;
    }


    switch (type)
    {
        case 1:
            {

                    string  command1 = " rm *.mp3 ";
                    exec(command1.c_str());
                    command1 = " rm *.jpg ";
                    exec(command1.c_str());
                    string command = "ffmpeg -i "+filename+">1.txt 2>&1";
                    exec(command.c_str());
                    if( file_type ==1)
                        {
                            // ffmpeg -i 1.mp3 -f segment -segment_time 1 -c copy %03d.mp3
                                command = "ffmpeg -i "+filename+"   -f segment -segment_time 1 -c copy %04d.mp3";
                                exec(command.c_str());
                        }
                        else
                        {
                            // ffmpeg -i ../testVideo.mp4 -vf fps=20 out%05d.jpg
                            //  ffmpeg -i ../testVideo.mp4 testmp3.mp3
                                command = " rm testmp3.mp3 ";
                                exec(command.c_str());
                                    command = "ffmpeg -i "+filename+"   -vf fps=20 %05d.jpg";
                                exec(command.c_str());
                                    std::cout << "MP3 輸出" << std::endl;
                                command = "ffmpeg -i "+filename+"   testmp3.mp3";
                                exec(command.c_str());
                                    std::cout << "MP3 切割" << std::endl;
                                command = " ffmpeg -i testmp3.mp3 -f segment -segment_time 1 -c copy  %04d.mp3";
                                exec(command.c_str());
                        }
                int hour = 0;
                int min = 0;
                int sec = 0;
                bool pos =false;
                std::string sFilename = "1.txt";
                std::ifstream fileSource(sFilename); // Creates an input file stream
                if (!fileSource) {
                    std::cerr << "Canot open " << sFilename << std::endl;
                    exit(-1);
                }
                else {
                    // Intermediate buffer
                    std::string buffer;
                    // By default, the >> operator reads word by workd (till whitespace)
                    while (fileSource >> buffer)
                    {
                        if(pos)
                        {
                            std::cout << buffer << std::endl;
                            string str =buffer;
                            string delimiter = ":";

                            vector<string> v = split (str, delimiter);
                            for (auto i : v) cout << i << endl;
                            std::cout<< v[2].substr(0, 2)<< endl;
                            hour = atoi( v[0].c_str() );
                            min = atoi( v[1].c_str() );
                            sec  = atoi( v[2].substr(0, 2).c_str() );
                            pos = false;
                        }

                        if(buffer=="Duration:")
                        {
                                std::cout << buffer << std::endl;
                                pos = true;
                        }
                    
                    }

		            std::cout<< hour<<"-"<<min<<"-"<<sec<<endl;
            }
                int  sec_total = hour*60*60+min*60+sec;
                int fps_20 = sec_total * 20;
                HelloWorldPublisher mypub;
                if (mypub.init(wan_ip, static_cast<uint16_t>(port), use_tls, whitelist))
                {
                   

                     std::cout<<"cout "<<fps_20<<std::endl;  
                    if(file_type ==1)
                    {
                          std::cout<<" ------MP3------------- "<<fps_20<<std::endl;  
                            std::cout<<"filename "<<filename<<std::endl;  
                             mypub.run1(fps_20,filename);
                    }
                    else
                    {
                          std::cout<<"------MP4------------- "<<fps_20<<std::endl;  
                           mypub.run1(fps_20,"1");
                    }
                }
                break;
            }
        case 2:
            {
                HelloWorldSubscriber mysub;
                if (mysub.init(wan_ip, static_cast<uint16_t>(port), use_tls, whitelist))
                {
                    mysub.run();
                }
                break;
            }
    }
    Domain::stopAll();
    return 0;
}
