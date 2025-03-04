#include "NativeInput.h"

namespace Immortal
{
 
static inline void GetClientCursorPosition(HWND handle, LPPOINT pos)
{
    if (::GetCursorPos(pos))
    {
        ::ScreenToClient(handle, pos);
    }
}

NativeInput::NativeInput(Window *window) :
    handle{ rcast<HWND>(window->Primitive()) }
{
    !!Input::That ? throw Exception(SError::InvalidSingleton) : That = this;
    CleanUpInputs();
}

NativeInput::~NativeInput()
{

}

bool NativeInput::InternalIsKeyPressed(const KeyCode key)
{
    return KeysDown[U32(key)];
}

bool NativeInput::InternalIsMouseButtonPressed(const MouseCode button)
{
    return MouseDown[U32(button)];
}

Vector2 NativeInput::InternalGetMousePosition()
{
    POINT pos;
    GetClientCursorPosition(handle, &pos);

    return Vector2{
        ncast<float>(pos.x),
        ncast<float>(pos.y),
    };
}

float NativeInput::InternalGetMouseX()
{
    POINT pos;
    GetClientCursorPosition(handle, &pos);

    return ncast<float>(pos.x);
}

float NativeInput::InternalGetMouseY()
{
    POINT pos;
    GetClientCursorPosition(handle, &pos);

    return ncast<float>(pos.y);
}

}
