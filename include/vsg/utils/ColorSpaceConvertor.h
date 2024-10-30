#pragma once

/* <editor-fold desc="MIT License">

Copyright(c) 2024 Chris Djali

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

</editor-fold> */

#include <vsg/core/Visitor.h>
#include <vsg/maths/color.h>
#include <vsg/nodes/Node.h>

namespace vsg
{
    // Goals:
    // * Convert material colours
    // * Convert vertex colours
    // * Traverse the scenegraph to find the things we want to convert

    /// BaseColorSpaceConvertor is a base class for switching colors in the scene graph between color spaces.
    class VSG_DECLSPEC BaseColorSpaceConvertor : public Inherit<Visitor, BaseColorSpaceConvertor>
    {
    public:

        BaseColorSpaceConvertor();

        // apply directly to colors
        void apply(vec3Value& value) override;
        void apply(dvec3Value& value) override;
        void apply(vec4Value& value) override;
        void apply(dvec4Value& value) override;

        void apply(vec3Array& value) override;
        void apply(dvec3Array& value) override;
        void apply(vec4Array& value) override;
        void apply(dvec4Array& value) override;

        //
        // provide virtual functions for concrete BaseColorSpaceConvertor implementations to actually convert colors
        //
        virtual void convert(vec4& color) = 0;
        virtual void convert(dvec4& color) = 0;
    };
    VSG_type_name(vsg::BaseColorSpaceConvertor);

    /// MaterialColorSpaceConvertor is a base class for switching colors in the scene graph between color spaces.
    /// It only affects material colors.
    class VSG_DECLSPEC MaterialColorSpaceConvertor : public Inherit<BaseColorSpaceConvertor, MaterialColorSpaceConvertor>
    {
    public:
        MaterialColorSpaceConvertor();

        //
        // handle traverse of the scene graph
        //
        void apply(Node& node) override;
        void apply(Command& command) override;
        void apply(DescriptorSet& descriptorSet) override;

         // Apply to things that might have material colours
        void apply(DescriptorBuffer& descriptorBuffer) override;
    };
    VSG_type_name(vsg::MaterialColorSpaceConvertor);

    class VSG_DECLSPEC FromSRGBMaterialColorSpaceConvertor : public Inherit<MaterialColorSpaceConvertor, FromSRGBMaterialColorSpaceConvertor>
    {
    public:
        FromSRGBMaterialColorSpaceConvertor() {}

        void convert(vec4& color) override { color = sRGB_to_linear(color); }
        void convert(dvec4& color) override { color = sRGB_to_linear(color); }
    };
    VSG_type_name(vsg::FromSRGBMaterialColorSpaceConvertor);

    /// VertexColorColorSpaceConvertor is a base class for switching colors in the scene graph between color spaces.
    /// It only affects material colors.
    class VSG_DECLSPEC VertexColorColorSpaceConvertor : public Inherit<BaseColorSpaceConvertor, VertexColorColorSpaceConvertor>
    {
    public:
        VertexColorColorSpaceConvertor();

        //
        // handle traverse of the scene graph
        //
    };
    VSG_type_name(vsg::VertexColorColorSpaceConvertor);

} // namespace vsg
