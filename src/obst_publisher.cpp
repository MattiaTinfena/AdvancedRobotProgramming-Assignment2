/**
 * @file obst_publisher.cpp
 *
 */

#include "Generated/ObstaclesPubSubTypes.hpp"

#include <chrono>
#include <thread>

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/publisher/DataWriterListener.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>

#include "obst_publisher.hpp"  // Include the header file
#include "auxfunc2.hpp"

using namespace eprosima::fastdds::dds;
using namespace eprosima::fastdds::rtps;

// Remove class definition from here

// Constructor implementations
ObstaclePublisher::ObstaclePublisher()
    : participant_(nullptr)
    , publisher_(nullptr)
    , topic_(nullptr)
    , writer_(nullptr)
    , type_(new ObstaclesPubSubType())
{
}

ObstaclePublisher::~ObstaclePublisher()
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
    DomainParticipantFactory::get_instance()->delete_participant(participant_);
}

bool ObstaclePublisher::init()
{

    DomainParticipantQos participantQos;
    participantQos.wire_protocol().builtin.discovery_config.discoveryProtocol = DiscoveryProtocol::CLIENT;
    Locator_t locator;
    locator.port = 8888;
    IPLocator::setIPv4 (locator, 127,0,0,1);
    participantQos.wire_protocol().builtin.discovery_config.m_DiscoveryServers.push_back(locator);

    participant_ = DomainParticipantFactory::get_instance()->create_participant(0, participantQos);

    if (participant_ == nullptr)
    {
        return false;
    }

    // Register the Type
    type_.register_type(participant_);

    // Create the publications Topic
    topic_ = participant_->create_topic("topic1", type_.get_type_name(), TOPIC_QOS_DEFAULT);

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

bool ObstaclePublisher::publish(MyObstacles myObstacles){
    
    if (listener_.matched_ > 0){

        my_message_.obstacles_x().clear();
        my_message_.obstacles_y().clear();

        for (int i = 0; i < myObstacles.number; i++){
            my_message_.obstacles_x().push_back(myObstacles.x[i]);
            my_message_.obstacles_y().push_back(myObstacles.y[i]);
        }

        my_message_.obstacles_number(myObstacles.number);

        writer_->write(&my_message_);
        return true;
    }
    return false;
    //--------------------
    // TO LOG
    //--------------------
}

// Implement the listener class methods
ObstaclePublisher::PubListener::PubListener()
    : matched_(0)
{
}

ObstaclePublisher::PubListener::~PubListener()
{
}

void ObstaclePublisher::PubListener::on_publication_matched(DataWriter* writer, const PublicationMatchedStatus& info)
{
    if (info.current_count_change == 1)
    {
        matched_ = info.total_count;
        // std::cout << "Obstacle Publisher matched." << std::endl;
    }
    else if (info.current_count_change == -1)
    {
        matched_ = info.total_count;
        // std::cout << "Obstacle Publisher unmatched." << std::endl;
    }
    else
    {
        // std::cout << info.current_count_change << " is not a valid value for PublicationMatchedStatus current count change." << std::endl;
    }
}
