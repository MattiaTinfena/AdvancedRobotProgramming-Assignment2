#ifndef TARG_SUBSCRIBER_HPP
#define TARG_SUBSCRIBER_HPP

#include "Generated/TargetsPubSubTypes.hpp"
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
#include "auxfunc2.hpp"

using namespace eprosima::fastdds::dds;

class TargetSubscriber
{
private:
    DomainParticipant* participant_;
    Subscriber* subscriber_;
    DataReader* reader_;
    Topic* topic_;
    TypeSupport type_;
    MyTargets received_targets_;
    bool new_data_;
    std::array<int, 4> ip_vector_server;
    int port_server_;
    std::array<int, 4> ip_vector_client;
    int port_client_;
    FILE *logFile;

    class SubListener : public DataReaderListener
    {
    public:
        SubListener(TargetSubscriber* parent);
        ~SubListener() override;
        void on_subscription_matched(DataReader*, const SubscriptionMatchedStatus& info) override;
        void on_data_available(DataReader* reader) override;

        Targets my_message_;
        std::atomic_int samples_;
        TargetSubscriber* parent_;  // Puntatore alla classe principale
        bool new_data_;  // Aggiungi questa linea
    } listener_;

public:
    TargetSubscriber();
    ~TargetSubscriber();

    bool init();
    void run();
    MyTargets getMyTargets();  // Rimuovi "const"
    bool hasNewData() const;  // Aggiungi questa linea
    bool parseFromJSON();

    #define LOGTARGSUBSCRIPTION(current_count_change) { \
    if (!logFile) { \
        perror("Log file not initialized.\n"); \
        raise(SIGTERM); \
    } \
    char date[50]; \
    getFormattedTime(date, sizeof(date)); \
    if (current_count_change == 1) { \
        fprintf(logFile, "%s Subscription target matched\n", date); \
    } else if (current_count_change == -1) { \
        fprintf(logFile, "%s Subscription target un-matched\n", date); \
    } else { \
        fprintf(logFile, "%s %d is not a valid value for SubscriptionMatchedStatus current count change with target\n", date, current_count_change); \
    } \
    fflush(logFile); \
}
};

#endif // TARG_SUBSCRIBER_HPP
