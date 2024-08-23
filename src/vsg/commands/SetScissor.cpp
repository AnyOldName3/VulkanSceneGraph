/* <editor-fold desc="MIT License">

Copyright(c) 2021 Robert Osfield

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

</editor-fold> */

#include <vsg/commands/SetScissor.h>
#include <vsg/io/Options.h>
#include <vsg/state/ViewDependentState.h>
#include <vsg/vk/CommandBuffer.h>

using namespace vsg;

SetScissor::SetScissor() :
    firstScissor(0)
{
}

SetScissor::SetScissor(uint32_t in_firstScissor, const Scissors& in_scissors) :
    firstScissor(in_firstScissor),
    scissors(in_scissors)
{
}

void SetScissor::record(CommandBuffer& commandBuffer) const
{
    vkCmdSetScissor(commandBuffer, firstScissor, static_cast<uint32_t>(scissors.size()), scissors.data());
    if (commandBuffer.viewDependentState)
        commandBuffer.viewDependentState->scissorsDirty = true;
}
