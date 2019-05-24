#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <iostream>

#include "ros/ros.h"

#include "std_msgs/MultiArrayLayout.h"
#include "std_msgs/MultiArrayDimension.h"
#include "std_msgs/Int32MultiArray.h"

int Input[200];


void raw_Callback(const std_msgs::Int32MultiArray::ConstPtr& array)
{

	int i = 0;
	// print all the remaining numbers
	for(std::vector<int>::const_iterator it = array->data.begin(); it != array->data.end(); ++it)
	{
                Input[i] = *it;
		i++;
	}

        printf("got message!\n");
        printf("Sensor_1: \n");

        for(int j = 0; j < 200; j++)
        {
                printf("%d, ", Input[j]);
        }
        printf(" \n \n");
        return;
}


void subtractive_clustering(std_msgs::Int32MultiArray input);

/*
Vorgehen:
-PING Ultraschallsensor mit 2 einzelnen Ultraschallwandlern
-CW (Continous Wave) Ultraschall auf Objekt mit 1m konstantem Abstand; f=2,5 MHz
-Echo aufzeichnen mit 200 MS/s
-Echogramm sollte 4.000 Samples beinhalten
-Über FFT Signal in Frequenzbereich überführen (0-40MHz)
-200 Echosignale des Menschen, 20 Echosignale für je ein fremdes Objekt (z.B. Stuhl, Tür)
-Merkmalextraktion aus dem Echogramm (Zeitbereich):
        -Durschnitt
        -Standardabweichung
        -RMS
        -Summe Absolutwerte
        -Energie
        -Maximum
-Merkmalextraktion aus dem Echogramm (Frequenzbereich):
        -Unterteilung in je 4MHz Bereiche -> 10 Bereiche
        -Erfassung des Maximums und Medianwerts für jeden Bereich

-Auswertung mittels Fuzzy-Regeln basiertem Klassifizierer:
        -Rule i:if(x1~xj1*)and ...and(xn~xin*) then class P
        -Rule i = Regel 1 bis i
        -x=[x1,x2,...,xn] ist die Inputvarible als Vektor mit Merkmalen
        -j ist das so und so vielste Fuzzyset der i-ten Fuzzy-Rule
        -xi* ist der Schwerpunkt/Brennpunkt der i-ten Ruzzy-Rule
        -P ist der Klassenname (Zielklasse)

-die Schwerpunkte xi* müssen über Subtraktives Clustering erzeugt werden (subtractive clustering)
-hierbei vorallem sinnvoll das Verfahren nach Chiu wobei jeder Punkt als potentielles Cluster Center angesehen wird

*/


int main(int argc, char **argv)
{

	ros::init(argc, argv, "analyse");
	ros::NodeHandle n;	

        ros::Subscriber sub_raw = n.subscribe("raw", 200, raw_Callback);

	printf("\n");
        ros::spin ();
        return 0;
}


