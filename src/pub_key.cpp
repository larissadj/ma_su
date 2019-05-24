#include "ros/ros.h"
#include "std_msgs/String.h"
#include <std_msgs/Int8.h>
#include "std_msgs/MultiArrayLayout.h"
#include "std_msgs/MultiArrayDimension.h"
#include "std_msgs/Int16MultiArray.h"
#include "std_msgs/Int32MultiArray.h"
#include <sstream>
#include <unistd.h>
#include <termios.h>
#include <string.h>
#include <iostream>
#include <stdio.h>
#include <iostream>
#include <vector>

using namespace std;

int* Arr = new int[200];

char getch() {
        char buf = 0;
        struct termios old = {0};
        if (tcgetattr(0, &old) < 0)
    perror("tcsetattr()");
        old.c_lflag &= ~ICANON;
        old.c_lflag &= ~ECHO;
        old.c_cc[VMIN] = 1;
        old.c_cc[VTIME] = 0;
        if (tcsetattr(0, TCSANOW, &old) < 0)
    perror("tcsetattr ICANON");
        if (read(0, &buf, 1) < 0)
    perror ("read()");
        old.c_lflag |= ICANON;
        old.c_lflag |= ECHO;
        if (tcsetattr(0, TCSADRAIN, &old) < 0)
    perror ("tcsetattr ~ICANON");
        return (buf);
}


void receivedData(const std_msgs::Int32MultiArray::ConstPtr& array){
    int i = 0;
    for(vector<int>::const_iterator it = array->data.begin(); it != array->data.end(); ++it)
    {
        Arr[i] = *it;
        i++;
    }

    return;
}

int pubKey(ros::Publisher chatter_pub, bool first = false){
    char key;
    key = getch();

    std_msgs::Int8 msg;
    msg.data = key - '0';

    chatter_pub.publish(msg);

    return msg.data;
}


int main(int argc, char **argv)
{

      ros::init(argc, argv, "key");
      ros::NodeHandle n;
      ros::Publisher chatter_pub = n.advertise<std_msgs::Int8>("key", 1000);
      ros::Subscriber sub_data = n.subscribe("us_data",100, &receivedData);
      ROS_INFO_STREAM("Select sensor with keyboard input");

      ros::Rate r(30);

      while (ros::ok())
      {
        int key = pubKey(chatter_pub);
        ros::spinOnce();

        if(key <= 2 && key >0){
            ROS_INFO_STREAM("Sensor #" << key);
            for(int i = 1; i < 200; i++)
            {
                cout << dec << Arr[i] << " ";
            }
        cout << endl;
        }else{
            ROS_INFO_STREAM("No sensor #" << key);
        }

        r.sleep();
      }
      return 0;
}
