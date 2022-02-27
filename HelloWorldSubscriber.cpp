// Copyright 2016 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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
 * @file HelloWorldSubscriber.cpp
 *
 */

#include "HelloWorldSubscriber.h"
#include <fastrtps/participant/Participant.h>
#include <fastrtps/attributes/ParticipantAttributes.h>
#include <fastrtps/attributes/SubscriberAttributes.h>
#include <fastrtps/transport/TCPv4TransportDescriptor.h>
#include <fastrtps/subscriber/Subscriber.h>
#include <fastrtps/Domain.h>
#include <fastrtps/utils/IPLocator.h>
#include"base64.h"

#include <string>
#include <chrono>
#include <thread>
#include <sstream>
#include <fstream>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#define MINIAUDIO_IMPLEMENTATION1
#include "miniaudio.h"
using namespace eprosima::fastrtps;
using namespace eprosima::fastrtps::rtps;
using namespace std;
HelloWorldSubscriber::HelloWorldSubscriber()
    : participant_(nullptr)
    , subscriber_(nullptr)
{
}

int count_audio = 0;
long count_img= 0;
string filename = "";

//write a string to the specified file
int write_string_to_file_append(const std::string & file_string, const std::string str )
{//第一个参数是文件存储地址和文件名字符串，第二个字符串是要写入的内容。
	std::ofstream OsWrite(file_string,std::ofstream::app);
	OsWrite<<str;
	OsWrite<<std::endl;
	OsWrite.close();
    return 0;
}



std::string exec2(const char* cmd) {
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
void data_callback1(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
    ma_decoder* pDecoder = (ma_decoder*)pDevice->pUserData;
    if (pDecoder == NULL) 
    {
        return;
    }

    ma_decoder_read_pcm_frames(pDecoder, pOutput, frameCount, NULL);

    (void)pInput;
}

string name_mp3_g;
bool play_flag = false;
bool play_flag1= false;
void player(int sec_,string name)//Play audio with miniaudio
{
    while (1)
    {  
        if(1)
        {
            //string name_mp3= name;
            string name_mp3=name_mp3_g;

            ma_result result;
            ma_decoder decoder;
            ma_device_config deviceConfig;
            ma_device device;
            result = ma_decoder_init_file(name_mp3.c_str(), NULL, &decoder);
            if (result != MA_SUCCESS)
            {
                // printf("Could not load file: %s\n", name_mp3.c_str());    
            }

            deviceConfig = ma_device_config_init(ma_device_type_playback);
            deviceConfig.playback.format   = decoder.outputFormat;
            deviceConfig.playback.channels = decoder.outputChannels;
            deviceConfig.sampleRate        = decoder.outputSampleRate;
            deviceConfig.dataCallback      = data_callback1;
            deviceConfig.pUserData         = &decoder;

            if (ma_device_init(NULL, &deviceConfig, &device) != MA_SUCCESS) 
            {
                printf("Failed to open playback device.\n");
                ma_decoder_uninit(&decoder);
            }

            if (ma_device_start(&device) != MA_SUCCESS) 
            {
                printf("Failed to start playback device.\n");
                ma_device_uninit(&device);
                ma_decoder_uninit(&decoder);
            }
            // getchar();
            // sleep(sec_/20);
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            ma_device_uninit(&device);
            ma_decoder_uninit(&decoder);
            play_flag = false;
   
        }
            
    }     
}

bool HelloWorldSubscriber::init(
        const std::string& wan_ip,
        unsigned short port,
        bool use_tls,
        const std::vector<std::string>& whitelist)
{
    ParticipantAttributes pparam;
    int32_t kind = LOCATOR_KIND_TCPv4;

    Locator_t initial_peer_locator;
    initial_peer_locator.kind = kind;

    std::shared_ptr<TCPv4TransportDescriptor> descriptor = std::make_shared<TCPv4TransportDescriptor>();

    for (std::string ip : whitelist)
    {
        descriptor->interfaceWhiteList.push_back(ip);
        std::cout << "Whitelisted " << ip << std::endl;
    }

    if (!wan_ip.empty())
    {
        IPLocator::setIPv4(initial_peer_locator, wan_ip);
        std::cout << wan_ip << ":" << port << std::endl;
    }
    else
    {
        IPLocator::setIPv4(initial_peer_locator, "127.0.0.1");
    }
    initial_peer_locator.port = port;
    pparam.rtps.builtin.initialPeersList.push_back(initial_peer_locator); // Publisher's meta channel

    pparam.rtps.builtin.discovery_config.leaseDuration = c_TimeInfinite;
    pparam.rtps.builtin.discovery_config.leaseDuration_announcementperiod = Duration_t(5, 0);
    pparam.rtps.setName("Participant_sub");

    pparam.rtps.useBuiltinTransports = false;

    if (use_tls)
    {
        using TLSVerifyMode = TCPTransportDescriptor::TLSConfig::TLSVerifyMode;
        using TLSOptions = TCPTransportDescriptor::TLSConfig::TLSOptions;
        descriptor->apply_security = true;
        descriptor->tls_config.password = "test";
        descriptor->tls_config.verify_file = "ca.pem";
        descriptor->tls_config.verify_mode = TLSVerifyMode::VERIFY_PEER;
        descriptor->tls_config.add_option(TLSOptions::DEFAULT_WORKAROUNDS);
    }

    pparam.rtps.userTransports.push_back(descriptor);

    participant_ = Domain::createParticipant(pparam);
    if (participant_ == nullptr)
    {
        return false;
    }

    //REGISTER THE TYPE
    Domain::registerType(participant_, &type_);

    //CREATE THE SUBSCRIBER
    SubscriberAttributes rparam;
    rparam.topic.topicKind = NO_KEY;
    rparam.topic.topicDataType = "HelloWorld";
    rparam.topic.topicName = "HelloWorldTopicTCP";
    rparam.topic.historyQos.kind = KEEP_LAST_HISTORY_QOS;
    rparam.topic.historyQos.depth = 30;
    rparam.topic.resourceLimitsQos.max_samples = 50;
    rparam.topic.resourceLimitsQos.allocated_samples = 20;
    rparam.qos.m_reliability.kind = RELIABLE_RELIABILITY_QOS;
    rparam.qos.m_durability.kind = TRANSIENT_LOCAL_DURABILITY_QOS;
    subscriber_ = Domain::createSubscriber(participant_, rparam, (SubscriberListener*)&listener);
    if (subscriber_ == nullptr)
    {
        return false;
    }

    return true;
}

HelloWorldSubscriber::~HelloWorldSubscriber()
{
    Domain::removeParticipant(participant_);
}

void HelloWorldSubscriber::SubListener::onSubscriptionMatched(
        Subscriber*,
        MatchingInfo& matching_info)
{
    if (matching_info.status == MATCHED_MATCHING)
    {
        n_matched++;
        //logError(HW, "Matched");
        std::cout << "[RTCP] Subscriber matched" << std::endl;
        //清空两个list
        ofstream outfile1;
        outfile1.open("mp3list.txt", ios::out | ios::trunc );
        outfile1.close();
        ofstream outfile2;
        outfile2.open("imglist.txt", ios::out | ios::trunc );
        outfile2.close();
    }
    else
    {
        n_matched--;
        std::cout << "[RTCP] Subscriber unmatched" << std::endl;
        if(filename.find(".mp3")!=-1)
        {
            //ffmpeg -f concat -safe 0 -i filelist.txt -c copy output.mp4
            std::string command3 = "ffmpeg -y -f concat -safe 0 -i mp3list.txt -c copy save/"+filename;   
            // exec2("ffmpeg -f concat -i mp3list.txt -c copy mp3/temp.mp3");
            exec2(command3.c_str());
        }
        else
        {
            //filename end with .mp4
            exec2("ffmpeg -f concat -i mp3list.txt -c copy mp3/temp.mp3");
            exec2("ffmpeg -r 20 -y -f concat -i imglist.txt -vcodec libx264 image/temp.mp4");

            std::string command4 = "ffmpeg -y -i mp3/temp.mp3 -i image/temp.mp4 -c:v copy -c:a copy save/"+filename;
            exec2(command4.c_str());
        }
        exec2("rm image/*");
        exec2("rm mp3/*.mp3");
        count_audio = 0;
        count_img= 0;
        cv::destroyAllWindows();
    }
}

void HelloWorldSubscriber::SubListener::onNewDataMessage( //hello3,hello4
        Subscriber* sub)
{
    if (sub->takeNextData((void*)&hello, &info))
    {
        if (info.sampleKind == ALIVE)
        {
            //get filename
            if(filename == "")
            {
                std::string title_decode= base64_decode(hello.message());
                if(title_decode.find("Filename"))
                {   
                    filename = title_decode.substr(8, title_decode.size()-8);
                    std::cout << filename << std::endl;
                }
            }

            cv::Mat imgRes;
            std::string data= hello.message();
            std::string data_decode= base64_decode(data);
            int size =data_decode.size();
            
            //process audio
            // if(data_decode.find('mp3'))
            if(data_decode[size-1]=='3'&&data_decode[size-2]=='p'&&data_decode[size-3]=='m')
            {
                std::cout<<"mp3"<<std::endl;
                std::string s(data_decode.begin(), data_decode.end()- 4);
                
                try
                {
                    count_audio++;
                    write_string_to_file_append(std::to_string(count_audio)+".mp3",s);
                    
                    play_flag = true;

                    name_mp3_g = std::to_string(count_audio)+".mp3";
                    std::string command = "mkdir mp3"; //Create storage directory
                    exec2(command.c_str());
                    std::string command1 = "cp "+name_mp3_g+" mp3/"+name_mp3_g;
                    exec2(command1.c_str());
                    ofstream mp3listfile("mp3list.txt", std::ios::app);
                    mp3listfile << "file " << name_mp3_g << std::endl;
                    mp3listfile.close();

                    std::cout<< name_mp3_g<<std::endl;
                               
                }

                catch(const std::exception& e)
                {
                    std::cerr << e.what() << '\n';
                }
            }
            
            else
            {  
                std::vector<uchar> v11(data_decode.begin(),data_decode.end());

                cv::Mat  imgRes = cv::imdecode(v11, 1);    
                if(!imgRes.empty())      
                {
                    cv::imshow("Subscriber", imgRes);
                    cv::waitKey(40);
                    //Create storage directory
                    std::string command2 = "mkdir image";
                    exec2(command2.c_str());
                    cv::imwrite("image/"+std::to_string(count_img)+".jpg",imgRes);
                    ofstream imglistfile("imglist.txt", std::ios::app);
                    //format: file 1.jpg
                    imglistfile << "file " << std::to_string(count_img) << ".jpg" << std::endl;
                    imglistfile.close();
                    count_img++;
                }       
                //  play_flag = false ;

            }
            std::cout << "[RTCP] Message " << "hello.message()  size "<<data .size()<< " " 
            << hello.index() << " RECEIVED" << std::endl;
            this->n_samples++;
        }
    }
}

void HelloWorldSubscriber::run()
{
    std::thread helper1(player,20,name_mp3_g);//其实不用传参
    std::cout << "[RTCP] Subscriber running. Please press enter to stop the Subscriber" << std::endl;
    std::cin.ignore();
    helper1.join(); 
}

void HelloWorldSubscriber::run(
        uint32_t number)
{
    std::thread helper1(player,20,name_mp3_g);
    std::cout << "[RTCP] Subscriber running until " << number << "samples have been received" << std::endl;
    while (number < this->listener.n_samples)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    helper1.join(); 
}
