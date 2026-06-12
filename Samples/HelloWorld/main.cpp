#include "HelloWorldCommon.h"
#include "FriendlyInput.h"
#include "Rendering.h"

int main()
{
    const float clear[] = { 0.98f, 0.976f, 0.972f, 1.0f };
    return Rendering::RunRoot(L"HelloWorld", 1280, 720, [] { return HelloWorld::CreateUi().Root; }, clear);
}
