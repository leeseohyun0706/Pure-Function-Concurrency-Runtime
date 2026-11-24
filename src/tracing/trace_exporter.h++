
#ifndef CONCAL_TRACE_EXPORTER_H
#define CONCAL_TRACE_EXPORTER_H
#include "tracing/trace_event.h++"
#include <span>

class TraceExporter {
public:
    // Only for perfetto web UI
    // TODO : Separate perfetto export and common json export.
    void exportTrace(
        const std::span<const TraceEvent>& events,
        std::ostream& output);

};


#endif //CONCAL_TRACE_EXPORTER_H
