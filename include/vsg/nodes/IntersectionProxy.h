#pragma once

/* <editor-fold desc="MIT License">

Copyright(c) 2025-2026 Chris Djali

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

</editor-fold> */

#include <vsg/nodes/Node.h>
#include <vsg/state/ArrayState.h>

namespace vsg
{
    class LineSegmentIntersector;

    /** IntersectionProxy is used instead of a subgraph for vsg::Intersector operations, e.g. to get the same result faster. */
    class VSG_DECLSPEC IntersectionProxy : public Inherit<Node, IntersectionProxy>
    {
    public:
        IntersectionProxy(Node* in_original);
        IntersectionProxy(const IntersectionProxy& rhs, const CopyOp& copyop = {});

        virtual bool valid() const = 0;
        virtual void intersect(LineSegmentIntersector& lineSegmentIntersector) const = 0;

    public:
        int compare(const Object& rhs) const override;

        template<class N, class V>
        static void t_traverse(N& node, V& visitor)
        {
            node.original->accept(visitor);
        }

        void traverse(Visitor& visitor) override { t_traverse(*this, visitor); }
        void traverse(ConstVisitor& visitor) const override { t_traverse(*this, visitor); }
        void traverse(RecordTraversal& visitor) const override { t_traverse(*this, visitor); }
        void traverse(ReplacementVisitor& visitor) override
        {
            if (visitor.tryReplacePointer(original))
            {
                // TODO mark dirty
            }
        }

        void read(Input& input) override;
        void write(Output& output) const override;

        ref_ptr<Node> original;
        std::vector<ref_ptr<const Node>> proxiedNodePath;

    protected:
        virtual ~IntersectionProxy();
    };
    VSG_type_name(vsg::IntersectionProxy);

    /** BVHIntersectionProxy wraps a node with a BVH to accelerate vsg::Intersector operations */
    class VSG_DECLSPEC BVHIntersectionProxy : public Inherit<IntersectionProxy, BVHIntersectionProxy>
    {
    public:
        BVHIntersectionProxy(Node* in_original);
        BVHIntersectionProxy(const BVHIntersectionProxy& rhs, const CopyOp& copyop = {});

        void rebuild(ArrayState& arrayState);

        bool valid() const override;

        void intersect(LineSegmentIntersector& lineSegmentIntersector) const override;

    public:
        ref_ptr<Object> clone(const CopyOp& copyop = {}) const override { return BVHIntersectionProxy::create(*this, copyop); }
        int compare(const Object& rhs) const override;

        void read(Input& input) override;
        void write(Output& output) const override;

    protected:
        virtual ~BVHIntersectionProxy();

        struct Triangle
        {
            vec3 vertex0;
            vec3 edge1;
            vec3 edge2;
        };
        static constexpr size_t trisPerLeaf = 4;
        struct Leaf
        {
            std::array<Triangle, trisPerLeaf> tris;
        };
        struct NodeRef
        {
            enum NodeType
            {
                LEAF,
                INTERNAL,
                INVALID
            };
            NodeType type;
            uint32_t index;
        };
        struct InternalNode
        {
            std::array<std::pair<box, NodeRef>, 2> children;
        };

        std::vector<InternalNode, allocator_affinity_data<InternalNode>> internalNodes;
        std::vector<Leaf, allocator_affinity_data<Leaf>> leaves;
        box bounds;
        NodeRef boundingVolumeHeirarchy;

        struct TriangleMetadata
        {
            uint32_t index0;
            uint32_t index1;
            uint32_t index2;
            uint32_t instance;
        };
        struct LeafMetadata
        {
            std::array<TriangleMetadata, trisPerLeaf> tris;
        };

        std::vector<LeafMetadata, allocator_affinity_data<LeafMetadata>> leafMetadata;
    };
    VSG_type_name(vsg::BVHIntersectionProxy)

    class VSG_DECLSPEC IntersectionOptimizeVisitor : public Inherit<ReplacementVisitor, IntersectionOptimizeVisitor>
    {
    public:
        using ArrayStateStack = std::vector<ref_ptr<ArrayState>>;

        IntersectionOptimizeVisitor(ref_ptr<ArrayState> initialArrayState = {});

        std::optional<ref_ptr<Object>> apply(Node& node) override;

        std::optional<ref_ptr<Object>> apply(StateGroup& stateGroup) override;

        std::optional<ref_ptr<Object>> apply(VertexDraw& vertexDraw) override;
        std::optional<ref_ptr<Object>> apply(VertexIndexDraw& vertexIndexDraw) override;

        std::optional<ref_ptr<Object>> apply(IntersectionProxy& intersectionProxy) override;

    protected:
        ArrayStateStack arrayStateStack;
    };
    VSG_type_name(vsg::IntersectionOptimizeVisitor)
} // namespace vsg
