/**
 * @file    rviz_sim.cpp
 * @author  gaohao zhou (gaohaozhou@deepglint.com)
 * @brief 
 *          这个文件用来实现rviz仿真，通过订阅 /cmd_vel 数据【无损】计算里程计并发布到tf树上
 * 
 * @version 0.1
 * @date    2025-03-06
 * 
 * @copyright Copyright (c) 2025
 * 
 */


#include <ros/ros.h>
#include <geometry_msgs/Twist.h>
#include <nav_msgs/Odometry.h>
#include <tf/transform_broadcaster.h>
#include "thread"


double th_, x_, y_, z_;
ros::Time last_time_;


void DealCmdVel(geometry_msgs::Twist cmd_vel){
    double dt = (ros::Time::now() - last_time_).toSec();
    last_time_ = ros::Time::now();
    if (dt > 1.0){
        printf("[Debug: skip]\n");
        return ;
    }

    // 读取速度
    double v_x = cmd_vel.linear.x;
    double v_y = cmd_vel.linear.y;  // 适用于全向移动
    double omega = cmd_vel.angular.z;

    // 计算新的位姿 (基于里程计积分)
    x_ += (v_x * cos(th_) - v_y * sin(th_)) * dt;
    y_ += (v_x * sin(th_) + v_y * cos(th_)) * dt;
    th_ += omega * dt;

    // 归一化角度到 [-π, π]
    th_ = atan2(sin(th_), cos(th_));
}


int main(int argc, char** argv) {
    ros::init(argc, argv, "rviz_sim_node");
    ros::NodeHandle public_hd;

    x_=0.0, y_=0.0, z_=0.0, th_=0.0;

    tf::TransformBroadcaster tf_broadcaster_;
    ros::Subscriber cmd_sub = public_hd.subscribe("/cmd_vel", 1, &DealCmdVel);
    ros::NodeHandle private_hd("~");
    float pelvis_to_foot_heigth = 0.8;
    private_hd.param<float>("pelvis_to_foot_heigth", pelvis_to_foot_heigth, 0.8);


    ros::Rate loop_rate(50);
    while(ros::ok()){
        loop_rate.sleep();

        geometry_msgs::Quaternion odom_quat = tf::createQuaternionMsgFromYaw(th_);
        geometry_msgs::TransformStamped map_trans;
        map_trans.header.stamp = ros::Time::now();

        map_trans.header.frame_id = "map";
        map_trans.child_frame_id = "pelvis";

        map_trans.transform.translation.x = x_;
        map_trans.transform.translation.y = y_;
        map_trans.transform.translation.z = pelvis_to_foot_heigth;        // 从腰关节到脚的高度
        map_trans.transform.rotation = odom_quat;
        tf_broadcaster_.sendTransform(map_trans);

        ros::spinOnce();
    }

    ros::waitForShutdown();
    return 0;
}
