/* <editor-fold desc="MIT License">

Copyright(c) 2025-2026 Chris Djali

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

</editor-fold> */

#include <vsg/commands/BindIndexBuffer.h>
#include <vsg/io/Logger.h>
#include <vsg/io/ReaderWriter.h>
#include <vsg/nodes/IntersectionProxy.h>
#include <vsg/nodes/StateGroup.h>
#include <vsg/nodes/VertexDraw.h>
#include <vsg/nodes/VertexIndexDraw.h>
#include <vsg/utils/Intersector.h>
#include <vsg/utils/LineSegmentIntersector.h>

using namespace vsg;

namespace
{
    class GetArrays : public ConstVisitor
    {
    public:
        DataList arrays;

        void apply(const VertexDraw& draw) override
        {
            arrays.reserve(draw.arrays.size());
            for (const auto& array : draw.arrays) arrays.push_back(array->data);
        }

        void apply(const VertexIndexDraw& draw) override
        {
            arrays.reserve(draw.arrays.size());
            for (const auto& array : draw.arrays) arrays.push_back(array->data);
        }
    };
}

IntersectionProxy::IntersectionProxy(Node* in_original) :
    Inherit(),
    original(in_original),
    proxiedNodePath({original})
{
}

IntersectionProxy::IntersectionProxy(const IntersectionProxy& rhs, const CopyOp& copyop) :
    Inherit(rhs, copyop),
    original(rhs.original),
    proxiedNodePath(rhs.proxiedNodePath)
{
}

IntersectionProxy::~IntersectionProxy() = default;

int IntersectionProxy::compare(const Object& rhs_object) const
{
    int result = Node::compare(rhs_object);
    if (result != 0) return result;

    const auto& rhs = static_cast<decltype(*this)>(rhs_object);
    return compare_pointer(original, rhs.original);
}

void IntersectionProxy::read(Input& input)
{
    Node::read(input);

    input.read("original", original);
}

void IntersectionProxy::write(Output& output) const
{
    Node::write(output);

    output.write("original", original);
}

namespace
{
    struct GetIndicesVisitor : public ConstVisitor
    {
        ref_ptr<const ubyteArray> ubyte_indices;
        ref_ptr<const ushortArray> ushort_indices;
        ref_ptr<const uintArray> uint_indices;

        void apply(const BufferInfo& bufferInfo) override
        {
            bufferInfo.data->accept(*this);
        }

        void apply(const ubyteArray& array) override
        {
            ubyte_indices = &array;
            ushort_indices = nullptr;
            uint_indices = nullptr;
        }
        void apply(const ushortArray& array) override
        {
            ubyte_indices = nullptr;
            ushort_indices = &array;
            uint_indices = nullptr;
        }
        void apply(const uintArray& array) override
        {
            ubyte_indices = nullptr;
            ushort_indices = nullptr;
            uint_indices = &array;
        }

        uint32_t operator[](size_t index)
        {
            if (ubyte_indices) return ubyte_indices->at(index);
            if (ushort_indices) return ushort_indices->at(index);
            return uint_indices->at(index);
        }
    };
}

BVHIntersectionProxy::BVHIntersectionProxy(Node* in_original) :
    Inherit(in_original),
    internalNodes(),
    leaves(),
    bounds(),
    boundingVolumeHeirarchy({NodeRef::INVALID, 0u})
{
}

BVHIntersectionProxy::BVHIntersectionProxy(const BVHIntersectionProxy& rhs, const CopyOp& copyop) :
    Inherit(rhs, copyop),
    internalNodes(rhs.internalNodes),
    leaves(rhs.leaves),
    bounds(rhs.bounds),
    boundingVolumeHeirarchy(rhs.boundingVolumeHeirarchy)
{
}

void vsg::BVHIntersectionProxy::rebuild(vsg::ArrayState& arrayState)
{
    leaves.clear();
    internalNodes.clear();

    if (!original)
    {
        warn("Attempting to build BVHIntersectionProxy for null node.");
        return;
    }

    // if instancing is used, accessing the nth triangle is a hassle, so grab them upfront
    std::vector<Triangle> triangles;
    std::vector<TriangleMetadata> metadata;

    if (auto* vertexDraw = ::cast<VertexDraw>(original))
    {
        arrayState.apply(*vertexDraw);

        uint32_t lastIndex = vertexDraw->instanceCount > 1 ? (vertexDraw->firstInstance + vertexDraw->instanceCount) : vertexDraw->firstInstance + 1;
        uint32_t endVertex = vertexDraw->firstVertex + vertexDraw->vertexCount;

        triangles.reserve(vertexDraw->instanceCount * vertexDraw->vertexCount / 3);
        metadata.reserve(triangles.size());

        for (uint32_t instanceIndex = vertexDraw->firstInstance; instanceIndex < lastIndex; ++instanceIndex)
        {
            if (auto vertices = arrayState.vertexArray(instanceIndex, {}, {}))
            {
                for (uint32_t i = vertexDraw->firstVertex; (i + 2) < endVertex; i += 3)
                {
                    triangles.emplace_back(Triangle{
                        vertices->at(i),
                        vertices->at(i + 1) - vertices->at(i),
                        vertices->at(i + 2) - vertices->at(i)
                    });
                    metadata.emplace_back(TriangleMetadata{i, i + 1, i + 2, instanceIndex});
                }
            }
        }
    }
    else if (auto* vertexIndexDraw = ::cast<VertexIndexDraw>(original))
    {
        arrayState.apply(*vertexIndexDraw);

        uint32_t lastIndex = vertexIndexDraw->instanceCount > 1 ? (vertexIndexDraw->firstInstance + vertexIndexDraw->instanceCount) : vertexIndexDraw->firstInstance + 1;
        uint32_t endIndex = vertexIndexDraw->firstIndex + ((vertexIndexDraw->indexCount + 2) / 3) * 3;

        triangles.reserve(vertexIndexDraw->instanceCount * vertexIndexDraw->indexCount / 3);
        metadata.reserve(triangles.size());

        if (!vertexIndexDraw->indices || !vertexIndexDraw->indices->data)
        {
            warn("Attempting to build BVHIntersectionProxy for VertexIndexDraw with no indices.");
            return;
        }

        GetIndicesVisitor indices;
        vertexIndexDraw->indices->accept(indices);

        for (uint32_t instanceIndex = vertexIndexDraw->firstInstance; instanceIndex < lastIndex; ++instanceIndex)
        {
            if (auto vertices = arrayState.vertexArray(instanceIndex, {}, {}))
            {
                for (uint32_t i = vertexIndexDraw->firstIndex; i < endIndex; i += 3)
                {
                    triangles.emplace_back(Triangle{
                        vertices->at(indices[i]),
                        vertices->at(indices[i + 1]) - vertices->at(indices[i]),
                        vertices->at(indices[i + 2]) - vertices->at(indices[i])
                    });
                    metadata.emplace_back(TriangleMetadata{indices[i], indices[i + 1], indices[i + 2], instanceIndex});
                }
            }
        }
    }
    else
    {
        warn("Unsupported node type when building BVHIntersectionProxy: ", original->className());
        return;
    }

    std::vector<vec3> barycenters;
    barycenters.reserve(triangles.size());
    for (const auto& triangle : triangles)
    {
        barycenters.emplace_back(triangle.vertex0 + (triangle.edge1 + triangle.edge2) / 3.f);
    }

    std::vector<size_t> indices;
    indices.reserve(triangles.size());
    for (size_t i = 0; i < triangles.size(); ++i)
    {
        indices.emplace_back(i);
    }

    leaves.reserve(triangles.size() / trisPerLeaf);
    internalNodes.reserve(triangles.size() / (trisPerLeaf * 2));

    using itr_t = decltype(indices)::iterator;

    auto computeKDTree = [&](itr_t first, itr_t last, auto&& computeKDTreeRecursive) -> std::pair<box, NodeRef> {
        if (static_cast<size_t>(std::distance(first, last)) <= trisPerLeaf)
        {
            leaves.emplace_back();
            leafMetadata.emplace_back();
            box bound;
            itr_t itr = first;
            for (size_t i = 0; i < trisPerLeaf; ++i)
            {
                if (itr != last)
                {
                    leaves.back().tris[i] = triangles[*itr];
                    bound.add(triangles[*itr].vertex0);
                    bound.add(triangles[*itr].vertex0 + triangles[*itr].edge1);
                    bound.add(triangles[*itr].vertex0 + triangles[*itr].edge2);
                    leafMetadata.back().tris[i] = metadata[*itr];
                    ++itr;
                }
                else
                {
                    // add a degenerate triangle as we have to have trisPerLeaf
                    leaves.back().tris[i] = {vec3(), vec3(), vec3()};
                }
            }
            return std::make_pair(bound, NodeRef{NodeRef::LEAF, static_cast<uint32_t>(leaves.size() - 1)});
        }
        else
        {
            box baryBound;
            for (itr_t itr = first; itr != last; ++itr)
            {
                baryBound.add(barycenters[*itr]);
            }
            vec3 range = baryBound.max - baryBound.min;
            size_t axisIndex = range.x > range.y ? (range.x > range.z ? 0 : 2) : (range.y > range.z ? 1 : 2);
            itr_t midpoint = first + (((std::distance(first, last) + trisPerLeaf - 1) / 2) / trisPerLeaf) * trisPerLeaf;
            std::nth_element(first, midpoint, last, [&, axisIndex](const size_t& lhs, const size_t& rhs) { return barycenters[lhs][axisIndex] < barycenters[rhs][axisIndex]; });
            internalNodes.emplace_back(InternalNode{{{computeKDTreeRecursive(first, midpoint, computeKDTreeRecursive), computeKDTreeRecursive(midpoint, last, computeKDTreeRecursive)}}});
            box overallBound;
            for (const auto& [bound, ref] : internalNodes.back().children)
            {
                overallBound.add(bound);
            }
            return std::make_pair(overallBound, NodeRef{NodeRef::INTERNAL, static_cast<uint32_t>(internalNodes.size() - 1)});
        }
    };

    std::tie(bounds, boundingVolumeHeirarchy) = computeKDTree(indices.begin(), indices.end(), computeKDTree);
}

bool BVHIntersectionProxy::valid() const
{
    return boundingVolumeHeirarchy.type != NodeRef::INVALID;
}

void vsg::BVHIntersectionProxy::intersect(LineSegmentIntersector& lineSegmentIntersector) const
{
    const auto& ls = lineSegmentIntersector.lineSegment();

    using value_type = double;
    using vec_type = t_vec3<value_type>;
    const value_type epsilon = 1e-10;

    vec_type start(ls.start);
    vec_type end(ls.end);

    vec_type d = end - start;
    vec_type inv_d(1.0 / d.x, 1.0 / d.y, 1.0 / d.z);

    auto intersectBox = [&](const box& bound) {
        value_type t1 = (bound.min.x - start.x) * inv_d.x;
        value_type t2 = (bound.max.x - start.x) * inv_d.x;
        value_type tmin = std::min(t1, t2);
        value_type tmax = std::max(t1, t2);
        t1 = (bound.min.y - start.y) * inv_d.y;
        t2 = (bound.max.y - start.y) * inv_d.y;
        tmin = std::max(tmin, std::min(t1, t2));
        tmax = std::min(tmax, std::max(t1, t2));
        t1 = (bound.min.z - start.z) * inv_d.z;
        t2 = (bound.max.z - start.z) * inv_d.z;
        tmin = std::max(tmin, std::min(t1, t2));
        tmax = std::min(tmax, std::max(t1, t2));
        return tmax >= tmin && tmin < 1.0 && tmax > 0.0;
    };

    if (!bounds.valid() || !intersectBox(bounds))
        return;

    value_type length = ::length(d);
    value_type inverseLength = length != 0.0 ? 1.0 / length : 0.0;

    auto intersectLeaf = [&](uint32_t index) {
        for (size_t i = 0; i < trisPerLeaf; ++i)
        {
            const auto& triangle = leaves[index].tris[i];

            vec_type P = cross(d, vec_type(triangle.edge2));
            value_type det = dot(P, vec_type(triangle.edge1));
            if (det > -epsilon && det < epsilon) continue;

            vec_type T = vec_type(start) - vec_type(triangle.vertex0);
            value_type u2 = dot(P, T);
            if (det * u2 < 0.0 || std::abs(u2) > std::abs(det)) continue;

            vec_type Q = cross(T, vec_type(triangle.edge1));
            value_type v2 = dot(Q, d);
            if (det * v2 < 0.0 || std::abs(u2) + std::abs(v2) > std::abs(det)) continue;

            value_type t2 = dot(Q, vec_type(triangle.edge2));
            if (std::abs(t2) < std::abs(epsilon * det)) continue;

            value_type inv_det = 1.0 / det;
            value_type r1 = u2 * inv_det;
            value_type r2 = v2 * inv_det;
            value_type r0 = 1.0 - r1 - r2;

            dvec3 intersection = dvec3(triangle.vertex0) * double(r0 + r1 + r2) + dvec3(triangle.edge1) * double(r1) + dvec3(triangle.edge2) * double(r2);
            const auto& metadata = leafMetadata[index].tris[i];
            GetArrays arrayGetter;
            original->accept(arrayGetter);
            lineSegmentIntersector.add(intersection, double(t2 * inv_det * inverseLength), {{metadata.index0, r0}, {metadata.index1 + 1, r1}, {metadata.index2 + 2, r2}}, metadata.instance, std::move(arrayGetter.arrays));
        }
    };

    auto intersectNode = [&](const NodeRef& nodeRef, auto&& intersectNodeRecursive) -> void {
        if (nodeRef.type == NodeRef::LEAF)
        {
            intersectLeaf(nodeRef.index);
        }
        else
        {
            const auto& node = internalNodes[nodeRef.index];
            for (const auto& [bound, child] : node.children)
            {
                if (intersectBox(bound))
                {
                    intersectNodeRecursive(child, intersectNodeRecursive);
                }
            }
        }
    };

    intersectNode(boundingVolumeHeirarchy, intersectNode);
}

BVHIntersectionProxy::~BVHIntersectionProxy() = default;

int BVHIntersectionProxy::compare(const Object& rhs_object) const
{
    int result = IntersectionProxy::compare(rhs_object);
    if (result != 0) return result;

    const auto& rhs = static_cast<decltype(*this)>(rhs_object);
    // computation of BVH should be deterministic, so if the input is the same and it's actually been computed, we shouldn't need to scan all the nodes
    if ((result = compare_value(boundingVolumeHeirarchy.type, rhs.boundingVolumeHeirarchy.type)) != 0) return result;
    return compare_value(boundingVolumeHeirarchy.index, boundingVolumeHeirarchy.index);
}

void BVHIntersectionProxy::read(Input& input)
{
    IntersectionProxy::read(input);

    // todo: deserialise BVH
}

void BVHIntersectionProxy::write(Output& output) const
{
    IntersectionProxy::write(output);

    // todo: serialise BVH
}

BypassIntersectionProxy::BypassIntersectionProxy(Node* in_original, Node* in_target) :
    Inherit(in_original),
    target(in_target)
{
}

BypassIntersectionProxy::BypassIntersectionProxy(const BypassIntersectionProxy& rhs, const CopyOp& copyop) :
    Inherit(rhs, copyop),
    target(rhs.target)
{
}

bool BypassIntersectionProxy::valid() const
{
    return target != nullptr;
}

void BypassIntersectionProxy::intersect(LineSegmentIntersector& lineSegmentIntersector) const
{
    target->accept(lineSegmentIntersector);
}

BypassIntersectionProxy::~BypassIntersectionProxy() = default;

int BypassIntersectionProxy::compare(const Object& rhs_object) const
{
    int result = IntersectionProxy::compare(rhs_object);
    if (result != 0) return result;

    const auto& rhs = static_cast<decltype(*this)>(rhs_object);

    return compare_pointer(target, rhs.target);
}

void BypassIntersectionProxy::read(Input& input)
{
    IntersectionProxy::read(input);

    input.read("target", target);
}

void BypassIntersectionProxy::write(Output& output) const
{
    IntersectionProxy::write(output);

    output.write("target", target);
}

MultiBypassIntersectionProxy::MultiBypassIntersectionProxy(Node* in_original, std::vector<Target>&& in_targets) :
    Inherit(in_original),
    targets(std::move(in_targets))
{
}

MultiBypassIntersectionProxy::MultiBypassIntersectionProxy(const MultiBypassIntersectionProxy& rhs, const CopyOp& copyop) :
    Inherit(rhs, copyop),
    targets(rhs.targets)
{
}

bool MultiBypassIntersectionProxy::valid() const
{
    if (targets.empty()) return false;
    for (const auto& target : targets)
    {
        if (target.node == nullptr) return false;
    }
    return true;
}

void MultiBypassIntersectionProxy::intersect(LineSegmentIntersector& lineSegmentIntersector) const
{
    Intersector::NodePath& nodePath = lineSegmentIntersector.nodePath();
    for (const auto& target : targets)
    {
        for (const auto& node : target.nodePath) nodePath.push_back(node);
        target.node->accept(lineSegmentIntersector);
        for (const auto& node : target.nodePath) nodePath.pop_back();
    }
}

MultiBypassIntersectionProxy::~MultiBypassIntersectionProxy() = default;

int MultiBypassIntersectionProxy::compare(const Object& rhs_object) const
{
    int result = IntersectionProxy::compare(rhs_object);
    if (result != 0) return result;

    const auto& rhs = static_cast<decltype(*this)>(rhs_object);

    if (targets.size() < rhs.targets.size()) return -1;
    if (targets.size() > rhs.targets.size()) return 1;
    if (targets.empty()) return 0;

    auto rhs_itr = rhs.targets.begin();
    for (auto lhs_itr = targets.begin(); lhs_itr != targets.end(); ++lhs_itr, ++rhs_itr)
    {
        if ((result = compare_pointer(lhs_itr->node, rhs_itr->node)) != 0) return result;
        if ((result = compare_pointer_container(lhs_itr->nodePath, rhs_itr->nodePath)) != 0) return result;
    }

    return 0;
}

void MultiBypassIntersectionProxy::read(Input& input)
{
    IntersectionProxy::read(input);

    targets.resize(input.readValue<uint32_t>("targets"));
    for (auto& target : targets)
    {
        input.read("target.node", target.node);
        input.readObjects("target.nodePath", target.nodePath);
    }
}

void MultiBypassIntersectionProxy::write(Output& output) const
{
    IntersectionProxy::write(output);

    output.writeValue<uint32_t>("targets", targets.size());
    for (const auto& target : targets)
    {
        output.write("target.node", target.node);
        output.writeObjects("target.nodePath", target.nodePath);
    }
}

namespace
{
    struct NoBypassDetector : public ConstVisitor
    {
        bool bypassable = true;

        void apply(const Transform&) override { bypassable = false; }
        void apply(const LOD&) override { bypassable = false; }
        void apply(const PagedLOD&) override { bypassable = false; }
        void apply(const CullNode&) override { bypassable = false; }
        void apply(const CullGroup&) override { bypassable = false; }
        void apply(const DepthSorted&) override { bypassable = false; }
        void apply(const Geometry&) override { bypassable = false; }
        void apply(const Draw&) override { bypassable = false; }
        void apply(const DrawIndexed&) override { bypassable = false; }
    };

    std::optional<ref_ptr<IntersectionProxy>> createBypassFor(Node& node, const std::vector<IntersectionOptimizeVisitor::IntersectableDescendant>& intersectableDescendants, IntersectionOptimizeVisitor::BypassCostEstimator& bypassCostEstimator)
    {
        bypassCostEstimator.pathCost = 0;
        for (const auto& [_, path] : intersectableDescendants)
        {
            node.accept(bypassCostEstimator);
            for (const auto* pathNode : path)
                pathNode->accept(bypassCostEstimator);
        }
        if (bypassCostEstimator.pathCost < bypassCostEstimator.threshold) return std::nullopt;

        if (intersectableDescendants.size() > 1)
        {
            IntersectionOptimizeVisitor::NodePath commonPrefix;
            for (size_t i = 0; i < intersectableDescendants.front().nodePath.size(); ++i)
            {
                Node* node = intersectableDescendants.front().nodePath[i];
                bool mismatch = false;
                for (const auto& [_, path] : intersectableDescendants)
                {
                    if (path[i] != node)
                    {
                        mismatch = true;
                        break;
                    }
                }

                if (mismatch) break;

                commonPrefix.push_back(node);
            }

            std::vector<MultiBypassIntersectionProxy::Target> targets;
            targets.reserve(intersectableDescendants.size());

            for (const auto& [descendant, path] : intersectableDescendants)
            {
                targets.push_back({ref_ptr(descendant), {path.begin() + commonPrefix.size(), path.end()}});
            }

            auto mbip = MultiBypassIntersectionProxy::create(&node, std::move(targets));
            mbip->proxiedNodePath.insert(mbip->proxiedNodePath.end(), commonPrefix.begin(), commonPrefix.end());
            return mbip;
        }
        else if (intersectableDescendants.size() > 0)
        {
            const auto& target = intersectableDescendants.front();
            auto bip = BypassIntersectionProxy::create(&node, target.node);
            bip->proxiedNodePath.insert(bip->proxiedNodePath.end(), target.nodePath.begin(), target.nodePath.end());
            return bip;
        }
        return std::nullopt;
    }
}

IntersectionOptimizeVisitor::IntersectionOptimizeVisitor(ref_ptr<ArrayState> initialArrayState, std::unique_ptr<BypassCostEstimator>&& in_bypassCostEstimator):
    bypassCostEstimator(std::move(in_bypassCostEstimator))
{
    arrayStateStack.reserve(4);
    arrayStateStack.emplace_back(initialArrayState ? initialArrayState : ArrayState::create());
}

std::optional<ref_ptr<Object>> IntersectionOptimizeVisitor::apply(Node& node)
{
    nodePath.push_back(&node);

    node.traverse(*this);

    nodePath.pop_back();

    NoBypassDetector nodeNbd;
    node.accept(nodeNbd);

    if (!nodePath.empty())
    {
        NoBypassDetector parentNbd;
        nodePath.back()->accept(parentNbd);

        if (parentNbd.bypassable)
        {

            if (nodeNbd.bypassable)
            {
                auto itr = intersectableDescendants.find(&node);
                if (itr != intersectableDescendants.end())
                {
                    for (auto& [descendant, path] : itr->second)
                    {
                        intersectableDescendants[nodePath.back()].push_back({descendant, {}});
                        auto& newPath = intersectableDescendants[nodePath.back()].back().nodePath;
                        newPath.reserve(path.size() + 1);
                        newPath.push_back(&node);
                        newPath.insert(newPath.end(), path.begin(), path.end());
                    }
                }
            }
            else
            {
                intersectableDescendants[nodePath.back()].push_back({&node, {}});
            }
        }
        else if (nodeNbd.bypassable)
        {
            return createBypassFor(node, intersectableDescendants[&node], *bypassCostEstimator);
        }
    }
    else
    {
        std::optional<ref_ptr<IntersectionProxy>> proxy;

        if (nodeNbd.bypassable)
        {
            proxy = createBypassFor(node, intersectableDescendants[&node], *bypassCostEstimator);
        }

        // We're finished with this scenegraph, get the visitor ready to process the next one
        intersectableDescendants.clear();

        return proxy;
    }

    return std::nullopt;
}

std::optional<ref_ptr<Object>> IntersectionOptimizeVisitor::apply(StateGroup& stategroup)
{
    auto arrayState = stategroup.prototypeArrayState ? stategroup.prototypeArrayState->cloneArrayState(arrayStateStack.back()) : arrayStateStack.back()->cloneArrayState();

    for (auto& statecommand : stategroup.stateCommands)
    {
        statecommand->accept(*arrayState);
    }

    arrayStateStack.emplace_back(arrayState);

    auto replacement = apply(static_cast<Node&>(stategroup));

    arrayStateStack.pop_back();

    return replacement;
}

std::optional<ref_ptr<Object>> IntersectionOptimizeVisitor::apply(VertexDraw& vertexDraw)
{
    auto optimized = BVHIntersectionProxy::create(&vertexDraw);
    optimized->rebuild(*arrayStateStack.back());

    if (!nodePath.empty())
    {
        intersectableDescendants[nodePath.back()].push_back({optimized, {}});
    }
    return optimized;
}

std::optional<ref_ptr<Object>> IntersectionOptimizeVisitor::apply(VertexIndexDraw& vertexIndexDraw)
{
    auto optimized = BVHIntersectionProxy::create(&vertexIndexDraw);
    optimized->rebuild(*arrayStateStack.back());

    if (!nodePath.empty())
    {
        intersectableDescendants[nodePath.back()].push_back({optimized, {}});
    }
    return optimized;
}

std::optional<ref_ptr<Object>> IntersectionOptimizeVisitor::apply(IntersectionProxy& intersectionProxy)
{
    if (!nodePath.empty())
    {
        intersectableDescendants[nodePath.back()].push_back({&intersectionProxy, {}});
    }
    return std::nullopt;
}
