#include "UAVServerApp.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/TimeTag_m.h"
#include "inet/common/packet/chunk/ByteCountChunk.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/transportlayer/contract/udp/UdpControlInfo_m.h"
#include "inet/networklayer/common/L3AddressTag_m.h"

Define_Module(UAVServerApp);

simsignal_t UAVServerApp::rcvdPkSignal = registerSignal("rcvdPk");

UAVServerApp::UAVServerApp() {
}

UAVServerApp::~UAVServerApp() {
    cancelAndDelete(statusUpdateTimer);
}

void UAVServerApp::initialize(int stage) {
    ApplicationBase::initialize(stage);

    if (stage == INITSTAGE_LOCAL) {
        localPort = par("localPort");
        statusInterval = par("statusInterval");
        numReceived = 0;
        WATCH(numReceived);
        WATCH(storedData);
    }
    else if (stage == INITSTAGE_APPLICATION_LAYER) {
        statusUpdateTimer = new cMessage("statusUpdateTimer");

        socket.setOutputGate(gate("socketOut"));
        socket.bind(localPort);
        socket.setCallback(this);

        // Démarrer les mises à jour de statut si en mode opérationnel
        if (operationalState == State::OPERATING) {
            scheduleAt(simTime() + uniform(0, 1), statusUpdateTimer);
        }
    }
}

void UAVServerApp::handleMessageWhenUp(cMessage *msg) {
    if (msg->isSelfMessage()) {
        if (msg == statusUpdateTimer) {
            sendStatusUpdate();
            scheduleAt(simTime() + statusInterval, statusUpdateTimer);
        }
    }
    else
        socket.processMessage(msg);
}

void UAVServerApp::sendStatusUpdate() {
    // Envoyer un message broadcast pour indiquer que ce drone est le serveur
    char msgName[32];
    sprintf(msgName, "ServerStatus-%d", numReceived);

    // Créer paquet
    Packet *packet = new Packet(msgName);

    // Ajouter un tag de temps
    auto creationTimeTag = packet->addTag<CreationTimeTag>();
    creationTimeTag->setCreationTime(simTime());

    // Message simple pour annoncer qu'il s'agit du serveur
    const auto& payload = makeShared<ByteCountChunk>(B(100), 'S');  // 'S' pour Server
    packet->insertAtBack(payload);

    // Envoyer en broadcast sur le port destiné aux UAVs capteurs
    socket.sendTo(packet, L3Address(), par("statusPort").intValue());

    EV_INFO << "Sent server status update broadcast" << endl;
}

void UAVServerApp::socketDataArrived(UdpSocket *socket, Packet *packet) {
    // Traitement des données reçues des UAVs
    auto addressInd = packet->getTag<L3AddressInd>();
    L3Address srcAddr = addressInd->getSrcAddress();

    // Calculer le délai de bout en bout
    auto creationTimeTag = packet->getTag<CreationTimeTag>();
    simtime_t delay = simTime() - creationTimeTag->getCreationTime();

    EV_INFO << "Received packet " << packet->getName() << " from UAV at "
            << srcAddr.str() << ". Delay: " << delay << "s" << endl;

    // Mettre à jour les statistiques
    numReceived++;
    packetsPerUAV[srcAddr]++;
    emit(rcvdPkSignal, packet);

    // Traiter et stocker les données
    processPacket(packet);

    delete packet;
}

void UAVServerApp::processPacket(Packet *pk) {
    // Extraction et stockage des données
    auto addressInd = pk->getTag<L3AddressInd>();
    auto creationTimeTag = pk->getTag<CreationTimeTag>();

    // Dans un cas réel, on extrairait les données du paquet
    // Ici on simule juste le stockage
    SensorData newData;
    newData.sourceAddr = addressInd->getSrcAddress();
    newData.timestamp = creationTimeTag->getCreationTime();
    newData.data = std::string(pk->getName()) + " received at " + std::to_string(simTime().dbl());

    // Stocker les données
    storedData.push_back(newData);

    EV_INFO << "Stored data from UAV at " << newData.sourceAddr.str()
            << ", total stored packets: " << storedData.size() << endl;
}

void UAVServerApp::socketErrorArrived(UdpSocket *socket, Indication *indication) {
    EV_WARN << "Socket error: " << indication->getName() << endl;
    delete indication;
}

void UAVServerApp::socketClosed(UdpSocket *socket) {
    if (operationalState == State::STOPPING_OPERATION) {
        startActiveOperationExtraTimeOrFinish(par("stopOperationExtraTime"));
    }
}

void UAVServerApp::handleStartOperation(LifecycleOperation *operation) {
    socket.setOutputGate(gate("socketOut"));
    socket.bind(localPort);
    socket.setCallback(this);

    statusUpdateTimer = new cMessage("statusUpdateTimer");
    scheduleAt(simTime() + uniform(0, 1), statusUpdateTimer);
}

void UAVServerApp::handleStopOperation(LifecycleOperation *operation) {
    cancelEvent(statusUpdateTimer);
    socket.close();
    delayActiveOperationFinish(par("stopOperationTimeout"));
}

void UAVServerApp::handleCrashOperation(LifecycleOperation *operation) {
    cancelEvent(statusUpdateTimer);
    socket.destroy();
}

void UAVServerApp::finish() {
    ApplicationBase::finish();

    EV_INFO << "UAV Server Application finished. Received: " << numReceived << " packets in total." << endl;
    EV_INFO << "Packets received from each UAV:" << endl;

    for (auto& pair : packetsPerUAV) {
        EV_INFO << "  UAV at " << pair.first.str() << ": " << pair.second << " packets" << endl;
    }

    EV_INFO << "Total stored data packets: " << storedData.size() << endl;
}
