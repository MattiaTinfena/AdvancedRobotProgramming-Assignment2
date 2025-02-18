/**
 * @file obst_publisher.cpp
 *
 */

#include "Generated/TargetsPubSubTypes.hpp"

#include <chrono>
#include <thread>
#include <cjson/cJSON.h>
#include <fstream>

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/publisher/DataWriterListener.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>
#include <fastdds/rtps/transport/TCPv4TransportDescriptor.hpp>

#include "targ_publisher.hpp"  // Include the header file
#include "auxfunc2.hpp"

using namespace eprosima::fastdds::dds;
using namespace eprosima::fastdds::rtps;

// Remove class definition from here

// Constructor implementations
TargetPublisher::TargetPublisher()
    : participant_(nullptr)
    , publisher_(nullptr)
    , topic_(nullptr)
    , writer_(nullptr)
    , type_(new TargetsPubSubType())
    , file(nullptr)
    , port_(0)
    {
        std::fill(std::begin(ip_vector), std::end(ip_vector), 0);
        file = fopen("log/target.log", "a");  // Apri il file di log in modalità scrittura
        if (!file) {        
            std::cerr << "Errore nell'aprire il file di log!" << std::endl;
        }
    }

TargetPublisher::~TargetPublisher()
{
    if (writer_ != nullptr)
    {
        publisher_->delete_datawriter(writer_);
    }
    if (publisher_ != nullptr)
    {
        participant_->delete_publisher(publisher_);
    }
    if (topic_ != nullptr)
    {
        participant_->delete_topic(topic_);
    }

    if (file) {
        fclose(file);
    }
    DomainParticipantFactory::get_instance()->delete_participant(participant_);
}

bool TargetPublisher::parseFromJSON()
{
    FILE* settingsFile = fopen("appsettings.json", "r");
    if (!settingsFile) {
        return false;
    }

    fseek(settingsFile, 0, SEEK_END);
    long length = ftell(settingsFile);
    fseek(settingsFile, 0, SEEK_SET);
    char* data = (char*)malloc(length + 1);
    if (!data) {
        fclose(settingsFile);
        return false;
    }
    fread(data, 1, length, settingsFile);
    data[length] = '\0';
    fclose(settingsFile);

    cJSON* config = cJSON_Parse(data);
    free(data);
    if (!config) {
        return false;
    }

    cJSON* ip_array = cJSON_GetObjectItem(config, "IPServer");
    if (!cJSON_IsArray(ip_array) || cJSON_GetArraySize(ip_array) != 4) {
        cJSON_Delete(config);
        return false;
    }

    for (int i = 0; i < 4; i++) {
        ip_vector[i] = cJSON_GetArrayItem(ip_array, i)->valueint;
    }


    cJSON* port_item = cJSON_GetObjectItem(config, "portServerTarget");
    if (cJSON_IsNumber(port_item)) {
        port_ = port_item->valueint;
    }

    cJSON_Delete(config);
    return true;
}

bool TargetPublisher::init()
{
    if (!parseFromJSON()) {
        return false;
    }
    
    // Get default participant QoS
    DomainParticipantQos client_qos = PARTICIPANT_QOS_DEFAULT;

    // Set participant as CLIENT
    client_qos.wire_protocol().builtin.discovery_config.discoveryProtocol =
            DiscoveryProtocol::CLIENT;

    // Set SERVER's listening locator for PDP
    Locator_t locator;
    IPLocator::setIPv4(locator, (int)ip_vector[0], (int)ip_vector[1], (int)ip_vector[2], (int)ip_vector[3]);
    locator.port = port_;

    if (file) {
        fprintf(file, "ip %d %d %d %d | port %d\n", ip_vector[0], ip_vector[1], ip_vector[2], ip_vector[3], port_);
        fflush(file);
    }

    // Add remote SERVER to CLIENT's list of SERVERs
    client_qos.wire_protocol().builtin.discovery_config.m_DiscoveryServers.push_back(locator);

    // Set ping period to 250 ms
    client_qos.wire_protocol().builtin.discovery_config.discoveryServer_client_syncperiod =
        Duration_t(0, 250000000);

    participant_ = DomainParticipantFactory::get_instance()->create_participant(0, client_qos);
    
       if (participant_ == nullptr)
    {
        return false;
    }

    // Register the Type
    type_.register_type(participant_);

    // Create the publications Topic
    topic_ = participant_->create_topic("topic2", type_.get_type_name(), TOPIC_QOS_DEFAULT);

    if (topic_ == nullptr)
    {
        return false;
    }

    // Create the Publisher
    publisher_ = participant_->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr);

    if (publisher_ == nullptr)
    {
        return false;
    }

    // Create the DataWriter
    writer_ = publisher_->create_datawriter(topic_, DATAWRITER_QOS_DEFAULT, &listener_);

    if (writer_ == nullptr)
    {
        return false;
    }
    return true;
}

bool TargetPublisher::publish(MyTargets myTargets){
    
    if (listener_.matched_ > 0){

        my_message_.targets_x().clear();
        my_message_.targets_y().clear();

        for (int i = 0; i < myTargets.number; i++){
            my_message_.targets_x().push_back(myTargets.x[i]);
            my_message_.targets_y().push_back(myTargets.y[i]);
        }

        my_message_.targets_number(myTargets.number);

        writer_->write(&my_message_);
        return true;
    }
    return false;
    //--------------------
    // TO LOG
    //--------------------
}

// Implement the listener class methods
TargetPublisher::PubListener::PubListener()
    : matched_(0)
{
}

TargetPublisher::PubListener::~PubListener()
{
}

void TargetPublisher::PubListener::on_publication_matched(DataWriter* writer, const PublicationMatchedStatus& info)
{
    if (info.current_count_change == 1)
    {
        matched_ = info.total_count;
        // std::cout << "Target Publisher matched." << std::endl;
    }
    else if (info.current_count_change == -1)
    {
        matched_ = info.total_count;
        // std::cout << "Target Publisher unmatched." << std::endl;
    }
    else
    {
        // std::cout << info.current_count_change << " is not a valid value for PublicationMatchedStatus current count change." << std::endl;
    }
}
