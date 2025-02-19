#ifndef TARG_PUBLISHER_HPP
#define TARG_PUBLISHER_HPP

#include "Generated/TargetsPubSubTypes.hpp"
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/publisher/DataWriterListener.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>
#include <atomic>
#include <iostream>
#include <thread>
#include <chrono>
#include "auxfunc2.hpp"

using namespace eprosima::fastdds::dds;
#define USE_DEBUG 1

class TargetPublisher
{
private:
    Targets my_message_;
    DomainParticipant* participant_;
    Publisher* publisher_;
    Topic* topic_;
    DataWriter* writer_;
    TypeSupport type_;
    std::array<int, 4> ip_vector;
    FILE *targFile;
    int port_;
    
    class PubListener : public DataWriterListener
    {
    public:
        PubListener(TargetPublisher* parent);
        ~PubListener() override;
        void on_publication_matched(DataWriter*, const PublicationMatchedStatus& info) override;
        std::atomic_int matched_;
        TargetPublisher* parent_;
    } listener_;

public:
    TargetPublisher();
    ~TargetPublisher();

    bool init();
    bool publish(MyTargets targets);
    bool parseFromJSON();

    #define LOGTARGPUBLISHERMATCHING(obj, current_count_change) { \
        if (!(obj)->targFile) { \
            perror("Log file not initialized.\n"); \
            raise(SIGTERM); \
        } \
        char date[50]; \
        getFormattedTime(date, sizeof(date)); \
        if (current_count_change == 1) { \
            fprintf((obj)->targFile, "%s Publisher matched\n", date); \
        } else if (current_count_change == -1) { \
            fprintf((obj)->targFile, "%s Publisher un-matched\n", date); \
        } else { \
            fprintf((obj)->targFile, "%s %d is not a valid value for PublicationMatchedStatus current count change\n", date, current_count_change); \
        } \
        fflush((obj)->targFile); \
    }

    #if USE_DEBUG
    #define LOGPUBLISHNEWTARGET(obj, targets) { \
        if (!(obj)->targFile) { \
            perror("Log file not initialized.\n"); \
        } \
        char date[50]; \
        getFormattedTime(date, sizeof(date)); \
        fprintf((obj)->targFile, "%s Target published correctly:\n", date); \
        for (int t = 0; t < targets.targets_number(); t++) { \
            fprintf((obj)->targFile, "(%d, %d) ", targets.targets_x()[t], targets.targets_y()[t]); \
        } \
        fprintf((obj)->targFile, "\n"); \
        fflush((obj)->targFile); \
    }
    #else
    #define LOGPUBLISHNEWTARGET(obj, targets) {}
    #endif
};

#endif // TARG_PUBLISHER_HPP
