import inet.applications.contract.IApp;

simple UAVSensorApp like IApp
{
    parameters:
        string interfaceTableModule;
        double startTime @unit(s) = default(1s);
        double sendInterval @unit(s) = default(1s);
        double serverCheckInterval @unit(s) = default(2s);  // How often to check for a server
        int localPort = default(-1);
        int destPort;
        int serverStatusPort = default(5001);  // Port to listen for server status
        int messageLength @unit(B) = default(100B);
        double stopOperationExtraTime @unit(s) = default(2s);
        double stopOperationTimeout @unit(s) = default(2s);
        
        @display("i=block/app");
        @signal[sentPk](type=inet::Packet);
        @signal[rcvdPk](type=inet::Packet);
        @statistic[sentPk](title="packets sent"; source=sentPk; record=count,"sum(packetBytes)","vector(packetBytes)"; interpolationmode=none);
        @statistic[rcvdPk](title="packets received"; source=rcvdPk; record=count,"sum(packetBytes)","vector(packetBytes)"; interpolationmode=none);
        
    gates:
        input socketIn;
        output socketOut;
}