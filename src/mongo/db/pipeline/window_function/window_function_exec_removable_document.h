/**
 *    Copyright (C) 2021-present MongoDB, Inc.
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the Server Side Public License, version 1,
 *    as published by MongoDB, Inc.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    Server Side Public License for more details.
 *
 *    You should have received a copy of the Server Side Public License
 *    along with this program. If not, see
 *    <http://www.mongodb.com/licensing/server-side-public-license>.
 *
 *    As a special exception, the copyright holders give permission to link the
 *    code of portions of this program with the OpenSSL library under certain
 *    conditions as described in each individual source file and distribute
 *    linked combinations including the program with the OpenSSL library. You
 *    must comply with the Server Side Public License in all respects for
 *    all of the code used other than as permitted herein. If you modify file(s)
 *    with this exception, you may extend this exception to your version of the
 *    file(s), but you are not obligated to do so. If you do not wish to do so,
 *    delete this exception statement from your version. If you delete this
 *    exception statement from all source files in the program, then also delete
 *    it in the license file.
 */

#pragma once

#include "mongo/db/pipeline/document_source.h"
#include "mongo/db/pipeline/expression.h"
#include "mongo/db/pipeline/window_function/partition_iterator.h"
#include "mongo/db/pipeline/window_function/window_bounds.h"
#include "mongo/db/pipeline/window_function/window_function_exec.h"

namespace mongo {

/**
 * An executor that specifically handles document-based window types which accumulate values while
 * removing old ones.
 */
class WindowFunctionExecRemovableDocument final : public WindowFunctionExecRemovable {
public:
    /**
     * Constructs a removable window function executor with the given input expression to be
     * evaluated and passed to the corresponding WindowFunc for each document in the window.
     *
     * The 'bounds' parameter is the user supplied bounds for the window.
     */
    WindowFunctionExecRemovableDocument(PartitionIterator* iter,
                                        boost::intrusive_ptr<Expression> input,
                                        std::unique_ptr<WindowFunctionState> function,
                                        WindowBounds::DocumentBased bounds);

    /**
     * Constructs a removable window function executor with the given input expression and sortBy
     * expression to be evaluated and passed the evaluation of both "input" and "sortBy" as a single
     * input Value of a 2-sized vector (Value{sortByValue, inputValue}) to the corresponding
     * WindowFunc for each document in the window.
     *
     * The "bounds" parameter is the user supplied bounds for the window.
     */
    WindowFunctionExecRemovableDocument(PartitionIterator* iter,
                                        boost::intrusive_ptr<Expression> input,
                                        boost::intrusive_ptr<Expression> sortBy,
                                        std::unique_ptr<WindowFunctionState> function,
                                        WindowBounds::DocumentBased bounds)
        : WindowFunctionExecRemovableDocument(iter, std::move(input), std::move(function), bounds) {
        _sortBy = std::move(sortBy);
    }

    void reset() final {
        _function->reset();
        _values = std::queue<Value>();
        _initialized = false;
    }

private:
    void update() final;
    void initialize();

    void removeFirstValueIfExists() {
        if (_values.size() == 0) {
            return;
        }
        _memUsageBytes -= _values.front().getApproximateSize();
        _function->remove(_values.front());
        _values.pop();
    }

    // In one of two states: either the initial window has not been populated or we are sliding and
    // accumulating/removing values.
    bool _initialized = false;

    boost::intrusive_ptr<Expression> _sortBy = nullptr;

    int _lowerBound;
    // Will stay boost::none if right unbounded.
    boost::optional<int> _upperBound = boost::none;
};
}  // namespace mongo
