#pragma once

/* <editor-fold desc="MIT License">

Copyright(c) 2021 Robert Osfield

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

</editor-fold> */

#include <vsg/io/ReaderWriter.h>

#include <map>

namespace vsg
{

    /// ReaderWriter for reading and writing spirv shader files.
    class VSG_DECLSPEC spirv : public Inherit<ReaderWriter, spirv>
    {
    public:
        spirv();

        ref_ptr<Object> read(const Path& filename, ref_ptr<const Options> options = {}) const override;

        ref_ptr<vsg::Object> read(std::istream& fin, ref_ptr<const Options> options = {}) const override;
        ref_ptr<vsg::Object> read(const uint8_t* ptr, size_t size, ref_ptr<const Options> = {}) const override;

        bool write(const Object* object, const Path& filename, ref_ptr<const Options> options = {}) const override;

        bool getFeatures(Features& features) const override;
    };
    VSG_type_name(vsg::spirv);

} // namespace vsg
