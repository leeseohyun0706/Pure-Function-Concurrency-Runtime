
#ifndef CONCAL_DAG_BUILDER_H
#define CONCAL_DAG_BUILDER_H
#include "pure/output_ref.h++"
#include <pure/runtime.h++>

namespace test::dag {

    // Get reduced one output ref
    template<typename T, typename F>
    OutputRef<T> reduce(
        Runtime& runtime,
        const F& operation,
        std::vector<OutputRef<T>> inputs
    ) {
        if (inputs.empty())
            throw std::invalid_argument("Empty reduction");


        while (inputs.size() > 1) {

            std::vector<OutputRef<T>>  nexts;

            nexts.reserve((inputs.size() + 1) / 2);
            for (size_t i = 0; i < inputs.size(); i += 2) {
                if (i + 1 < inputs.size()) {
                    nexts.push_back(
                        runtime.invoke(operation, inputs[i], inputs[i + 1])
                    );
                }else {
                    nexts.push_back(inputs[i]);
                }
            }

            inputs = std::move(nexts);
        }

        return std::move(inputs.front());
    }
}

#endif //CONCAL_DAG_BUILDER_H
