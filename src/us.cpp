#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <iostream>
//#include <ctime>
//#include <time.h>
//#include <cstring>
#include <iomanip>
//#include <vector>
#include <ros/ros.h>
#include <visualization_msgs/Marker.h>
#include <sensor_msgs/Range.h>
#include <cmath>
//#include <chrono>
#include <sys/time.h>
#include <unistd.h>
#include <termios.h>
#include "std_msgs/Int8.h"
#include "std_msgs/MultiArrayLayout.h"
#include "std_msgs/MultiArrayDimension.h"
#include "std_msgs/Int32MultiArray.h"
#include <fstream>
#include <sstream>
#include <iostream>


using namespace std;


int key;
vector <unsigned short> empty;




int connectToServer(const char* ip,const char* port)
{
    //Variables Declaration
    struct addrinfo hints, * res;
    int status;
    int socket_id;
    int n;

    //clear hints
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    status = getaddrinfo(ip,port, &hints, &res);

    socket_id = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

    status = connect(socket_id, res->ai_addr, res->ai_addrlen);
    if(status < 0)
    {
            fprintf(stderr, "Error connect \n");
            exit(3);
    }
    else{
            fprintf(stderr, "Connected \n");
    }
    fprintf(stderr, "before return socket_id \n");
    return socket_id;
    fprintf(stderr, "after return socket_id \n");
}


vector <unsigned short> getPackage(int socket_id, const unsigned char packet[128])
{
    fprintf(stderr, "getPackage started \n");
    int numbytes = 0;
    const char* mode = reinterpret_cast<const char*>(packet);

    unsigned char receivedBytes[50000];

    fprintf(stderr, "sending socket_id \n");

    send(socket_id, mode,strlen(mode),0);
    fprintf(stderr, "socket_id sent  \n");
    usleep(500);
    fprintf(stderr, "starting receiving bytes \n");
    numbytes = recv(socket_id,receivedBytes,50000,0);
    fprintf(stderr, "bytes_received \n");

    if(numbytes == -1)
    {
            fprintf(stderr, "Error received \n");
            exit(4);
    }
    receivedBytes[numbytes] = '\0';

    //Ausgabe empfangene Daten vor der Verarbeitung
    //
    //Header: 16 Byte
    //Daten Sensor1: 7 Byte Header + 200(0) Byte Analog + 7 Byte Header + 200(0) Byte Digital
    //Daten Sensor2: 7 Byte Header + 200(0) Byte Analog + 7 Byte Header + 200(0) Byte Digital
    //-> 16 Byte (Header) + 828 Byte (Daten inkl. Header) = 844 Byte (ALL cm)
    //-> 16 Byte (Header) + 8028 Byte (Daten inkl. Header) = 8044 Byte (ALL mm)

    printf("Allgemeiner Header (16 Byte): ");
    for(int i=0; i<16; i++)
    {
        printf("%02u ", receivedBytes[i]);
    }


    //Headerlänge, Packet-ID und reservierter Teil
    int const header_length = 16;
    char packet_id = receivedBytes[0];
    unsigned char reserved[3] = {receivedBytes[1],receivedBytes[2],receivedBytes[3]};

    printf("\n\nPacket-ID: %02X\n", packet_id);
    cout << "Reserved: ";
    for(int i=0; i< 3; i++)
    {
       printf("%02X ", reserved[i]);
    }


    //Zeitstempel
    unsigned char buffIn[8] = {receivedBytes[4],receivedBytes[5],receivedBytes[6],receivedBytes[7],receivedBytes[8],receivedBytes[9],receivedBytes[10],receivedBytes[11]};
    int64_t DTFromCDull = *reinterpret_cast<int64_t*>(buffIn);
    int64_t UnixEpoch = 0x089f7ff5f7b58000;

    struct tm *tm;
    struct tm *tmL;

    time_t t=(time_t)((DTFromCDull-UnixEpoch)/10000000);
    time_t ms = (time_t)((DTFromCDull-UnixEpoch)/10000);

    struct timeval tp;
    gettimeofday(&tp,NULL);
    time_t msL = (tp.tv_sec * 1000 + tp.tv_usec /1000) +3600000;
    time_t delay = (msL) - ms;

    time_t tL=(time_t)(msL/1000);
    tm=gmtime(&t);
    printf("\nPi: %s",asctime(tm));
    tmL=gmtime(&tL);
    printf("WS: %s",asctime(tmL));
    cout << "Zeit RPi: " << dec << ms << endl << "Zeit WS : " << msL << endl << "Latenz : " << delay << endl;


    //Länge des Datenblocks
    unsigned char laenge[4] = {receivedBytes[12],receivedBytes[13],receivedBytes[14],receivedBytes[15]};
    int32_t length = *reinterpret_cast<int32_t*>(laenge);

    cout << "Länge Datenblock: " << dec << length << "\nBuffergröße: " << sizeof(receivedBytes) << endl << endl;


    //Sensordaten (Trennung des Headers)
    vector<unsigned short> sensor_data;
    if(length!=0 && length == 828 || length == 1656 || length == 844) //844 bei mm Auflösung
    {
        sensor_data.insert(sensor_data.begin(),receivedBytes+header_length,receivedBytes+(header_length+length)); //length = 8(0)28, header_length = 16 => 8(0)48

        cout << endl<< endl << "sensor_data_size: " << sensor_data.size() << endl << endl;

        for (int i = 0; i < sensor_data.size(); ++i)
        {
            if(i==207 || i == 414 ||  i==621 || i==sensor_data.size()-1 )//einfach nur leichte Formatierung der Rohdaten
                cout << endl << endl;

            cout << dec << setfill('0') << setw(2) << sensor_data[i] << " ";
        }

    }
     return sensor_data;
}


void pubEcho(ros::Publisher marker_pub, float f, vector <unsigned short> sensor, int colour)
{
    //ROS
    visualization_msgs::Marker points, line_strip;
    points.header.frame_id = line_strip.header.frame_id = "/my_frame";
    points.header.stamp= line_strip.header.stamp = ros::Time::now();
    points.ns= line_strip.ns =  "spheres";
    points.action= line_strip.action = visualization_msgs::Marker::ADD;
    points.pose.orientation.w = line_strip.pose.orientation.w = 1.0;

    points.id = 0;
    line_strip.id = 1;

    points.type = visualization_msgs::Marker::POINTS;
    line_strip.type = visualization_msgs::Marker::LINE_STRIP;


    // POINTS markers use x and y scale for width/height respectively
    points.scale.x = 5;
    points.scale.y = 5;

    line_strip.scale.x = 2;

    // Points are red
    points.color.r = 1.0f;
    points.color.a = 1.0;

    // Line strip is blue
    line_strip.color.a = 1.0;

    switch(colour)
    {
        case 1:
            line_strip.color.r = 1.0;
        case 2:
            line_strip.color.g = 1.0;
        case 3:
            line_strip.color.b = 1.0;
    }


    // Create the vertices for the points and lines
    for (int i = 0; i < 200; ++i)
    {
      float x = i*10;
      float y = sensor[i];

      geometry_msgs::Point p;
      p.x = x;
      p.y = y;
      p.z = 0;

      points.points.push_back(p);
      line_strip.points.push_back(p);

    }
    marker_pub.publish(points);
    marker_pub.publish(line_strip);

}


void pubRange(ros::Publisher range_pub, vector <unsigned short> sensor){
    sensor_msgs::Range points;
    points.header.frame_id  = "/my_frame";
    points.header.stamp= ros::Time::now();

    points.radiation_type = 0;
    points.field_of_view = 0.59;
    points.min_range = 1;
    points.max_range = 2000;

    for (int i = 0; i < 200; ++i)
    {
        if(i > 9 && sensor[i] == 255){
            points.range = i*10;
        }
    }
    range_pub.publish(points);
}


void pubScale(ros::Publisher scale_pub,float f, int colour){
    visualization_msgs::Marker points, line_strip;
    points.header.frame_id = line_strip.header.frame_id = "/my_frame";
    points.header.stamp= line_strip.header.stamp = ros::Time::now();
    points.ns= line_strip.ns =  "spheres";
    points.action= line_strip.action = visualization_msgs::Marker::ADD;
    points.pose.orientation.w = line_strip.pose.orientation.w = 0;

    points.id = 0;
    line_strip.id = 1;

    points.type = visualization_msgs::Marker::POINTS;
    line_strip.type = visualization_msgs::Marker::LINE_STRIP;


    // POINTS markers use x and y scale for width/height respectively
    points.scale.x = 4;
    points.scale.y = 25;

    line_strip.scale.x = 4;

    // Points are red
    points.color.r = 0;
    points.color.a = 1;

    // Line strip is blue
    line_strip.color.a = 1;

    switch(colour)
    {
        case 1:
            line_strip.color.r = 0;
        case 2:
            line_strip.color.g = 0;
        case 3:
            line_strip.color.b = 0;
    }


    // Create the vertices for the points and lines
    for (int i = 0; i < 200; ++i)
    {
      float x = 0;
      if(i%10==0)
        x = i*10;

      geometry_msgs::Point p;
      p.x = x;
      p.y = 0;
      p.z = 0;

      points.points.push_back(p);
      line_strip.points.push_back(p);

    }
    scale_pub.publish(points);
    scale_pub.publish(line_strip);

}


void input(const std_msgs::Int8& msg){
    key = msg.data;
    //ROS_INFO_STREAM("KEY: " << msg.data);
    return;
}


void pubData(ros::Publisher pub_data,vector <unsigned short> sensor){

    std_msgs::Int32MultiArray array;

    array.layout.dim.push_back(std_msgs::MultiArrayDimension());
    array.layout.dim[0].size = sensor.size();
    array.layout.dim[0].stride = 1;
    array.layout.dim[0].label = "x";

    array.data.clear();
    array.data.insert(array.data.end(),sensor.begin(),sensor.end());

    pub_data.publish(array);
}

void writeIntoFile(vector <unsigned short> dat){
    ofstream myfile("testdata.txt", ios::out | ios::binary);
    ofstream myfile_2("testdata_raw.txt", ios::out | ios::binary);
    for (int i = 0;i<200;i++){

        std::ostringstream os;
        os << dat[i]; //<<"  i:  "<< i;
        myfile << "Sensor_1:  " <<os.str() << endl;
        myfile_2 << os.str() << endl;
    }

    myfile.close();
    myfile_2.close();

}



int main(int argc, char ** argv)
{
        fprintf(stderr, "started main function \n");
        //Socket-Verbindung
        int socket_id = connectToServer("10.10.1.36","55015");
        fprintf(stderr, "got socket_id \n");

        //Serveranfragen
        const unsigned char subscribe[] = {0xAA, 0x43, 0x17, 0xFF};
        const unsigned char unsubscribe[256] = {0x55, 0xBC, 0xE8, 0x00};
        const unsigned char packet[128] = {0xBB, 0x53, 0x27, 0xFF};

        //Echogramm Datenaufteilung
        const int echo_Header = 7;
        const int data_Amount = 200;

        fprintf(stderr, "starting rosnodes... \n");
        //Rosknoten starten
        ros::init(argc, argv, "spheres");
        ros::NodeHandle n;
        ros::Publisher marker_pub1 = n.advertise<visualization_msgs::Marker>("us_sensor_1", 10);
        ros::Publisher marker_pub2 = n.advertise<visualization_msgs::Marker>("us_sensor_2", 10);
        ros::Subscriber key_input = n.subscribe("key",100,&input);
        ros::Publisher pub_data = n.advertise<std_msgs::Int32MultiArray>("us_data",10);

        ros::Publisher range_pub1 = n.advertise<sensor_msgs::Range>("us_range_1",10);
        ros::Publisher range_pub2 = n.advertise<sensor_msgs::Range>("us_range_2",10);
        ros::Publisher scale_pub = n.advertise<visualization_msgs::Marker>("scale", 10);

        ros::Publisher raw_pub = n.advertise<std_msgs::Int32MultiArray>("raw",200);

        //Messungen in Hz
        ros::Rate r(1);
        float f = 0.0;
        fprintf(stderr, "started rosnodes! \n");
        while(ros::ok())
        {
            fprintf(stderr, "while loop started...starting getPackage() \n");
            vector <unsigned short> sensor_data = getPackage(socket_id,packet);
            //fprintf(stderr, "got Package! \n");

            if (!sensor_data.empty())
            {

                //fprintf(stderr, "sensor_data not empty :) \n \n");
                //Aufteilung der Sensoren
                ROS_INFO("got Data ");
                vector <unsigned short> sensor1(sensor_data.begin()+echo_Header,sensor_data.begin()+(data_Amount+echo_Header));
                vector <unsigned short> sensor2(sensor_data.begin()+(3*echo_Header + 2*data_Amount),sensor_data.begin()+(3*echo_Header + 3*data_Amount));


                cout << endl << endl << "Sensor 1 : " << sensor1.size() << endl;
                for (int i = 0; i < sensor1.size(); ++i)
                    cout << dec << setfill('0') << setw(2) << sensor1[i] << " ";

                writeIntoFile(sensor1);


                cout << endl << endl << "Sensor 2 : " << sensor2.size() << endl;
                for (int i = 0; i < sensor2.size(); ++i)
                    cout << dec << setfill('0') << setw(2) << sensor2[i] << " ";


                //Sensordaten als RAW publishen
                std_msgs::Int32MultiArray array;
                array.data.clear();
                for (int i=0;i<200;i++)
                {
                    array.data.push_back(sensor1[i]);
                }
                raw_pub.publish(array);



                //Sensordaten publishen als Rostopic
                //falls Kanäle in sensor_data vertauscht, dann auch hier tauschen
                if(sensor_data[0]==1){
                    pubEcho(marker_pub1,f,sensor1,3);
                    pubEcho(marker_pub2,f,sensor2,3);
                }else{
                    pubEcho(marker_pub1,f,sensor2,3);
                    pubEcho(marker_pub2,f,sensor1,3);
                }
                pubRange(range_pub1,sensor1);
                pubRange(range_pub2,sensor2);
                pubScale(scale_pub,f,3);

                f+=0.04;

                switch(key){
                    //case 0: pubData(pub_data,empty);
                    case 1:
                        cout << endl << "\nKEY PRESSED: " << key <<endl;
                        pubData(pub_data,sensor1);
                        break;
                    case 2:
                        cout << endl << "\nKEY PRESSED: " << key <<endl;
                        pubData(pub_data,sensor2);
                        break;
                    default: pubData(pub_data,empty);
                }
            }else {
                fprintf(stderr, "sensor_data empty!!\n");
            }

        cout << endl << endl  << "*****************************************************************************************" << endl << endl;



        ros::spinOnce();
        r.sleep();

        }
    close(socket_id);
    return 0;
}










