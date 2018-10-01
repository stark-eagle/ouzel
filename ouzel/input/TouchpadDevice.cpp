// Copyright 2015-2018 Elviss Strazdins. All rights reserved.

#include "TouchpadDevice.hpp"
#include "InputSystem.hpp"

namespace ouzel
{
    namespace input
    {
        void TouchpadDevice::handleTouchBegin(uint64_t touchId, const Vector2& position, float force)
        {
            InputSystem::Event event;
            event.type = InputSystem::Event::Type::TOUCH_BEGIN;
            event.deviceId = id;
            event.touchId = touchId;
            event.position = position;
            event.force = force;
            inputSystem.addEvent(event);
        }

        void TouchpadDevice::handleTouchEnd(uint64_t touchId, const Vector2& position, float force)
        {
            InputSystem::Event event;
            event.type = InputSystem::Event::Type::TOUCH_END;
            event.deviceId = id;
            event.touchId = touchId;
            event.position = position;
            event.force = force;
            inputSystem.addEvent(event);
        }

        void TouchpadDevice::handleTouchMove(uint64_t touchId, const Vector2& position, float force)
        {
            InputSystem::Event event;
            event.type = InputSystem::Event::Type::TOUCH_MOVE;
            event.deviceId = id;
            event.touchId = touchId;
            event.position = position;
            event.force = force;
            inputSystem.addEvent(event);
        }

        void TouchpadDevice::handleTouchCancel(uint64_t touchId, const Vector2& position, float force)
        {
            InputSystem::Event event;
            event.type = InputSystem::Event::Type::TOUCH_CANCEL;
            event.deviceId = id;
            event.touchId = touchId;
            event.position = position;
            event.force = force;
            inputSystem.addEvent(event);
        }
    } // namespace input
} // namespace ouzel