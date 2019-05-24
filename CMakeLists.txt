cmake_minimum_required(VERSION 2.8.3)
project(ma_usi)
find_Package(catkin REQUIRED COMPONENTS roscpp geometry_msgs turtlesim message_generation std_msgs)
#find_package(Curses REQUIRED)

#find_package( PkgConfig REQUIRED )
#pkg_check_modules ( ncurses++ REQUIRED ncurses++ )


##add_message_files(
	#FILES
       # Num.msg
#	pi.msg
#)

generate_messages(
	DEPENDENCIES
	std_msgs
	#geometry_msgs
)

catkin_package(
	CATKIN_DEPENDS message_runtime
)

include_directories(include ${catkin_INCLUDE_DIRS})

add_executable(client src/client.cpp)
target_link_libraries(client ${catkin_LIBRARIES})

add_executable(us src/us.cpp)
target_link_libraries(us ${catkin_LIBRARIES})

add_executable(pub_key src/pub_key.cpp)
target_link_libraries(pub_key ${catkin_LIBRARIES})

add_executable(analyse src/analyse.cpp)
target_link_libraries(analyse ${catkin_LIBRARIES})

#add_executable(pir pir.cpp)
#target_link_libraries(pir ${catkin_LIBRARIES})

#add_executable(pub_mayser pub_mayser.cpp)
#target_link_libraries(pub_mayser ${catkin_LIBRARIES})

#add_executable(sub_mayser sub_mayser.cpp)
#target_link_libraries(sub_mayser ${catkin_LIBRARIES})

#add_executable(pub_x pub_x.cpp)
#target_link_libraries(pub_x ${catkin_LIBRARIES})

#add_executable(ball_trajectory src/ball_trajectory.cpp) 
#target_link_libraries(ball_trajectory ${catkin_LIBRARIES})




