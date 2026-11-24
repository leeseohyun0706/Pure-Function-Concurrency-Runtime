

#include "trace_exporter.h++"
#include <nlohmann/json.hpp>

using nlohmann::json;

json encode_event(const TraceEvent& event) {
    json result = {
        {"ts", event._ts / 1000.0},
        {"name", to_string(event._kind)},
        {"pid", 1},
        {"tid", event._workerId._value + 1},
    };

    switch (event._shape) {
        case TraceEventShape::Span:
            result["ph"] = "X";
            result["dur"] = event._dur / 1000.0;
            break;
        case TraceEventShape::Counter:
            result["ph"] = "C";
            break;
        case TraceEventShape::Instant:
            result["ph"] = "I";
            result["s"] = "t";
            break;
    }

    result["args"] = {
        {"taskId", event._taskId._value},
        {"value", event._value}
    };
    return result;
}

void TraceExporter::exportTrace(const std::span<const TraceEvent> &events, std::ostream &output) {
    json js = json::array();

    for (auto& event : events) {
        js.push_back(encode_event(event));
    }

    output << json{{"traceEvents", std::move(js)}};

}
