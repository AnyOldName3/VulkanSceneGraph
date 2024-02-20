#pragma once

/* <editor-fold desc="MIT License">

Copyright(c) 2024 Robert Osfield

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

</editor-fold> */

#include <vsg/animation/Animation.h>

namespace vsg
{

    struct MorphKey
    {
        double time;
        std::vector<unsigned int> values;
        std::vector<double> weights;

        bool operator<(const MorphKey& rhs) const { return time < rhs.time; }
    };
    VSG_type_name(vsg::MorphKey);

    class VSG_DECLSPEC MorphKeyframes : public Inherit<Object, MorphKeyframes>
    {
    public:
        MorphKeyframes();

        /// name of animation
        std::string name;

        /// key frames
        std::vector<MorphKey> keyframes;

        void read(Input& input) override;
        void write(Output& output) const override;
    };
    VSG_type_name(vsg::MorphKeyframes);

    class VSG_DECLSPEC MorphSampler : public Inherit<AnimationSampler, MorphSampler>
    {
    public:
        MorphSampler();
        MorphSampler(const MorphSampler& rhs, const CopyOp& copyop = {});

        ref_ptr<MorphKeyframes> keyframes;
        ref_ptr<Object> object;

        void update(double time) override;
        double maxTime() const override;

    public:
        ref_ptr<Object> clone(const CopyOp& copyop = {}) const override { return MorphSampler::create(*this, copyop); }
        int compare(const Object& rhs) const override;

        void read(Input& input) override;
        void write(Output& output) const override;
    };
    VSG_type_name(vsg::MorphSampler);

} // namespace vsg
