#ifndef OBST_SUBSCRIBER_HPP
#define OBST_SUBSCRIBER_HPP

#include "Generated/ObstaclesPubSubTypes.hpp"
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/DataReaderListener.hpp>
#include <fastdds/dds/subscriber/qos/DataReaderQos.hpp>
#include <fastdds/dds/subscriber/SampleInfo.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>
#include <atomic>
#include <iostream>
#include <thread>
#include <chrono>
#include "auxfunc.h"

using namespace eprosima::fastdds::dds;

class ObstacleSubscriber
{
private:
    DomainParticipant* participant_;
    Subscriber* subscriber_;
    DataReader* reader_;
    Topic* topic_;
    TypeSupport type_;
    MyObstacles received_obstacles_;  // Variabile che contiene i dati ricevuti
    bool new_data_;

    class SubListener : public DataReaderListener
    {
    public:
        SubListener(ObstacleSubscriber* parent);
        ~SubListener() override;
        void on_subscription_matched(DataReader*, const SubscriptionMatchedStatus& info) override;
        void on_data_available(DataReader* reader) override;

        Obstacles my_message_;
        std::atomic_int samples_;
        ObstacleSubscriber* parent_;  // Puntatore alla classe principale
        bool new_data_;
    } listener_;

public:
    ObstacleSubscriber();
    ~ObstacleSubscriber();

    bool init();
    void run();
    MyObstacles getMyObstacles();  // Metodo per accedere ai dati ricevuti
    bool hasNewData() const;
};

#endif // HELLO_WORLD_SUBSCRIBER_HPP
