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
#include <cmath>
//#include <chrono>
#include <sys/time.h>

using namespace std;

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
    return socket_id;
}


vector <unsigned short> getPackage(int socket_id, const unsigned char packet[128])
{
    int numbytes = 0;
    const char* mode = reinterpret_cast<const char*>(packet);

    unsigned char receivedBytes[5000];

    send(socket_id, mode,strlen(mode),0);
    usleep(500);
    numbytes = recv(socket_id,receivedBytes,5000,0);

    if(numbytes == -1)
    {
            fprintf(stderr, "Error received \n");
            exit(4);
    }
    receivedBytes[numbytes] = '\0';

    //Ausgabe empfangene Daten vor der Verarbeitung
        //Header 128bit + 828bit Daten
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
    if(length!=0 && length == 828)
    {
        sensor_data.insert(sensor_data.begin(),receivedBytes+header_length,receivedBytes+(header_length+length));

        for (int i = 0; i < sensor_data.size(); ++i)
            cout << dec << setfill('0') << setw(2) << sensor_data[i] << " ";

        return sensor_data;
    }
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


int main(int argc, char ** argv)
{
        //Socket-Verbindung
        int socket_id = connectToServer("10.10.1.36","55015");

        //Serveranfragen
        const unsigned char subscribe[] = {0xAA, 0x43, 0x17, 0xFF};
        const unsigned char unsubscribe[256] = {0x55, 0xBC, 0xE8, 0x00};
        const unsigned char packet[128] = {0xBB, 0x53, 0x27, 0xFF};

        //Echogramm Datenaufteilung
        const int echo_Header = 7;
        const int data_Amount = 200;

        //Rosknoten starten
        ros::init(argc, argv, "spheres");
        ros::NodeHandle n;
        ros::Publisher marker_pub1 = n.advertise<visualization_msgs::Marker>("us_sensor_1", 10);
        ros::Publisher marker_pub2 = n.advertise<visualization_msgs::Marker>("us_sensor_2", 10);

        //Messungen in Hz
        ros::Rate r(30);
        float f = 0.0;

        while(ros::ok())
        {

            vector <unsigned short> sensor_data = getPackage(socket_id,packet);

            string key;
            //cin >> key;
            if (key == "s")
                return 0;

            if (!sensor_data.empty())
            {
                //Aufteilung der Sensoren
                vector <unsigned short> sensor1(sensor_data.begin()+echo_Header,sensor_data.begin()+(data_Amount+echo_Header));
                vector <unsigned short> sensor2(sensor_data.begin()+(3*echo_Header + 2*data_Amount),sensor_data.begin()+(3*echo_Header + 3*data_Amount));

                cout << endl << endl << "Sensor 1 : " << sensor1.size() << endl;
                for (int i = 0; i < sensor1.size(); ++i)
                    cout << dec << setfill('0') << setw(2) << sensor1[i] << " ";

                cout << endl << endl << "Sensor 2 : " << sensor2.size() << endl;
                for (int i = 0; i < sensor2.size(); ++i)
                    cout << dec << setfill('0') << setw(2) << sensor2[i] << " ";

                //Sensordaten publishen als Rostopic
                pubEcho(marker_pub1,f,sensor1,3);
                pubEcho(marker_pub2,f,sensor2,3);

                f+=0.04;
            }

        cout << endl << endl  << "*****************************************************************************************" << endl << endl;

        r.sleep();

        }
    close(socket_id);
    return 0;
}










