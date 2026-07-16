/* <editor-fold desc="MIT License">

Copyright(c) 2025 Chris Djali

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

</editor-fold> */

#include <vsg/io/ApplyVisitorReader.h>

using namespace vsg;

ApplyVisitorReader::ApplyVisitorReader(ref_ptr<ReaderWriter> in_child, ref_ptr<Visitor> in_visitor) :
    child(in_child),
    visitor(in_visitor)
{
}

ApplyVisitorReader::ApplyVisitorReader(const ApplyVisitorReader& rhs, const CopyOp& copyop) :
    Inherit(rhs),
    child(copyop(rhs.child)),
    visitor(copyop(rhs.visitor))
{
}

void ApplyVisitorReader::read(Input& input)
{
    input.readObject("child", child);
    input.readObject("visitor", visitor);
}

void ApplyVisitorReader::write(Output& output) const
{
    output.writeObject("child", child);
    output.writeObject("visitor", visitor);
}

ref_ptr<Object> ApplyVisitorReader::read(const Path& filename, ref_ptr<const Options> options) const
{
    auto object = child->read(filename, options);
    if (object)
    {
        std::scoped_lock<std::mutex> lock(_visitorMutex);
        object->accept(*visitor);
    }
    return object;
}

ref_ptr<Object> ApplyVisitorReader::read(std::istream& fin, ref_ptr<const Options> options) const
{
    auto object = child->read(fin, options);
    if (object)
    {
        std::scoped_lock<std::mutex> lock(_visitorMutex);
        object->accept(*visitor);
    }
    return object;
}

ref_ptr<Object> ApplyVisitorReader::read(const uint8_t* ptr, size_t size, ref_ptr<const Options> options) const
{
    auto object = child->read(ptr, size, options);
    if (object)
    {
        std::scoped_lock<std::mutex> lock(_visitorMutex);
        object->accept(*visitor);
    }
    return object;
}

bool ApplyVisitorReader::write(const Object* object, const Path& filename, ref_ptr<const Options> options) const
{
    return child->write(object, filename, options);
}

bool ApplyVisitorReader::write(const Object* object, std::ostream& fout, ref_ptr<const Options> options) const
{
    return child->write(object, fout, options);
}

bool ApplyVisitorReader::readOptions(Options& options, CommandLine& arguments) const
{
    return child->readOptions(options, arguments);
}

bool ApplyVisitorReader::getFeatures(Features& features) const
{
    return child->getFeatures(features);
}

ApplyReplacementVisitorReader::ApplyReplacementVisitorReader(ref_ptr<ReaderWriter> in_child, ref_ptr<ReplacementVisitor> in_visitor) :
    child(in_child),
    visitor(in_visitor)
{
}

ApplyReplacementVisitorReader::ApplyReplacementVisitorReader(const ApplyReplacementVisitorReader& rhs, const CopyOp& copyop) :
    Inherit(rhs),
    child(copyop(rhs.child)),
    visitor(copyop(rhs.visitor))
{
}

void ApplyReplacementVisitorReader::read(Input& input)
{
    input.readObject("child", child);
    input.readObject("visitor", visitor);
}

void ApplyReplacementVisitorReader::write(Output& output) const
{
    output.writeObject("child", child);
    output.writeObject("visitor", visitor);
}

ref_ptr<Object> ApplyReplacementVisitorReader::read(const Path& filename, ref_ptr<const Options> options) const
{
    auto object = child->read(filename, options);
    if (object)
    {
        std::scoped_lock<std::mutex> lock(_visitorMutex);
        visitor->tryReplacePointer(object);
    }
    return object;
}

ref_ptr<Object> ApplyReplacementVisitorReader::read(std::istream& fin, ref_ptr<const Options> options) const
{
    auto object = child->read(fin, options);
    if (object)
    {
        std::scoped_lock<std::mutex> lock(_visitorMutex);
        visitor->tryReplacePointer(object);
    }
    return object;
}

ref_ptr<Object> ApplyReplacementVisitorReader::read(const uint8_t* ptr, size_t size, ref_ptr<const Options> options) const
{
    auto object = child->read(ptr, size, options);
    if (object)
    {
        std::scoped_lock<std::mutex> lock(_visitorMutex);
        visitor->tryReplacePointer(object);
    }
    return object;
}

bool ApplyReplacementVisitorReader::write(const Object* object, const Path& filename, ref_ptr<const Options> options) const
{
    return child->write(object, filename, options);
}

bool ApplyReplacementVisitorReader::write(const Object* object, std::ostream& fout, ref_ptr<const Options> options) const
{
    return child->write(object, fout, options);
}

bool ApplyReplacementVisitorReader::readOptions(Options& options, CommandLine& arguments) const
{
    return child->readOptions(options, arguments);
}

bool ApplyReplacementVisitorReader::getFeatures(Features& features) const
{
    return child->getFeatures(features);
}
