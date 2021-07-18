//Include different network libs for different OSes
#include <imgui.h>
#include <module.h>
#include <gui/gui.h>
#include <signal_path/signal_path.h>
#include <signal_path/sink.h>
#include <dsp/audio.h>
#include <dsp/processing.h>
#include <dsp/convertion.h>
#include <spdlog/spdlog.h>
#include <config.h>
#include <options.h>
#include <utils/networking.h>



#define CONCAT(a, b) ((std::string(a) + b).c_str())

/*Format:
 * 48 kHz samp rate
 * S16LE
 * 1 channel(mono)
 */
#define SAMPLE_RATE 48000

SDRPP_MOD_INFO {
    /* Name:            */ "network_sink",
    /* Description:     */ "GQRX-like UDP and other network sink module for SDR++",
    /* Author:          */  "arf20 & cropinghigh",
    /* Version:         */ 0, 1, 0,
    /* Max instances    */ 1
};

const std::string protocols[] = {
"UDP CLIENT",
"UDP SERVER",
"TCP CLIENT",
"TCP SERVER"
};

const char* protocolsTxt = "UDP CLIENT\0"
                           "UDP SERVER\0"
                           "TCP CLIENT\0"
                           "TCP SERVER\0";

ConfigManager config;

class NetSink : SinkManager::Sink {
public:
    NetSink(SinkManager::Stream* stream, std::string streamName) {
        _stream = stream;
        _streamName = streamName;
        s2m.init(_stream->sinkOut);
        monoPacker.init(&s2m.out, 512);
        typeConv.init(&monoPacker.out);
        netHandler.init(&typeConv.out, _handler, this);
        _stream->setSampleRate(SAMPLE_RATE);

        bool created = false;
        config.acquire();
        if (!config.conf.contains(_streamName)) {
            created = true;
            config.conf[_streamName]["port"] = 7355;
            config.conf[_streamName]["host"] = "127.0.0.1";
            config.conf[_streamName]["proto"] = "UDP CLIENT";
            config.conf[_streamName]["spp"] = spp;
        }
        port = config.conf[_streamName]["port"];
        strcpy(host, ((std::string)config.conf[_streamName]["host"]).c_str());
        proto = config.conf[_streamName]["proto"];
        oldproto = "";
        spp = config.conf[_streamName]["spp"];
        config.release(created);

        protoId = std::distance(protocols, std::find(protocols, protocols + sizeof(protocols), proto)); //Find index in array by value
    }

    ~NetSink() {
        doStop();
    }

    void start() {
        if (running) {
            return;
        }
        doStart();
        running = true;
    }

    void stop() {
        if (!running) {
            return;
        }
        doStop();
        running = false;
    }

    void menuHandler() {
        float menuWidth = ImGui::GetContentRegionAvailWidth();

        ImGui::Text("Protocol: ");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(menuWidth - ImGui::GetCursorPosX());
        if (ImGui::Combo("##_network_sink_proto_sel", &protoId, protocolsTxt)) {
            proto = protocols[protoId];
            config.acquire();
            config.conf[_streamName]["proto"] = proto;
            config.release(true);
        }
        ImGui::Text("Host: ");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(menuWidth - ImGui::GetCursorPosX());
        if(ImGui::InputText("##_network_sink_addr", host, INET_ADDRSTRLEN)) {
            config.acquire();
            config.conf[_streamName]["host"] = host;
            config.release(true);
        }
        ImGui::Text("Port: ");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(menuWidth - ImGui::GetCursorPosX());
        if(ImGui::InputInt("##_network_sink_port",(int*) &port, 1, 100)) {
            if(port < 0) {
                port = 0;
            } else if(port > 65535) {
                port = 65535;
            }
            config.acquire();
            config.conf[_streamName]["port"] = port;
            config.release(true);
        }
        ImGui::Text("Samples per packet: ");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(menuWidth - ImGui::GetCursorPosX());
        if(ImGui::InputInt("Samples per packet##_network_sink_spp",&spp, 1, 100)) {
            if(spp < 0) {
                spp = 0;
            }
            config.acquire();
            config.conf[_streamName]["spp"] = spp;
            config.release(true);
        }

        if(ImGui::Button("Apply settings/Reconnect##_network_sink_rec", ImVec2(menuWidth, 0))) {
            if(running) {
                doStop();
                doStart();
            }
        }
        ImGui::Text("Status: ");
        ImGui::SameLine();
        if(proto == "UDP CLIENT" || proto == "TCP CLIENT") {
            if(connection == nullptr) {
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Not connected/connection failed");
            } else {
                if(connection->isOpen()) {
                    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Connected");
                } else {
                    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Connection broken");
                }
            }
        } else {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Not implemented");
        }
    }

private:
    void doStart() {
        monoPacker.setSampleCount(spp);
        if(proto != oldproto || ((proto == "UDP CLIENT" || proto == "TCP CLIENT") && (connection == nullptr || !connection->isOpen()))) {
            //protocol changed or connection failed
            if(proto == "UDP CLIENT") {
                if(connection != nullptr && connection->isOpen()) {
                    connection->close();
                }
                try {
                    connection = net::connect(net::Protocol::PROTO_UDP, std::string(host), port);
                } catch(std::runtime_error& e) {
                    spdlog::error("Network Sink: Connection failed({0})!", e.what());
                }
                if(connection == NULL) {
                    connection = nullptr;
                }
            } else if(proto == "TCP CLIENT") {
                if(connection != nullptr && connection->isOpen()) {
                    connection->close();
                }
                try {
                    connection = net::connect(net::Protocol::PROTO_TCP, std::string(host), port);
                } catch(std::runtime_error& e) {
                    spdlog::error("Network Sink: Connection failed({0})!", e.what());
                }
            } else {
                spdlog::error("Network Sink: {0} not implemented!", proto);
                return;
            }
            oldproto = proto;
        }
        
        netHandler.start();
        typeConv.start();
        monoPacker.start();
        s2m.start();
    }

    void doStop() {
        s2m.stop();
        monoPacker.stop();
        monoPacker.out.stopReader();
        typeConv.stop();
        if(proto == "UDP CLIENT" || proto == "TCP CLIENT") {
            if(connection != nullptr) {
                if(connection->isOpen()) {
                    connection->close();
                }
                connection = nullptr;
            }
        }
        monoPacker.out.clearReadStop();
    }

    static void _handler(int16_t* data, int count, void* ctx) {
        NetSink* _this = (NetSink*) ctx;
        
        if(_this->proto == "UDP CLIENT" || _this->proto == "TCP CLIENT") {
            if(_this->connection != nullptr && _this->connection->isOpen()) {
                _this->connection->write(count*(sizeof(int16_t)/sizeof(uint8_t)), (uint8_t*)data);
            }
        }
    }

    SinkManager::Stream* _stream;
    dsp::StereoToMono s2m;
    dsp::Packer<float> monoPacker;
    dsp::FloatToInt16 typeConv;
    dsp::HandlerSink<int16_t> netHandler;

    std::string _streamName;

    bool running = false;

    int sockfd = -1;

    int port = 7355;
    char host[INET_ADDRSTRLEN];
    std::string proto;
    std::string oldproto;
    int protoId;
    int spp = SAMPLE_RATE/60;
    net::Conn connection = nullptr;
    net::Listener listener = nullptr;


};

class NetworkSinkModule : public ModuleManager::Instance {
public:
    NetworkSinkModule(std::string name) {
        this->name = name;
        provider.create = create_sink;
        provider.ctx = this;

        sigpath::sinkManager.registerSinkProvider("Network", provider);
    }

    ~NetworkSinkModule() {
        if(netSink != nullptr) {
            delete netSink;
        }
    }

    void enable() {
        enabled = true;
    }

    void disable() {
        enabled = false;
    }

    bool isEnabled() {
        return enabled;
    }

private:

    static SinkManager::Sink* create_sink(SinkManager::Stream* stream, std::string streamName, void* ctx) {
        NetworkSinkModule* _this = (NetworkSinkModule*) ctx;
        _this->netSink = new NetSink(stream, streamName);
        return (SinkManager::Sink*)(_this->netSink);
    }

    std::string name;
    bool enabled = true;
    SinkManager::SinkProvider provider;
    NetSink* netSink = nullptr;

};

MOD_EXPORT void _INIT_() {
    json def = json({});
    config.setPath(options::opts.root + "/network_sink_config.json");
    config.load(def);
    config.enableAutoSave();
}

MOD_EXPORT void* _CREATE_INSTANCE_(std::string name) {
    NetworkSinkModule* instance = new NetworkSinkModule(name);
    return instance;
}

MOD_EXPORT void _DELETE_INSTANCE_() {
    config.disableAutoSave();
    config.save();
}

MOD_EXPORT void _END_() {

}

