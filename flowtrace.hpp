// TODO: put into dedicated git repo. There is a duplicate also in jonnor/rebirth

#ifdef HAVE_JSON11
#include <json11.hpp>
#endif

namespace flowtrace {


struct NodePort {
    std::string node;
    std::string port;

    NodePort(std::string n, std::string p)
        : node(n)
        , port(p)
    {
    }

    json11::Json to_json() const {
        return json11::Json::object {
            {"port", port },
            {"node", node },
        };
    }
};

/*
struct Connection {
    NodePort src;
    NodePort tgt;    

    Connection(NodePort s, NodePort t)
        : src(s)
        , tgt(t)
    {
    }

    json11::Json to_json() const {
        return json11::Json::object {
            {"src", src.to_json() },
            {"tgt", tgt.to_json() },
    };
}
        "src": {
            "process": "load",
            "port": "canvas"
          },
          "tgt": {
            "process": "colors",
            "port": "canvas"
          }

json11::Json dummyGraph() {
    using namespace json11;
    return Json::object {
        {"properties", Json::object {} },
        {"inports", Json::object {} },
        {"outports", Json::object {} },
        {"processes", Json::object {} },
        {"groups", Json::array {} },
        {"connections", Json::array {} },
    };
}


json11::Json flowtrace() {
    using namespace json11;
    return Json::object {
        {"header", Json::object {} },
        {"events", Json::array {} },
    };
}
*/

struct Event {
    std::string command = "data";
    std::string graph = "default";
    std::string time = "2016-12-10T22:01:11.383Z";
    NodePort src = {"first", "out"};
    NodePort tgt = {"next", "in"};
    json11::Json data;

    Event(const json11::Json &d)
        : data(d)
    {
    }
    
    json11::Json to_json() const {
        using namespace json11;
        return Json::object {
            { "protocol", "network" },
            { "command", command },
            { "payload", Json::object {
                { "time", time },
                { "graph", graph },
                { "src", src.to_json() },
                { "tgt", tgt.to_json() },
                { "data", data },
            }}
        };
    }
};

} // end namespace flowtrace
