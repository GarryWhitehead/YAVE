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

#pragma once

#include "utility/cstring.h"

#include <string>
#include <vector>

namespace yave
{
namespace rg
{
// forward declerations
class DependencyGraph;
class Node;

struct Edge
{
    Edge(DependencyGraph& graph, Node* from, Node* to);

    // the node Id that this edge projects from
    size_t fromId;
    // the node Id that this edge projects to
    size_t toId;
};

class Node
{
public:
    Node(const util::CString& name, DependencyGraph& graph);

    void declareSideEffect() { refCount = 0xFFFF; }

    bool isCulled() const { return refCount == 0; }

    uint64_t getId() const noexcept { return id_; }

    util::CString& getName() noexcept { return name_; }

    std::string getGraphViz();

    // this is public for convienence reasons, myabe shouldn't be.
    size_t refCount;

protected:
    util::CString name_;

    uint64_t id_;
};

class DependencyGraph
{
public:
    DependencyGraph();

    void addNode(Node* node);

    uint64_t createId() const;

    Node* getNode(const uint64_t id);

    void addEdge(Edge* edge);

    bool isValidEdge(const Edge* edge) const;

    void exportGraphViz(std::string& output);

    std::vector<Edge*> getReaderEdges(Node* node) const;
    std::vector<Edge*> getWriterEdges(Node* node) const;

    void cull();

    void clear() noexcept;

private:
    // The nodes are not owned by the dependency graph so we only
    // keep reference with a raw pointer. Thus, its imperative that
    // the owner only destroys the node after finishing with the
    // dependency graph.
    std::vector<Node*> nodes_;

    // As nodes, edges are not ownded by the dependency graph but the
    // render graph, so be careful with the lifetime of the edge.
    std::vector<Edge*> edges_;
};

} // namespace rg
} // namespace yave
