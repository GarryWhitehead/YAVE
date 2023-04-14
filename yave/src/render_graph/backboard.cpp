/* Copyright (c) 2022 Garry Whitehead
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "backboard.h"

#include "render_graph.h"
#include "utility/assertion.h"

namespace yave
{
namespace rg
{

BlackBoard::BlackBoard() {}
BlackBoard::~BlackBoard() {}

void BlackBoard::add(const std::string& name, const RenderGraphHandle& handle)
{
    blackboard_.emplace(name, handle);
}

const RenderGraphHandle& BlackBoard::get(const std::string& name) const
{
    auto iter = blackboard_.find(name);
    ASSERT_FATAL(
        iter != blackboard_.end(),
        "Cannot retrieve from blackboard: resource name %s not found.",
        name.c_str());
    return iter->second;
}

void BlackBoard::remove(const std::string& name)
{
    auto iter = blackboard_.find(name);
    ASSERT_FATAL(
        iter != blackboard_.end(),
        "Cannot remove from blackboard: resource name %s not found.",
        name.c_str());
    blackboard_.erase(iter);
}

void BlackBoard::reset() noexcept
{
    blackboard_.clear();
}

} // namespace rg
} // namespace yave
