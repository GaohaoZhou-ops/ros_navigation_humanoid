/**
 * @file    footstep_sim.cpp
 * @author  gaohao zhou (343905080@qq.com)
 * @brief 
 *          这个文件用来实现rviz仿真，通过订阅 /footstep_planner/footsteps_array 话题让机器人模型在rviz上运动
 * 
 * @version 0.1
 * @date    2025-04-09
 * 
 * @copyright Copyright (c) 2025
 * 
 * [Note]: 该节点目前为预览节点，精确的关节控制将在后期进行优化
 * 
 */


#include <ros/ros.h>
#include <geometry_msgs/Twist.h>
#include <nav_msgs/Odometry.h>
#include <tf/transform_broadcaster.h>
#include "geometry_msgs/PoseWithCovarianceStamped.h"        // /initialpose topic
#include "geometry_msgs/PoseStamped.h"                      // /move_base_simple/goal topic
#include "thread"
#include "visualization_msgs/MarkerArray.h"                 // /footstep_planner/footsteps_array
#include "thread"
#include "sensor_msgs/JointState.h"                         // 关节角度话题数据

geometry_msgs::PoseStamped current_pose;    // 当前全局定位pose
geometry_msgs::PoseStamped current_goal;    // 当前规划的全局目标点
ros::Publisher footplanner_goal_pub;        // 将 /move_base_simple/goal 话题数据转发到 /goal 话题上
ros::Publisher initialize_pose_pub;         // /initialpose 
ros::Subscriber footstep_marker_sub;        // 订阅规划出来的脚步marker话题
visualization_msgs::MarkerArray planned_footstep_marker;    // 规划出来的脚步marker

ros::Subscriber joint_states_sub;           // 关节状态订阅者
ros::Publisher joint_states_pub;            // 关节状态发布者

bool is_plann_update;                       // 当前目标点是否已经更新
sensor_msgs::JointState current_joint_states;   // 当前关节角度

geometry_msgs::PoseStamped left_ankle_pose;        // 左脚pose
geometry_msgs::PoseStamped right_ankle_pose;       // 右脚pose

float pelvis_to_foot_heigth;                // 腰关节高度


// 定位初始化话题
void DealInitializePoseCallback(geometry_msgs::PoseWithCovarianceStamped init_pose){
    ROS_WARN("[DealInitializePoseCallback]: Received initialize pose at position=[%.3lf, %.3lf, %.3lf] que=[%.3lf, %.3lf, %.3lf, %.3lf]",
        init_pose.pose.pose.position.x, init_pose.pose.pose.position.y, init_pose.pose.pose.position.z,
        init_pose.pose.pose.orientation.x, init_pose.pose.pose.orientation.y, init_pose.pose.pose.orientation.z, init_pose.pose.pose.orientation.w
    );
    current_pose.pose = init_pose.pose.pose;
    ROS_WARN("[DealInitializePoseCallback]: Robot localization reset done.");
}

// 导航目标点话题 - 这里只负责进行话题信息转发
void DealMoveGoalCallback(geometry_msgs::PoseStamped goal){
    ROS_WARN("[DealMoveGoalCallback]: Received Goal pose at position=[%.3lf, %.3lf, %.3lf], que=[%.3lf, %.3lf, %.3lf, %.3lf]",
        goal.pose.position.x, goal.pose.position.y, goal.pose.position.z,
        goal.pose.orientation.x, goal.pose.orientation.y, goal.pose.orientation.z, goal.pose.orientation.w
    );
    // 发布起点话题
    geometry_msgs::PoseWithCovarianceStamped init_pose;
    init_pose.header.frame_id = "map";
    init_pose.header.stamp = ros::Time::now();
    init_pose.pose.pose = current_pose.pose;
    initialize_pose_pub.publish(init_pose);
    ros::Duration(1).sleep();
    // 发布终点话题
    current_goal = goal;
    footplanner_goal_pub.publish(goal);
}

// 辅助函数，根据两只脚的位置与姿态计算腰当前的姿态
geometry_msgs::Pose CalculatePelvisPose(geometry_msgs::Pose left_ankle_pose, geometry_msgs::Pose right_ankle_pose){
    // Step1. 计算位置，因为在2D平面上所以直接取平均值即可
    geometry_msgs::Pose pelvis_pose;
    pelvis_pose.position.x = (left_ankle_pose.position.x + right_ankle_pose.position.x) / 2.0;
    pelvis_pose.position.y = (left_ankle_pose.position.y + right_ankle_pose.position.y) / 2.0;
    // Step2. 计算yaw姿态， 取两个yaw角的中间角度值
    double x_dir_m = (std::cos(tf::getYaw(left_ankle_pose.orientation)) + std::cos(tf::getYaw(right_ankle_pose.orientation))) / 2.0;
    double y_dir_m = (std::sin(tf::getYaw(left_ankle_pose.orientation)) + std::sin(tf::getYaw(right_ankle_pose.orientation))) / 2.0;
    pelvis_pose.orientation = tf::createQuaternionMsgFromYaw(std::atan2(y_dir_m, x_dir_m));
    return pelvis_pose;
}


// 辅助函数：pose1、pose2均在map坐标系下，计算pose1在pose2下的相对位置
geometry_msgs::PoseStamped ConvertPose1ToPose2Frame(geometry_msgs::PoseStamped pose1, geometry_msgs::PoseStamped pose2){
    geometry_msgs::PoseStamped converted_pose;
    
    tf::Transform pose1_trans;
    tf::poseMsgToTF(pose1.pose, pose1_trans);
    tf::Transform pose2_trans;
    tf::poseMsgToTF(pose2.pose, pose2_trans);
    tf::Transform trans_C = pose2_trans.inverse() * pose1_trans;

    tf::poseTFToMsg(trans_C, converted_pose.pose);
    return converted_pose;
}


// 更新关节角度信息
void UpdateJointStates(geometry_msgs::PoseStamped left_ankle_pose, geometry_msgs::PoseStamped pelive_pose, geometry_msgs::PoseStamped right_ankle_pose){
    // 计算两只脚在腰关节的相对位置，然后给一个acos值得到开合角度，然后更新到joint_states话题中
    geometry_msgs::PoseStamped left_in_pelive_pose = ConvertPose1ToPose2Frame(left_ankle_pose, pelive_pose);
    geometry_msgs::PoseStamped right_in_pelive_pose = ConvertPose1ToPose2Frame(right_ankle_pose, pelive_pose);

    double left_ankle_angle = std::atan2(fabs(left_in_pelive_pose.pose.position.x), pelvis_to_foot_heigth);
    double right_ankle_angle = std::atan2(fabs(right_in_pelive_pose.pose.position.x), pelvis_to_foot_heigth);

    if (left_in_pelive_pose.pose.position.x > 0.0){
        // Situation1. 左脚在前，右脚在后 -> 左负，右正
        left_ankle_angle = -1.0 * left_ankle_angle;
    }else{
        // Situation2. 右脚在前，左脚在后 -> 左正，右负
        right_ankle_angle = -1.0 * right_ankle_angle;
    }

    // 更新到joint_state话题中
    int left_hip_index = -1;
    int rigth_hip_index = -1;
    for (int i = 0; i < current_joint_states.name.size(); ++i){
        if ("left_hip_pitch_joint" == current_joint_states.name[i]){
            left_hip_index = i;
        }
        if ("right_hip_pitch_joint" == current_joint_states.name[i]){
            rigth_hip_index = i;
        }
    }
    if (left_hip_index == -1 || -1 == rigth_hip_index){
        return ;
    }

    printf("[Angle]: left_value=%.3lf, right_value=%.3lf\n", left_ankle_angle, right_ankle_angle);

    sensor_msgs::JointState new_states;
    new_states = current_joint_states;
    new_states.position[left_hip_index] = left_ankle_angle * 1.0;
    new_states.position[rigth_hip_index] = right_ankle_angle * 1.0;
    new_states.header.stamp = ros::Time::now();
    joint_states_pub.publish(new_states);
    return ;
}


// 关节运动计算线程
void JointsMotionCalculateThreadFunc(){
    ROS_WARN("[JointsMotionCalculateThreadFunc]: Enter joints motion calculate thread func.");
    ros::Rate loop_rate(10);
    while(ros::ok()){
        // 如果当前目标点没有更新，则以2Hz的频率待命
        if (false == is_plann_update){
            ros::Duration(0.5).sleep();
            continue;
        }
        ROS_INFO("[JointsMotionCalculateThreadFunc]: Goal updated at [%.3lf, %.3lf, %.3lf]-[%.3lf]", 
            current_goal.pose.position.x, current_goal.pose.position.y, current_goal.pose.position.z,
            tf::getYaw(current_goal.pose.orientation)
        );
        is_plann_update = false;                    // 重置标志位

        while(false == is_plann_update && ros::ok()){
            printf("Simulating robot joints...\n");
            for (int cal_index = 0; cal_index < planned_footstep_marker.markers.size() - 1; cal_index++){
                // 这里为了方便只算第一步是左脚的，如果第一步是右脚则直接跳过
                if (cal_index == 0 && planned_footstep_marker.markers[cal_index].color.g == 1){
                    ROS_WARN("First step is right, skip this frame...");
                    continue;
                }
                // 检查当前是否弹出
                if (true == is_plann_update){
                    ROS_WARN("Goal has changed.");
                    break;
                }
                // 需要根据当前左右脚与腰位置更新定位，核心是始终让腰在两只脚连线的中间，并保持姿态
                if (1 == planned_footstep_marker.markers[cal_index].color.r){               // 当前需要迈出左脚
                    left_ankle_pose.pose = planned_footstep_marker.markers[cal_index].pose;
                }
                if (1 == planned_footstep_marker.markers[cal_index].color.g){               // 当前需要迈出右脚
                    right_ankle_pose.pose = planned_footstep_marker.markers[cal_index+1].pose;
                }
                // 更新腰应该在的位置
                current_pose.pose = CalculatePelvisPose(left_ankle_pose.pose, right_ankle_pose.pose);
                // 更新关节信息
                UpdateJointStates(left_ankle_pose, current_pose, right_ankle_pose);
                ros::Duration(0.5).sleep();
            }
            ROS_INFO("Current goal sim done.");
            break;
        }
        ROS_WARN("[JointsMotionCalculateThreadFunc]: Detected goal changed, break this sim cricle.");
        // 在此处将姿态进行一次重置，主要是将两个大腿拧正
        loop_rate.sleep();
    }
    ROS_WARN("[JointsMotionCalculateThreadFunc]: Leave joints motion calculate thread func.");
}

// 订阅关节状态信息
void DealJointStatesSub(sensor_msgs::JointState joint_state){
    current_joint_states = joint_state;
}

// 订阅规划得到的脚步话题信息
void DealVisionliationMarker(visualization_msgs::MarkerArray marker_array){
    ROS_INFO("================================================================================");
    ROS_WARN("[DealVisionliationMarker]: Received planned footsteps array for [%.3lf, %.3lf, %.3lf]-[%.3lf], size=[%d]", 
        current_goal.pose.position.x, current_goal.pose.position.y, current_goal.pose.position.z,
        tf::getYaw(current_goal.pose.orientation),
        marker_array.markers.size()
    );
    // 在规划器中左脚是红色，右脚是绿色
    int step_count = 1;
    for(auto && iter : marker_array.markers){
        // 左脚
        if (1 == iter.color.r){
            printf("[%d/%d]: Left at pos=[%.3lf, %.3lf, %.3lf], yaw=[%.3lf]\n", 
                step_count, marker_array.markers.size(),
                iter.pose.position.x, iter.pose.position.y, iter.pose.position.z, 
                tf::getYaw(iter.pose.orientation)
            );
        }else if (1 == iter.color.g){
            printf("[%d/%d]: Right at pos=[%.3lf, %.3lf, %.3lf], yaw=[%.3lf]\n", 
                step_count, marker_array.markers.size(),
                iter.pose.position.x, iter.pose.position.y, iter.pose.position.z, 
                tf::getYaw(iter.pose.orientation)
            );
        }
        step_count++;
    }
    is_plann_update = true;                     // 规划结果更新
    planned_footstep_marker = marker_array;
}


int main(int argc, char** argv) {
    ros::init(argc, argv, "footstep_sim_node");
    ros::NodeHandle public_hd;

    tf::TransformBroadcaster tf_broadcaster_;
    ros::Subscriber init_sub = public_hd.subscribe("/initialpose", 1, DealInitializePoseCallback);
    ros::Subscriber goal_sub = public_hd.subscribe("/move_base_simple/goal", 1, DealMoveGoalCallback);
    footplanner_goal_pub = public_hd.advertise<geometry_msgs::PoseStamped>("/goal", 1);
    footstep_marker_sub = public_hd.subscribe("/footstep_planner/footsteps_array", 1, DealVisionliationMarker);
    initialize_pose_pub = public_hd.advertise<geometry_msgs::PoseWithCovarianceStamped>("/initialpose", 1);

    joint_states_sub = public_hd.subscribe("/joint_states", 1, DealJointStatesSub);
    joint_states_pub = public_hd.advertise<sensor_msgs::JointState>("/joint_states", 1);

    current_pose.header.frame_id = "map";
    current_pose.header.stamp = ros::Time::now();
    current_pose.pose.position.x = 0.0; current_pose.pose.position.y = 0.0; current_pose.pose.position.z = 0.0;
    current_pose.pose.orientation.x = 0.0; current_pose.pose.orientation.y = 0.0; current_pose.pose.orientation.z = 0.0; current_pose.pose.orientation.w = 1.0;

    left_ankle_pose.header.frame_id = "map";
    right_ankle_pose.header.frame_id = "map";

    is_plann_update = false;

    ros::NodeHandle private_hd("~");
    private_hd.param<float>("pelvis_to_foot_heigth", pelvis_to_foot_heigth, 0.8);

    std::thread joint_calc_thread = std::thread(JointsMotionCalculateThreadFunc);

    // 持续发布tf树信息
    ros::Rate loop_rate(50);
    while(ros::ok()){
        loop_rate.sleep();

        geometry_msgs::Quaternion odom_quat = current_pose.pose.orientation;
        geometry_msgs::TransformStamped map_trans;
        map_trans.header.stamp = ros::Time::now();

        map_trans.header.frame_id = "map";
        map_trans.child_frame_id = "pelvis";

        map_trans.transform.translation.x = current_pose.pose.position.x;
        map_trans.transform.translation.y = current_pose.pose.position.y;
        map_trans.transform.translation.z = pelvis_to_foot_heigth;        // 从腰关节到脚的高度
        map_trans.transform.rotation = odom_quat;
        tf_broadcaster_.sendTransform(map_trans);

        ros::spinOnce();
    }

    ros::waitForShutdown();
    joint_calc_thread.join();
    return 0;
}
