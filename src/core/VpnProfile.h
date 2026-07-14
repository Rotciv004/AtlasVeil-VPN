#include <string>
#include "VpnState.h"
using namespace std;

class VpnProfile {
    private:
        string UUIDNetworkManager;
        string ProfileName;
        string Description;
        string Country;
        string City;
        string EndpointAdress;
        VpnState State;

    public:
        VpnProfile
            (
                string uuid, 
                string name, 
                string description, 
                string country, 
                string city,
                string endpoint,
                VpnState state
            ) : UUIDNetworkManager(uuid), 
                ProfileName(name), 
                Description(description), 
                Country(country), 
                City(city),
                EndpointAdress(endpoint),
                State(state)
        {}

        string getUUIDNetworkManager() const { return UUIDNetworkManager; }
        string getProfileName() const { return ProfileName; }
        string getDescription() const { return Description; }
        string getCountry() const { return Country; }
        string getCity() const { return City; }
        string getEndpointAdress() const { return EndpointAdress; }
        VpnState getState() const { return State; }

        void setUUIDNetworkManager(string uuid) { UUIDNetworkManager = uuid; }
        void setProfileName(string name) { ProfileName = name; }
        void setDescription(string description) { Description = description; }
        void setCountry(string country) { Country = country; }
        void setCity(string city) { City = city; }
        void setEndpointAdress(string endpoint) { EndpointAdress = endpoint; }
        void setState(VpnState state) { State = state; }
        

        void setAll
            (
                string uuid, 
                string name, 
                string description,
                string country, 
                string city, 
                string endpoint, 
                VpnState state
            )
        {
            UUIDNetworkManager = uuid;
            ProfileName = name;
            Description = description;
            Country = country;
            City = city;
            EndpointAdress = endpoint;
            State = state;
        }

        ~VpnProfile() = default;

    

};