#include "Generated/ObstaclesPubSubTypes.hpp"
#include <chrono>
#include <thread>
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/DataReaderListener.hpp>
#include <fastdds/dds/subscriber/qos/DataReaderQos.hpp>
#include <fastdds/rtps/transport/TCPv4TransportDescriptor.hpp>
#include <fastdds/dds/subscriber/SampleInfo.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>
#include "obst_subscriber.hpp"  // Include the header file
#include "auxfunc2.hpp"

using namespace eprosima::fastdds::dds;
using namespace eprosima::fastdds::rtps;

// Le definizioni delle classi non dovrebbero essere incluse in obst_subscriber.cpp, ma solo dichiarate nel file header

// Constructor implementations
ObstacleSubscriber::ObstacleSubscriber()
    : participant_(nullptr)
    , subscriber_(nullptr)
    , topic_(nullptr)
    , reader_(nullptr)
    , type_(new ObstaclesPubSubType())
    , listener_(this)  
    , new_data_(false)
{
}

ObstacleSubscriber::~ObstacleSubscriber()
{
    if (reader_ != nullptr)
    {
        subscriber_->delete_datareader(reader_);
    }
    if (topic_ != nullptr)
    {
        participant_->delete_topic(topic_);
    }
    if (subscriber_ != nullptr)
    {
        participant_->delete_subscriber(subscriber_);
    }
    DomainParticipantFactory::get_instance()->delete_participant(participant_);
}

bool ObstacleSubscriber::init()
{
    DomainParticipantQos server_qos = PARTICIPANT_QOS_DEFAULT;

    // Set participant as SERVER
    server_qos.wire_protocol().builtin.discovery_config.discoveryProtocol =
            DiscoveryProtocol::SERVER;

    // Set SERVER's listening locator for PDP
    Locator_t locator;
    IPLocator::setIPv4(locator, 192, 168, 15, 118);
    locator.port = 11811;
    server_qos.wire_protocol().builtin.metatrafficUnicastLocatorList.push_back(locator);

    /* Add a remote serve to which this server will connect */
    // Set remote SERVER's listening locator for PDP
    Locator_t remote_locator;
    IPLocator::setIPv4(remote_locator, 192, 168, 15, 96);
    remote_locator.port = 11812;

    // Add remote SERVER to SERVER's list of SERVERs
    server_qos.wire_protocol().builtin.discovery_config.m_DiscoveryServers.push_back(remote_locator);

    participant_ = DomainParticipantFactory::get_instance()->create_participant(0, server_qos);

    if (participant_ == nullptr)
    {
        return false;
    }

    // Register the Type
    type_.register_type(participant_);

    // Create the subscriptions Topic
    topic_ = participant_->create_topic("topic1", type_.get_type_name(), TOPIC_QOS_DEFAULT);

    if (topic_ == nullptr)
    {
        return false;
    }

    // Create the Subscriber
    subscriber_ = participant_->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr);

    if (subscriber_ == nullptr)
    {
        return false;
    }

    // Create the DataReader
    reader_ = subscriber_->create_datareader(topic_, DATAREADER_QOS_DEFAULT, &listener_);

    if (reader_ == nullptr)
    {
        return false;
    }

    return true;
}

void ObstacleSubscriber::run(){

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

MyObstacles ObstacleSubscriber::getMyObstacles()
{
    listener_.new_data_ = false;  // Resetta il flag quando i dati vengono letti
    return received_obstacles_;
}


// Implement the listener class methods
ObstacleSubscriber::SubListener::SubListener(ObstacleSubscriber* parent)
    : samples_(0), parent_(parent), new_data_(false)
{
}

ObstacleSubscriber::SubListener::~SubListener()
{
}

void ObstacleSubscriber::SubListener::on_subscription_matched(DataReader* reader, const SubscriptionMatchedStatus& info)
{
    if (info.current_count_change == 1)
    {
        // std::cout << "Obstacle Subscriber matched." << std::endl;
    }
    else if (info.current_count_change == -1)
    {
        // std::cout << "Obstacle Subscriber unmatched." << std::endl;
    }
    else
    {
        // std::cout << info.current_count_change << " is not a valid value for SubscriptionMatchedStatus current count change." << std::endl;
    }
}

void convertObstaclesToMyObstacles(const Obstacles& obstacles, MyObstacles& myObstacles)
{
    myObstacles.number = obstacles.obstacles_number();

    for (int i = 0; i < myObstacles.number; i++)
    {
        myObstacles.x[i] = obstacles.obstacles_x()[i];
        myObstacles.y[i] = obstacles.obstacles_y()[i];
    }
}

bool ObstacleSubscriber::hasNewData() const
{
    return listener_.new_data_;
}

void ObstacleSubscriber::SubListener::on_data_available(DataReader* reader)
{
    SampleInfo info;
    if (reader->take_next_sample(&my_message_, &info) == eprosima::fastdds::dds::RETCODE_OK)
    {
        if (info.valid_data)
        {
            new_data_ = true;
            convertObstaclesToMyObstacles(my_message_, parent_->received_obstacles_);
        }
    }
}
