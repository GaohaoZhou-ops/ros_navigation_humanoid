/**
 * @file    bag_play.cpp
 * @author  gaohaozhou (gaohaozhou@deepglint.com)
 * @brief 
 *          这个文件是用来订阅包中的位置信息，然后播放到rviz上
 *      
 * 
 * @version 0.1
 * @date 2025-03-06
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include <ros/ros.h>
#include <geometry_msgs/Twist.h>
#include "nav_msgs/Path.h"
#include <nav_msgs/Odometry.h>
#include <tf/transform_broadcaster.h>
#include "queue"
#include "thread"
#include "mutex"

static geometry_msgs::PoseStamped global_pose;
static nav_msgs::Path trajectory_path;
static std::queue<geometry_msgs::PoseStamped> loc_queue;
static std::mutex loc_queue_mutex;

void DealLocaliazation3DCallback(geometry_msgs::PoseStamped loc){
    global_pose = loc;
    loc_queue.push(global_pose);
    loc_queue_mutex.lock();
    while(loc_queue.size() > 500){
        loc_queue.pop();
    }
    loc_queue_mutex.unlock();
}

int main(int argc, char *argv[])
{
    ros::init(argc, argv, "bag_play_node");
    ros::NodeHandle public_hd;
    ros::NodeHandle private_hd("~");

    float pelvis_to_foot_heigth = 0.8;
    std::string loc_topic;
    std::string trajectory_topic;

    private_hd.param<float>("pelvis_to_foot_heigth", pelvis_to_foot_heigth, 0.8);
    private_hd.param<std::string>("loc_topic", loc_topic, "/localization_3d");
    private_hd.param<std::string>("trajectory_topic", trajectory_topic, "/trajectory_path");

    tf::TransformBroadcaster tf_broadcaster_;

    ros::Subscriber loc_sub = public_hd.subscribe(loc_topic, 1, DealLocaliazation3DCallback);
    ros::Publisher trajectory_pub = public_hd.advertise<nav_msgs::Path>(trajectory_topic, 1);

    global_pose.pose.position.x=0.0; global_pose.pose.position.y=0.0; global_pose.pose.position.z=0.0;
    global_pose.pose.orientation.w = 1.0; global_pose.pose.orientation.x=0.0; global_pose.pose.orientation.y=0.0; global_pose.pose.orientation.z=0.0;

    trajectory_path.header.frame_id = "map";

    ros::Rate loop_rate(50);
    while(ros::ok()){
        loop_rate.sleep();

        geometry_msgs::TransformStamped map_trans;
        map_trans.header.stamp = ros::Time::now();

        map_trans.header.frame_id = "map";
        map_trans.child_frame_id = "pelvis";

        map_trans.transform.translation.x = global_pose.pose.position.x;
        map_trans.transform.translation.y = global_pose.pose.position.y;
        map_trans.transform.translation.z = pelvis_to_foot_heigth;        // 从腰关节到脚的高度
        map_trans.transform.rotation = global_pose.pose.orientation;
        tf_broadcaster_.sendTransform(map_trans);

        trajectory_path.header.stamp = map_trans.header.stamp;
        trajectory_path.poses.clear();
        loc_queue_mutex.lock();
        auto loc_que_copyed = loc_queue;
        loc_queue_mutex.unlock();
        while(!loc_que_copyed.empty()){
            trajectory_path.poses.emplace_back(loc_que_copyed.front());
            loc_que_copyed.pop();
        }
        trajectory_pub.publish(trajectory_path);

        ros::spinOnce();
    }

    return 0;
}
