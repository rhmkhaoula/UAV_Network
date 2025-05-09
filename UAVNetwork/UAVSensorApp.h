#ifndef __UAVSENSORAPP_H
#define __UAVSENSORAPP_H

#include <omnetpp.h>
#include "inet/applications/base/ApplicationBase.h"
#include "inet/transportlayer/contract/udp/UdpSocket.h"
#include "inet/common/lifecycle/LifecycleOperation.h"
#include "inet/common/packet/Packet.h"

using namespace omnetpp;
using namespace inet;

class UAVSensorApp : public ApplicationBase, public UdpSocket::ICallback {
  protected:
    // Configuration
    int localPort = -1;
    int destPort = -1;
    int serverStatusPort = -1;
    L3Address serverAddress;
    bool serverFound = false;

    // État
    UdpSocket socket;
    cMessage *sendTimer = nullptr;
    cMessage *serverCheckTimer = nullptr;
    simtime_t sendInterval;
    simtime_t serverCheckInterval;

    // Statistiques
    int numSent = 0;
    int numReceived = 0;
    static simsignal_t sentPkSignal;
    static simsignal_t rcvdPkSignal;

  protected:
    virtual void initialize(int stage) override;
    virtual void handleMessageWhenUp(cMessage *msg) override;
    virtual void finish() override;

    // Méthodes d'application
    virtual void sendSensorData();
    virtual void collectSensorData();
    virtual void checkForServer();

    // Méthodes du socket
    virtual void socketDataArrived(UdpSocket *socket, Packet *packet) override;
    virtual void socketErrorArrived(UdpSocket *socket, Indication *indication) override;
    virtual void socketClosed(UdpSocket *socket) override;

    // LifecycleOperation
    virtual void handleStartOperation(LifecycleOperation *operation) override;
    virtual void handleStopOperation(LifecycleOperation *operation) override;
    virtual void handleCrashOperation(LifecycleOperation *operation) override;

  public:
    UAVSensorApp();
    virtual ~UAVSensorApp();
};

#endif
