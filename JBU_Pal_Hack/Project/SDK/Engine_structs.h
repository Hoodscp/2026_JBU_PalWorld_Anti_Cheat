#pragma once
#include <cstdint>

// Unreal Engine 5 basic structure placeholders
// To be filled out via reversing or UE4SS/Dumper tools

namespace SDK
{
    struct FName {
        int32_t Index;
        int32_t Number;
    };

    class UObject {
        void** VTable;
        int32_t ObjectFlags;
        int32_t InternalIndex;
        class UClass* ClassPrivate;
        FName NamePrivate;
        class UObject* OuterPrivate;
    };

    class AActor : public UObject {
        // ...
    };

    class UWorld : public UObject {
        // ...
    };
}
