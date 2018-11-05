
#include <iostream>
#include "KDIS/PDU/Entity_Info_Interaction/Entity_State_PDU.h"
#include "KDIS/Network/Connection.h" // A cross platform connection class.


#include <iostream>
#include <fstream>
#include <algorithm>

// Lets declare all namespaces to keep the code small.
using namespace std;
using namespace KDIS;
using namespace DATA_TYPE;
using namespace PDU;
using namespace ENUMS;
using namespace UTILS;
using namespace NETWORK;

#define HALFpi	(1.570796326794896619)   /* pi / 2 */
#define SQR(x) ((x)*(x))
#define WGS84_ECC1_SQR	0.006694379990141399742371834884	/* first eccentricity squared */
#define WGS84_MAJOR		6378137.0		/* meters */
#define A				WGS84_MAJOR

struct PlaneInfo
{
	PlaneInfo(KFLOAT64 gpstime, KFLOAT64 geox, KFLOAT64 geoy, KFLOAT64 geoz, KFLOAT32 psi, KFLOAT32 theta, KFLOAT32 phi, KFLOAT32 r, Entity_State_PDU* entity) :GPSTIME(gpstime), loc(geox, geoy, geoz), ptp(psi, theta, phi), linearVelocity(0, 0, 0), angularVelocity(0, 0, 0), roll(r), deltaTimeInSec(0), entityPDU(entity) {}//linearAcceleration(0,0,0),
	KFLOAT64 GPSTIME;
	WorldCoordinates loc;
	EulerAngles ptp;
	Vector linearVelocity;
	Vector angularVelocity;
	//Vector linearAcceleration;
	KFLOAT32 roll;
	KFLOAT64 deltaTimeInSec;
	Entity_State_PDU* entityPDU;

};

void readFile(std::vector <PlaneInfo*>* data, char * name, Entity_State_PDU * entityPDU)
{
	ifstream file(name);
	std::string line;
	if (!file.good()){
		std::cout << __FUNCTION__ << "-could not read file" << std::endl;
		return;
	}
	KFLOAT64 Lat, Lon, Alt, GPSTIME;
	KFLOAT64 Heading, Pitch, Roll;
	KFLOAT64 velocity;
	KFLOAT64 GeoX, GeoY, GeoZ;
	KFLOAT64 Psi, Theta, Phi;
	float MDPTIME;

	while (file.good())
	{
		getline(file, line);
		if (line.compare("GPS TIME, MDP TIME, Alt,AOA,FPM_Az,FPM_El,Lat,Long,MACH,Mag_Hdg,Pitch,Roll,Velocity, Acc_Body_Z,HPT_TYPE")!=0)
			continue;
		
		getline(file, line);
		stringstream strstr(line);
		std::string value = "";
		getline(strstr, value, ',');
		GPSTIME = atof(value.c_str());
		getline(strstr, value, ',');
		MDPTIME = atof(value.c_str());
		getline(strstr, value, ',');
		Alt = atof(value.c_str())-10000;
		getline(strstr, value, ',');
		getline(strstr, value, ',');
		getline(strstr, value, ',');
		getline(strstr, value, ',');
		Lat = atof(value.c_str())*M_PI;
		getline(strstr, value, ',');
		Lon = atof(value.c_str())*M_PI;
		getline(strstr, value, ',');
		getline(strstr, value, ',');
		Heading = atof(value.c_str())*M_PI;
		getline(strstr, value, ',');
		Pitch = atof(value.c_str())*M_PI;
		getline(strstr, value, ',');
		Roll = atof(value.c_str())*M_PI;

		KDIS::UTILS::GeodeticToGeocentric(RadToDeg(Lat), RadToDeg(Lon), Alt, GeoX, GeoY, GeoZ, WGS_1984);
		KDIS::UTILS::HeadingPitchRollToEuler(Heading, Pitch, Roll, (Lat), (Lon), Psi, Theta, Phi);

		if (data->size() > 0) {
			PlaneInfo* prev = data->back();
			if (prev->entityPDU == entityPDU) {
				prev->deltaTimeInSec = (GPSTIME - prev->GPSTIME) / 1000;
				prev->angularVelocity.Set((Roll - prev->roll) / prev->deltaTimeInSec, 0, 0);
				prev->linearVelocity.Set((GeoX - prev->loc.GetX()) / prev->deltaTimeInSec, (GeoY - prev->loc.GetY()) / prev->deltaTimeInSec, (GeoZ - prev->loc.GetZ()) / prev->deltaTimeInSec);
			}
		}

		PlaneInfo* current = new PlaneInfo(GPSTIME, GeoX, GeoY, GeoZ, Psi, Theta, Phi, Roll, entityPDU);
		data->push_back(current);
	}
	//configureVelocityAcceleration(data);
}

Entity_State_PDU* createEntity(std::string mark, int ID)
{
	// First create the PDU we are going to send, for this example I will use a
	// Entity_State_PDU, When this PDU is sent most DIS applications should then show a new entity
	EntityIdentifier EntID(1, 3002, ID);
	ForceID EntForceID(Friendly);
	EntityType EntType(1, 2, 225, 1, 5, 1, 0); // F-16
	Vector EntityLinearVelocity(0, 0, 0);
	DeadReckoningParameter DRP(DeadReckoningAlgorithm::DRM_R_V_W, Vector(0, 0, 0), Vector(0, 0, 0));
	mark += std::to_string(long long(ID));
	EntityMarking EntMarking(ASCII, mark.c_str() , 5);
	EntityCapabilities EntEC(false, false, false, false);
	// Create the PDU
	Entity_State_PDU* entity = new Entity_State_PDU(EntID, EntForceID, EntType, EntType, EntityLinearVelocity, WorldCoordinates(0, 0, 0), EulerAngles(0, 0, 0), EntityAppearance(), DRP, EntMarking, EntEC);
	// Set the PDU Header values
	entity->SetExerciseID(1);
	// Set the time stamp to automatically calculate each time encode is called.
	entity->SetTimeStamp(TimeStamp(RelativeTime, 0, true));
	return entity;
}
bool compareFunc(PlaneInfo* i, PlaneInfo* j)
{
	return i->GPSTIME < j->GPSTIME;
}

int main(int argc , char* argv[])
{
	if (argc < 2) {
		std::cout << "Need files to read locations" << std::endl;
		system("pause");
		return 0;
	}
	
	std::vector <PlaneInfo*> data;

	int count = 1;
	while (count < argc)
	{
		Entity_State_PDU* plane = createEntity("KARL", count);
		readFile(&data, argv[count], plane);
		count++;
	}
	std::sort(data.begin(), data.end(), compareFunc);

	try {
		while (true)
		{
			// Note: This address will probably be different for your network.
			Connection myConnection("255.255.255.255");
			// Encode the PDU contents into network data
			KDataStream stream;
			std::vector<PlaneInfo*>::iterator it = data.begin();
			std::vector<PlaneInfo*>::iterator itEnd = data.end();
			std::cout.precision(10);
			while (it != itEnd)
			{
				//KDataStream* stream = ((*it)->entityPDU->GetEntityIdentifier().GetEntityID() == 1) ? &stream1 : &stream2;
				stream.Clear();
				(*it)->entityPDU->SetEntityLocation((*it)->loc);
				(*it)->entityPDU->SetEntityOrientation((*it)->ptp);
				(*it)->entityPDU->SetEntityLinearVelocity((*it)->linearVelocity);
				//Entity.GetDeadReckoningParameter().SetLinearAcceleration((*it).linearAcceleration);
				(*it)->entityPDU->GetDeadReckoningParameter().SetAngularVelocity((*it)->angularVelocity);
				(*it)->entityPDU->Encode(stream);
				myConnection.Send(stream.GetBufferPtr(), stream.GetBufferSize());
				cout << "Sent the PDU for " << (*it)->entityPDU->GetEntityMarking().GetAsString() << " GPS Time:" << (*it)->GPSTIME << endl;

				KFLOAT64 t = (*it)->GPSTIME;
				it++;
				if (it != itEnd) {
					cout << "Sleep for " << (*it)->GPSTIME - t << std::endl;
					Sleep(((*it)->GPSTIME - t));
				}
			}
		}
	}
	catch (exception & e)
	{
		cout << e.what() << endl;
	}

	return 0;
}

//
//void configureVelocityAcceleration(std::vector <PlaneInfo>* data)
//{
//	for (int count = 0; count < data->size(); count++)
//	{
//		PlaneInfo* current = &(*data)[count];
//		if (count + 1 < data->size()) {
//			PlaneInfo* next = &(*data)[count + 1];
//			current->deltaTimeInSec = (next->MDPTIME - current->MDPTIME) / 1000;
//			WorldCoordinates length = next->loc - current->loc;
//			
//			Vector angularDelta = (next->HPR - current->HPR);
//			current->angularVelocity.Set(angularDelta.GetZ() / current->deltaTimeInSec, 0,  0);
//			current->linearVelocity.Set((next->loc.GetX() - current->loc.GetX()) / current->deltaTimeInSec,
//										(next->loc.GetY() - current->loc.GetY()) / current->deltaTimeInSec,
//										(next->loc.GetZ() - current->loc.GetZ()) / current->deltaTimeInSec);
//		}
//	}
//}
//std::cout << speed.GetMagnitude() << " " <<  speed.GetMagnitude()*3.6 << std::endl;

//KFLOAT64 getAxisAcceleration(KFLOAT64 length, KFLOAT64 currentVelocity, KFLOAT64 time)
//{
//	/// a = 2*(D-Vi*t)/t^2
//	return 2 * (length - currentVelocity*time) / pow(time, 2);
//}
//std::cout << i.MDPTIME << " " << i.Alt << " " << i.Lat << " " << i.Lon << " " << i.Heading << " " << i.Pitch << " " << i.Roll << std::endl;
//,, angularDelta.GetX() / current->deltaTimeInSec angularDelta.GetY() / current->deltaTimeInSec

/*if (count > 0) {
PlaneInfo* prev = &(*data)[count - 1];
current->linearVelocity = getVelocity(prev->linearVelocity, prev->linearAcceleration, prev->deltaTimeInSec);
}*/
/*current->linearAcceleration.Set(getAxisAcceleration(length.GetX(), current->linearVelocity.GetX(), current->deltaTimeInSec),
getAxisAcceleration(length.GetY(), current->linearVelocity.GetY(), current->deltaTimeInSec),
getAxisAcceleration(length.GetZ(), current->linearVelocity.GetZ(), current->deltaTimeInSec));*/


//void readFile(std::vector <PlaneInfo*>* data, char * name, Entity_State_PDU * entityPDU)
//{
//	ifstream file(name);
//	std::string line;
//	if (file.good())
//		getline(file, line); // read first row- column description
//	else {
//		std::cout << __FUNCTION__ << "-could not read file" << std::endl;
//		return;
//	}
//	KFLOAT64 Lat, Lon, Alt, GPSTIME;
//	KFLOAT64 Heading, Pitch, Roll;
//	KFLOAT64 velocity;
//	KFLOAT64 GeoX, GeoY, GeoZ;
//	KFLOAT64 Psi, Theta, Phi;
//	float MDPTIME;
//
//	while (file.good())
//	{
//		getline(file, line);
//		stringstream strstr(line);
//		std::string value = "";
//		getline(strstr, value, ',');
//		GPSTIME = atof(value.c_str());
//		getline(strstr, value, ',');
//		MDPTIME = atof(value.c_str());
//		getline(strstr, value, ',');
//		Alt = atof(value.c_str());
//		getline(strstr, value, ',');
//		getline(strstr, value, ',');
//		getline(strstr, value, ',');
//		getline(strstr, value, ',');
//		Lat = atof(value.c_str())*M_PI;
//		getline(strstr, value, ',');
//		Lon = atof(value.c_str())*M_PI;
//		getline(strstr, value, ',');
//		getline(strstr, value, ',');
//		Heading = atof(value.c_str())*M_PI;
//		getline(strstr, value, ',');
//		Pitch = atof(value.c_str())*M_PI;
//		getline(strstr, value, ',');
//		Roll = atof(value.c_str())*M_PI;
//
//		KDIS::UTILS::GeodeticToGeocentric(RadToDeg(Lat), RadToDeg(Lon), Alt, GeoX, GeoY, GeoZ, WGS_1984);
//		KDIS::UTILS::HeadingPitchRollToEuler(Heading, Pitch, Roll, (Lat), (Lon), Psi, Theta, Phi);
//
//		if (data->size() > 0) {
//			PlaneInfo* prev = data->back();
//			if (prev->entityPDU == entityPDU) {
//				prev->deltaTimeInSec = (GPSTIME - prev->GPSTIME) / 1000;
//				prev->angularVelocity.Set((Roll - prev->roll) / prev->deltaTimeInSec, 0, 0);
//				prev->linearVelocity.Set((GeoX - prev->loc.GetX()) / prev->deltaTimeInSec, (GeoY - prev->loc.GetY()) / prev->deltaTimeInSec, (GeoZ - prev->loc.GetZ()) / prev->deltaTimeInSec);
//			}
//		}
//
//		PlaneInfo* current = new PlaneInfo(GPSTIME, GeoX, GeoY, GeoZ, Psi, Theta, Phi, Roll, entityPDU);
//		data->push_back(current);
//	}
//	//configureVelocityAcceleration(data);
//}
