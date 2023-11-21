/* Copyright (c) 2022 Garry Whitehead
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#pragma once

#include "yave/object.h"
#include "yave/object_manager.h"

#include <deque>

namespace yave
{

/**
 Based heavily on the implementation described in the bitsquid engine
 blogpost: http://bitsquid.blogspot.com/2014/08/building-data-oriented-entity-system.html
 */
class IObjectManager : public ObjectManager
{
public:
    static constexpr int IndexBits = 22;
    static constexpr int IndexMask = (1 << IndexBits) - 1;
    static constexpr int IndexCount = (1 << IndexBits);

    static constexpr int GenerationBits = 8;
    static constexpr int GenerationMask = (1 << GenerationBits) - 1;

    constexpr static int MinimumFreeIds = 1024;

    IObjectManager();
    virtual ~IObjectManager();

    Object createObjectI();

    void destroyObjectI(Object& obj);

    [[nodiscard]] static uint32_t getIndex(const Object& obj);

    [[nodiscard]] static uint8_t getGeneration(const Object& obj);

    bool isAlive(Object& obj) const noexcept;

    // ================= client api =============================

    Object createObject() override;

    void destroyObject(Object& obj) override;

private:
    static Object makeObject(uint8_t generation, uint32_t index);

private:
    int currentIdx_;
    std::deque<uint32_t> freeIds_;
    uint8_t* generations_;
};

} // namespace yave