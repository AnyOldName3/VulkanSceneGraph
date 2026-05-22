#pragma once

/* <editor-fold desc="MIT License">

Copyright(c) 2026 Chris Djali

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

</editor-fold> */

#include <vsg/core/Auxiliary.h>
#include <vsg/core/Object.h>
#include <vsg/io/Logger.h>

namespace vsg
{

    /// Mechanism to give vsg::Object subclasses a way to specify their deleter per-instance instead of per-type
    /// Deleter is a pure virtual base class for deleters to type-erase the implementation
    class VSG_DECLSPEC Deleter
    {
    public:
        virtual void operator()(vsg::Object* object) = 0;
    };

    /// Helper to replicate the default vsg::Object::_attemptDelete behaviour
    class VSG_DECLSPEC DeleteDispatcher
    {
    public:
        Deleter* deleter;

        DeleteDispatcher() :
            deleter(nullptr)
        {}

        DeleteDispatcher(Deleter* in_deleter) :
            deleter(in_deleter)
        {}
   
        // templated because vsg::Object destructor is private, so delete call doesn't work on the common base class
        template<typename T>
        void attemptDelete(const T* object) const
        {
            // what should happen when _delete is called on an Object with ref() of zero?  Need to decide whether this buggy application usage should be tested for.

            // if there is an auxiliary attached signal to it we wish to delete, and give it an opportunity to decide whether a delete is appropriate.
            // if no auxiliary is attached then go straight ahead and delete.
            if (object->getAuxiliary() == nullptr || const_cast<Auxiliary*>(object->getAuxiliary())->signalConnectedObjectToBeDeleted())
            {
                //debug("DeleteDispatcher::attemptDelete() ", this, " calling delete");
                if (deleter)
                    (*deleter)(const_cast<T*>(object));
                else
                    delete object;
            }
            else
            {
                debug("DeleteDispatcher::attemptDelete() ", this, " choosing not to delete");
            }
        }
    };

    // polyfill for C++20's uses_allocator_construction_args
    template<class T, class Alloc, class... Args>
    constexpr auto uses_allocator_construction_args(const Alloc& alloc, Args&&... args) noexcept
    {
        if constexpr (!std::uses_allocator_v<std::remove_cv_t<T>, Alloc> && std::is_constructible_v<T, Args...>)
        {
            return std::forward_as_tuple(std::forward<Args>(args)...);
        }
        else if constexpr (std::uses_allocator_v<std::remove_cv_t<T>, Alloc> && std::is_constructible_v<T, std::allocator_arg_t, const Alloc&, Args...>)
        {
            return std::tuple<std::allocator_arg_t, const Alloc&, Args&&...>(std::allocator_arg, alloc, std::forward<Args>(args)...);
        }
        else if constexpr (std::uses_allocator_v<std::remove_cv_t<T>, Alloc> && std::is_constructible_v<T, Args..., const Alloc&>)
        {
            return std::forward_as_tuple(std::forward<Args>(args)..., alloc);
        }
        else
        {
            static_assert(sizeof(T) + 1 == 0, "Type uses_allocator but isn't allocator-constructible");
        }
    }

    template<class T, class Alloc>
    class DeleterForAllocator : public Deleter
    {
    private:
        static_assert(std::is_same_v<T, std::remove_cv_t<T>>, "create_with_allocator didn't remove_cv_t");

    public:
        using allocator_type = typename std::allocator_traits<Alloc>::template rebind_alloc<DeleterForAllocator>;

        template<class... Args>
        explicit DeleterForAllocator(std::allocator_arg_t, const Alloc& alloc, Args&&... args) :
            storage(std::make_from_tuple<T>(uses_allocator_construction_args<T>(alloc, std::forward<Args>(args)...))),
            allocator(alloc)
        {
            storage.deleteDispatcher.deleter = this;
        }

        virtual ~DeleterForAllocator() = default;

        void operator()(vsg::Object*) override
        {
            // we could assert that the passed argument is storage
            std::allocator_traits<allocator_type>::destroy(allocator, this);
            std::allocator_traits<allocator_type>::deallocate(allocator, this, 1);
        }

        T storage;

        // todo: empty base optimisation/no_unique_address
        allocator_type allocator;
    };

    template<class T, class Alloc, class... Args>
    [[nodiscard]] ref_ptr<T> create_with_allocator(const Alloc& allocator, Args&&... args)
    {
        using ControlBlock = DeleterForAllocator<std::remove_cv_t<T>, Alloc>;
        using ReboundAlloc = typename ControlBlock::allocator_type;
        auto reboundAlloc = ReboundAlloc(allocator);
        ControlBlock* controlBlock = std::allocator_traits<ReboundAlloc>::allocate(reboundAlloc, 1);
        std::allocator_traits<ReboundAlloc>::construct(reboundAlloc, controlBlock, std::forward<Args>(args)...);
        return ref_ptr<T>(std::addressof(controlBlock->storage));
    }

} // namespace vsg
