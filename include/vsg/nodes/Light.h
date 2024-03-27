#pragma once

/* <editor-fold desc="MIT License">

Copyright(c) 2022 Robert Osfield

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

</editor-fold> */

#include <vsg/maths/common.h>
#include <vsg/nodes/Node.h>

namespace vsg
{

    class VSG_DECLSPEC ShadowSettings : public Inherit<Object, ShadowSettings>
    {
    public:
        explicit ShadowSettings(uint32_t shadowMaps = 0);
        ShadowSettings(const ShadowSettings& rhs, const CopyOp& copyop = {});

        uint32_t shadowMaps = 0;

    public:
        ref_ptr<Object> clone(const CopyOp& copyop = {}) const override { return ShadowSettings::create(*this, copyop); }
        int compare(const Object& rhs) const override;

        void read(Input& input) override;
        void write(Output& output) const override;

    protected:
        virtual ~ShadowSettings() {}
    };
    VSG_type_name(vsg::ShadowSettings);

    class VSG_DECLSPEC HardShadows : public Inherit<ShadowSettings, HardShadows>
    {
    public:
    };

    class VSG_DECLSPEC SoftShadows : public Inherit<ShadowSettings, SoftShadows>
    {
    public:
        float penumbraRadius = 0.05f;
    };

    class VSG_DECLSPEC PercentageCloserSoftShadows : public Inherit<ShadowSettings, PercentageCloserSoftShadows>
    {
    public:
    };

    /// Light is a base node class for different light types - AmbientLight, DirectionalLight, PointLight and SpotLight.
    /// Used by the RecordTraversal to represent a light source that is placed in the LightData uniform used by the shaders when implementing lighting.
    /// Provides name, color and intensity settings common to all Light types.
    class VSG_DECLSPEC Light : public Inherit<Node, Light>
    {
    public:
        Light();
        Light(const Light& rhs, const CopyOp& copyop = {});

        std::string name;
        vec3 color = vec3(1.0f, 1.0f, 1.0f);
        float intensity = 1.0f;
        ref_ptr<ShadowSettings> shadowSettings;

    public:
        ref_ptr<Object> clone(const CopyOp& copyop = {}) const override { return Light::create(*this, copyop); }
        int compare(const Object& rhs) const override;

        void read(Input& input) override;
        void write(Output& output) const override;

    protected:
        virtual ~Light() {}
    };
    VSG_type_name(vsg::Light);

    /// AmbientLight represents an ambient light source
    class VSG_DECLSPEC AmbientLight : public Inherit<Light, AmbientLight>
    {
    public:
        AmbientLight();
        AmbientLight(const AmbientLight& rhs, const CopyOp& copyop = {});

    public:
        ref_ptr<Object> clone(const CopyOp& copyop = {}) const override { return AmbientLight::create(*this, copyop); }

        void read(Input& input) override;
        void write(Output& output) const override;

    protected:
        virtual ~AmbientLight() {}
    };
    VSG_type_name(vsg::AmbientLight);

    /// DirectionalLight represents a directional light source - used for light sources that are treated as if at infinite distance, like sun/moon.
    class VSG_DECLSPEC DirectionalLight : public Inherit<Light, DirectionalLight>
    {
    public:
        DirectionalLight();
        DirectionalLight(const DirectionalLight& rhs, const CopyOp& copyop = {});

        dvec3 direction = dvec3(0.0, 0.0, -1.0);
        float angleSubtended = 0.0090f;

    public:
        ref_ptr<Object> clone(const CopyOp& copyop = {}) const override { return DirectionalLight::create(*this, copyop); }
        int compare(const Object& rhs) const override;

        void read(Input& input) override;
        void write(Output& output) const override;

    protected:
        virtual ~DirectionalLight() {}
    };
    VSG_type_name(vsg::DirectionalLight);

    /// PointLight represents a local point light source where all light radiants event from the light position.
    class VSG_DECLSPEC PointLight : public Inherit<Light, PointLight>
    {
    public:
        PointLight();
        PointLight(const PointLight& rhs, const CopyOp& copyop = {});

        dvec3 position = dvec3(0.0, 0.0, 0.0);
        double radius = 0.0f;

    public:
        ref_ptr<Object> clone(const CopyOp& copyop = {}) const override { return PointLight::create(*this, copyop); }
        int compare(const Object& rhs) const override;

        void read(Input& input) override;
        void write(Output& output) const override;

    protected:
        virtual ~PointLight() {}
    };
    VSG_type_name(vsg::PointLight);

    /// SpotLight represents a local point light source whose intensity varies as a spot light.
    class VSG_DECLSPEC SpotLight : public Inherit<Light, SpotLight>
    {
    public:
        SpotLight();
        SpotLight(const SpotLight& rhs, const CopyOp& copyop = {});

        dvec3 position = dvec3(0.0, 0.0, 0.0);
        dvec3 direction = dvec3(0.0, 0.0, -1.0);
        double innerAngle = radians(30.0);
        double outerAngle = radians(45.0);
        double radius = 0.0f;

    public:
        ref_ptr<Object> clone(const CopyOp& copyop = {}) const override { return SpotLight::create(*this, copyop); }
        int compare(const Object& rhs) const override;

        void read(Input& input) override;
        void write(Output& output) const override;

    protected:
        virtual ~SpotLight() {}
    };
    VSG_type_name(vsg::SpotLight);

    /// convenience method for creating a subgraph with a headlight illumination using a white AmbientLight and DirectionalLight with intensity of 0.05 and 0.95 respectively.
    extern VSG_DECLSPEC ref_ptr<vsg::Node> createHeadlight();

} // namespace vsg
