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

#include "dependency_graph.h"

#include "render_graph/render_pass_node.h"

#include <deque>

namespace yave::rg
{

DependencyGraph::DependencyGraph() = default;

void DependencyGraph::addNode(Node* node) { nodes_.emplace_back(node); }

uint64_t DependencyGraph::createId() const { return static_cast<uint64_t>(nodes_.size()); }

Node* DependencyGraph::getNode(const uint64_t id)
{
    ASSERT_LOG(id < nodes_.size());
    return nodes_[id];
}

void DependencyGraph::addEdge(Edge* edge) { edges_.emplace_back(edge); }

bool DependencyGraph::isValidEdge(const Edge* edge) const
{
    Node* fromNode = nodes_[edge->fromId];
    Node* toNode = nodes_[edge->toId];
    return !fromNode->isCulled() && !toNode->isCulled();
}

std::vector<Edge*> DependencyGraph::getReaderEdges(Node* node) const
{
    std::vector<Edge*> readersForPass;
    for (auto* edge : edges_)
    {
        if (edge->toId == node->getId())
        {
            readersForPass.emplace_back(edge);
        }
    }
    return readersForPass;
}

std::vector<Edge*> DependencyGraph::getWriterEdges(Node* node) const
{
    std::vector<Edge*> readersForPass;
    for (auto* edge : edges_)
    {
        if (edge->fromId == node->getId())
        {
            readersForPass.emplace_back(edge);
        }
    }
    return readersForPass;
}

void DependencyGraph::cull()
{
    // increase the reference count for all nodes that have a writer
    for (const auto* edge : edges_)
    {
        ASSERT_LOG(edge->fromId < nodes_.size());
        Node* node = nodes_[edge->fromId];
        node->refCount++;
    }

    std::deque<Node*> nodesToCull;
    for (Node* node : nodes_)
    {
        if (!node->refCount)
        {
            nodesToCull.push_back(node);
        }
    }
    while (!nodesToCull.empty())
    {
        Node* node = nodesToCull.back();
        nodesToCull.pop_back();
        const auto readerEdges = getReaderEdges(static_cast<RenderPassNode*>(node));
        for (const auto& edge : readerEdges)
        {
            // remove any linked nodes that have no reference after the
            // culling of the parent node.
            Node* childNode = getNode(edge->fromId);
            childNode->refCount--;
            if (!childNode->refCount)
            {
                nodesToCull.push_back(childNode);
            }
        }
    }
}

void DependencyGraph::exportGraphViz(std::string& output)
{
    output += "digraph \"rendergraph\" { \n";
    output += "bgcolor = white\n";
    output += "node [shape=rectangle, fontname=\"arial\", fontsize=12]\n";

    // add each node
    for (auto* node : nodes_)
    {
        std::string nodeStr = node->getGraphViz();
        output += "\"N" + std::to_string(node->getId()) + "\" " + nodeStr + "\n";
    }

    output += "\n";

    for (auto* node : nodes_)
    {
        auto writerEdges = getWriterEdges(node);
        std::string validStr;
        std::string invalidStr;

        for (const auto edge : writerEdges)
        {
            const Node* link = getNode(edge->toId);
            if (isValidEdge(edge))
            {
                validStr =
                    validStr.empty() ? "N" + std::to_string(node->getId()) + " -> { " : validStr;
                validStr += "N" + std::to_string(link->getId()) + " ";
            }
            else
            {
                invalidStr = invalidStr.empty() ? "N" + std::to_string(node->getId()) + " -> { "
                                                : invalidStr;
                invalidStr += "N" + std::to_string(link->getId()) + " ";
            }
        }
        if (!validStr.empty())
        {
            validStr += "} [color=red4]\n";
            output += validStr;
        }

        if (!invalidStr.empty())
        {
            invalidStr += "} [color=red4 style=dashed]\n";
            output += invalidStr;
        }
    }

    output += "}\n";
}

void DependencyGraph::clear() noexcept
{
    nodes_.clear();
    edges_.clear();
}

// ================= node functions ===================

Node::Node(util::CString name, DependencyGraph& graph)
    : refCount(0), name_(std::move(name)), id_(graph.createId())
{
    graph.addNode(this);
}

std::string Node::getGraphViz()
{
    std::string output;
    output += "[label=\"node\\n name: " + std::string(name_.c_str()) +
        " id: " + std::to_string(id_) + ", refCount: " + std::to_string(refCount) + "\",";
    output += " style=filled, fillcolor=green]";
    return output;
}

// ================ edge functions ====================

Edge::Edge(DependencyGraph& dGraph, Node* from, Node* to) : fromId(from->getId()), toId(to->getId())
{
    dGraph.addEdge(this);
}

} // namespace yave::rg
