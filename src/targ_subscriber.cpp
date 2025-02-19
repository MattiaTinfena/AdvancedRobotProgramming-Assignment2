#include "Generated/TargetsPubSubTypes.hpp"
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
#include "targ_subscriber.hpp"  // Include il file header
#include "auxfunc.h"

using namespace eprosima::fastdds::dds;
using namespace eprosima::fastdds::rtps;
// Constructor implementations
TargetSubscriber::TargetSubscriber()
    : participant_(nullptr)
    , subscriber_(nullptr)
    , topic_(nullptr)
    , reader_(nullptr)
    , type_(new TargetsPubSubType())
    , listener_(this)
    , new_data_(false)  // Inizializza new_data_
{
}

TargetSubscriber::~TargetSubscriber()
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

bool TargetSubscriber::init()
{

    DomainParticipantQos server_qos = PARTICIPANT_QOS_DEFAULT;

    // Set participant as SERVER
    server_qos.wire_protocol().builtin.discovery_config.discoveryProtocol =
            DiscoveryProtocol::SERVER;

    // Set SERVER's listening locator for PDP
    Locator_t locator;
    IPLocator::setIPv4(locator, 192, 168, 15, 118);
    locator.port = 11812;
    server_qos.wire_protocol().builtin.metatrafficUnicastLocatorList.push_back(locator);

    /* Add a remote serve to which this server will connect */
    // Set remote SERVER's listening locator for PDP
    Locator_t remote_locator;
    IPLocator::setIPv4(remote_locator, 192, 168, 15, 96);
    remote_locator.port = 11813;

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
    topic_ = participant_->create_topic("topic2", type_.get_type_name(), TOPIC_QOS_DEFAULT);

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

void TargetSubscriber::run(){
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

// Implementazione del metodo per ottenere i dati ricevuti
MyTargets TargetSubscriber::getMyTargets()
{
    listener_.new_data_ = false;  // Resetta il flag quando i dati vengono letti
    return received_targets_;
}

// Implementazione del listener
TargetSubscriber::SubListener::SubListener(TargetSubscriber* parent)
    : samples_(0), parent_(parent), new_data_(false)
{
}

TargetSubscriber::SubListener::~SubListener()
{
}

void TargetSubscriber::SubListener::on_subscription_matched(DataReader* reader, const SubscriptionMatchedStatus& info)
{
    if (info.current_count_change == 1)
    {
        // std::cout << "Target Subscriber matched." << std::endl;
    }
    else if (info.current_count_change == -1)
    {
        // std::cout << "Target Subscriber unmatched." << std::endl;
    }
    else
    {
        // std::cout << info.current_count_change << " is not a valid value for SubscriptionMatchedStatus current count change." << std::endl;
    }
}

void convertTargetsToMyTargets(const Targets& targets, MyTargets& myTargets)
{
    myTargets.number = targets.targets_number();

    for (int i = 0; i < myTargets.number; i++)
    {
        myTargets.x[i] = targets.targets_x()[i];
        myTargets.y[i] = targets.targets_y()[i];
    }
}

bool TargetSubscriber::hasNewData() const
{
    return listener_.new_data_;
}


void TargetSubscriber::SubListener::on_data_available(DataReader* reader)
{
    SampleInfo info;
    if (reader->take_next_sample(&my_message_, &info) == eprosima::fastdds::dds::RETCODE_OK)
    {
        if (info.valid_data)
        {
            new_data_ = true;
            convertTargetsToMyTargets(my_message_, parent_->received_targets_);
        }
    }
}
