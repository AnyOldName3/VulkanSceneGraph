/* <editor-fold desc="MIT License">

Copyright(c) 2024 Chris Djali

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

</editor-fold> */

#include <vsg/utils/ColorSpaceConvertor.h>

#include <vsg/commands/Command.h>
#include <vsg/state/DescriptorBuffer.h>
#include <vsg/state/DescriptorSet.h>
#include <vsg/state/material.h>

using namespace vsg;

BaseColorSpaceConvertor::BaseColorSpaceConvertor()
{
}

void BaseColorSpaceConvertor::apply(vec3Value& value)
{
    vec4 temp(value.value(), 1.0);
    convert(temp);
    value.value() = temp.xyz;
}

void BaseColorSpaceConvertor::apply(dvec3Value& value)
{
    dvec4 temp(value.value(), 1.0);
    convert(temp);
    value.value() = temp.xyz;
}

void BaseColorSpaceConvertor::apply(vec4Value& value)
{
    convert(value.value());
}

void BaseColorSpaceConvertor::apply(dvec4Value& value)
{
    convert(value.value());
}

void BaseColorSpaceConvertor::apply(vec3Array& value)
{
    for (vec3& color : value)
    {
        vec4 temp(color, 1.0);
        convert(temp);
        color = temp.xyz;
    }
}

void BaseColorSpaceConvertor::apply(dvec3Array& value)
{
    for (dvec3& color : value)
    {
        dvec4 temp(color, 1.0);
        convert(temp);
        color = temp.xyz;
    }
}

void BaseColorSpaceConvertor::apply(vec4Array& value)
{
    for (vec4& color : value)
        convert(color);
}

void BaseColorSpaceConvertor::apply(dvec4Array& value)
{
    for (dvec4& color : value)
        convert(color);
}

vsg::MaterialColorSpaceConvertor::MaterialColorSpaceConvertor()
{
}

void vsg::MaterialColorSpaceConvertor::apply(Node& node)
{
    node.traverse(*this);
}

void vsg::MaterialColorSpaceConvertor::apply(Command& command)
{
    command.traverse(*this);
}

void vsg::MaterialColorSpaceConvertor::apply(DescriptorSet& descriptorSet)
{
    descriptorSet.traverse(*this);
}

void vsg::MaterialColorSpaceConvertor::apply(DescriptorBuffer& descriptorBuffer)
{
    for (const auto& bufferInfo : descriptorBuffer.bufferInfoList)
    {
        if (bufferInfo && bufferInfo->data)
        {
            if (auto* pbrMaterial = dynamic_cast<PbrMaterialValue*>(bufferInfo->data.get()); pbrMaterial)
            {
                convert(pbrMaterial->value().baseColorFactor);
                convert(pbrMaterial->value().emissiveFactor);
                convert(pbrMaterial->value().diffuseFactor);
                convert(pbrMaterial->value().specularFactor);
            }
            else if (auto* phongMaterial = dynamic_cast<PhongMaterialValue*>(bufferInfo->data.get()); phongMaterial)
            {
                convert(phongMaterial->value().ambient);
                convert(phongMaterial->value().diffuse);
                convert(phongMaterial->value().emissive);
                convert(phongMaterial->value().specular);
            }
            else if (auto* material = dynamic_cast<materialValue*>(bufferInfo->data.get()); material)
            {
                convert(material->value().ambientColor);
                convert(material->value().diffuseColor);
                convert(material->value().specularColor);
            }
        }
    }
}

vsg::VertexColorColorSpaceConvertor::VertexColorColorSpaceConvertor()
{
}
