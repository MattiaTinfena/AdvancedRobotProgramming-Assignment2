#include "Generated/TargetsPubSubTypes.hpp"
#include <chrono>
#include <thread>
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/DataReaderListener.hpp>
#include <fastdds/dds/subscriber/qos/DataReaderQos.hpp>
#include <fastdds/dds/subscriber/SampleInfo.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>
#include "targ_subscriber.hpp"  // Include il file header
#include "auxfunc2.hpp"

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
    // DomainParticipantQos participantQos;
    // participantQos.name("Participant_subscriber");
    // participant_ = DomainParticipantFactory::get_instance()->create_participant(1, participantQos);

    DomainParticipantQos qos;
    qos.wire_protocol().builtin.discovery_config.discoveryProtocol = DiscoveryProtocol::SERVER;
    qos.wire_protocol().builtin.discovery_config.m_DiscoveryServers.push_back(eprosima::fastdds::rtps::Locator_t());

    // qos.discoveryProtocol = DiscoveryProtocol_t::SERVER;
    // qos.builtin.discovery_config.m_DiscoveryServers.push_back();

    participant_ = DomainParticipantFactory::get_instance()->create_participant(0, qos);

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
