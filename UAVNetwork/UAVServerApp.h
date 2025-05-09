#ifndef __UAVSERVERAPP_H
#define __UAVSERVERAPP_H

#include <omnetpp.h>
#include "inet/applications/base/ApplicationBase.h"
#include "inet/transportlayer/contract/udp/UdpSocket.h"
#include "inet/common/lifecycle/LifecycleOperation.h"
#include "inet/common/packet/Packet.h"

using namespace omnetpp;
using namespace inet;

/**
 * UAVServerApp: Application pour le drone serveur qui collecte les données
 * des autres drones et les stocke
 */
class UAVServerApp : public ApplicationBase, public UdpSocket::ICallback {
  protected:
    // Configuration
    int localPort = -1;

    // État
    UdpSocket socket;
    bool isServerMode = true;

    // Pour le stockage des données
    struct SensorData {
        L3Address sourceAddr;
        simtime_t timestamp;
        std::string data;
    };
    std::vector<SensorData> storedData;

    // Statistiques
    int numReceived = 0;
    std::map<L3Address, int> packetsPerUAV;
    static simsignal_t rcvdPkSignal;

    // Pour la découverte des drones
    cMessage *statusUpdateTimer = nullptr;
    simtime_t statusInterval;

  protected:
    virtual void initialize(int stage) override;
    virtual void handleMessageWhenUp(cMessage *msg) override;
    virtual void finish() override;

    // Méthodes d'application
    virtual void processPacket(Packet *pk);
    virtual void sendStatusUpdate();

    // Méthodes du socket
    virtual void socketDataArrived(UdpSocket *socket, Packet *packet) override;
    virtual void socketErrorArrived(UdpSocket *socket, Indication *indication) override;
    virtual void socketClosed(UdpSocket *socket) override;

    // LifecycleOperation
    virtual void handleStartOperation(LifecycleOperation *operation) override;
    virtual void handleStopOperation(LifecycleOperation *operation) override;
    virtual void handleCrashOperation(LifecycleOperation *operation) override;

  public:
    UAVServerApp();
    virtual ~UAVServerApp();
};

#endif
