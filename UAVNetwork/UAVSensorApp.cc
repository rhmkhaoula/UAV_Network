#include "UAVSensorApp.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/TimeTag_m.h"
#include "inet/common/packet/chunk/ByteCountChunk.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/transportlayer/contract/udp/UdpControlInfo_m.h"
#include "inet/networklayer/common/L3AddressTag_m.h"

Define_Module(UAVSensorApp);

simsignal_t UAVSensorApp::sentPkSignal = registerSignal("sentPk");
simsignal_t UAVSensorApp::rcvdPkSignal = registerSignal("rcvdPk");

UAVSensorApp::UAVSensorApp() {
}

UAVSensorApp::~UAVSensorApp() {
    cancelAndDelete(sendTimer);
    cancelAndDelete(serverCheckTimer);
}

void UAVSensorApp::initialize(int stage) {
    ApplicationBase::initialize(stage);

    if (stage == INITSTAGE_LOCAL) {
        sendInterval = par("sendInterval");
        serverCheckInterval = par("serverCheckInterval");
        localPort = par("localPort");
        destPort = par("destPort");
        serverStatusPort = par("serverStatusPort");
        serverFound = false;

        numSent = 0;
        numReceived = 0;
        WATCH(numSent);
        WATCH(numReceived);
        WATCH(serverFound);
        WATCH(serverAddress);
    }
    else if (stage == INITSTAGE_APPLICATION_LAYER) {
        sendTimer = new cMessage("sendTimer");
        serverCheckTimer = new cMessage("serverCheckTimer");

        socket.setOutputGate(gate("socketOut"));
        socket.bind(localPort);
        // Bind to server status port too for receiving server broadcasts
        socket.joinLocalMulticastGroups(); // Join multicast groups for broadcast reception
        socket.setCallback(this);

        // Start looking for a server
        if (operationalState == State::OPERATING) {
            scheduleAt(simTime() + par("startTime"), serverCheckTimer);
        }
    }
}

void UAVSensorApp::handleMessageWhenUp(cMessage *msg) {
    if (msg->isSelfMessage()) {
        if (msg == sendTimer) {
            if (serverFound) {
                sendSensorData();
                scheduleAt(simTime() + sendInterval, sendTimer);
            } else {
                EV_WARN << "No server found yet, can't send data" << endl;
                // Don't reschedule, we'll start sending when server is found
            }
        }
        else if (msg == serverCheckTimer) {
            checkForServer();
            scheduleAt(simTime() + serverCheckInterval, serverCheckTimer);
        }
    }
    else
        socket.processMessage(msg);
}

void UAVSensorApp::checkForServer() {
    if (!serverFound) {
        EV_INFO << "Looking for UAV server..." << endl;
        // Listen for server broadcasts - this is handled in socketDataArrived
    }
}

void UAVSensorApp::collectSensorData() {
    // Simulation de la collecte de données d'un capteur
    EV_INFO << "Collecting sensor data..." << endl;
    // Pour cet exemple, nous générons simplement des données aléatoires
}

void UAVSensorApp::sendSensorData() {
    if (!serverFound) {
        EV_WARN << "Cannot send sensor data, no server found" << endl;
        return;
    }

    collectSensorData();

    char msgName[32];
    sprintf(msgName, "UAVSensorData-%d", numSent);

    // Créer un paquet avec les données du capteur
    Packet *packet = new Packet(msgName);

    // Ajouter un tag de temps pour mesurer la latence
    auto creationTimeTag = packet->addTag<CreationTimeTag>();
    creationTimeTag->setCreationTime(simTime());

    // Simuler les données du capteur avec un payload de taille fixe
    const auto& payload = makeShared<ByteCountChunk>(B(par("messageLength")));
    packet->insertAtBack(payload);

    // Envoyer au serveur UAV
    socket.sendTo(packet, serverAddress, destPort);

    EV_INFO << "Sent sensor data to server at " << serverAddress.str() << endl;

    numSent++;
    emit(sentPkSignal, packet);
}

void UAVSensorApp::socketDataArrived(UdpSocket *socket, Packet *packet) {
    auto addressInd = packet->getTag<L3AddressInd>();
    L3Address srcAddr = addressInd->getSrcAddress();

    // Vérifier si c'est un message du serveur (broadcast d'état)
    if (packet->getKind() == 0 && !serverFound) { // Regular UDP packet
        const auto& chunk = packet->peekDataAt(B(0), B(1));
        auto byteChunk = dynamicPtrCast<const ByteCountChunk>(chunk);

        if (byteChunk && byteChunk->getData() == 'S') {  // 'S' for Server
            EV_INFO << "Server found at " << srcAddr.str() << endl;
            serverAddress = srcAddr;
            serverFound = true;

            // Start sending data now that we have a server
            if (!sendTimer->isScheduled()) {
                scheduleAt(simTime() + uniform(0, 1), sendTimer);
            }
        }
    }

    // Notez que nous pourrions aussi recevoir d'autres types de messages du serveur
    EV_INFO << "Received packet: " << packet->getName() << " from " << srcAddr.str() << endl;

    numReceived++;
    emit(rcvdPkSignal, packet);
    delete packet;
}

void UAVSensorApp::socketErrorArrived(UdpSocket *socket, Indication *indication) {
    EV_WARN << "Socket error: " << indication->getName() << endl;
    delete indication;
}

void UAVSensorApp::socketClosed(UdpSocket *socket) {
    if (operationalState == State::STOPPING_OPERATION) {
        startActiveOperationExtraTimeOrFinish(par("stopOperationExtraTime"));
    }
}

void UAVSensorApp::handleStartOperation(LifecycleOperation *operation) {
    socket.setOutputGate(gate("socketOut"));
    socket.bind(localPort);
    socket.joinLocalMulticastGroups(); // Join multicast groups for broadcast reception
    socket.setCallback(this);

    serverCheckTimer = new cMessage("serverCheckTimer");
    scheduleAt(simTime() + par("startTime"), serverCheckTimer);
}

void UAVSensorApp::handleStopOperation(LifecycleOperation *operation) {
    cancelEvent(sendTimer);
    cancelEvent(serverCheckTimer);
    socket.close();
    delayActiveOperationFinish(par("stopOperationTimeout"));
}

void UAVSensorApp::handleCrashOperation(LifecycleOperation *operation) {
    cancelEvent(sendTimer);
    cancelEvent(serverCheckTimer);
    socket.destroy();
}

void UAVSensorApp::finish() {
    ApplicationBase::finish();
    EV_INFO << "UAV Sensor Application finished. Sent: " << numSent << " packets, Received: " << numReceived << " packets." << endl;
}
