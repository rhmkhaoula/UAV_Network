// Fichier de configuration du réseau décentralisé
import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import inet.physicallayer.wireless.common.contract.packetlevel.IRadioMedium;
import inet.physicallayer.wireless.ieee80211.packetlevel.Ieee80211ScalarRadioMedium;
import inet.visualizer.contract.IIntegratedVisualizer;
import inet.node.inet.AdhocHost;
import inet.mobility.contract.IMobility;
import inet.mobility.single.CircleMobility;
import inet.mobility.single.MassMobility;

network DecentralizedUAVNetwork
{
    parameters:
        @display("bgb=800,600;bgi=background/terrain,s");
        int numUAVs = default(5); // Un drone serveur + n drones capteurs
        
    submodules:
        visualizer: <default("IntegratedCanvasVisualizer")> like IIntegratedVisualizer {
            parameters:
                @display("p=100,50");
        }
        
        radioMedium: Ieee80211ScalarRadioMedium {
            parameters:
                @display("p=100,150");
        }
        
        configurator: Ipv4NetworkConfigurator {
            parameters:
                @display("p=100,250");
        }
        
        serverDrone: AdhocHost {
            parameters:
                @display("p=400,300;i=misc/drone,,#FF0000");
        }
        
        sensorDrone[numUAVs-1]: AdhocHost {
            parameters:
                @display("i=misc/drone");
        }
    
    connections allowunconnected:
}