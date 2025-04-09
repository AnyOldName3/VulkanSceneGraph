#pragma once

/* <editor-fold desc="MIT License">

Copyright(c) 2025 Chris Djali

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

</editor-fold> */

#include <vsg/state/StateCommand.h>

namespace vsg
{

    /// NoOpStateCommand occupies a state slot but does nothing when it's recorded.
    /// It's intended to protect subgraphs from incompatible state in parent nodes and ensure VSG state is dirtied when the assiciated Vulkan state is disturbed.
    class VSG_DECLSPEC NoOpStateCommand : public Inherit<StateCommand, NoOpStateCommand>
    {
    public:
        NoOpStateCommand(const NoOpStateCommand& rhs, const CopyOp& copyop = {});

        NoOpStateCommand(uint32_t in_slot = 0) :
            Inherit(in_slot)
        {
        }

        ref_ptr<Object> clone(const CopyOp& copyop = {}) const override { return NoOpStateCommand::create(*this, copyop); }
        int compare(const Object& rhs_object) const override;

        template<class N, class V>
        static void t_traverse(N& bds, V& visitor) {}

        void traverse(Visitor& visitor) override { t_traverse(*this, visitor); }
        void traverse(ConstVisitor& visitor) const override { t_traverse(*this, visitor); }

        void read(Input& input) override;
        void write(Output& output) const override;

        // compile the Vulkan object, context parameter used for Device
        void compile(Context& context) override {};

        void record(CommandBuffer& commandBuffer) const override {};

    protected:
        virtual ~NoOpStateCommand() {}
    };
    VSG_type_name(vsg::NoOpStateCommand);

} // namespace vsg
