cmake_minimum_required(VERSION 3.0.2)
project(ros_navigation_humanoid)

find_package(catkin REQUIRED COMPONENTS
  geometry_msgs
  roscpp
  rospy
  std_msgs
  pcd2pgm
  nav_msgs
  tf
  visualization_msgs
)

catkin_package(
 INCLUDE_DIRS 
 include
)

###########
## Build ##
###########
include_directories(
  include
  ${catkin_INCLUDE_DIRS}
)

add_executable(rviz_sim_node src/rviz_sim.cpp)
add_dependencies(rviz_sim_node ${${PROJECT_NAME}_EXPORTED_TARGETS} ${catkin_EXPORTED_TARGETS})
target_link_libraries(rviz_sim_node
  ${catkin_LIBRARIES}
)

add_executable(bag_play_node src/bag_play.cpp)
add_dependencies(bag_play_node ${${PROJECT_NAME}_EXPORTED_TARGETS} ${catkin_EXPORTED_TARGETS})
target_link_libraries(bag_play_node
  ${catkin_LIBRARIES}
)

add_executable(footstep_sim_node src/footstep_sim.cpp)
add_dependencies(footstep_sim_node ${${PROJECT_NAME}_EXPORTED_TARGETS} ${catkin_EXPORTED_TARGETS})
target_link_libraries(footstep_sim_node
  ${catkin_LIBRARIES}
)

#############
## Install ##
#############

if(CMAKE_INSTALL_PREFIX)
  message(STATUS "Installing to ${CMAKE_INSTALL_PREFIX}")

  install(TARGETS rviz_sim_node
    ARCHIVE DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
    LIBRARY DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
    RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
  )
  install(TARGETS bag_play_node
    ARCHIVE DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
    LIBRARY DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
    RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
  )

  install(TARGETS footstep_sim_node
    ARCHIVE DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
    LIBRARY DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
    RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
  )

  install(DIRECTORY include/${PROJECT_NAME}/
    DESTINATION ${CATKIN_PACKAGE_INCLUDE_DESTINATION}
  )

  install(DIRECTORY launch DESTINATION ${CATKIN_PACKAGE_SHARE_DESTINATION})
  install(DIRECTORY maps DESTINATION ${CATKIN_PACKAGE_SHARE_DESTINATION})
  install(DIRECTORY move_base_config DESTINATION ${CATKIN_PACKAGE_SHARE_DESTINATION})
  install(DIRECTORY rviz DESTINATION ${CATKIN_PACKAGE_SHARE_DESTINATION})
  install(DIRECTORY robot_descriptions DESTINATION ${CATKIN_PACKAGE_SHARE_DESTINATION})
  install(DIRECTORY resources DESTINATION ${CATKIN_PACKAGE_SHARE_DESTINATION})
endif()