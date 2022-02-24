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
 * @file HelloWorldPublisher.cpp
 *
 */

#include "HelloWorldPublisher.h"
#include <fastrtps/participant/Participant.h>
#include <fastrtps/attributes/ParticipantAttributes.h>
#include <fastrtps/attributes/PublisherAttributes.h>
#include <fastrtps/publisher/Publisher.h>
#include <fastrtps/transport/TCPv4TransportDescriptor.h>
#include <fastrtps/Domain.h>
#include <fastrtps/utils/IPLocator.h>
#include"base64.h"
#include <thread>
#include <sstream>
#include <fstream>
#include <string>
 #include <chrono>
#include <thread>

#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
 #include <chrono>
#include <thread>



#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

using namespace std;
using namespace eprosima::fastrtps;

using namespace eprosima::fastrtps::rtps;

HelloWorldPublisher::HelloWorldPublisher()
    : participant_(nullptr)
    , publisher_(nullptr)
{
}



int count1 = 0;

void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
    ma_decoder* pDecoder = (ma_decoder*)pDevice->pUserData;
    if (pDecoder == NULL) {
        return;
    }
    ma_decoder_read_pcm_frames(pDecoder, pOutput, frameCount, NULL);

    (void)pInput;
}


void  foo(int sec_ ,std::string name)
{
    // 模拟耗费大量资源的操作
    // std::this_thread::sleep_for(std::chrono::seconds(1));

    {
                string name_mp3= name;

                ma_result result;
                ma_decoder decoder;
                ma_device_config deviceConfig;
                ma_device device;
                result = ma_decoder_init_file(name_mp3.c_str(), NULL, &decoder);
                if (result != MA_SUCCESS) {
                    printf("Could not load file: %s\n", name_mp3.c_str());
                  
                }

                deviceConfig = ma_device_config_init(ma_device_type_playback);
                deviceConfig.playback.format   = decoder.outputFormat;
                deviceConfig.playback.channels = decoder.outputChannels;
                deviceConfig.sampleRate        = decoder.outputSampleRate;
                deviceConfig.dataCallback      = data_callback;
                deviceConfig.pUserData         = &decoder;

                if (ma_device_init(NULL, &deviceConfig, &device) != MA_SUCCESS) {
                    printf("Failed to open playback device.\n");
                    ma_decoder_uninit(&decoder);

                }

                if (ma_device_start(&device) != MA_SUCCESS) {
                    printf("Failed to start playback device.\n");
                    ma_device_uninit(&device);
                    ma_decoder_uninit(&decoder);

                }

                printf("end Enter to quit...");
                // getchar();
                  std::this_thread::sleep_for(std::chrono::milliseconds(sec_/20*1000));
                // std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                ma_device_uninit(&device);
                ma_decoder_uninit(&decoder);


            }
}


bool HelloWorldPublisher::init(
        const std::string& wan_ip,
        unsigned short port,
        bool use_tls,
        const std::vector<std::string>& whitelist)
{
    stop_ = false;
    hello_.index(0);
    hello_.message("RZF");
    ParticipantAttributes pparam;

    pparam.rtps.builtin.discovery_config.leaseDuration = c_TimeInfinite;
    pparam.rtps.builtin.discovery_config.leaseDuration_announcementperiod = Duration_t(5, 0);
    pparam.rtps.setName("Participant_pub");

    pparam.rtps.useBuiltinTransports = false;

    std::shared_ptr<TCPv4TransportDescriptor> descriptor = std::make_shared<TCPv4TransportDescriptor>();

    for (std::string ip : whitelist)
    {
        descriptor->interfaceWhiteList.push_back(ip);
        std::cout << "Whitelisted " << ip << std::endl;
    }

    if (use_tls)
    {
        using TLSOptions = TCPTransportDescriptor::TLSConfig::TLSOptions;
        descriptor->apply_security = true;
        descriptor->tls_config.password = "test";
        descriptor->tls_config.cert_chain_file = "server.pem";
        descriptor->tls_config.private_key_file = "server.pem";
        descriptor->tls_config.tmp_dh_file = "dh2048.pem";
        descriptor->tls_config.add_option(TLSOptions::DEFAULT_WORKAROUNDS);
        descriptor->tls_config.add_option(TLSOptions::SINGLE_DH_USE);
        descriptor->tls_config.add_option(TLSOptions::NO_SSLV2);
    }

    descriptor->sendBufferSize = 0;
    descriptor->receiveBufferSize = 0;

    if (!wan_ip.empty())
    {
        descriptor->set_WAN_address(wan_ip);
        std::cout << wan_ip << ":" << port << std::endl;
    }
    descriptor->add_listener_port(port);
    pparam.rtps.userTransports.push_back(descriptor);

    participant_ = Domain::createParticipant(pparam);

    if (participant_ == nullptr)
    {
        return false;
    }
    //REGISTER THE TYPE

    Domain::registerType(participant_, &type_);

    //CREATE THE PUBLISHER
    PublisherAttributes wparam;
    wparam.topic.topicKind = NO_KEY;
    wparam.topic.topicDataType = "HelloWorld";
    wparam.topic.topicName = "HelloWorldTopicTCP";
    wparam.topic.historyQos.kind = KEEP_LAST_HISTORY_QOS;
    wparam.topic.historyQos.depth = 30;
    wparam.topic.resourceLimitsQos.max_samples = 50;
    wparam.topic.resourceLimitsQos.allocated_samples = 20;
    wparam.times.heartbeatPeriod.seconds = 2;
    wparam.times.heartbeatPeriod.nanosec = 200 * 1000 * 1000;
    wparam.qos.m_reliability.kind = RELIABLE_RELIABILITY_QOS;
    publisher_ = Domain::createPublisher(participant_, wparam, (PublisherListener*)&listener_);
    if (publisher_ == nullptr)
    {
        return false;
    }

    return true;
}

HelloWorldPublisher::~HelloWorldPublisher()
{
    Domain::removeParticipant(participant_);
}

void HelloWorldPublisher::PubListener::onPublicationMatched(
        Publisher*,
        MatchingInfo& info)
{
    if (info.status == MATCHED_MATCHING)
    {
        n_matched++;
        firstConnected = true;
        //logError(HW, "Matched");
        std::cout << "[RTCP] Publisher matched" << std::endl;
    }
    else
    {
        n_matched--;
        std::cout << "[RTCP] Publisher unmatched" << std::endl;
    }
}


void HelloWorldPublisher::runThread(
        uint32_t samples,
        long sleep_ms)
{
  
}



void HelloWorldPublisher::runThread1(
        uint32_t samples,
        std::string   video)
{
std::string name_ ;
    if(video=="1")
    {
             name_ = "testmp3.mp3";
                
    }else{
         name_ = video;
    }
           
    std::cout << "mp3 name "<<name_<<std::endl;
    std::cout << "starting first helper...\n";
    std::thread helper1(foo,samples,video);
  if(video=="1")
    {
          std::cout << "video \n";
        std::cout<<count1<<std::endl;
        count1+=1; 
        string mp3_filename = "out000.mp3";
        char str[3];
        string name_mp3_send;
     for(uint32_t i = 0;i<=samples;++i)
    {
            std::string mp3_contents;
            bool send_audio = false;

            std::stringstream ss;
            ss<<std::setw(5)<<std::setfill('0')<<i+1;
            string name= ss.str()+".jpg";
            std::cout<<name<<std::endl;     
            cv::Mat img = cv::imread(name,1); //CV_LOAD_IMAGE_GRAYSCALE
            if(!img.empty())
            {
            cv::imshow("Pubclisher",img);
            cv::waitKey(40);
            }
          
            if(i%20==0)
            {
                std::stringstream ss1;
                ss1<<std::setw(4)<<std::setfill('0')<<i/20+1;
                string name_mp3= ss1.str()+".mp3";
                std::cout<<name_mp3<<std::endl;
                std::ifstream t(name_mp3);
                std::stringstream buffer;
                buffer << t.rdbuf();
                std::string contents(buffer.str());
                std::cout<<contents.size()<<std::endl;
                mp3_contents = contents;

                    
                if(mp3_contents.empty())
                {

                    std::cout<<name_mp3<<std::endl;
                    break;
                }
                // buf
                mp3_contents.insert(mp3_contents.size(),"mp3");
                std::string  base64_str = base64_encode(mp3_contents);
                HelloWorld hello_3;
                hello_3.index(i);
                hello_3.message(base64_str);
                if (publish1(hello_3))
                {
                std::cout << "[RTCP] Message: "  <<hello_3.message().size()<< " with index: "
                << hello_3.index() << " SENT" << std::endl;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            }

            std::string encoding;
            std::vector<uchar> m_data;

            cv::imencode(".jpg"+encoding, img, m_data);
            std::stringstream out;
            std::vector<uchar> m_data1;
            for (int i = 0; i < m_data.size(); i++)
            {
                out << (uchar)m_data[i];
            }
            std::string send_str  ;
            send_str  += out.str();

            std::string jpg_base64 = base64_encode(send_str);
            HelloWorld hello_4;
            hello_4.index(i);
            hello_4.message(jpg_base64);
            if (publish1(hello_4))
            {
            std::cout << "[RTCP] Message: "  <<hello_4.message().size()<< " with index: "
            << hello_4.index() << " SENT" << std::endl;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    }
    else
    {
         std::cout << "mp3 \n";
        samples = samples/20;
        for (uint32_t i = 0; i < samples; ++i)
        {
                std::stringstream ss1;
                ss1<<std::setw(4)<<std::setfill('0')<<i+1;
                string name_mp3= ss1.str()+".mp3";
                std::cout<<name_mp3<<std::endl;
                std::ifstream t(name_mp3);
                std::stringstream buffer;
                buffer << t.rdbuf();
                std::string contents(buffer.str());
                std::cout<<contents.size()<<std::endl;

                if(contents.empty())
                {

                    std::cout<<name_mp3<<std::endl;
                    break;
                }
                // buf
                contents.insert(contents.size(),"mp3");

                std::string  base64_str = base64_encode(contents);
                HelloWorld hello_3;
                hello_3.index(i);
                hello_3.message(base64_str);
                if (publish1(hello_3))
                {
                //  std::cout<<"hello_3.message() "<<hello_3.message()<<std::endl;
                //logError(HW, "SENT " <<  hello_.index());
                std::cout << "[RTCP] Message: "  <<hello_3.message().size()<< " with index: "
                << hello_3.index() << " SENT" << std::endl;
                
                }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    }
      helper1.join();
}

void HelloWorldPublisher::run(
        uint32_t samples,
        long sleep_ms)
{
    std::thread thread(&HelloWorldPublisher::runThread, this, samples, sleep_ms);
    if (samples == 0)
    {
        std::cout << "Publisher running. Please press enter to stop_ the Publisher at any time." << std::endl;
        std::cin.ignore();
        stop_ = true;
    }
    else
    {
        std::cout << "Publisher running " << samples << " samples." << std::endl;
    }
    thread.join();
}



void HelloWorldPublisher::run1(
        uint32_t samples,
        std::string  name)
{
    std::thread thread(&HelloWorldPublisher::runThread1, this, samples, name);
    if (samples == 0)
    {
        std::cout << "Publisher running. Please press enter to stop_ the Publisher at any time." << std::endl;
        std::cin.ignore();
        stop_ = true;
    }
    else
    {
        std::cout << "Publisher running " << samples << " samples." << std::endl;
    }
    thread.join();
}




bool HelloWorldPublisher::publish(
        bool waitForListener)
{
    if (listener_.firstConnected || !waitForListener || listener_.n_matched > 0)
    {
        hello_.index(hello_.index() + 1);
        publisher_->write((void*)&hello_);
        return true;
    }
    return false;
}

bool HelloWorldPublisher::publish1(HelloWorld data)
{
    if (listener_.firstConnected ||  listener_.n_matched > 0)
    {
        data.index(data.index() + 1);
        publisher_->write((void*)&data);
        return true;
    }
    return false;
}
