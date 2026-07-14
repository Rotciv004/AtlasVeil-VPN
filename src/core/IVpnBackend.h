#include <string>
using namespace std;


class IVpnBackend {
protected:

    virtual ~IVpnBackend() = default;
    virtual void refreshProfiles() = 0;
    virtual void connectToProfile(const string& profileUUID) = 0;
    virtual void disconnectFromProfile(const string& profileUUID) = 0;

};