# ROS Navigation Humanoid

This package is used to implement ROS Navigation control and simulation of humanoid robots. There has a built-in Unitree G1 demo and a rosbag file package.

中文 [README.md](README_CN.md)

---

## Dependencies and third-party Tools

* ROS packages:
```bash
$ sudo apt-get install libpcl-dev ros-<distros>-nagivation ros-<distors>-tf ros-<distors>-sbpl ros-<distors>-visualization_msgs
```

* pgm map modify tool:
```bash
$ sudo apt-get install kolourpaint
```

* Hinson-A point cloud converter package:
```bash
$ cd src
$ git clone git@github.com:Hinson-A/pcd2pgm_package.git
```

* [Optional] OpenCV-3

If you do not need to perform gait planning experiments, you do not need to install the OpenCV-3 library, but the `foot_planner.launch` file in the repository will not work.

------

## Changelog

### April 10, 2025
* Add relocation function on rviz in simulation and real machine navigation;
* Fixed basic spelling errors in the README file;
* Add the function of Unitree G1 robot gait simulation;

### March 31, 2025
* Add launch file for subscribe target position and publish /cmd_vel `rviz_nav.launch`；

### March 06, 2025
* Add launch file for static display robot models in rviz: `static_display_without_map.launch`;
* Add launch file for path planning and navigation simulation with free control in rviz: `rviz_sim.launch`;
* Add launch file for rosbag playback: `bag_play.launch`;

------

## Compile

Two compilation methods are provided here. You can choose one of them according to your needs.

### Develop mode

```bash
$ catkin build
$ source devel/setup.bash
```

### Install mode

Notes:
* If you need to install after develop, first delete those folders `build`, `devel`, `logs`, `.catkin_tools` and then recompile using the following command;
* Install compile mode will copy the resource files to the install directory, if you make any changes to the launch or robot description files, it is recommended to recompile;

compile command:
```bash
$ catkin config --install     # Executed during the first compilation only
$ catkin build
$ source install/setup.bash
```

---

## How to Use

The built-in Unitree-G1 robot URDF and rosbag file can be used for static display and simulation. Four demos are provided in the `launch` folder:

* `static_display.launch`: Static display of robot model in rviz;
* `conv_pcd2pgm.launch`: Convert 3D point cloud map file to pgm map;
* `bag_play.launch`: Replay the data packet;
* `rviz_sim.launch`: Navigation simulation in rviz;

---

### static_display.launch

This file is used to statically display the robot urdf model in rviz. Run it with the following command:
```bash
$ roslaunch ros_navigation_humanoid static_display.launch
```

If you want to use your own robot model file, you can modify the following fields according to the actual situation:
```xml
<arg name="pelvis_to_foot_heigth" default="0.8" />
<arg name="robot_name" default="g1_29dof_with_hand"/>
<param name="robot_description" textfile="$(find ros_navigation_humanoid)/robot_descriptions/unitree_g1/$(arg robot_name).urdf" />
<node pkg="tf" type="static_transform_publisher" name="pelvis_2_map" args="0 0 -$(arg pelvis_to_foot_heigth) 0 0 0 1 pelvis map 100" />
```

![static_display](resources/static_display.gif)

---

### conv_pcd2pgm.launch

This file can convert point cloud map into pgm, relying on the `Hinson-A` package.

Note: This function only supports `pcd` format.

#### Step1. Move the point cloud map file to `ros_navigation_humanoid/maps` position.

```bash
$ roscd ros_navigation_humanoid/
$ mv map.pcd maps/
```

#### Step2. Adjust parameters in the launch file.

Adjust the configuration in `ros_navigation_humanoid/launch/conv_pcd2pgm.launch` as needed:

* `thre_z_min`: Minimum height of Z-axis;
* `thre_z_max`: Maximum height of Z-axis;
* `flag_pass_through`: 0 selects within the height range, 1 selects outside the height range;
* `thre_radius`: Radius: The radius of the filter;
* `thres_point_count`: The number of points for radius filtering;
* `map_resolution`: The resolution of pgm map;

#### Step3. Run launch file

```bash
$ roslaunch ros_navigation_humanoid conv_pcd2pgm.launch
```

#### Step4. Use map_server tool to save.


```bash
$ roscd ros_navigation_humanoid/maps/
$ rosrun map_server map_saver
```

After running, `map.pgm` and `map.yaml` files will be generated in the current path folder.

![conv_pcd2pgm](resources/conv_pcd2pgm.gif)

#### Step5. Edit pgm file by kolourpaint.

It is recommended to use the `kolourpaint` software to edit the map after conversion, mainly modifying obstacles and passable areas, and adding electronic fences to avoid planning in unexpected areas.

![edit_pgm](resources/edit_pgm.gif)

---

### rviz_nav.launch

This file uses `move_base` to simulate navigation in `rviz`, by subscribe rviz's button `2D Nav Goal` and publish `/move_base_simple/goal` topic to calculate cammand `/cmd_vel` 。

```bash
$ roslaunch ros_navigation_humanoid rviz_nav.launch
```

[Comming soom...]



---

### rviz_sim.launch

This file uses `move_base` to simulate navigation in `rviz`. You need to configure the path planner according to your needs, node will subscribe to the `/move_base_simple/goal` topic publish by rviz interface button `2D Nav Goal` and `/cmd_vel` topic will be published by the planner. This simulatiion will execute lossless.

```bash
$ roslaunch ros_navigation_humanoid rviz_sim.launch
```

![rviz_sim](resources/rviz_sim.gif)

---

### bag_play.launch

This file is used to replay rosbag to view the robot's movement. The data in the rosbag is required to contain at least **localization** information of the `geometry_msgs/PoseStamped` type, that is, the robot's position and posture in space.

```bash
$ roslaunch g1_ros1_nav bag_play.launch
```

![bag_play](resources/bag_play.gif)

---

### [Optional] foot_planner.launch

This file implements the robot gait simulation function. If you want to run it, you need to install the following third-party package:
```bash
$ cd src
$ git clone https://github.com/ahornung/humanoid_msgs
```

Then pull the `humanoid_navigation` package, but only two parts of this package are used here, so there is no need to put all of them in the compilation space:

```bash
$ cd ..
$ git clone git@github.com:ROBOTIS-GIT/humanoid_navigation.git
$ mv humanoid_navigation/footstep_planner src
$ mv humanoid_navigation/gridmap_2d src
```

Your workspace structure should be as follows:

```bash
$ tree

├── src
│ ├── footstep_planner
│ ├── gridmap_2d
│ ├── humanoid_msgs
│ ├── pcd2pgm_package
│ └── ros_navigation_humanoid
```

Compile the entire workspace:
```bash
$ catkin build
```

Choose one of the following commands:
```bash
$ source install/setup.bash # install mode
$ source devel/setup.bash   # devel mode
```

Run example:
```bash
$ roslaunch ros_navigation_humanoid foot_planner.launch
```

![foot_plan](resources/foot_plann.gif)

---

## Future Works

* Add control and feedback of each joint of the robot in the simulation environment;
* Integrate point cloud conversion and provide dynamic parameter server;