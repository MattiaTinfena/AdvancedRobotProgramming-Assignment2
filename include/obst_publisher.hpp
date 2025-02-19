#ifndef OBST_PUBLISHER_HPP
#define OBST_PUBLISHER_HPP

#include "Generated/ObstaclesPubSubTypes.hpp"
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
#include "auxfunc.h"

using namespace eprosima::fastdds::dds;
#define USE_DEBUG 1

class ObstaclePublisher
{
private:
    Obstacles my_message_;
    DomainParticipant* participant_;
    Publisher* publisher_;
    Topic* topic_;
    DataWriter* writer_;
    TypeSupport type_;
    std::array<int, 4> ip_vector;
    FILE* obstFile;
    int port_;

    class PubListener : public DataWriterListener
    {
    public:
        PubListener(ObstaclePublisher* parent);
        ~PubListener() override;
        void on_publication_matched(DataWriter*, const PublicationMatchedStatus& info) override;
        std::atomic_int matched_;
        ObstaclePublisher* parent_;
    } listener_;

public:
    ObstaclePublisher();
    ~ObstaclePublisher();

    bool init();
    bool publish(MyObstacles myObstacles);
    bool parseFromJSON();


    #define LOGOBSTPUBLISHERMATCHING(obj, current_count_change) { \
        if (!(obj)->obstFile) { \
            perror("Log file not initialized.\n"); \
        } \
        char date[50]; \
        getFormattedTime(date, sizeof(date)); \
        if (current_count_change == 1) { \
            fprintf((obj)->obstFile, "%s Obstacle Publisher matched\n", date); \
        } else if (current_count_change == -1) { \
            fprintf((obj)->obstFile, "%s Obstacle Publisher unmatched.\n", date); \
        } else { \
            fprintf((obj)->obstFile, "%s %d is not a valid value\n", date, current_count_change); \
        } \
        fflush((obj)->obstFile); \
    }

    #if USE_DEBUG
    #define LOGPUBLISHNEWOBSTACLES(obj, obstacles) {\
        if (!(obj)->obstFile) { \
            perror("Log file not initialized.\n"); \
        } \
        char date[50]; \
        getFormattedTime(date, sizeof(date)); \
        fprintf((obj)->obstFile, "%s Obstacle published correctly:\n", date); \
        for (int t = 0; t < obstacles.obstacles_number(); t++) { \
            fprintf((obj)->obstFile, "(%d, %d) ", obstacles.obstacles_x()[t], obstacles.obstacles_y()[t]); \
        } \
        fprintf((obj)->obstFile, "\n"); \
        fflush((obj)->obstFile); \
    }
    #else
    #define LOGPUBLISHNEWOBSTACLES(obj, obstacles) 
    #endif
};

#endif // OBST_PUBLISHER_HPP
